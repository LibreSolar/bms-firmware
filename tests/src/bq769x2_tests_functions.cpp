/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bq769x2_tests.h"

#include "bms.h"
#include "bq769x2/interface.h"

#include "unity.h"

#include <stdio.h>
#include <time.h>

extern BmsConfig bms_conf;
extern BmsStatus bms_status;

// defined in bq769x2_interface_stub
extern uint8_t mem_bq_direct[BQ_DIRECT_MEM_SIZE];
extern uint8_t mem_bq_subcmd[BQ_SUBCMD_MEM_SIZE];

int bq769x2_tests_functions()
{
    UNITY_BEGIN();

    // ToDo

    return UNITY_END();
}
