/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "board.h"
#include "helper.h"

#include "interface.h"
#include "registers.h"

#include <stdio.h>

#ifndef UNIT_TEST

#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <string.h>
#include <zephyr.h>

LOG_MODULE_REGISTER(isl94202_if, CONFIG_LOG_DEFAULT_LEVEL);

#define I2C_DEV     DT_LABEL(DT_PARENT(DT_INST(0, renesas_isl94202)))
#define I2C_ADDRESS DT_REG_ADDR(DT_INST(0, renesas_isl94202))

#define I2C_PULLUP_GPIO DT_CHILD(DT_PATH(switches), i2c_pullup)
#define I2C_PULLUP_PORT DT_GPIO_LABEL(I2C_PULLUP_GPIO, gpios)
#define I2C_PULLUP_PIN  DT_GPIO_PIN(I2C_PULLUP_GPIO, gpios)

static const struct device *i2c_dev;

int isl94202_write_bytes(uint8_t reg_addr, uint8_t *data, uint32_t num_bytes)
{
    uint8_t buf[5];
    if ((reg_addr > 0x58 && reg_addr < 0x7F) || reg_addr + num_bytes > 0xAB || num_bytes > 4)
        return -1;

    buf[0] = reg_addr; // first byte contains register address
    memcpy(buf + 1, data, num_bytes);

    return i2c_write(i2c_dev, buf, num_bytes + 1, I2C_ADDRESS);
}

int isl94202_read_bytes(uint8_t reg_addr, uint8_t *data, uint32_t num_bytes)
{
    return i2c_write_read(i2c_dev, I2C_ADDRESS, &reg_addr, 1, data, num_bytes);
}

int isl94202_init()
{
    // activate pull-up at I2C SDA and SCL
    const struct device *i2c_pullup;
    i2c_pullup = device_get_binding(I2C_PULLUP_PORT);
    gpio_pin_configure(i2c_pullup, I2C_PULLUP_PIN, GPIO_OUTPUT);
    gpio_pin_set(i2c_pullup, I2C_PULLUP_PIN, 1);

    i2c_dev = device_get_binding(I2C_DEV);
    if (!i2c_dev) {
        LOG_ERR("I2C device not found");
        return -ENODEV;
    }

    return 0;
}

#endif // UNIT_TEST

int isl94202_write_word(uint8_t reg_addr, uint16_t word)
{
    uint8_t buf[2];
    buf[0] = word;
    buf[1] = word >> 8;
    return isl94202_write_bytes(reg_addr, buf, 2);
}

int isl94202_write_delay(uint8_t reg_addr, uint8_t delay_unit, uint16_t delay_value,
                         uint8_t extra_bits)
{
    if (delay_unit > ISL94202_DELAY_MIN || extra_bits > 0xF) {
        return -1;
    }

    uint8_t unit_final = delay_unit;
    uint16_t value_final = delay_value;
    if (value_final > 1023) {
        if (unit_final < ISL94202_DELAY_S) {
            value_final = value_final / 1000;
        }
        else if (unit_final == ISL94202_DELAY_S) {
            value_final = value_final / 60;
        }
        else {
            return -1;
        }
        unit_final++;
    }

    // delay value: bits 0-9, unit: bits A-B, extra bits: C-E
    uint16_t reg = value_final | (unit_final << 0xA) | (extra_bits << 0xC);

    return isl94202_write_word(reg_addr, reg);
}

float isl94202_write_current_limit(uint8_t reg_addr, const uint16_t *voltage_thresholds_mV,
                                   int num_thresholds, float current_limit, float shunt_res_mOhm,
                                   uint8_t delay_unit, uint16_t delay_value)
{
    uint8_t threshold_raw = 0;
    float actual_current_limit =
        voltage_thresholds_mV[0] / shunt_res_mOhm; // initialize with lowest value

    // choose lower current limit if target setting not exactly possible
    for (int i = num_thresholds - 1; i >= 0; i--) {
        if ((uint16_t)(current_limit * shunt_res_mOhm) >= voltage_thresholds_mV[i]) {
            threshold_raw = (uint8_t)i;
            actual_current_limit = voltage_thresholds_mV[i] / shunt_res_mOhm;
            break;
        }
    }
    isl94202_write_delay(reg_addr, delay_unit, delay_value, threshold_raw);

    return actual_current_limit;
}

int isl94202_write_voltage(uint8_t reg_addr, float voltage, uint8_t extra_bits)
{
    uint16_t reg = extra_bits << 12;

    uint16_t voltage_raw = voltage * 4095 * 3 / 1.8 / 8;

    if (voltage_raw > 0x0FFF) {
        return -1;
    }

    reg |= (voltage_raw & 0x0FFF);

    return isl94202_write_word(reg_addr, reg);
}

int isl94202_read_word(uint8_t reg_addr, uint16_t *value)
{
    uint8_t buf[2] = { 0 };
    if (isl94202_read_bytes(reg_addr, buf, 2) == 0) {
        *value = buf[0] + (buf[1] << 8);
        return 0;
    }
    else {
        return -1;
    }
}
