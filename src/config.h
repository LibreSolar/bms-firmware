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

#include "bms.h"

// select BMS board
#define BMS_PCB_5S
//#define BMS_PCB_48V

#define ADC_AVG_SAMPLES 8       // number of ADC values to read for averaging

// general configuration
//----------------------------------------------------------------------------

#define CAN_SPEED 250
#define CAN_NODE_ID 0

// board-specific confirugation
//----------------------------------------------------------------------------

// 5s BMS with bq76920
// https://github.com/LibreSolar/BMS-5s
#ifdef BMS_PCB_5S
#define BMS_BQ_TYPE bq76920
#include "config_5s.h"
#endif // BMS_PCB_5S

// 48V BMS with bq76930 or bq76940
// https://github.com/LibreSolar/BMS48V
#ifdef BMS_PCB_48V
#define BMS_BQ_TYPE bq76930
#include "config_48v.h"
#endif // BMS_PCB_48V

#endif // CONFIG_H
