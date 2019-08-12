/* LibreSolar Battery Management System firmware
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

#ifndef ISL94202_HW
#define ISL94202_HW

/** @file
 *
 * @brief
 * Hardware interface for ISL94202 IC
 */

#include "pcb.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Initialization of ISL94202 IC
 *
 * - Determines I2C address
 * - Sets ALERT pin interrupt
 */
void isl94202_init();

/**
 * Writes one byte to ISL94202 IC registers
 *
 * @param addr The address to read the word from
 * @param data The pointer to where the data should be stored
 * @param num_bytes Number of bytes to read
 *
 * @returns 0 if successful
 */
int isl94202_write_bytes(uint8_t reg_addr, uint8_t *data, uint32_t num_bytes);

/**
 * Read num_bytes from ISL94202 IC registers
 *
 * @param addr The address to read the word from
 * @param data The pointer to where the data should be stored
 * @param num_bytes Number of bytes to read
 *
 * @returns 0 if successful
 */
int isl94202_read_bytes(uint8_t reg_addr, uint8_t *data, uint32_t num_bytes);

/**
 * \returns status of the alert pin
 */
bool isl94202_alert_flag();

/**
 * Reset alert pin interrupt flag
 */
void isl94202_alert_flag_reset();

/**
 * Returns the time when the interrupt was triggered
 */
time_t isl94202_alert_timestamp();

#ifdef __cplusplus
}
#endif

#endif // ISL94202_HW
