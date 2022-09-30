/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BOARD_H_
#define BOARD_H_

#include <zephyr/kernel.h>

#define BOARD_NUM_CELLS_MAX       DT_PROP(DT_PATH(pcb), num_cells_max)
#define BOARD_NUM_THERMISTORS_MAX DT_PROP(DT_PATH(pcb), num_thermistors_max)

#define BOARD_MAX_CURRENT    DT_PROP(DT_PATH(pcb), current_max)
#define BOARD_SHUNT_RESISTOR (DT_PROP(DT_PATH(pcb), shunt_res) / 1000.0)

#endif // BOARD_H_
