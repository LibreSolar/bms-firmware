/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bms/bms.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/ztest.h>

#include "isl94202_emul.h"

#include "bms_setup.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

static const struct emul *bms_ic_emul = EMUL_DT_GET(DT_ALIAS(bms_ic));

extern struct bms_context bms;

static const float shunt_res_mohm = DT_PROP(DT_ALIAS(bms_ic), shunt_resistor_uohm) / 1000.0F;

void isl94202_init()
{
    isl94202_emul_set_mem_defaults(bms_ic_emul);

    // expected feature control register
    uint16_t fc_reg = 0;
    fc_reg |= 1U << 5;  // XT2M
    fc_reg |= 1U << 8;  // CB_EOC
    fc_reg |= 1U << 14; // CBDC
    zassert_equal(fc_reg, isl94202_emul_get_word(bms_ic_emul, 0x4A));
}

ZTEST(isl94202, test_isl94202_read_cell_voltages)
{
    isl94202_emul_set_mem_defaults(bms_ic_emul);

    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_CELL_VOLTAGES | BMS_IC_DATA_PACK_VOLTAGES);

    zassert_equal(3.0F, roundf(bms.ic_data.cell_voltages[0] * 100) / 100);
    zassert_equal(3.1F, roundf(bms.ic_data.cell_voltages[1] * 100) / 100);
    zassert_equal(3.2F, roundf(bms.ic_data.cell_voltages[2] * 100) / 100);
    zassert_equal(3.3F, roundf(bms.ic_data.cell_voltages[3] * 100) / 100);
    zassert_equal(3.4F, roundf(bms.ic_data.cell_voltages[4] * 100) / 100);
    zassert_equal(3.5F, roundf(bms.ic_data.cell_voltages[5] * 100) / 100);
    zassert_equal(3.6F, roundf(bms.ic_data.cell_voltages[6] * 100) / 100);
    zassert_equal(3.7F, roundf(bms.ic_data.cell_voltages[7] * 100) / 100);
}

ZTEST(isl94202, test_isl94202_read_total_voltage)
{
    isl94202_emul_set_mem_defaults(bms_ic_emul);

    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_CELL_VOLTAGES | BMS_IC_DATA_PACK_VOLTAGES);

    // previous test (if battery voltage measurement is used)
    // zassert_equal(3.3*8, roundf(bms.ic_data.total_voltage * 10) / 10);

    // now using sum of cell voltages instead of battery voltage
    zassert_equal((3.0F + 3.7F) / 2.0F * 8, roundf(bms.ic_data.total_voltage * 10) / 10);
}

ZTEST(isl94202, test_isl94202_read_min_max_avg_voltage)
{
    isl94202_emul_set_mem_defaults(bms_ic_emul);

    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_CELL_VOLTAGES | BMS_IC_DATA_PACK_VOLTAGES);
    zassert_equal(3.0F, roundf(bms.ic_data.cell_voltage_min * 100) / 100);
    zassert_equal(3.7F, roundf(bms.ic_data.cell_voltage_max * 100) / 100);
    zassert_equal(3.35F, roundf(bms.ic_data.cell_voltage_avg * 100) / 100);
}

