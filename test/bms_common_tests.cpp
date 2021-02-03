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

void init_conf()
{
    bms_conf.cell_ov_limit = 3.65;
    bms_conf.cell_ov_delay_ms = 2000;

    bms_conf.cell_uv_limit = 2.8;
    bms_conf.cell_uv_delay_ms = 2000;

    bms_conf.dis_ut_limit = -20;
    bms_conf.dis_ot_limit = 45;
    bms_conf.chg_ut_limit = 0;
    bms_conf.chg_ot_limit = 45;
    bms_conf.t_limit_hyst = 2;

    bms_conf.bal_cell_voltage_min = 3.2;
    bms_conf.bal_idle_delay = 5*60;
    bms_conf.bal_cell_voltage_diff = 0.01;

    for (int i = 0; i < NUM_CELLS_MAX; i++) {
        bms_status.cell_voltages[i] = 3.3;
    }
    bms_status.cell_voltage_min = 3.3;
    bms_status.cell_voltage_max = 3.3;
    bms_status.cell_voltage_avg = 3.3;

    for (int i = 0; i < NUM_THERMISTORS_MAX; i++) {
        bms_status.bat_temps[i] = 25;
    }
    bms_status.bat_temp_min = 25;
    bms_status.bat_temp_max = 25;
    bms_status.bat_temp_avg = 25;

    bms_status.state = BMS_STATE_OFF;
    bms_status.error_flags = 0;

    bms_status.full = false;
    bms_status.empty = false;

    bms_status.chg_enable = true;
    bms_status.dis_enable = true;
}

void no_off2dis_if_dis_nok()
{
    init_conf();
    bms_status.empty = true;
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_NOT_EQUAL(BMS_STATE_DIS, bms_status.state);

    init_conf();
    bms_status.error_flags |= (1U << BMS_ERR_DIS_OVERTEMP);
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_NOT_EQUAL(BMS_STATE_DIS, bms_status.state);
}

void off2dis_if_dis_ok()
{
    init_conf();
    bms_status.full = true;
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_DIS, bms_status.state);
}

void no_off2chg_if_chg_ok()
{
    init_conf();
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_NOT_EQUAL(BMS_STATE_CHG, bms_status.state);
}

void off2chg_if_chg_ok_and_dis_nok()
{
    init_conf();
    bms_status.empty = true;
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_CHG, bms_status.state);

    init_conf();
    bms_status.error_flags |= (1U << BMS_ERR_DIS_OVERTEMP);
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_CHG, bms_status.state);
}

void chg2off_if_chg_nok()
{
    init_conf();
    bms_status.state = BMS_STATE_CHG;
    bms_status.full = true;
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_OFF, bms_status.state);

    init_conf();
    bms_status.state = BMS_STATE_CHG;
    bms_status.error_flags |= (1U << BMS_ERR_CHG_OVERTEMP);
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_OFF, bms_status.state);
}

void chg2normal_if_dis_ok()
{
    init_conf();
    bms_status.state = BMS_STATE_CHG;
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_NORMAL, bms_status.state);
}

void dis2off_if_dis_nok()
{
    init_conf();
    bms_status.state = BMS_STATE_DIS;
    bms_status.empty = true;
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_OFF, bms_status.state);

    init_conf();
    bms_status.state = BMS_STATE_DIS;
    bms_status.error_flags |= (1U << BMS_ERR_DIS_OVERTEMP);
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_OFF, bms_status.state);
}

void dis2normal_if_chg_ok()
{
    init_conf();
    bms_status.state = BMS_STATE_DIS;
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_NORMAL, bms_status.state);
}

void normal2dis_if_chg_nok()
{
    init_conf();
    bms_status.state = BMS_STATE_NORMAL;
    bms_status.full = true;
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_DIS, bms_status.state);

    init_conf();
    bms_status.state = BMS_STATE_NORMAL;
    bms_status.error_flags |= (1U << BMS_ERR_CHG_OVERTEMP);
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_DIS, bms_status.state);
}

void normal2chg_if_dis_nok()
{
    init_conf();
    bms_status.state = BMS_STATE_NORMAL;
    bms_status.empty = true;
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_CHG, bms_status.state);

    init_conf();
    bms_status.state = BMS_STATE_NORMAL;
    bms_status.error_flags |= (1U << BMS_ERR_DIS_OVERTEMP);
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_CHG, bms_status.state);
}
/*
void no_normal2balancing_if_nok()
{
    init_conf();
    bms_status.state = BMS_STATE_NORMAL;
    bms_status.cell_voltages[3] += 0.011;
    bms_status.cell_voltage_max = bms_status.cell_voltages[3];
    bms_status.cell_voltage_min = bms_status.cell_voltages[2];

    // idle time not long enough
    bms_status.no_idle_timestamp = time(NULL) - 5*60 + 1;
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_NORMAL, bms_status.state);

    // SOC too low
    bms_status.pack_current = bms_conf.bal_idle_current - 0.1;
    bms_status.cell_voltages[3] = bms_conf.bal_cell_voltage_min + 0.1;
    bms_status.cell_voltage_max = bms_status.cell_voltages[3];
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_NORMAL, bms_status.state);
}

void normal2balancing_if_ok()
{
    init_conf();
    bms_status.state = BMS_STATE_NORMAL;
    bms_status.no_idle_timestamp = time(NULL) - 5*60 - 1;
    bms_status.cell_voltages[3] += 0.011;
    bms_status.cell_voltage_max = bms_status.cell_voltages[3];
    bms_status.cell_voltage_min = bms_status.cell_voltages[2];
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_BALANCING, bms_status.state);
}

void balancing2normal_at_increased_current()
{
    init_conf();
    bms_status.state = BMS_STATE_BALANCING;
    bms_status.no_idle_timestamp = time(NULL);
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_NORMAL, bms_status.state);
}

void balancing2normal_if_done()
{
    init_conf();
    bms_status.state = BMS_STATE_BALANCING;
    bms_status.no_idle_timestamp = time(NULL) - 5*60 - 1;
    bms_status.cell_voltages[3] = 3.309;
    bms_status.cell_voltage_max = bms_status.cell_voltages[3];
    bms_status.cell_voltages[2] = 3.3;
    bms_status.cell_voltage_min = bms_status.cell_voltages[2];
    bms_state_machine(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL(BMS_STATE_NORMAL, bms_status.state);
}
*/

void common_tests()
{
    UNITY_BEGIN();

    // state machine
    RUN_TEST(no_off2dis_if_dis_nok);
    RUN_TEST(off2dis_if_dis_ok);

    RUN_TEST(no_off2chg_if_chg_ok);
    RUN_TEST(off2chg_if_chg_ok_and_dis_nok);

    RUN_TEST(chg2off_if_chg_nok);
    RUN_TEST(chg2normal_if_dis_ok);

    RUN_TEST(dis2off_if_dis_nok);
    RUN_TEST(dis2normal_if_chg_ok);
    //RUN_TEST(dis2balancing_if_ok);

    RUN_TEST(normal2dis_if_chg_nok);

    RUN_TEST(normal2chg_if_dis_nok);

    //RUN_TEST(no_normal2balancing_if_nok);
    //RUN_TEST(normal2balancing_if_ok);   // ok = high SOC and current low
    //RUN_TEST(balancing2normal_at_increased_current);
    //RUN_TEST(balancing2normal_if_done);
    //RUN_TEST(balancing2dis_at_increased_current);
    //RUN_TEST(balancing2dis_if_done);

    UNITY_END();
}
