/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_BMS_IC_BMS_IC_BQ769X2_PRIV_H_
#define DRIVERS_BMS_IC_BMS_IC_BQ769X2_PRIV_H_

/** @file
 *
 * @brief
 * Private functions and definitions for bq769x2 IC driver
 */

#include <drivers/bms_ic.h>

#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

/**
 * Writes multiple bytes to bq769x2 IC registers
 *
 * @param reg_addr The address to write to
 * @param data The pointer to the data buffer
 * @param num_bytes Number of bytes to write
 *
 * @returns 0 if successful, negative errno otherwise
 */
typedef int (*bq769x2_write_bytes_t)(const struct device *dev, const uint8_t reg_addr,
                                     const uint8_t *data, const size_t num_bytes);

/**
 * Reads multiple bytes from bq769x2 IC registers
 *
 * @param reg_addr The address to read the bytes from
 * @param data The pointer to where the data should be stored
 * @param num_bytes Number of bytes to read
 *
 * @returns 0 if successful, negative errno otherwise
 */
typedef int (*bq769x2_read_bytes_t)(const struct device *dev, const uint8_t reg_addr, uint8_t *data,
                                    const size_t num_bytes);

/* read-only driver configuration */
struct bms_ic_bq769x2_config
{
    struct i2c_dt_spec i2c;
    struct gpio_dt_spec alert_gpio;
    uint32_t shunt_resistor_uohm;
    uint32_t board_max_current;
    uint16_t used_cell_channels;
    uint8_t pin_config[9];
    uint8_t cell_temp_pins[CONFIG_BMS_IC_MAX_THERMISTORS];
    uint8_t num_cell_temps;
    uint8_t fet_temp_pin;
    bool crc_enabled;
    bool auto_pdsg;
    uint8_t reg12_config;
    bq769x2_write_bytes_t write_bytes;
    bq769x2_read_bytes_t read_bytes;
};

/* driver run-time data */
struct bms_ic_bq769x2_data
{
    struct bms_ic_data *ic_data;
    bool config_update_mode_enabled;
    bool auto_balancing;
};

#endif /* DRIVERS_BMS_IC_BMS_IC_BQ769X2_PRIV_H_ */
