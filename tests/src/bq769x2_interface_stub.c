/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bq769x2/registers.h"
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

    memcpy(&mem_bq_direct[reg_addr], data, num_bytes);

    if (reg_addr == BQ769X2_SUBCMD_DATA_CHECKSUM || reg_addr == BQ769X2_SUBCMD_DATA_LENGTH) {
        // writing to BQ769X2_SUBCMD_DATA_LENGTH starts execution of a subcommand

        uint16_t subcmd_addr = (mem_bq_direct[BQ769X2_CMD_SUBCMD_UPPER] << 8)
                               + mem_bq_direct[BQ769X2_CMD_SUBCMD_LOWER];
        uint8_t subcmd_bytes = mem_bq_direct[BQ769X2_SUBCMD_DATA_LENGTH] - 4;

        if (subcmd_addr >= sizeof(mem_bq_subcmd)) {
            return -EINVAL;
        }

        memcpy(&mem_bq_subcmd[subcmd_addr], &mem_bq_direct[BQ769X2_SUBCMD_DATA_START],
               subcmd_bytes);
    }
    else if (reg_addr == BQ769X2_CMD_SUBCMD_LOWER && num_bytes == 2) {
        // writing to SUBCMD register initiates a subcmd read

        uint16_t subcmd_addr = (mem_bq_direct[BQ769X2_CMD_SUBCMD_UPPER] << 8)
                               + mem_bq_direct[BQ769X2_CMD_SUBCMD_LOWER];

        if (subcmd_addr >= sizeof(mem_bq_subcmd)) {
            return -EINVAL;
        }

        memcpy(&mem_bq_direct[BQ769X2_SUBCMD_DATA_START], &mem_bq_subcmd[subcmd_addr], 4);

        // always assume maximum data type length of 4 bytes
        uint8_t checksum =
            mem_bq_direct[BQ769X2_CMD_SUBCMD_UPPER] + mem_bq_direct[BQ769X2_CMD_SUBCMD_LOWER];
        for (int i = 0; i < 4; i++) {
            checksum += mem_bq_direct[BQ769X2_SUBCMD_DATA_START + i];
        }
        checksum = ~checksum;

        mem_bq_direct[BQ769X2_SUBCMD_DATA_LENGTH] = 4 + 4;
        mem_bq_direct[BQ769X2_SUBCMD_DATA_CHECKSUM] = checksum;
    }

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
