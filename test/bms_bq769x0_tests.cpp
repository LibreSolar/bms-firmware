/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tests.h"

#include "bms.h"

#include <time.h>
#include <stdio.h>

extern BmsConfig bms_conf;
extern BmsStatus bms_status;

extern uint8_t mem_bq[0x60];     // defined in bq769x0_interface_stub

// fill RAM and flash with suitable values
static void init_bq769x0_ram()
{
    /* TODO */
}

void test_bq769x0_init()
{
    init_bq769x0_ram();
    bms_init_hardware();
    TEST_ASSERT(0);
}

void test_bq769x0_read_cell_voltages()
{
    init_bq769x0_ram();
    bms_read_voltages(&bms_status);
    TEST_ASSERT_EQUAL_FLOAT(3.0, roundf(bms_status.cell_voltages[0] * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.1, roundf(bms_status.cell_voltages[1] * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.2, roundf(bms_status.cell_voltages[2] * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.3, roundf(bms_status.cell_voltages[3] * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.4, roundf(bms_status.cell_voltages[4] * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.5, roundf(bms_status.cell_voltages[5] * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.6, roundf(bms_status.cell_voltages[6] * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.7, roundf(bms_status.cell_voltages[7] * 100) / 100);
}

void test_bq769x0_read_pack_voltage()
{
    init_bq769x0_ram();
    bms_read_voltages(&bms_status);
    TEST_ASSERT_EQUAL_FLOAT(3.3*8, roundf(bms_status.pack_voltage * 10) / 10);
}

void test_bq769x0_read_min_max_avg_voltage()
{
    init_bq769x0_ram();
    bms_read_voltages(&bms_status);
    TEST_ASSERT_EQUAL_FLOAT(3.0, roundf(bms_status.cell_voltage_min * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.7, roundf(bms_status.cell_voltage_max * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.35, roundf(bms_status.cell_voltage_avg * 100) / 100);
}

void test_bq769x0_read_pack_current()
{
    /* TODO */
    TEST_ASSERT(0);
}

void test_bq769x0_read_error_flags()
{
    /* TODO */
    TEST_ASSERT(0);
}

void test_bq769x0_read_temperatures()
{
    /* TODO */
    TEST_ASSERT(0);
}

void test_bq769x0_apply_dis_ocp_limits()
{
    /* TODO */
    TEST_ASSERT(0);
}

void test_bq769x0_apply_chg_ocp_limits()
{
    /* TODO */
    TEST_ASSERT(0);
}

void test_bq769x0_apply_dis_scp_limits()
{
    /* TODO */
    TEST_ASSERT(0);
}

void test_bq769x0_apply_cell_ov_limits()
{
    /* TODO */
    TEST_ASSERT(0);
}

void test_bq769x0_apply_cell_uv_limits()
{
    /* TODO */
    TEST_ASSERT(0);
}

void bq769x0_tests()
{
    UNITY_BEGIN();

    //RUN_TEST(test_bq769x0_init);

    //RUN_TEST(test_bq769x0_read_cell_voltages);
    //RUN_TEST(test_bq769x0_read_pack_voltage);
    //RUN_TEST(test_bq769x0_read_min_max_avg_voltage);
    //RUN_TEST(test_bq769x0_read_pack_current);
    //RUN_TEST(test_bq769x0_read_error_flags);
    //RUN_TEST(test_bq769x0_read_temperatures);

    //RUN_TEST(test_bq769x0_apply_dis_ocp_limits);
    //RUN_TEST(test_bq769x0_apply_chg_ocp_limits);
    //RUN_TEST(test_bq769x0_apply_dis_scp_limits);

    //RUN_TEST(test_bq769x0_apply_cell_ov_limits);
    //RUN_TEST(test_bq769x0_apply_cell_uv_limits);

    UNITY_END();
}
