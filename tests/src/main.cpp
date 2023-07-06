/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bms.h"
#include "board.h"

#include "bq769x0_tests.h"
#include "bq769x2_tests.h"
#include "common_tests.h"
#include "helper_tests.h"
#include "isl94202_tests.h"

#include "unity.h"

#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef CONFIG_ARCH_POSIX
#include "posix_board_if.h"
#endif

extern "C" {

void setUp(void)
{}

void tearDown(void)
{}

} /* extern "C" */

Bms bms;

float OCV[] = { // 100, 95, ..., 0 %
    3.392, 3.314, 3.309, 3.308, 3.304, 3.296, 3.283, 3.275, 3.271, 3.268, 3.265,
    3.264, 3.262, 3.252, 3.240, 3.226, 3.213, 3.190, 3.177, 3.132, 2.833
};

void setup()
{
    bms_init_hardware(&bms);

    bms.conf.dis_sc_limit = 35.0;
    bms.conf.dis_sc_delay_us = 200;

    bms.conf.dis_oc_limit = 25.0;
    bms.conf.dis_oc_delay_ms = 320;

    bms.conf.chg_oc_limit = 20.0;
    bms.conf.chg_oc_delay_ms = 320;

    bms.conf.cell_ov_limit = 3.65;
    bms.conf.cell_ov_delay_ms = 2000;

    bms.conf.cell_uv_limit = 2.8;
    bms.conf.cell_uv_delay_ms = 2000;

    bms.conf.dis_ut_limit = -20;
    bms.conf.dis_ot_limit = 45;
    bms.conf.chg_ut_limit = 0;
    bms.conf.chg_ot_limit = 45;
    bms.conf.t_limit_hyst = 2;

    bms.conf.shunt_res_mOhm = BOARD_SHUNT_RESISTOR;
    bms.conf.ocv = OCV;

    bms.conf.nominal_capacity_Ah = 45.0;
    bms.status.connected_cells = 4;

    bms.conf.bal_cell_voltage_min = 3.2;
    bms.conf.bal_idle_delay = 10 * 60;
    bms.conf.bal_cell_voltage_diff = 0.01;
    bms.conf.bal_idle_current = 0.1;

    bms_configure(&bms);

    bms_update(&bms);

    bms_soc_reset(&bms, -1);
    bms_dis_switch(&bms, true);
    bms_chg_switch(&bms, true);
}

int main(void)
{
    int err = 0;

    k_sleep(K_MSEC(100));

    setup();

    err += common_tests();

#ifdef CONFIG_BQ769X0
    err += bq769x0_tests();
#elif defined(CONFIG_BQ769X2)
    err += bq769x2_tests_functions();
    err += bq769x2_tests_interface();
#elif defined(CONFIG_ISL94202)
    err += isl94202_tests();
#endif

    err += helper_tests();

#ifdef CONFIG_ARCH_POSIX
    posix_exit(err);
#endif

    return 0;
}
