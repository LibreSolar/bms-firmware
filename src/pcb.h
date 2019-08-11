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

#ifndef PCB_H
#define PCB_H

// board-specific confirugation
//----------------------------------------------------------------------------

// 5s BMS with bq76920
// https://github.com/LibreSolar/BMS-5s
#ifdef BMS_PCB_3_5S
#define BMS_BQ76920
#include "pcbs/pcb_3-5s.h"
#endif

// 48V BMS with bq76930
// https://github.com/LibreSolar/BMS48V
#ifdef BMS_PCB_6_10S
#define BMS_BQ76930
#include "pcbs/pcb_6-15s.h"
#endif

// 48V BMS with bq76940
// https://github.com/LibreSolar/BMS48V
#ifdef BMS_PCB_9_15S
#define BMS_BQ76940
#include "pcbs/pcb_6-15s.h"
#endif

#endif // PCB_H
