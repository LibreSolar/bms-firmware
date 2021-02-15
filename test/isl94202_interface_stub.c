/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pcb.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t mem_isl[0xAB+1];  // Memory of ILS94202 (registers 0x00 to 0xAB)

int isl94202_write_bytes(uint8_t reg_addr, uint8_t *data, uint32_t num_bytes)
{
	if ((reg_addr > 0x58 && reg_addr < 0x7F) || reg_addr + num_bytes > 0xAB || num_bytes > 4) {
	    return -1;
    }
	memcpy(mem_isl + reg_addr, data, num_bytes);
    return 0;
}

int isl94202_read_bytes(uint8_t reg_addr, uint8_t *data, uint32_t num_bytes)
{
	if ((reg_addr > 0x58 && reg_addr < 0x7F) || reg_addr + num_bytes > 0xAB || num_bytes > 4) {
	    return -1;
    }
	memcpy(data, mem_isl + reg_addr, num_bytes);
    return 0;
}

void isl94202_init()
{
    /* nothing to do here */
}
