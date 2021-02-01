/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pcb.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t mem_bq[0x60];  // Memory of bq769x0 (registers 0x00 to 0x59)

void bq769x0_write_byte(uint8_t reg_addr, int data)
{
	if (reg_addr < sizeof(mem_bq)) {
        mem_bq[reg_addr] = data;
    }
}

int bq769x0_read_byte(uint8_t reg_addr)
{
	if (reg_addr >= sizeof(mem_bq)) {
	    return -1;
    }
    else {
        return mem_bq[reg_addr];
    }
}

int bq769x0_read_word(uint8_t reg_addr)
{
	if (reg_addr >= sizeof(mem_bq)) {
	    return -1;
    }
    else {
        return *((uint16_t *)&mem_bq[reg_addr]);
    }
}

void bq769x0_init()
{
    /* nothing to do here */
}