ZTEST(isl94202, test_isl94202_read_current)
{
    isl94202_emul_set_mem_defaults(bms_ic_emul);

    // charge current, gain 5
    isl94202_emul_set_byte(bms_ic_emul, 0x82, 0x01U << 2); // CHING
    isl94202_emul_set_byte(bms_ic_emul, 0x85, 0x01U << 4); // gain 5
    isl94202_emul_set_word(bms_ic_emul, 0x8E,
                           117.14F / 1.8F * 4095 * 5 * shunt_res_mohm / 1000); // ADC reading

    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_CURRENT);
    zassert_equal(117.1F, roundf(bms.ic_data.current * 10) / 10);

    // discharge current, gain 50
    isl94202_emul_set_byte(bms_ic_emul, 0x82, 0x01U << 3); // DCHING
    isl94202_emul_set_byte(bms_ic_emul, 0x85, 0x00U);      // gain 50
    isl94202_emul_set_word(bms_ic_emul, 0x8E,
                           12.14F / 1.8F * 4095 * 50 * shunt_res_mohm / 1000); // ADC reading

    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_CURRENT);
    zassert_equal(-12.14F, roundf(bms.ic_data.current * 100) / 100);

    // low current, gain 500
    isl94202_emul_set_byte(bms_ic_emul, 0x82, 0x00U);      // neither CHING nor DCHING
    isl94202_emul_set_byte(bms_ic_emul, 0x85, 0x02U << 4); // gain 500
    isl94202_emul_set_word(bms_ic_emul, 0x8E,
                           1.14 / 1.8 * 4095 * 500 * shunt_res_mohm / 1000); // ADC reading

    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_CURRENT);
    zassert_equal(0.0F, roundf(bms.ic_data.current * 100) / 100);
}

ZTEST(isl94202, test_isl94202_read_error_flags)
{
    /* assume CFET and DFET are on */
    isl94202_emul_set_word(bms_ic_emul, 0x86, 0x03U);

    isl94202_emul_set_word(bms_ic_emul, 0x80, 0x01U << 2);
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_ERROR_FLAGS);
    zassert_equal(BMS_ERR_CELL_UNDERVOLTAGE, bms.ic_data.error_flags);

    isl94202_emul_set_word(bms_ic_emul, 0x80, 0x01U << 0);
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_ERROR_FLAGS);
    zassert_equal(BMS_ERR_CELL_OVERVOLTAGE, bms.ic_data.error_flags);

    isl94202_emul_set_word(bms_ic_emul, 0x80, 0x01U << 11);
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_ERROR_FLAGS);
    zassert_equal(BMS_ERR_SHORT_CIRCUIT, bms.ic_data.error_flags);

    isl94202_emul_set_word(bms_ic_emul, 0x80, 0x01U << 10);
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_ERROR_FLAGS);
    zassert_equal(BMS_ERR_DIS_OVERCURRENT, bms.ic_data.error_flags);

    isl94202_emul_set_word(bms_ic_emul, 0x80, 0x01U << 9);
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_ERROR_FLAGS);
    zassert_equal(BMS_ERR_CHG_OVERCURRENT, bms.ic_data.error_flags);

    isl94202_emul_set_word(bms_ic_emul, 0x80, 0x01U << 13);
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_ERROR_FLAGS);
    zassert_equal(BMS_ERR_OPEN_WIRE, bms.ic_data.error_flags);

    isl94202_emul_set_word(bms_ic_emul, 0x80, 0x01U << 5);
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_ERROR_FLAGS);
    zassert_equal(BMS_ERR_DIS_UNDERTEMP, bms.ic_data.error_flags);

    isl94202_emul_set_word(bms_ic_emul, 0x80, 0x01U << 4);
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_ERROR_FLAGS);
    zassert_equal(BMS_ERR_DIS_OVERTEMP, bms.ic_data.error_flags);

    isl94202_emul_set_word(bms_ic_emul, 0x80, 0x01U << 7);
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_ERROR_FLAGS);
    zassert_equal(BMS_ERR_CHG_UNDERTEMP, bms.ic_data.error_flags);

    isl94202_emul_set_word(bms_ic_emul, 0x80, 0x01U << 6);
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_ERROR_FLAGS);
    zassert_equal(BMS_ERR_CHG_OVERTEMP, bms.ic_data.error_flags);

    isl94202_emul_set_word(bms_ic_emul, 0x80, 0x01U << 12);
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_ERROR_FLAGS);
    zassert_equal(BMS_ERR_CELL_FAILURE, bms.ic_data.error_flags);

    /* preparation for additional CHG / DSG FET error flags */
    isl94202_emul_set_word(bms_ic_emul, 0x80, 0);
    bms_ic_set_switches(bms.ic_dev, BMS_SWITCH_CHG | BMS_SWITCH_DIS, true);

    /* turn CFET off */
    isl94202_emul_set_word(bms_ic_emul, 0x86, 0x01);
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_ERROR_FLAGS);
    zassert_equal(BMS_ERR_CHG_OFF, bms.ic_data.error_flags);

    /* turn DFET off */
    isl94202_emul_set_word(bms_ic_emul, 0x86, 0x02);
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_ERROR_FLAGS);
    zassert_equal(BMS_ERR_DIS_OFF, bms.ic_data.error_flags);
}

