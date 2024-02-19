/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_BMS_IC_ISL94202_INTERFACE_H_
#define DRIVERS_BMS_IC_ISL94202_INTERFACE_H_

/** @file
 *
 * @brief
 * Hardware interface for ISL94202 IC
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>

#include <stdbool.h>
#include <stdint.h>

/**
 * Write multiple bytes to ISL94202 IC registers
 *
 * @param reg_addr The address to write to
 * @param data The pointer to the data buffer
 * @param num_bytes Number of bytes to write
 *
 * @returns 0 on success, otherwise negative error code.
 */
int isl94202_write_bytes(const struct device *dev, uint8_t reg_addr, uint8_t *data,
                         uint32_t num_bytes);

/**
 * Write a word (two bytes) to ISL94202 IC registers
 *
 * @param addr The address to write to
 * @param word The word to be written
 *
 * @returns 0 on success, otherwise negative error code.
 */
int isl94202_write_word(const struct device *dev, uint8_t reg_addr, uint16_t word);

/**
 * Read multiple bytes from ISL94202 IC registers
 *
 * @param addr The address to read the bytes from
 * @param data The pointer to where the data should be stored
 * @param num_bytes Number of bytes to read
 *
 * @returns 0 on success, otherwise negative error code.
 */
int isl94202_read_bytes(const struct device *dev, uint8_t reg_addr, uint8_t *data,
                        uint32_t num_bytes);

/**
 * Read a word from ISL94202 IC registers
 *
 * @param addr The address of the word
 * @param value Pointer to the variable to store the result
 *
 * @returns 0 on success, otherwise negative error code.
 */
int isl94202_read_word(const struct device *dev, uint8_t reg_addr, uint16_t *value);

/**
 * Write a delay + extra bits to specified register
 *
 * @param reg_addr Register address
 * @param delay_unit Unit (us, ms, s or min) of the threshold value
 * @param delay_value Value of the delay in the given unit
 * @param extra_bits Four extra bits C-F
 *
 * @returns 0 on success, otherwise negative error code.
 */
int isl94202_write_delay(const struct device *dev, uint8_t reg_addr, uint8_t delay_unit,
                         uint16_t delay_value, uint8_t extra_bits);

/**
 * Write a current limit (threshold + delay) to specified register
 *
 * @param reg_addr Register address
 * @param voltage_thresholds Array of threshold values as defined in datasheet (mV)
 * @param num_thresholds Number of elements in array voltage_thresholds
 * @param current_limit Current limit (A)
 * @param shunt_res_mOhm Resistance of the current measurement shunt (mOhm)
 * @param delay_unit Unit (us, ms, s or min) of the threshold value
 * @param delay_value Value of the delay in the given unit
 *
 * @returns Actual threshold current in A or 0 in case of error
 */
float isl94202_write_current_limit(const struct device *dev, uint8_t reg_addr,
                                   const uint16_t *voltage_thresholds_mV, int num_thresholds,
                                   float current_limit, float shunt_res_mOhm, uint8_t delay_unit,
                                   uint16_t delay_value);

/**
 * Write a voltage setting to specified register
 *
 * @param reg_addr Register address
 * @param voltage Voltage setting (V)
 * @param extra_bits Four extra bits left of voltage setting, set to 0 if not applicable
 *
 * @returns 0 on success, otherwise negative error code.
 */
int isl94202_write_voltage(const struct device *dev, uint8_t reg_addr, float voltage,
                           uint8_t extra_bits);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_BMS_IC_ISL94202_INTERFACE_H_ */
