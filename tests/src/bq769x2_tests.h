/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BQ769X2_TESTS_H_
#define BQ769X2_TESTS_H_

/*
 * Memory layout of bq769x2 for direct commands and subcommands
 *
 * Used subcommand address space starts with 0x9180 and ends below 0x9400
 */
#define BQ_DIRECT_MEM_SIZE (0x80)
#define BQ_SUBCMD_MEM_SIZE (0x9400)

int bq769x2_tests_interface();

int bq769x2_tests_functions();

#endif /* BQ769X2_TESTS_H_ */