ZTEST(isl94202, test_isl94202_read_temperatures)
{
    // assuming 22k resistor and gain 2

    // Internal temperature
    isl94202_emul_set_word(bms_ic_emul, 0xA0, (22.0 + 273.15) * 1.8527 / 1000 / 1.8 * 4095);
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_TEMPERATURES);
    zassert_equal(22.0, roundf(bms.ic_data.ic_temp * 10) / 10);

    // // External temperature 1 (check incl. interpolation)
    isl94202_emul_set_word(bms_ic_emul, 0xA2, 0.463 * 2 / 1.8 * 4095); // 25°C
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_TEMPERATURES);
    zassert_equal(25.0, roundf(bms.ic_data.cell_temp_avg * 10) / 10);

    isl94202_emul_set_word(bms_ic_emul, 0xA2, 0.150 * 2 / 1.8 * 4095); // >80°C
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_TEMPERATURES);
    zassert_equal(80.0, roundf(bms.ic_data.cell_temp_avg * 10) / 10);

    isl94202_emul_set_word(bms_ic_emul, 0xA2, 0.760 * 2 / 1.8 * 4095); // <-40°C
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_TEMPERATURES);
    zassert_equal(-40.0, roundf(bms.ic_data.cell_temp_avg * 10) / 10);

    isl94202_emul_set_word(bms_ic_emul, 0xA2, 0.4295 * 2 / 1.8 * 4095); // 30°C
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_TEMPERATURES);
    zassert_equal(30.0, roundf(bms.ic_data.cell_temp_avg * 10) / 10);

    // External temperature 2 (simple check)
    isl94202_emul_set_word(bms_ic_emul, 0xA4, 0.463 * 2 / 1.8 * 4095); // 25°C
    bms_ic_read_data(bms.ic_dev, BMS_IC_DATA_TEMPERATURES);
    zassert_equal(25.0, roundf(bms.ic_data.mosfet_temp * 10) / 10);
}

