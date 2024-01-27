/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bms/bms.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

struct bms_context bms;

const struct device *bms_ic = DEVICE_DT_GET(DT_ALIAS(bms_ic));

float OCV[] = { // 100, 95, ..., 0 %
    3.392, 3.314, 3.309, 3.308, 3.304, 3.296, 3.283, 3.275, 3.271, 3.268, 3.265,
    3.264, 3.262, 3.252, 3.240, 3.226, 3.213, 3.190, 3.177, 3.132, 2.833
};

void common_setup_bms_defaults()
{
    bms_ic_assign_data(bms_ic, &bms.ic_data);

    bms_ic_set_mode(bms_ic, BMS_IC_MODE_ACTIVE);

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
