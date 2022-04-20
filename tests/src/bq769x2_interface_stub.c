/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bq769x2_tests.h"

#include "board.h"

#include <stdint.h>
#include <string.h>

// Memory of bq769x2 for direct commands
uint8_t mem_bq_direct[BQ_DIRECT_MEM_SIZE];

// Memory of bq769x2 for subcommands
uint8_t mem_bq_subcmd[BQ_SUBCMD_MEM_SIZE];

int bq769x2_write_bytes(const uint8_t reg_addr, const uint8_t *data, const size_t num_bytes)
{
    if (reg_addr >= sizeof(mem_bq_direct)) {
        return -EINVAL;
    }

    memcpy(mem_bq_direct + reg_addr, data, num_bytes);

    return 0;
}

int bq769x2_read_bytes(const uint8_t reg_addr, uint8_t *data, const size_t num_bytes)
{
    if (reg_addr >= sizeof(mem_bq_direct)) {
        return -EINVAL;
    }

    memcpy(data, mem_bq_direct + reg_addr, num_bytes);

    return 0;
}

void bq769x2_init()
{
    /* nothing to do here */
}
