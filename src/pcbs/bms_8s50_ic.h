/* LibreSolar Battery Management System firmware
 * Copyright (c) 2016-2018 Martin Jäger (www.libre.solar)
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

#ifndef BMS_8S50_IC_H
#define BMS_8S50_IC_H

#define BMS_ISL94202

#define ISL94202_I2C_ADDRESS (0x50 >> 1)

#define SHUNT_RESISTOR 1.0  // mOhm

#define NUM_CELLS_MAX 8
#define NUM_THERMISTORS_MAX 2

#define PIN_UEXT_TX   PA_2
#define PIN_UEXT_RX   PA_3
#define PIN_UEXT_SCL  PB_6
#define PIN_UEXT_SDA  PB_7
#define PIN_UEXT_MISO PB_4
#define PIN_UEXT_MOSI PB_5
#define PIN_UEXT_SCK  PB_3
#define PIN_UEXT_SSEL PA_15

#define PIN_SWD_TX    PA_9
#define PIN_SWD_RX    PA_10

#define PIN_CAN_RX    PB_8
#define PIN_CAN_TX    PB_9
#define PIN_CAN_STB   PC_13

#define PIN_LED_RED   PB_14
#define PIN_LED_GREEN PB_15

#define PIN_BMS_SCL   PB_10
#define PIN_BMS_SDA   PB_11
#define PIN_I2C_PU    PB_2
#define PIN_ALERT_IN  PB_12
#define PIN_SW_POWER  PA_8

#define PIN_V_EXT     PA_5
#define PIN_TEMP_1    PA_6
#define PIN_TEMP_2    PA_7

#endif
