/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BQ769X2_INTERFACE_H
#define BQ769X2_INTERFACE_H

/** @file
 *
 * @brief
 * Hardware interface for bq769x2 IC
 */

#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

uint8_t bq769x2_checksum_add(uint8_t previous, uint8_t data);

/**
 * Initialization of bq769x2 IC
 */
int bq769x2_init();

/**
 * Set bq769x2 config update mode
 *
 * @param config_update True if config update mode should be entered
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_config_update_mode(bool config_update);

/**
 * Writes multiple bytes to bq769x2 IC registers
 *
 * @param reg_addr The address to write to
 * @param data The pointer to the data buffer
 * @param num_bytes Number of bytes to write
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_write_bytes(const uint8_t reg_addr, const uint8_t *data, const size_t num_bytes);

/**
 * Reads multiple bytes from bq769x2 IC registers
 *
 * @param reg_addr The address to read the bytes from
 * @param data The pointer to where the data should be stored
 * @param num_bytes Number of bytes to read
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_read_bytes(const uint8_t reg_addr, uint8_t *data, const size_t num_bytes);

/**
 * Read 16-bit unsigned integer via direct command from bq769x2 IC
 *
 * @param reg_addr The address to read the bytes from
 * @param value Pointer to where the value should be stored
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_direct_read_u2(const uint8_t reg_addr, uint16_t *value);

/**
 * Read 16-bit integer via direct command from bq769x2 IC
 *
 * @param reg_addr The address to read the bytes from
 * @param value Pointer to where the value should be stored
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_direct_read_i2(const uint8_t reg_addr, int16_t *value);

/**
 * Execute subcommand without data (command-only) in bq769x2 IC
 *
 * @param subcmd The subcommand to execute
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_subcmd_cmd_only(const uint16_t subcmd);

/**
 * Read 8-bit unsigned integer via subcommand from bq769x2 IC
 *
 * @param subcmd The subcommand to read the bytes from
 * @param value Pointer to where the value should be stored
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_subcmd_read_u1(const uint16_t subcmd, uint8_t *value);

/**
 * Read 16-bit unsigned integer via subcommand from bq769x2 IC
 *
 * @param subcmd The subcommand to read the bytes from
 * @param value Pointer to where the value should be stored
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_subcmd_read_u2(const uint16_t subcmd, uint16_t *value);

/**
 * Read 32-bit unsigned integer via subcommand from bq769x2 IC
 *
 * @param subcmd The subcommand to read the bytes from
 * @param value Pointer to where the value should be stored
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_subcmd_read_u4(const uint16_t subcmd, uint32_t *value);

/**
 * Read 8-bit signed integer via subcommand from bq769x2 IC
 *
 * @param subcmd The subcommand to read the bytes from
 * @param value Pointer to where the value should be stored
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_subcmd_read_i1(const uint16_t subcmd, int8_t *value);

/**
 * Read 16-bit signed integer via subcommand from bq769x2 IC
 *
 * @param subcmd The subcommand to read the bytes from
 * @param value Pointer to where the value should be stored
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_subcmd_read_i2(const uint16_t subcmd, int16_t *value);

/**
 * Read 32-bit signed integer via subcommand from bq769x2 IC
 *
 * @param subcmd The subcommand to read the bytes from
 * @param value Pointer to where the value should be stored
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_subcmd_read_i4(const uint16_t subcmd, int32_t *value);

/**
 * Read 32-bit float via subcommand from bq769x2 IC
 *
 * @param subcmd The subcommand to read the bytes from
 * @param value Pointer to where the value should be stored
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_subcmd_read_f4(const uint16_t subcmd, float *value);

/**
 * Write 8-bit unsigned integer via subcommand to bq769x2 IC
 *
 * @param subcmd The subcommand to write the bytes to
 * @param value Value that should be written
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_subcmd_write_u1(const uint16_t subcmd, uint8_t value);

/**
 * Write 16-bit unsigned integer via subcommand to bq769x2 IC
 *
 * @param subcmd The subcommand to write the bytes to
 * @param value Value that should be written
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_subcmd_write_u2(const uint16_t subcmd, uint16_t value);

/**
 * Write 32-bit unsigned integer via subcommand to bq769x2 IC
 *
 * @param subcmd The subcommand to write the bytes to
 * @param value Value that should be written
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_subcmd_write_u4(const uint16_t subcmd, uint32_t value);

/**
 * Write 8-bit signed integer via subcommand to bq769x2 IC
 *
 * @param subcmd The subcommand to write the bytes to
 * @param value Value that should be written
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_subcmd_write_i1(const uint16_t subcmd, int8_t value);

/**
 * Write 16-bit signed integer via subcommand to bq769x2 IC
 *
 * @param subcmd The subcommand to write the bytes to
 * @param value Value that should be written
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_subcmd_write_i2(const uint16_t subcmd, int16_t value);

/**
 * Write 32-bit signed integer via subcommand to bq769x2 IC
 *
 * @param subcmd The subcommand to write the bytes to
 * @param value Value that should be written
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_subcmd_write_i4(const uint16_t subcmd, int32_t value);

/**
 * Write 32-bit float via subcommand to bq769x2 IC
 *
 * @param subcmd The subcommand to write the bytes to
 * @param value Value that should be written
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_subcmd_write_f4(const uint16_t subcmd, float value);

/**
 * Read 8-bit unsigned integer from bq769x2 data memory
 *
 * @param reg_addr The data memory register address to read the value from
 * @param value Pointer to where the value should be stored
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_datamem_read_u1(const uint16_t reg_addr, uint8_t *value);

/**
 * Write 8-bit unsigned integer to bq769x2 data memory
 *
 * @param reg_addr The data memory register address to write the bytes to
 * @param value Value that should be written
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_datamem_write_u1(const uint16_t reg_addr, uint8_t value);

/**
 * Write 16-bit unsigned integer to bq769x2 data memory
 *
 * @param reg_addr The data memory register address to write the bytes to
 * @param value Value that should be written
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_datamem_write_u2(const uint16_t reg_addr, uint16_t value);

/**
 * Write 8-bit signed integer to bq769x2 data memory
 *
 * @param reg_addr The data memory register address to write the bytes to
 * @param value Value that should be written
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_datamem_write_i1(const uint16_t reg_addr, int8_t value);

/**
 * Write 16-bit signed integer to bq769x2 data memory
 *
 * @param reg_addr The data memory register address to write the bytes to
 * @param value Value that should be written
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_datamem_write_i2(const uint16_t reg_addr, int16_t value);

/**
 * Write 32-bit float to bq769x2 data memory
 *
 * @param reg_addr The data memory register address to write the bytes to
 * @param value Value that should be written
 *
 * @returns 0 if successful, negative errno otherwise
 */
int bq769x2_datamem_write_f4(const uint16_t reg_addr, float value);

#ifdef __cplusplus
}
#endif

#endif // BQ769X2_INTERFACE_H
