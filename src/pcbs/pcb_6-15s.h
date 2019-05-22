/* LibreSolar BMS 5s software
 * Copyright (c) 2016-2017 Martin Jäger (www.libre.solar)
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

#ifndef CONFIG_6_15S_H
#define CONFIG_6_15S_H

#define SHUNT_RESISTOR 1.5  // mOhm

#ifdef BMS_BQ76930
#define NUM_CELLS_MAX 10
#define NUM_THERMISTORS_MAX 2
#else
#define NUM_CELLS_MAX 15
#define NUM_THERMISTORS_MAX 3
#endif

#define PIN_UEXT_TX   PA_2
#define PIN_UEXT_RX   PA_3
#define PIN_UEXT_SCL  PB_6
#define PIN_UEXT_SDA  PB_7
#define PIN_UEXT_MISO PB_4
#define PIN_UEXT_MOSI PB_5
#define PIN_UEXT_SCK  PB_3
#define PIN_UEXT_SSEL PA_1

#define PIN_SWD_TX    PB_10     // changed in BMS-5s
#define PIN_SWD_RX    PB_11     // changed in BMS-5s

#define PIN_CAN_RX    PB_8
#define PIN_CAN_TX    PB_9
#define PIN_CAN_STB   PA_15

#define PIN_LED_RED   PA_9     // changed in BMS-5s
#define PIN_LED_GREEN PA_10    // changed in BMS-5s

#define PIN_BMS_SCL    PB_13    // changed in BMS-5s
#define PIN_BMS_SDA    PB_14    // changed in BMS-5s
#define PIN_BQ_ALERT  PB_12
#define PIN_PCHG_EN   PB_2
#define PIN_SW_POWER  PA_8

#define PIN_V_REF    PA_0      // not existing in 48V board
#define PIN_V_BAT    PA_4
#define PIN_V_LOAD   PA_5
#define PIN_TEMP_1   PA_6      // not existing in 48V board
#define PIN_TEMP_2   PA_7      // not existing in 48V board

#endif
