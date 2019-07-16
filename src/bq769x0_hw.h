/* Battery management system based on bq769x0 for ARM mbed
 * Copyright (c) 2015-2018 Martin Jäger (www.libre.solar)
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

#ifndef BQ769X0_HW
#define BQ769X0_HW

#include "pcb.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

void bq769x0_init();
void bq769x0_write_byte(int address, int data);
int bq769x0_read_byte(int address);

/**
 * Read 16-bit word (two bytes) from bq769x0 IC
 *
 * @returns the (unsigned) word or -1 in case of CRC error
 */
int bq769x0_read_word(char reg);

bool bq769x0_alert_flag();
void bq769x0_alert_flag_reset();
time_t bq769x0_alert_timestamp();

#ifdef __cplusplus
}
#endif

#endif // BQ769X0_HW
