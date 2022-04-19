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
static uint8_t mem_bq[BQ_MEM_SIZE];

uint8_t *bq769x2_get_mem(void)
{
    return mem_bq;
}

int bq769x2_write_bytes(const uint8_t reg_addr, const uint8_t *data, const size_t num_bytes)
{
    if (reg_addr >= sizeof(mem_bq)) {
        return -EINVAL;
    }

    memcpy(mem_bq + reg_addr, data, num_bytes);

    return 0;
}

int bq769x2_read_bytes(const uint8_t reg_addr, uint8_t *data, const size_t num_bytes)
{
    if (reg_addr >= sizeof(mem_bq)) {
        return -EINVAL;
    }

    memcpy(data, mem_bq + reg_addr, num_bytes);

    return 0;
}

void bq769x2_init()
{
    /* nothing to do here */
}