ZTEST(isl94202, test_isl94202_apply_dis_ocp)
{
    int err;

    // see datasheet table 10.4
    bms.ic_conf.dis_oc_delay_ms = 444;
    uint16_t delay = 444 + (1U << 10);

    // lower than minimum possible setting
    bms.ic_conf.dis_oc_limit = 1;
    err = bms_ic_configure(bms.ic_dev, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(BMS_IC_CONF_CURRENT_LIMITS, err);
    zassert_equal(2, bms.ic_conf.dis_oc_limit); // take lowest possible value
    zassert_equal(delay | (0x0 << 12), isl94202_emul_get_word(bms_ic_emul, 0x16));

    // something in the middle
    bms.ic_conf.dis_oc_limit = 20;
    err = bms_ic_configure(bms.ic_dev, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(BMS_IC_CONF_CURRENT_LIMITS, err);
    zassert_equal(16, bms.ic_conf.dis_oc_limit); // round to next lower value
    zassert_equal(delay | (0x4U << 12), isl94202_emul_get_word(bms_ic_emul, 0x16));

    // higher than maximum possible setting
    bms.ic_conf.dis_oc_limit = 50;
    err = bms_ic_configure(bms.ic_dev, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(BMS_IC_CONF_CURRENT_LIMITS, err);
    zassert_equal(48, bms.ic_conf.dis_oc_limit);
    zassert_equal(delay | (0x7U << 12), isl94202_emul_get_word(bms_ic_emul, 0x16));
}

ZTEST(isl94202, test_isl94202_apply_chg_ocp)
{
    int err;

    // see datasheet table 10.5
    bms.ic_conf.chg_oc_delay_ms = 333;
    uint16_t delay = 333 + (1U << 10);

    // lower than minimum possible setting
    bms.ic_conf.chg_oc_limit = 0.4;
    err = bms_ic_configure(bms.ic_dev, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(BMS_IC_CONF_CURRENT_LIMITS, err);
    zassert_equal(0.5, bms.ic_conf.chg_oc_limit); // take lowest possible value
    zassert_equal(delay | (0x0 << 12), isl94202_emul_get_word(bms_ic_emul, 0x18));

    // something in the middle
    bms.ic_conf.chg_oc_limit = 5.0;
    err = bms_ic_configure(bms.ic_dev, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(BMS_IC_CONF_CURRENT_LIMITS, err);
    zassert_equal(4.0, bms.ic_conf.chg_oc_limit); // round to next lower value
    zassert_equal(delay | (0x4U << 12), isl94202_emul_get_word(bms_ic_emul, 0x18));

    // higher than maximum possible setting
    bms.ic_conf.chg_oc_limit = 50.0;
    err = bms_ic_configure(bms.ic_dev, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(BMS_IC_CONF_CURRENT_LIMITS, err);
    zassert_equal(12.0, bms.ic_conf.chg_oc_limit);
    zassert_equal(delay | (0x7U << 12), isl94202_emul_get_word(bms_ic_emul, 0x18));
}

ZTEST(isl94202, test_isl94202_apply_dis_scp)
{
    int err;

    // see datasheet table 10.6
    bms.ic_conf.dis_sc_delay_us = 222;
    uint16_t delay = 222 + (0U << 10);

    // lower than minimum possible setting
    bms.ic_conf.dis_sc_limit = 5;
    err = bms_ic_configure(bms.ic_dev, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(BMS_IC_CONF_CURRENT_LIMITS, err);
    zassert_equal(8, bms.ic_conf.dis_sc_limit); // take lowest possible value
    zassert_equal(delay | (0x0 << 12), isl94202_emul_get_word(bms_ic_emul, 0x1A));

    // something in the middle
    bms.ic_conf.dis_sc_limit = 40;
    err = bms_ic_configure(bms.ic_dev, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(BMS_IC_CONF_CURRENT_LIMITS, err);
    zassert_equal(32, bms.ic_conf.dis_sc_limit); // round to next lower value
    zassert_equal(delay | (0x4U << 12), isl94202_emul_get_word(bms_ic_emul, 0x1A));

    // higher than maximum possible setting
    bms.ic_conf.dis_sc_limit = 150;
    err = bms_ic_configure(bms.ic_dev, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(BMS_IC_CONF_CURRENT_LIMITS, err);
    zassert_equal(128, bms.ic_conf.dis_sc_limit);
    zassert_equal(delay | (0x7U << 12), isl94202_emul_get_word(bms_ic_emul, 0x1A));
}

ZTEST(isl94202, test_isl94202_apply_cell_ovp)
{
    bms.ic_conf.cell_ov_limit = 4.251; // default value
    bms.ic_conf.cell_ov_reset = 4.15;
    bms.ic_conf.cell_ov_delay_ms = 999;
    uint16_t delay = 999 + (1U << 10);
    int err = bms_ic_configure(bms.ic_dev, &bms.ic_conf, BMS_IC_CONF_VOLTAGE_LIMITS);
    zassert_equal(BMS_IC_CONF_VOLTAGE_LIMITS, err);
    zassert_equal(0x1E2A, isl94202_emul_get_word(bms_ic_emul, 0x00)); // limit voltage
    zassert_equal(0x0DD4, isl94202_emul_get_word(bms_ic_emul, 0x02)); // recovery voltage
    zassert_equal(delay, isl94202_emul_get_word(bms_ic_emul, 0x10));  // delay
}

ZTEST(isl94202, test_isl94202_apply_cell_uvp)
{
    bms.ic_conf.cell_uv_limit = 2.7; // default value
    bms.ic_conf.cell_uv_reset = 3.0;
    bms.ic_conf.cell_uv_delay_ms = 2222;
    uint16_t delay = 2222 / 1000 + (2U << 10);
    int err = bms_ic_configure(bms.ic_dev, &bms.ic_conf, BMS_IC_CONF_VOLTAGE_LIMITS);
    zassert_equal(BMS_IC_CONF_VOLTAGE_LIMITS, err);
    zassert_equal(0x18FF, isl94202_emul_get_word(bms_ic_emul, 0x04));
    zassert_equal(0x09FF, isl94202_emul_get_word(bms_ic_emul, 0x06));
    zassert_equal(delay, isl94202_emul_get_word(bms_ic_emul, 0x12));
}

ZTEST(isl94202, test_isl94202_apply_chg_ot_limit)
{
    bms.ic_conf.chg_ot_limit = 55;
    bms.ic_conf.temp_limit_hyst = 5;
    int err = bms_ic_configure(bms.ic_dev, &bms.ic_conf, BMS_IC_CONF_TEMP_LIMITS);
    zassert_equal(BMS_IC_CONF_TEMP_LIMITS, err);
    zassert_equal(0x04D2, isl94202_emul_get_word(bms_ic_emul, 0x30)); // datasheet: 0x04B6
    zassert_equal(0x053E, isl94202_emul_get_word(bms_ic_emul, 0x32));
}

ZTEST(isl94202, test_isl94202_apply_chg_ut_limit)
{
    bms.ic_conf.chg_ut_limit = -10;
    bms.ic_conf.temp_limit_hyst = 15;
    int err = bms_ic_configure(bms.ic_dev, &bms.ic_conf, BMS_IC_CONF_TEMP_LIMITS);
    zassert_equal(BMS_IC_CONF_TEMP_LIMITS, err);
    zassert_equal(0x0CD1, isl94202_emul_get_word(bms_ic_emul, 0x34)); // datasheet: 0x0BF2
    zassert_equal(0x0BBD, isl94202_emul_get_word(bms_ic_emul, 0x36)); // datasheet: 0x0A93
}

ZTEST(isl94202, test_isl94202_apply_dis_ot_limit)
{
    bms.ic_conf.dis_ot_limit = 55;
    bms.ic_conf.temp_limit_hyst = 5;
    int err = bms_ic_configure(bms.ic_dev, &bms.ic_conf, BMS_IC_CONF_TEMP_LIMITS);
    zassert_equal(BMS_IC_CONF_TEMP_LIMITS, err);
    zassert_equal(0x04D2, isl94202_emul_get_word(bms_ic_emul, 0x38)); // datasheet: 0x04B6
    zassert_equal(0x053E, isl94202_emul_get_word(bms_ic_emul, 0x3A));
}

ZTEST(isl94202, test_isl94202_apply_dis_ut_limit)
{
    bms.ic_conf.dis_ut_limit = -10;
    bms.ic_conf.temp_limit_hyst = 15;
    int err = bms_ic_configure(bms.ic_dev, &bms.ic_conf, BMS_IC_CONF_TEMP_LIMITS);
    zassert_equal(BMS_IC_CONF_TEMP_LIMITS, err);
    zassert_equal(0x0CD1, isl94202_emul_get_word(bms_ic_emul, 0x3C)); // datasheet: 0x0BF2
    zassert_equal(0x0BBD, isl94202_emul_get_word(bms_ic_emul, 0x3E)); // datasheet: 0x0A93
}

static void *isl94202_setup(void)
{
    common_setup_bms_defaults();

    return NULL;
}

ZTEST_SUITE(isl94202, NULL, isl94202_setup, NULL, NULL, NULL);
