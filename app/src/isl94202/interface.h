/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ISL94202_INTERFACE_H
#define ISL94202_INTERFACE_H

/** @file
 *
 * @brief
 * Hardware interface for ISL94202 IC
 */

#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/**
 * Initialization of ISL94202 IC
 */
void isl94202_init();

/**
 * Writes multiple bytes to ISL94202 IC registers
 *
 * @param reg_addr The address to write to
 * @param data The pointer to the data buffer
 * @param num_bytes Number of bytes to write
 *
 * @returns 0 if successful
 */
int isl94202_write_bytes(uint8_t reg_addr, uint8_t *data, uint32_t num_bytes);

/**
 * Writes a word (two bytes) to ISL94202 IC registers
 *
 * @param addr The address to write to
 * @param word The word to be written
 *
 * @returns 0 if successful
 */
int isl94202_write_word(uint8_t reg_addr, uint16_t word);

/**
 * Reads multiple bytes from ISL94202 IC registers
 *
 * @param addr The address to read the bytes from
 * @param data The pointer to where the data should be stored
 * @param num_bytes Number of bytes to read
 *
 * @returns 0 if successful
 */
int isl94202_read_bytes(uint8_t reg_addr, uint8_t *data, uint32_t num_bytes);

/**
 * Reads a word from ISL94202 IC registers
 *
 * @param addr The address of the word
 *
 * @returns the (unsigned) word or -1 in case of CRC error
 */
int isl94202_read_word(uint8_t reg_addr);

/**
 * Writes a delay + extra bits to specified register
 *
 * @param reg_addr Register address
 * @param delay_unit Unit (us, ms, s or min) of the threshold value
 * @param delay_value Value of the delay in the given unit
 * @param extra_bits Four extra bits C-F
 *
 * @returns Actual threshold current in A or 0 in case of error
 */
int isl94202_write_delay(uint8_t reg_addr, uint8_t delay_unit, uint16_t delay_value,
                         uint8_t extra_bits);

/**
 * Writes a current limit (threshold + delay) to specified register
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
float isl94202_write_current_limit(uint8_t reg_addr, const uint16_t *voltage_thresholds_mV,
                                   int num_thresholds, float current_limit, float shunt_res_mOhm,
                                   uint8_t delay_unit, uint16_t delay_value);

/**
 * Writes a voltage setting to specified register
 *
 * @param reg_addr Register address
 * @param voltage Voltage setting (V)
 * @param extra_bits Four extra bits left of voltage setting, set to 0 if not applicable
 *
 * @returns 1 for success or 0 in case of error
 */
int isl94202_write_voltage(uint8_t reg_addr, float voltage, uint8_t extra_bits);

#ifdef __cplusplus
}
#endif

#endif // ISL94202_INTERFACE_H
