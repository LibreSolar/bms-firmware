/* Libre Solar Battery Management System firmware
 * Copyright (c) 2016-2019 Martin Jäger (www.libre.solar)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "config.h"
#include "pcb.h"
#include "helper.h"

#if CONFIG_BMS_BQ76920 || CONFIG_BMS_BQ76930 || CONFIG_BMS_BQ76940

#include "bq769x0_interface.h"
#include "bq769x0_registers.h"

int adc_gain;    // factory-calibrated, read out from chip (uV/LSB)
int adc_offset;  // factory-calibrated, read out from chip (mV)

static bool alert_interrupt_flag;   // indicates if a new current reading or an error is available from BMS IC
static time_t alert_interrupt_timestamp;

#ifndef UNIT_TEST

#ifdef __MBED__

#include "mbed.h"

static I2C bq_i2c(PIN_BMS_SDA, PIN_BMS_SCL);
static InterruptIn alert_interrupt(PIN_BQ_ALERT);

#elif defined(__ZEPHYR__)

#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <string.h>

#define I2C_DEV "I2C_2"

static struct device *i2c_dev;
static struct device *alert_pin_dev;

#endif // MBED or ZEPHYR

static int i2c_address;
static bool crc_enabled;

static uint8_t _crc8_ccitt_update (uint8_t in_crc, uint8_t in_data)
{
    uint8_t data;
    data = in_crc ^ in_data;

    for (uint8_t i = 0; i < 8; i++) {
        if ((data & 0x80) != 0) {
            data <<= 1;
            data ^= 0x07;
        }
        else {
            data <<= 1;
        }
    }
    return data;
}

// The bq769x0 drives the ALERT pin high if the SYS_STAT register contains
// a new value (either new CC reading or an error)
#ifdef __MBED__
void bq769x0_alert_isr()
#else
static void bq769x0_alert_isr(struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
#endif
{
    alert_interrupt_timestamp = uptime();
    alert_interrupt_flag = true;
}

void bq769x0_write_byte(uint8_t reg_addr, uint8_t data)
{
    uint8_t crc = 0;
    uint8_t buf[3];

    buf[0] = reg_addr;
    buf[1] = data;

    if (crc_enabled == true) {
        // CRC is calculated over the slave address (including R/W bit), register address, and data.
        crc = _crc8_ccitt_update(crc, (i2c_address << 1) | 0);
        crc = _crc8_ccitt_update(crc, buf[0]);
        crc = _crc8_ccitt_update(crc, buf[1]);
        buf[2] = crc;
#ifdef __MBED__
        bq_i2c.write(i2c_address << 1, (char *)buf, 3);
#elif defined(__ZEPHYR__)
        i2c_write(i2c_dev, buf, 3, i2c_address);
#endif
    }
    else {
#ifdef __MBED__
        bq_i2c.write(i2c_address << 1, (char *)buf, 2);
#elif defined(__ZEPHYR__)
        i2c_write(i2c_dev, buf, 2, i2c_address);
#endif
    }
}

uint8_t bq769x0_read_byte(uint8_t reg_addr)
{
    uint8_t crc = 0;
    uint8_t buf[2];

    #if BMS_DEBUG
    //printf("Read register: 0x%x \n", address);
    #endif

#ifdef __MBED__
    bq_i2c.write(i2c_address << 1, (char *)&reg_addr, 1);
#elif defined(__ZEPHYR__)
    i2c_write(i2c_dev, &reg_addr, 1, i2c_address);
#endif

    if (crc_enabled == true) {
        do {
#ifdef __MBED__
            bq_i2c.read(i2c_address << 1, (char *)buf, 2);
#elif defined(__ZEPHYR__)
            i2c_read(i2c_dev, buf, 2, i2c_address);
#endif
            // CRC is calculated over the slave address (including R/W bit) and data.
            crc = _crc8_ccitt_update(crc, (i2c_address << 1) | 1);
            crc = _crc8_ccitt_update(crc, buf[0]);
        } while (crc != buf[1]);
        return buf[0];
    }
    else {
#ifdef __MBED__
        bq_i2c.read(i2c_address << 1, (char *)buf, 1);
#elif defined(__ZEPHYR__)
        i2c_read(i2c_dev, buf, 1, i2c_address);
#endif
        return buf[0];
    }
}

int32_t bq769x0_read_word(uint8_t reg_addr)
{
    int32_t val = 0;
    uint8_t buf[4];
    uint8_t crc;

    // write starting register
#ifdef __MBED__
    bq_i2c.write(i2c_address << 1, (char *)&reg_addr, 1);
#elif defined(__ZEPHYR__)
    i2c_write(i2c_dev, &reg_addr, 1, i2c_address);
#endif

    if (crc_enabled == true) {
#ifdef __MBED__
        bq_i2c.read(i2c_address << 1, (char *)buf, 4);
#elif defined(__ZEPHYR__)
        i2c_read(i2c_dev, buf, 4, i2c_address);
#endif

        // CRC of first bytes includes slave address (including R/W bit) and data
        crc = _crc8_ccitt_update(0, (i2c_address << 1) | 1);
        crc = _crc8_ccitt_update(crc, buf[0]);
        if (crc != buf[1]) {
            return -1;
        }

        // CRC of subsequent bytes contain only data
        crc = _crc8_ccitt_update(0, buf[2]);
        if (crc != buf[3]) {
            return -1;
        }

        val = buf[0] << 8 | buf[2];
    }
    else {
#ifdef __MBED__
        bq_i2c.read(i2c_address << 1, (char *)buf, 2);
#elif defined(__ZEPHYR__)
        i2c_read(i2c_dev, buf, 2, i2c_address);
#endif
        val = buf[0] << 8 | buf[1];
    }
    return val;
}

// automatically find out address and CRC setting
static bool determine_address_and_crc(void)
{
    i2c_address = 0x08;
    crc_enabled = true;
    bq769x0_write_byte(CC_CFG, 0x19);
    if (bq769x0_read_byte(CC_CFG) == 0x19) {
        return true;
    }

    i2c_address = 0x18;
    crc_enabled = true;
    bq769x0_write_byte(CC_CFG, 0x19);
    if (bq769x0_read_byte(CC_CFG) == 0x19) {
        return true;
    }

    i2c_address = 0x08;
    crc_enabled = false;
    bq769x0_write_byte(CC_CFG, 0x19);
    if (bq769x0_read_byte(CC_CFG) == 0x19) {
        return true;
    }

    i2c_address = 0x18;
    crc_enabled = false;
    bq769x0_write_byte(CC_CFG, 0x19);
    if (bq769x0_read_byte(CC_CFG) == 0x19) {
        return true;
    }

    return false;
}

void bq769x0_init()
{
#ifdef __MBED__
    alert_interrupt.rise(bq769x0_alert_isr);
#elif defined(__ZEPHYR__)
    alert_pin_dev = device_get_binding(DT_INPUTS_BQ_ALERT_GPIOS_CONTROLLER);
    gpio_pin_configure(alert_pin_dev, DT_INPUTS_BQ_ALERT_GPIOS_PIN, GPIO_INPUT);

    struct gpio_callback gpio_cb;
    gpio_init_callback(&gpio_cb, bq769x0_alert_isr, DT_INPUTS_BQ_ALERT_GPIOS_PIN);
    gpio_add_callback(alert_pin_dev, &gpio_cb);

    //gpio_pin_interrupt_configure(alert_pin_dev, DT_INPUTS_BQ_ALERT_GPIOS_PIN, GPIO_INT_EDGE_RISING);

    i2c_dev = device_get_binding(I2C_DEV);
    if (!i2c_dev) {
        printk("I2C: Device driver not found.\n");
    }
#endif

    alert_interrupt_flag = true;   // init with true to check and clear errors at start-up

    if (determine_address_and_crc())
    {
        // initial settings for bq769x0
        bq769x0_write_byte(SYS_CTRL1, 0b00011000);  // switch external thermistor and ADC on
        bq769x0_write_byte(SYS_CTRL2, 0b01000000);  // switch CC_EN on

        // get ADC offset and gain
        adc_offset = (signed int) bq769x0_read_byte(ADCOFFSET);  // convert from 2's complement
        adc_gain = 365 + (((bq769x0_read_byte(ADCGAIN1) & 0b00001100) << 1) |
            ((bq769x0_read_byte(ADCGAIN2) & 0b11100000) >> 5)); // uV/LSB
    }
    else {
        // TODO: do something else... e.g. set error flag
#if BMS_DEBUG
        printf("BMS communication error\n");
#endif
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
