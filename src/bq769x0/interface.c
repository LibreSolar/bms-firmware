/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pcb.h"
#include "helper.h"

#if CONFIG_BMS_BQ76920 || CONFIG_BMS_BQ76930 || CONFIG_BMS_BQ76940

#include "interface.h"
#include "registers.h"

int adc_gain;    // factory-calibrated, read out from chip (uV/LSB)
int adc_offset;  // factory-calibrated, read out from chip (mV)

// indicates if a new current reading or an error is available from BMS IC
static bool alert_interrupt_flag;
static time_t alert_interrupt_timestamp;

#ifndef UNIT_TEST

#include <string.h>

#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <sys/crc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(bms, CONFIG_LOG_DEFAULT_LEVEL);

#define BQ769X0_INST DT_INST(0, ti_bq769x0)

#define I2C_DEV DT_LABEL(DT_PARENT(BQ769X0_INST))

#define BQ_ALERT_PORT DT_GPIO_LABEL(BQ769X0_INST, alert_gpios)
#define BQ_ALERT_PIN  DT_GPIO_PIN(BQ769X0_INST, alert_gpios)

static const struct device *i2c_dev;
static const struct device *alert_pin_dev;

static int i2c_address;
static bool crc_enabled;

// The bq769x0 drives the ALERT pin high if the SYS_STAT register contains
// a new value (either new CC reading or an error)
static void bq769x0_alert_isr(const struct device*port, struct gpio_callback *cb,
    gpio_port_pins_t pins)
{
    alert_interrupt_timestamp = uptime();
    alert_interrupt_flag = true;
}

void bq769x0_write_byte(uint8_t reg_addr, uint8_t data)
{
    uint8_t buf[4] = {
        i2c_address << 1,       // slave address incl. R/W bit required for CRC calculation
        reg_addr,
        data
    };

    if (crc_enabled) {
        buf[3] = crc8_ccitt(0, buf, 3);
        i2c_write(i2c_dev, buf + 1, 3, i2c_address);
    }
    else {
        i2c_write(i2c_dev, buf + 1, 2, i2c_address);
    }
}

uint8_t bq769x0_read_byte(uint8_t reg_addr)
{
    uint8_t buf[3];

    i2c_write(i2c_dev, &reg_addr, 1, i2c_address);

    if (crc_enabled) {
        // CRC is calculated over the slave address (incl. R/W bit) and data
        buf[0] = (i2c_address << 1) | 1U;
        do {
            i2c_read(i2c_dev, buf + 1, 2, i2c_address);
        } while (crc8_ccitt(0, buf, 2) != buf[2]);

        return buf[1];
    }
    else {
        i2c_read(i2c_dev, buf, 1, i2c_address);
        return buf[0];
    }
}

int32_t bq769x0_read_word(uint8_t reg_addr)
{
    uint8_t buf[5];

    // write starting register
    i2c_write(i2c_dev, &reg_addr, 1, i2c_address);

    if (crc_enabled) {
        // CRC is calculated over the slave address (incl. R/W bit) and data
        buf[0] = (i2c_address << 1) | 1U;
        i2c_read(i2c_dev, buf + 1, 4, i2c_address);

        if (crc8_ccitt(0, buf, 2) != buf[2]) {
            return -1;
        }

        // CRC of subsequent bytes only considering data
        if (crc8_ccitt(0, buf + 3, 1) != buf[4]) {
            return -1;
        }

        return buf[1] << 8 | buf[3];
    }
    else {
        i2c_read(i2c_dev, buf, 2, i2c_address);
        return buf[0] << 8 | buf[1];
    }
}

// automatically find out address and CRC setting
static bool determine_address_and_crc(void)
{
    i2c_address = 0x08;
    crc_enabled = true;
    bq769x0_write_byte(BQ769X0_CC_CFG, 0x19);
    if (bq769x0_read_byte(BQ769X0_CC_CFG) == 0x19) {
        return true;
    }

    i2c_address = 0x18;
    crc_enabled = true;
    bq769x0_write_byte(BQ769X0_CC_CFG, 0x19);
    if (bq769x0_read_byte(BQ769X0_CC_CFG) == 0x19) {
        return true;
    }

    i2c_address = 0x08;
    crc_enabled = false;
    bq769x0_write_byte(BQ769X0_CC_CFG, 0x19);
    if (bq769x0_read_byte(BQ769X0_CC_CFG) == 0x19) {
        return true;
    }

    i2c_address = 0x18;
    crc_enabled = false;
    bq769x0_write_byte(BQ769X0_CC_CFG, 0x19);
    if (bq769x0_read_byte(BQ769X0_CC_CFG) == 0x19) {
        return true;
    }

    return false;
}

void bq769x0_init()
{
    alert_pin_dev = device_get_binding(BQ_ALERT_PORT);
    gpio_pin_configure(alert_pin_dev, BQ_ALERT_PIN, GPIO_INPUT);

    struct gpio_callback gpio_cb;
    gpio_init_callback(&gpio_cb, bq769x0_alert_isr, BQ_ALERT_PIN);
    gpio_add_callback(alert_pin_dev, &gpio_cb);

    i2c_dev = device_get_binding(I2C_DEV);
    if (!i2c_dev) {
        LOG_ERR("I2C device not found");
    }

    alert_interrupt_flag = true;   // init with true to check and clear errors at start-up

    if (determine_address_and_crc())
    {
        // initial settings for bq769x0
        bq769x0_write_byte(BQ769X0_SYS_CTRL1, 0b00011000);  // switch external thermistor and ADC on
        bq769x0_write_byte(BQ769X0_SYS_CTRL2, 0b01000000);  // switch CC_EN on

        // get ADC offset and gain
        adc_offset = (signed int) bq769x0_read_byte(BQ769X0_ADCOFFSET);  // 2's complement
        adc_gain = 365 + (((bq769x0_read_byte(BQ769X0_ADCGAIN1) & 0b00001100) << 1) |
            ((bq769x0_read_byte(BQ769X0_ADCGAIN2) & 0b11100000) >> 5)); // uV/LSB
    }
    else {
        // TODO: do something else... e.g. set error flag
        LOG_ERR("BMS communication error");
    }
}

#endif // UNIT_TEST

bool bq769x0_alert_flag()
{
    return alert_interrupt_flag;
}

void bq769x0_alert_flag_reset()
{
    alert_interrupt_flag = false;
}

time_t bq769x0_alert_timestamp()
{
    return alert_interrupt_timestamp;
}

#endif // defined BQ769x0
