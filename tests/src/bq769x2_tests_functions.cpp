/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bq769x2_tests.h"

#include "bms.h"
#include "bq769x2/interface.h"

#include "unity.h"

#include <time.h>
#include <stdio.h>

extern BmsConfig bms_conf;
extern BmsStatus bms_status;

// defined in bq769x2_interface_stub
static uint8_t *mem_bq;

int bq769x2_tests_functions()
{
    mem_bq = bq769x2_get_mem();

    UNITY_BEGIN();

    // ToDo

    return UNITY_END();
}
