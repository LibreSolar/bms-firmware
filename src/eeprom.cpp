/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "eeprom.h"

#ifdef CONFIG_EEPROM

#include <zephyr.h>
#include <device.h>
#include <drivers/eeprom.h>

#include "soc.h"
#include "thingset.h"
#include "data_nodes.h"

#include <stdio.h>
#include <stdint.h>

#define EEPROM_HEADER_SIZE 8    // bytes

K_MUTEX_DEFINE(eeprom_buf_lock);

// Buffer used by store and restore functions (must be word-aligned for hardware CRC calculation)
static uint8_t buf[512] __aligned(sizeof(uint32_t));

extern ThingSet ts;

uint32_t _calc_crc(const uint8_t *buf, size_t len)
{
#if defined(CONFIG_SOC_SERIES_STM32L4X)
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
#else
    RCC->AHBENR |= RCC_AHBENR_CRCEN;
#endif

    // we keep standard polynomial 0x04C11DB7 (same for STM32L0 and STM32F0)
    // and we don't care for endianness here
    CRC->CR |= CRC_CR_RESET;
    for (size_t i = 0; i < len; i += 4) {
        //printf("CRC buf: %.8x, CRC->DR: %.8x\n", *((uint32_t*)&buf[i]), CRC->DR);
        size_t remaining_bytes = len - i;
        if (remaining_bytes >= 4) {
            CRC->DR = *((uint32_t*)&buf[i]);
        }
        else {
            // ignore bytes >= len if len is not a multiple of 4
            CRC->DR = *((uint32_t*)&buf[i]) & (0xFFFFFFFFU >> (32 - remaining_bytes * 8));
        }
    }

#if defined(CONFIG_SOC_SERIES_STM32L4X)
    RCC->AHB1ENR &= ~(RCC_AHB1ENR_CRCEN);
#else
    RCC->AHBENR &= ~(RCC_AHBENR_CRCEN);
#endif

    return CRC->DR;
}

// EEPROM layout:
// bytes 0-1: Version number
// bytes 2-3: number of data bytes
// bytes 4-7: CRC32
// byte 8: start of data

void eeprom_restore_data()
{
    int err;

	const struct device *eeprom_dev = device_get_binding("EEPROM_0");

    // EEPROM header
    uint8_t buf_header[EEPROM_HEADER_SIZE] = {};    // initialize to avoid compiler warning
    err = eeprom_read(eeprom_dev, 0, buf_header, EEPROM_HEADER_SIZE);
    if (err != 0) {
        printf("EEPROM: read error: %d\n", err);
        return;
    }
    uint16_t version = *((uint16_t*)&buf_header[0]);
    uint16_t len     = *((uint16_t*)&buf_header[2]);
    uint32_t crc     = *((uint32_t*)&buf_header[4]);

    //printf("EEPROM header restore: ver %d, len %d, CRC %.8x\n", version, len, (unsigned int)crc);
    //printf("Header: %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\n",
    //    buf_header[0], buf_header[1], buf_header[2], buf_header[3],
    //    buf_header[4], buf_header[5], buf_header[6], buf_header[7]);

    if (version == DATA_NODES_VERSION && len <= sizeof(buf)) {
        k_mutex_lock(&eeprom_buf_lock, K_FOREVER);
        err = eeprom_read(eeprom_dev, EEPROM_HEADER_SIZE, buf, len);

        //printf("Data (len=%d): ", len);
        //for (int i = 0; i < len; i++) printf("%.2x ", buf[i]);

        if (_calc_crc(buf, len) == crc) {
            int status = ts.bin_sub(buf, sizeof(buf), TS_WRITE_MASK, PUB_NVM);
            printf("EEPROM: Data objects read and updated, ThingSet result: 0x%x\n", status);
        }
        else {
            printf("EEPROM: CRC of data not correct, expected 0x%x (data_len = %d)\n",
                (unsigned int)crc, len);
        }
        k_mutex_unlock(&eeprom_buf_lock);
    }
    else {
        printf("EEPROM: Empty or data layout version changed\n");
    }
}

void eeprom_store_data()
{
    int err;

	const struct device *eeprom_dev = device_get_binding("EEPROM_0");

    k_mutex_lock(&eeprom_buf_lock, K_FOREVER);

    int len = ts.bin_pub(buf + EEPROM_HEADER_SIZE, sizeof(buf) - EEPROM_HEADER_SIZE, PUB_NVM);
    uint32_t crc = _calc_crc(buf + EEPROM_HEADER_SIZE, len);

    // store EEPROM_VERSION, number of bytes and CRC
    *((uint16_t*)&buf[0]) = (uint16_t)DATA_NODES_VERSION;
    *((uint16_t*)&buf[2]) = (uint16_t)(len);   // length of data
    *((uint32_t*)&buf[4]) = crc;

    //printf("Data (len=%d): ", len);
    //for (int i = 0; i < len; i++) printf("%.2x ", buf[i + EEPROM_HEADER_SIZE]);

    //printf("Header: %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\n",
    //    buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

    if (len == 0) {
        printf("EEPROM: Data could not be stored. ThingSet error (len = %d)\n", len);
    }
    else {
        err = eeprom_write(eeprom_dev, 0, buf, len + EEPROM_HEADER_SIZE);
        if (err == 0) {
            printf("EEPROM: Data successfully stored.\n");
        }
        else {
            printf("EEPROM: Write error.\n");
        }
    }
    k_mutex_unlock(&eeprom_buf_lock);
}

void eeprom_update()
{
    uint32_t uptime = k_uptime_get() / 1000;
    if (uptime % EEPROM_UPDATE_INTERVAL == 0 && uptime > 0) {
        eeprom_store_data();
    }
}

#else

void eeprom_store_data() {;}
void eeprom_restore_data() {;}
void eeprom_update() {;}

#endif
