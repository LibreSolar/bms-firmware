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

#ifndef BMS_8S50_IC_H
#define BMS_8S50_IC_H

#define DEVICE_TYPE "BMS-8S50-IC"
#define HARDWARE_VERSION "v0.1"

#define BMS_ISL94202

#define ISL94202_I2C_ADDRESS (0x50 >> 1)

#define SHUNT_RESISTOR 1.0  // mOhm
#define PCB_MAX_CURRENT 50  // A

#define NUM_CELLS_MAX 8
#define NUM_THERMISTORS_MAX 2
#define MOSFET_TEMP_SENSOR

#define PIN_V_EXT     PA_5
#define PIN_TEMP_1    PA_6
#define PIN_TEMP_2    PA_7

#endif
