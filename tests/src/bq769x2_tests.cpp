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
extern uint8_t mem_bq_direct[BQ_DIRECT_MEM_SIZE];
extern uint8_t mem_bq_subcmd[BQ_SUBCMD_MEM_SIZE];

void test_bq769x2_direct_read_u2()
{
    uint16_t u2 = 0;

    mem_bq_direct[0] = 0x00;
    mem_bq_direct[1] = 0x00;
    bq769x2_direct_read_u2(0, &u2);
    TEST_ASSERT_EQUAL(0, u2);

    mem_bq_direct[0] = 0xFF;
    mem_bq_direct[1] = 0xFF;
    bq769x2_direct_read_u2(0, &u2);
    TEST_ASSERT_EQUAL(UINT16_MAX, u2);
}

void test_bq769x2_direct_read_i2()
{
    int16_t i2 = 0;

    mem_bq_direct[0] = 0x00;
    mem_bq_direct[1] = 0x00;
    bq769x2_direct_read_i2(0, &i2);
    TEST_ASSERT_EQUAL(0, i2);

    mem_bq_direct[0] = 0xFF;
    mem_bq_direct[1] = 0xFF;
    bq769x2_direct_read_i2(0, &i2);
    TEST_ASSERT_EQUAL(-1, i2);

    mem_bq_direct[0] = 0xFF;
    mem_bq_direct[1] = 0x7F;
    bq769x2_direct_read_i2(0, &i2);
    TEST_ASSERT_EQUAL(INT16_MAX, i2);

    mem_bq_direct[0] = 0x00;
    mem_bq_direct[1] = 0x80;
    bq769x2_direct_read_i2(0, &i2);
    TEST_ASSERT_EQUAL(INT16_MIN, i2);
}

int bq769x2_tests()
{
    UNITY_BEGIN();

    RUN_TEST(test_bq769x2_direct_read_u2);
    RUN_TEST(test_bq769x2_direct_read_i2);

    return UNITY_END();
}
