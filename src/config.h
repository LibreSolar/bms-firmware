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

#define DEVICE_ID 12345678

#define ADC_AVG_SAMPLES 8       // number of ADC values to read for averaging

// UEXT interfaces
//----------------------------------------------------------------------------

/// OLED display at UEXT port
#define OLED_ENABLED

/// DOG LCD display at UEXT port
//#define DOGLCD_ENABLED

// Empty UEXT port
//#define UEXT_DUMMY_ENABLED

// general configuration
//----------------------------------------------------------------------------

#define CAN_SPEED 250
#define CAN_NODE_ID 0

#endif // CONFIG_H
