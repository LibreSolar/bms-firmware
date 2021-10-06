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

#include "pcb.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

/**
 * Initialization of bq769x2 IC
 */
void bq769x2_init();

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

#ifdef __cplusplus
}
#endif

#endif // BQ769X2_INTERFACE_H
