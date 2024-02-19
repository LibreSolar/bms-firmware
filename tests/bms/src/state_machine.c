/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <bms/bms.h>

#include <stdio.h>
#include <time.h>

struct bms_context bms = {
    .ic_dev = DEVICE_DT_GET(DT_ALIAS(bms_ic)),
};

void init_conf()
{
    bms.ic_conf.cell_ov_limit = 3.65;
    bms.ic_conf.cell_ov_delay_ms = 2000;

    bms.ic_conf.cell_uv_limit = 2.8;
    bms.ic_conf.cell_uv_delay_ms = 2000;

    bms.ic_conf.dis_ut_limit = -20;
    bms.ic_conf.dis_ot_limit = 45;
    bms.ic_conf.chg_ut_limit = 0;
    bms.ic_conf.chg_ot_limit = 45;
    bms.ic_conf.temp_limit_hyst = 2;

    bms.ic_conf.bal_cell_voltage_min = 3.2;
    bms.ic_conf.bal_idle_delay = 5 * 60;
    bms.ic_conf.bal_cell_voltage_diff = 0.01;

    for (int i = 0; i < CONFIG_BMS_IC_MAX_CELLS; i++) {
        bms.ic_data.cell_voltages[i] = 3.3;
    }
    bms.ic_data.cell_voltage_min = 3.3;
    bms.ic_data.cell_voltage_max = 3.3;
    bms.ic_data.cell_voltage_avg = 3.3;

    for (int i = 0; i < CONFIG_BMS_IC_MAX_THERMISTORS; i++) {
        bms.ic_data.cell_temps[i] = 25;
    }
    bms.ic_data.cell_temp_min = 25;
    bms.ic_data.cell_temp_max = 25;
    bms.ic_data.cell_temp_avg = 25;

    bms.state = BMS_STATE_OFF;
    bms.ic_data.error_flags = 0;

    bms.full = false;
    bms.empty = false;

    bms.chg_enable = true;
    bms.dis_enable = true;
}

ZTEST(state_machine, test_no_off2dis_if_dis_nok)
{
    init_conf();
    bms.empty = true;
    bms_state_machine(&bms);
    zassert_not_equal(BMS_STATE_DIS, bms.state);

    init_conf();
    bms.ic_data.error_flags |= BMS_ERR_DIS_OVERTEMP;
    bms_state_machine(&bms);
    zassert_not_equal(BMS_STATE_DIS, bms.state);
}

ZTEST(state_machine, test_off2dis_if_dis_ok)
{
    init_conf();
    bms.full = true;
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_DIS, bms.state);
}

ZTEST(state_machine, test_no_off2chg_if_chg_ok)
{
    init_conf();
    bms_state_machine(&bms);
    zassert_not_equal(BMS_STATE_CHG, bms.state);
}

ZTEST(state_machine, test_off2chg_if_chg_ok_and_dis_nok)
{
    init_conf();
    bms.empty = true;
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_CHG, bms.state);

    init_conf();
    bms.ic_data.error_flags |= BMS_ERR_DIS_OVERTEMP;
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_CHG, bms.state);
}

ZTEST(state_machine, test_chg2off_if_chg_nok)
{
    init_conf();
    bms.state = BMS_STATE_CHG;
    bms.full = true;
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_OFF, bms.state);

    init_conf();
    bms.state = BMS_STATE_CHG;
    bms.ic_data.error_flags |= BMS_ERR_CHG_OVERTEMP;
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_OFF, bms.state);
}

ZTEST(state_machine, test_chg2normal_if_dis_ok)
{
    init_conf();
    bms.state = BMS_STATE_CHG;
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_NORMAL, bms.state);
}

ZTEST(state_machine, test_dis2off_if_dis_nok)
{
    init_conf();
    bms.state = BMS_STATE_DIS;
    bms.empty = true;
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_OFF, bms.state);

    init_conf();
    bms.state = BMS_STATE_DIS;
    bms.ic_data.error_flags |= BMS_ERR_DIS_OVERTEMP;
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_OFF, bms.state);
}

ZTEST(state_machine, test_dis2normal_if_chg_ok)
{
    init_conf();
    bms.state = BMS_STATE_DIS;
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_NORMAL, bms.state);
}

ZTEST(state_machine, test_normal2dis_if_chg_nok)
{
    init_conf();
    bms.state = BMS_STATE_NORMAL;
    bms.full = true;
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_DIS, bms.state);

    init_conf();
    bms.state = BMS_STATE_NORMAL;
    bms.ic_data.error_flags |= BMS_ERR_CHG_OVERTEMP;
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_DIS, bms.state);
}

ZTEST(state_machine, test_normal2chg_if_dis_nok)
{
    init_conf();
    bms.state = BMS_STATE_NORMAL;
    bms.empty = true;
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_CHG, bms.state);

    init_conf();
    bms.state = BMS_STATE_NORMAL;
    bms.ic_data.error_flags |= BMS_ERR_DIS_OVERTEMP;
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_CHG, bms.state);
}

/*
ZTEST(state_machine, test_no_normal2balancing_if_nok)
{
    init_conf();
    bms.state = BMS_STATE_NORMAL;
    bms.ic_data.cell_voltages[3] += 0.011;
    bms.ic_data.cell_voltage_max = bms.ic_data.cell_voltages[3];
    bms.ic_data.cell_voltage_min = bms.ic_data.cell_voltages[2];

    // idle time not long enough
    bms.ic_data.no_idle_timestamp = time(NULL) - 5*60 + 1;
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_NORMAL, bms.state);

    // SOC too low
    bms.ic_data.current = bms.ic_conf.bal_idle_current - 0.1;
    bms.ic_data.cell_voltages[3] = bms.ic_conf.bal_cell_voltage_min + 0.1;
    bms.ic_data.cell_voltage_max = bms.ic_data.cell_voltages[3];
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_NORMAL, bms.state);
}

ZTEST(state_machine, test_normal2balancing_if_ok)
{
    init_conf();
    bms.state = BMS_STATE_NORMAL;
    bms.ic_data.no_idle_timestamp = time(NULL) - 5*60 - 1;
    bms.ic_data.cell_voltages[3] += 0.011;
    bms.ic_data.cell_voltage_max = bms.ic_data.cell_voltages[3];
    bms.ic_data.cell_voltage_min = bms.ic_data.cell_voltages[2];
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_BALANCING, bms.state);
}

ZTEST(state_machine, test_normal2balancing_if_ok)
{
    init_conf();
    bms.state = BMS_STATE_BALANCING;
    bms.ic_data.no_idle_timestamp = time(NULL);
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_NORMAL, bms.state);
}

ZTEST(state_machine, test_balancing2normal_if_done)
{
    init_conf();
    bms.state = BMS_STATE_BALANCING;
    bms.ic_data.no_idle_timestamp = time(NULL) - 5*60 - 1;
    bms.ic_data.cell_voltages[3] = 3.309;
    bms.ic_data.cell_voltage_max = bms.ic_data.cell_voltages[3];
    bms.ic_data.cell_voltages[2] = 3.3;
    bms.ic_data.cell_voltage_min = bms.ic_data.cell_voltages[2];
    bms_state_machine(&bms);
    zassert_equal(BMS_STATE_NORMAL, bms.state);
}
*/

ZTEST_SUITE(state_machine, NULL, NULL, NULL, NULL, NULL);
