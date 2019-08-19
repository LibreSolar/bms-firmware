
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

    for (int i = 0; i < NUM_CELLS_MAX; i++) {
        bms_status.cell_voltages[i] = 3.3;
    }

    for (int i = 0; i < NUM_THERMISTORS_MAX; i++) {
        bms_status.temperatures[i] = 25;
    }
}

void check_chg_low_temp_limits()
{
    init_conf();
    TEST_ASSERT(bms_chg_allowed(&bms_conf, &bms_status) == true);
    bms_status.temperatures[0] = -1;
    TEST_ASSERT(bms_chg_allowed(&bms_conf, &bms_status) == false);
}

void check_chg_high_temp_limits()
{
    init_conf();
    TEST_ASSERT(bms_chg_allowed(&bms_conf, &bms_status) == true);
    bms_status.temperatures[0] = 46;
    TEST_ASSERT(bms_chg_allowed(&bms_conf, &bms_status) == false);
}

void check_chg_low_voltage_limits()
{
    init_conf();
    TEST_ASSERT(bms_chg_allowed(&bms_conf, &bms_status) == true);
    bms_status.cell_voltages[3] = 3.66;
    bms_status.id_cell_voltage_max = 3;
    TEST_ASSERT(bms_chg_allowed(&bms_conf, &bms_status) == false);
}

void check_dis_low_temp_limits()
{
    init_conf();
    TEST_ASSERT(bms_dis_allowed(&bms_conf, &bms_status) == true);
    bms_status.temperatures[0] = -21;
    TEST_ASSERT(bms_dis_allowed(&bms_conf, &bms_status) == false);
}

void check_dis_high_temp_limits()
{
    init_conf();
    TEST_ASSERT(bms_dis_allowed(&bms_conf, &bms_status) == true);
    bms_status.temperatures[0] = 46;
    TEST_ASSERT(bms_dis_allowed(&bms_conf, &bms_status) == false);
}

void check_dis_high_voltage_limits()
{
    init_conf();
    TEST_ASSERT(bms_dis_allowed(&bms_conf, &bms_status) == true);
    bms_status.cell_voltages[3] = 2.79;
    bms_status.id_cell_voltage_min = 3;
    TEST_ASSERT(bms_dis_allowed(&bms_conf, &bms_status) == false);
}

void common_tests()
{
    UNITY_BEGIN();

    // charging allowed function
    RUN_TEST(check_chg_low_temp_limits);
    RUN_TEST(check_chg_high_temp_limits);
    RUN_TEST(check_chg_low_voltage_limits);

    // discharging allowed function
    RUN_TEST(check_dis_low_temp_limits);
    RUN_TEST(check_dis_high_temp_limits);
    RUN_TEST(check_dis_high_voltage_limits);

    UNITY_END();
}
