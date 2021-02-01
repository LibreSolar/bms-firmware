/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PCB_H
#define PCB_H

#include <zephyr.h>

#define NUM_CELLS_MAX           DT_PROP(DT_PATH(pcb), num_cells_max)
#define NUM_THERMISTORS_MAX     DT_PROP(DT_PATH(pcb), num_thermistors_max)

#define PCB_MAX_CURRENT         DT_PROP(DT_PATH(pcb), current_max)
#define SHUNT_RESISTOR          (DT_PROP(DT_PATH(pcb), shunt_res) / 1000.0)

#endif // PCB_H
