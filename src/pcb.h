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

#include <zephyr.h>

#define NUM_CELLS_MAX           DT_PROP(DT_PATH(pcb), num_cells_max)
#define NUM_THERMISTORS_MAX     DT_PROP(DT_PATH(pcb), num_thermistors_max)

#define PCB_MAX_CURRENT         DT_PROP(DT_PATH(pcb), current_max)
#define SHUNT_RESISTOR          (DT_PROP(DT_PATH(pcb), shunt_res) / 1000.0)

#endif // PCB_H
