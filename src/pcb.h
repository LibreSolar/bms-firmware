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

#ifndef UNIT_TEST
#include <zephyr.h>
#endif

/*
 * Board-specific configuration
 */

// 12V BMS with bq76920
// https://github.com/LibreSolar/bms-5s50-sc
#ifdef CONFIG_BOARD_BMS_5S50_SC
#include "pcbs/bms_5s50_sc.h"
#endif

// 24 to 48V BMS with bq76930/40
// https://github.com/LibreSolar/bms-15s80-sc
#ifdef CONFIG_BOARD_BMS_15S80_SC
#include "pcbs/bms_15s80_sc.h"
#endif

// 24V BMS with ISL94202
// https://github.com/LibreSolar/bms-8s50-ic
#if defined(CONFIG_BOARD_BMS_8S50_IC_F072) || defined(CONFIG_BOARD_BMS_8S50_IC_L452)
#include "pcbs/bms_8s50_ic.h"
#endif

#endif // PCB_H
