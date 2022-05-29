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

/*
 * Below tests use actual register numbers (instead of defines from registers.h) in order to
 * double-check the register defines vs. datasheet. Also magic numbers for min/max/default
 * values are obtained from the datasheet
 */

void test_bq769x2_apply_dis_scp()
{
    int err;

    // default
    bms_conf.dis_sc_limit = 10.0F / bms_conf.shunt_res_mOhm; // reg value 0 = 10 mV
    bms_conf.dis_sc_delay_us = (2 - 1) * 15;                 // 15 us (reg value 1 = no delay)
    err = bms_apply_dis_scp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT(10.0F / bms_conf.shunt_res_mOhm, bms_conf.dis_sc_limit);
    TEST_ASSERT_EQUAL_FLOAT((2 - 1) * 15, bms_conf.dis_sc_delay_us);
    TEST_ASSERT_EQUAL_UINT8(0, mem_bq_subcmd[0x9286 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2, mem_bq_subcmd[0x9287 - BQ_SUBCMD_MEM_OFFSET]);

    // min
    bms_conf.dis_sc_delay_us = (1 - 1) * 15; // 15 us (reg value 1 = no delay)
    err = bms_apply_dis_scp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT((1 - 1) * 15, bms_conf.dis_sc_delay_us);
    TEST_ASSERT_EQUAL_UINT8(1, mem_bq_subcmd[0x9287 - BQ_SUBCMD_MEM_OFFSET]);

    // too little
    bms_conf.dis_sc_limit = 5.0F / bms_conf.shunt_res_mOhm;
    err = bms_apply_dis_scp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT(10.0F / bms_conf.shunt_res_mOhm, bms_conf.dis_sc_limit);
    TEST_ASSERT_EQUAL_UINT8(0, mem_bq_subcmd[0x9286 - BQ_SUBCMD_MEM_OFFSET]);

    // max
    bms_conf.dis_sc_limit = 500.0F / bms_conf.shunt_res_mOhm; // reg value 015 = 500 mV
    bms_conf.dis_sc_delay_us = (31 - 1) * 15;
    err = bms_apply_dis_scp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT(500.0F / bms_conf.shunt_res_mOhm, bms_conf.dis_sc_limit);
    TEST_ASSERT_EQUAL_FLOAT((31 - 1) * 15, bms_conf.dis_sc_delay_us);
    TEST_ASSERT_EQUAL_UINT8(15, mem_bq_subcmd[0x9286 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(31, mem_bq_subcmd[0x9287 - BQ_SUBCMD_MEM_OFFSET]);

    // too much
    bms_conf.dis_sc_limit = 600.0F / bms_conf.shunt_res_mOhm;
    bms_conf.dis_sc_delay_us = (32 - 1) * 15;
    err = bms_apply_dis_scp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT(500.0F / bms_conf.shunt_res_mOhm, bms_conf.dis_sc_limit);
    TEST_ASSERT_EQUAL_FLOAT((31 - 1) * 15, bms_conf.dis_sc_delay_us);
    TEST_ASSERT_EQUAL_UINT8(15, mem_bq_subcmd[0x9286 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(31, mem_bq_subcmd[0x9287 - BQ_SUBCMD_MEM_OFFSET]);
}

void test_bq769x2_apply_cell_uvp()
{
    int err;

    // default
    bms_conf.cell_uv_limit = 50 * 50.6F / 1000.0F;
    bms_conf.cell_uv_reset = 52 * 50.6F / 1000.0F;
    bms_conf.cell_uv_delay_ms = 74 * 3.3F;
    err = bms_apply_cell_uvp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT(50 * 50.6F / 1000.0F, bms_conf.cell_uv_limit);
    TEST_ASSERT_EQUAL_FLOAT(52 * 50.6F / 1000.0F, bms_conf.cell_uv_reset);
    TEST_ASSERT_EQUAL_FLOAT(roundf(74 * 3.3F), bms_conf.cell_uv_delay_ms);
    TEST_ASSERT_EQUAL_UINT8(50, mem_bq_subcmd[0x9275 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(74, mem_bq_subcmd[0x9276 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(0, mem_bq_subcmd[0x9277 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2, mem_bq_subcmd[0x927B - BQ_SUBCMD_MEM_OFFSET]);

    // min
    bms_conf.cell_uv_limit = 20 * 50.6F / 1000.0F;
    bms_conf.cell_uv_reset = 22 * 50.6F / 1000.0F;
    bms_conf.cell_uv_delay_ms = 1 * 3.3F;
    err = bms_apply_cell_uvp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT(20 * 50.6F / 1000.0F, bms_conf.cell_uv_limit);
    TEST_ASSERT_EQUAL_FLOAT(22 * 50.6F / 1000.0F, bms_conf.cell_uv_reset);
    TEST_ASSERT_EQUAL_FLOAT(roundf(1 * 3.3F), bms_conf.cell_uv_delay_ms);
    TEST_ASSERT_EQUAL_UINT8(20, mem_bq_subcmd[0x9275 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(1, mem_bq_subcmd[0x9276 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(0, mem_bq_subcmd[0x9277 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2, mem_bq_subcmd[0x927B - BQ_SUBCMD_MEM_OFFSET]);

    // too little
    bms_conf.cell_uv_limit = 19 * 50.6F / 1000.0F;
    bms_conf.cell_uv_reset = 18 * 50.6F / 1000.0F;
    bms_conf.cell_uv_delay_ms = 0.0F;
    err = bms_apply_cell_uvp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT(20 * 50.6F / 1000.0F, bms_conf.cell_uv_limit);
    TEST_ASSERT_EQUAL_FLOAT(22 * 50.6F / 1000.0F, bms_conf.cell_uv_reset);
    TEST_ASSERT_EQUAL_FLOAT(roundf(1 * 3.3F), bms_conf.cell_uv_delay_ms);
    TEST_ASSERT_EQUAL_UINT8(20, mem_bq_subcmd[0x9275 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(1, mem_bq_subcmd[0x9276 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(0, mem_bq_subcmd[0x9277 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2, mem_bq_subcmd[0x927B - BQ_SUBCMD_MEM_OFFSET]);

    // max
    bms_conf.cell_uv_limit = 90 * 50.6F / 1000.0F;
    bms_conf.cell_uv_reset = 110 * 50.6F / 1000.0F;
    bms_conf.cell_uv_delay_ms = 2047 * 3.3F;
    err = bms_apply_cell_uvp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT(90 * 50.6F / 1000.0F, bms_conf.cell_uv_limit);
    TEST_ASSERT_EQUAL_FLOAT(110 * 50.6F / 1000.0F, bms_conf.cell_uv_reset);
    TEST_ASSERT_EQUAL_FLOAT(roundf(2047 * 3.3F), bms_conf.cell_uv_delay_ms);
    TEST_ASSERT_EQUAL_UINT8(90, mem_bq_subcmd[0x9275 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2047U & 0xFF, mem_bq_subcmd[0x9276 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2047U >> 8, mem_bq_subcmd[0x9277 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(20, mem_bq_subcmd[0x927B - BQ_SUBCMD_MEM_OFFSET]);

    // too much
    bms_conf.cell_uv_limit = 91 * 50.6F / 1000.0F;
    bms_conf.cell_uv_reset = 112 * 50.6F / 1000.0F;
    bms_conf.cell_uv_delay_ms = 2048 * 3.3F;
    err = bms_apply_cell_uvp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT(90 * 50.6F / 1000.0F, bms_conf.cell_uv_limit);
    TEST_ASSERT_EQUAL_FLOAT(110 * 50.6F / 1000.0F, bms_conf.cell_uv_reset);
    TEST_ASSERT_EQUAL_FLOAT(roundf(2047 * 3.3F), bms_conf.cell_uv_delay_ms);
    TEST_ASSERT_EQUAL_UINT8(90, mem_bq_subcmd[0x9275 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2047U & 0xFF, mem_bq_subcmd[0x9276 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2047U >> 8, mem_bq_subcmd[0x9277 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(20, mem_bq_subcmd[0x927B - BQ_SUBCMD_MEM_OFFSET]);
}

void test_bq769x2_apply_cell_ovp()
{
    int err;

    // default
    bms_conf.cell_ov_limit = 86 * 50.6F / 1000.0F;
    bms_conf.cell_ov_reset = 84 * 50.6F / 1000.0F;
    bms_conf.cell_ov_delay_ms = 74 * 3.3F;
    err = bms_apply_cell_ovp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT(86 * 50.6F / 1000.0F, bms_conf.cell_ov_limit);
    TEST_ASSERT_EQUAL_FLOAT(84 * 50.6F / 1000.0F, bms_conf.cell_ov_reset);
    TEST_ASSERT_EQUAL_FLOAT(roundf(74 * 3.3F), bms_conf.cell_ov_delay_ms);
    TEST_ASSERT_EQUAL_UINT8(86, mem_bq_subcmd[0x9278 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(74, mem_bq_subcmd[0x9279 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(0, mem_bq_subcmd[0x927A - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2, mem_bq_subcmd[0x927C - BQ_SUBCMD_MEM_OFFSET]);

    // min
    bms_conf.cell_ov_limit = 20 * 50.6F / 1000.0F;
    bms_conf.cell_ov_reset = 18 * 50.6F / 1000.0F;
    bms_conf.cell_ov_delay_ms = 1 * 3.3F;
    err = bms_apply_cell_ovp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT(20 * 50.6F / 1000.0F, bms_conf.cell_ov_limit);
    TEST_ASSERT_EQUAL_FLOAT(18 * 50.6F / 1000.0F, bms_conf.cell_ov_reset);
    TEST_ASSERT_EQUAL_FLOAT(roundf(1 * 3.3F), bms_conf.cell_ov_delay_ms);
    TEST_ASSERT_EQUAL_UINT8(20, mem_bq_subcmd[0x9278 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(1, mem_bq_subcmd[0x9279 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(0, mem_bq_subcmd[0x927A - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2, mem_bq_subcmd[0x927C - BQ_SUBCMD_MEM_OFFSET]);

    // too little
    bms_conf.cell_ov_limit = 19 * 50.6F / 1000.0F;
    bms_conf.cell_ov_reset = 20 * 50.6F / 1000.0F;
    bms_conf.cell_ov_delay_ms = 0.0F;
    err = bms_apply_cell_ovp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT(20 * 50.6F / 1000.0F, bms_conf.cell_ov_limit);
    TEST_ASSERT_EQUAL_FLOAT(18 * 50.6F / 1000.0F, bms_conf.cell_ov_reset);
    TEST_ASSERT_EQUAL_FLOAT(roundf(1 * 3.3F), bms_conf.cell_ov_delay_ms);
    TEST_ASSERT_EQUAL_UINT8(20, mem_bq_subcmd[0x9278 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(1, mem_bq_subcmd[0x9279 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(0, mem_bq_subcmd[0x927A - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2, mem_bq_subcmd[0x927C - BQ_SUBCMD_MEM_OFFSET]);

    // max
    bms_conf.cell_ov_limit = 110 * 50.6F / 1000.0F;
    bms_conf.cell_ov_reset = 90 * 50.6F / 1000.0F;
    bms_conf.cell_ov_delay_ms = 2047 * 3.3F;
    err = bms_apply_cell_ovp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT(110 * 50.6F / 1000.0F, bms_conf.cell_ov_limit);
    TEST_ASSERT_EQUAL_FLOAT(90 * 50.6F / 1000.0F, bms_conf.cell_ov_reset);
    TEST_ASSERT_EQUAL_FLOAT(roundf(2047 * 3.3F), bms_conf.cell_ov_delay_ms);
    TEST_ASSERT_EQUAL_UINT8(110, mem_bq_subcmd[0x9278 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2047U & 0xFF, mem_bq_subcmd[0x9279 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2047U >> 8, mem_bq_subcmd[0x927A - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(20, mem_bq_subcmd[0x927C - BQ_SUBCMD_MEM_OFFSET]);

    // too much
    bms_conf.cell_ov_limit = 111 * 50.6F / 1000.0F;
    bms_conf.cell_ov_reset = 112 * 50.6F / 1000.0F;
    bms_conf.cell_ov_delay_ms = 2048 * 3.3F;
    err = bms_apply_cell_ovp(&bms_conf);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT(110 * 50.6F / 1000.0F, bms_conf.cell_ov_limit);
    TEST_ASSERT_EQUAL_FLOAT(108 * 50.6F / 1000.0F, bms_conf.cell_ov_reset);
    TEST_ASSERT_EQUAL_FLOAT(roundf(2047 * 3.3F), bms_conf.cell_ov_delay_ms);
    TEST_ASSERT_EQUAL_UINT8(110, mem_bq_subcmd[0x9278 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2047U & 0xFF, mem_bq_subcmd[0x9279 - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2047U >> 8, mem_bq_subcmd[0x927A - BQ_SUBCMD_MEM_OFFSET]);
    TEST_ASSERT_EQUAL_UINT8(2, mem_bq_subcmd[0x927C - BQ_SUBCMD_MEM_OFFSET]);
}

int bq769x2_tests_functions()
{
    UNITY_BEGIN();

    RUN_TEST(test_bq769x2_apply_dis_scp);
    RUN_TEST(test_bq769x2_apply_cell_uvp);
    RUN_TEST(test_bq769x2_apply_cell_ovp);

    return UNITY_END();
}
