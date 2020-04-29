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

#ifndef BQ769X0_INTERFACE_H
#define BQ769X0_INTERFACE_H

/** @file
 *
 * @brief
 * Hardware interface for bq769x0 IC
 */

#include "pcb.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <stdbool.h>

/**
 * Initialization of bq769x0 IC
 *
 * - Determines I2C address
 * - Sets ALERT pin interrupt
 */
void bq769x0_init();

/**
 * Writes one byte to bq769x0 IC
 */
void bq769x0_write_byte(uint8_t reg_addr, uint8_t data);

/**
 * Read 1 byte from bq769x0 IC
 */
uint8_t bq769x0_read_byte(uint8_t reg_addr);

/**
 * Read 16-bit word (two bytes) from bq769x0 IC
 *
 * @returns the (unsigned) word or -1 in case of CRC error
 */
int32_t bq769x0_read_word(uint8_t reg_addr);

/**
 * \returns status of the alert pin
 */
bool bq769x0_alert_flag();

/**
 * Reset alert pin interrupt flag
 */
void bq769x0_alert_flag_reset();

/**
 * Returns the time when the interrupt was triggered
 */
time_t bq769x0_alert_timestamp();

#ifdef __cplusplus
}
#endif

#endif // BQ769X0_INTERFACE_H
