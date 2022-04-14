/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BQ769X2_TESTS_H_
#define BQ769X2_TESTS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*
 * Memory layout of bq769x2 for direct commands
 *
 * Data memory and subcommands are mapped to the direct command memory by the chip
 */
#define BQ_MEM_SIZE      (0x80)

int bq769x2_tests_interface();

int bq769x2_tests_functions();

uint8_t *bq769x2_get_mem(void);

#ifdef __cplusplus
}
#endif

#endif /* BQ769X2_TESTS_H_ */
