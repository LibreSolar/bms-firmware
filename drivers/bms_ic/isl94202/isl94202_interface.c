/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "isl94202_interface.h"
#include "isl94202_priv.h"
#include "isl94202_registers.h"

#include <stdint.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(isl94202_if, CONFIG_LOG_DEFAULT_LEVEL);

int isl94202_write_bytes(const struct device *dev, uint8_t reg_addr, uint8_t *data,
                         uint32_t num_bytes)
{
    const struct bms_ic_isl94202_config *dev_config = dev->config;

    uint8_t buf[5];
    if ((reg_addr > 0x58 && reg_addr < 0x7F) || reg_addr + num_bytes > 0xAB || num_bytes > 4)
        return -1;

    buf[0] = reg_addr; // first byte contains register address
    memcpy(buf + 1, data, num_bytes);

    return i2c_write_dt(&dev_config->i2c, buf, num_bytes + 1);
}

int isl94202_read_bytes(const struct device *dev, uint8_t reg_addr, uint8_t *data,
                        uint32_t num_bytes)
{
    const struct bms_ic_isl94202_config *dev_config = dev->config;

    return i2c_write_read_dt(&dev_config->i2c, &reg_addr, 1, data, num_bytes);
}

int isl94202_write_word(const struct device *dev, uint8_t reg_addr, uint16_t word)
{
    uint8_t buf[2] = { word, word >> 8 };

    return isl94202_write_bytes(dev, reg_addr, buf, 2);
}

int isl94202_write_delay(const struct device *dev, uint8_t reg_addr, uint8_t delay_unit,
                         uint16_t delay_value, uint8_t extra_bits)
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

    /* delay value: bits 0-9, unit: bits A-B, extra bits: C-E */
    uint16_t reg = value_final | (unit_final << 0xA) | (extra_bits << 0xC);

    return isl94202_write_word(dev, reg_addr, reg);
}

int isl94202_write_current_limit(const struct device *dev, uint8_t reg_addr,
                                 const uint16_t *voltage_thresholds_mv, int num_thresholds,
                                 float *current_limit, float shunt_res_mohm, uint8_t delay_unit,
                                 uint16_t delay_value)
{
    uint8_t threshold_raw = 0;
    float actual_current_limit;
    int err;

    if (current_limit == NULL) {
        return -EINVAL;
    }

    /* initialize with lowest value */
    actual_current_limit = voltage_thresholds_mv[0] / shunt_res_mohm;

    /* choose lower current limit if target setting not exactly possible */
    for (int i = num_thresholds - 1; i >= 0; i--) {
        if ((uint16_t)(*current_limit * shunt_res_mohm) >= voltage_thresholds_mv[i]) {
            threshold_raw = (uint8_t)i;
            actual_current_limit = voltage_thresholds_mv[i] / shunt_res_mohm;
            break;
        }
    }

    err = isl94202_write_delay(dev, reg_addr, delay_unit, delay_value, threshold_raw);
    if (err == 0) {
        *current_limit = actual_current_limit;
    }

    return err;
}

int isl94202_write_voltage(const struct device *dev, uint8_t reg_addr, float voltage,
                           uint8_t extra_bits)
{
    uint16_t reg = extra_bits << 12;

    uint16_t voltage_raw = voltage * 4095 * 3 / 1.8F / 8;

    if (voltage_raw > 0x0FFF) {
        return -1;
    }

    reg |= (voltage_raw & 0x0FFF);

    return isl94202_write_word(dev, reg_addr, reg);
}

int isl94202_read_word(const struct device *dev, uint8_t reg_addr, uint16_t *value)
{
    uint8_t buf[2] = { 0 };
    if (isl94202_read_bytes(dev, reg_addr, buf, 2) == 0) {
        *value = buf[0] + (buf[1] << 8);
        return 0;
    }
    else {
        return -1;
    }
}
