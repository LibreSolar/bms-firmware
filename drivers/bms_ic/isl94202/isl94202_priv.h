/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_BMS_IC_BMS_IC_ISL94202_PRIV_H_
#define DRIVERS_BMS_IC_BMS_IC_ISL94202_PRIV_H_

/**
 * @file
 * @brief Private functions and definitions for isl94202 IC driver
 */

#include <drivers/bms_ic.h>

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

/* read-only driver configuration */
struct bms_ic_isl94202_config
{
    struct i2c_dt_spec i2c;
    struct gpio_dt_spec i2c_pullup;
    uint32_t shunt_resistor_uohm;
    uint32_t board_max_current;
    uint8_t used_cell_channels;
};

/* driver run-time data */
struct bms_ic_isl94202_data
{
    struct bms_ic_data *ic_data;
    const struct device *dev;
    struct k_work_delayable balancing_work;
    uint8_t fet_state;
    bool auto_balancing;
};

#endif /* DRIVERS_BMS_IC_BMS_IC_ISL94202_PRIV_H_ */
