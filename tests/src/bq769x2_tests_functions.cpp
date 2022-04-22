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

void test_bq769x2_apply_cell_uvp()
{
    int err;

    // default
    bms_conf.cell_uv_limit = 50 * 50.6F / 1000.0F;
    bms_conf.cell_uv_delay_ms = 74 * 3.3F;
    err = bms_apply_cell_uvp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);

    uint8_t def[] = { 50, 74, 0 };
    TEST_ASSERT_EQUAL_UINT8_ARRAY(def, &mem_bq_subcmd[0x9275 - BQ_SUBCMD_MEM_OFFSET], sizeof(def));

    // min
    bms_conf.cell_uv_limit = 20 * 50.6F / 1000.0F;
    bms_conf.cell_uv_delay_ms = 1 * 3.3F;
    err = bms_apply_cell_uvp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);

    uint8_t min[] = { 20, 1, 0 };
    TEST_ASSERT_EQUAL_UINT8_ARRAY(min, &mem_bq_subcmd[0x9275 - BQ_SUBCMD_MEM_OFFSET], sizeof(min));

    // too little
    bms_conf.cell_uv_limit = 19 * 50.6F / 1000.0F;
    bms_conf.cell_uv_delay_ms = 0.0F;
    err = bms_apply_cell_uvp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(min, &mem_bq_subcmd[0x9275 - BQ_SUBCMD_MEM_OFFSET], sizeof(min));

    // max
    bms_conf.cell_uv_limit = 90 * 50.6F / 1000.0F;
    bms_conf.cell_uv_delay_ms = 2047 * 3.3F;
    err = bms_apply_cell_uvp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);

    uint8_t max[] = { 90, 2047U & 0xFF, 2047U >> 8 };
    TEST_ASSERT_EQUAL_UINT8_ARRAY(max, &mem_bq_subcmd[0x9275 - BQ_SUBCMD_MEM_OFFSET], sizeof(max));

    // too much
    bms_conf.cell_uv_limit = 91 * 50.6F / 1000.0F;
    bms_conf.cell_uv_delay_ms = 2048 * 3.3F;
    err = bms_apply_cell_uvp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(max, &mem_bq_subcmd[0x9275 - BQ_SUBCMD_MEM_OFFSET], sizeof(max));
}

int bq769x2_tests_functions()
{
    UNITY_BEGIN();

    RUN_TEST(test_bq769x2_apply_cell_uvp);

    return UNITY_END();
}
