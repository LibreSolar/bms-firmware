/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bms/bms.h>

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

static const struct device *bms_ic = DEVICE_DT_GET(DT_ALIAS(bms_ic));

struct bms_context bms;

float OCV[] = { // 100, 95, ..., 0 %
    3.392, 3.314, 3.309, 3.308, 3.304, 3.296, 3.283, 3.275, 3.271, 3.268, 3.265,
    3.264, 3.262, 3.252, 3.240, 3.226, 3.213, 3.190, 3.177, 3.132, 2.833
};

void setUp(void)
{}

void tearDown(void)
{}

void setup()
{
    bms_ic_assign_data(bms_ic, &bms.ic_data);

    bms.ic_conf.dis_sc_limit = 35.0;
    bms.ic_conf.dis_sc_delay_us = 200;

    bms.ic_conf.dis_oc_limit = 25.0;
    bms.ic_conf.dis_oc_delay_ms = 320;

    bms.ic_conf.chg_oc_limit = 20.0;
    bms.ic_conf.chg_oc_delay_ms = 320;

    bms.ic_conf.cell_ov_limit = 3.65;
    bms.ic_conf.cell_ov_delay_ms = 2000;

    bms.ic_conf.cell_uv_limit = 2.8;
    bms.ic_conf.cell_uv_delay_ms = 2000;

    bms.ic_conf.dis_ut_limit = -20;
    bms.ic_conf.dis_ot_limit = 45;
    bms.ic_conf.chg_ut_limit = 0;
    bms.ic_conf.chg_ot_limit = 45;
    bms.ic_conf.temp_limit_hyst = 2;

    bms.ocv_points = OCV;

    bms.nominal_capacity_Ah = 45.0;
    bms.ic_data.connected_cells = 4;

    bms.ic_conf.bal_cell_voltage_min = 3.2;
    bms.ic_conf.bal_idle_delay = 10 * 60;
    bms.ic_conf.bal_cell_voltage_diff = 0.01;
    bms.ic_conf.bal_idle_current = 0.1;

    bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_ALL);

    bms_ic_read_data(bms_ic, BMS_IC_DATA_CELL_VOLTAGES);

    bms_soc_reset(&bms, -1);
    bms_ic_set_switches(bms_ic, BMS_SWITCH_DIS, true);
    bms_ic_set_switches(bms_ic, BMS_SWITCH_CHG, true);
}

int main(void)
{
    int err = 0;

    k_sleep(K_MSEC(100));

    setup();

    err += common_tests();

#ifdef CONFIG_BMS_IC_BQ769X0
    err += bq769x0_tests();
#elif defined(CONFIG_BMS_IC_BQ769X2)
    err += bq769x2_tests_functions();
    err += bq769x2_tests_interface();
#elif defined(CONFIG_BMS_IC_ISL94202)
    err += isl94202_tests();
#endif

    err += helper_tests();

#ifdef CONFIG_ARCH_POSIX
    posix_exit(err);
#endif

    return 0;
}
