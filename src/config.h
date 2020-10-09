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

#ifndef CONFIG_H
#define CONFIG_H

#define ADC_AVG_SAMPLES 8       // number of ADC values to read for averaging

// UEXT interfaces
//----------------------------------------------------------------------------

/// OLED display at UEXT port
#define CONFIG_EXT_OLED_DISPLAY 1

// general configuration
//----------------------------------------------------------------------------

#ifdef UNIT_TEST

#define CONFIG_THINGSET_EXPERT_PASSWORD "expert123"
#define CONFIG_THINGSET_MAKER_PASSWORD "maker456"

#define CONFIG_THINGSET_SERIAL 1
#define CONFIG_THINGSET_SERIAL_TX_BUF_SIZE 500
#define CONFIG_THINGSET_SERIAL_RX_BUF_SIZE 500

#define CONFIG_THINGSET_CAN 1
#define CONFIG_THINGSET_CAN_DEFAULT_NODE_ID 20

#endif

#define CAN_SPEED 250
#define CAN_NODE_ID 0

#endif // CONFIG_H
