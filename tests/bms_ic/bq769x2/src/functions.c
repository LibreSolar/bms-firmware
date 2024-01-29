/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bms/bms.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/ztest.h>

#include "bq769x2_emul.h"

#include "bms_setup.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

static const struct device *bms_ic = DEVICE_DT_GET(DT_ALIAS(bms_ic));
static const struct emul *bms_ic_emul = EMUL_DT_GET(DT_ALIAS(bms_ic));

extern struct bms_context bms;

static const float shunt_res_mohm = DT_PROP(DT_ALIAS(bms_ic), shunt_resistor_uohm) / 1000.0F;

/*
 * Below tests use actual register numbers (instead of defines from registers.h) in order to
 * double-check the register defines vs. datasheet. Also magic numbers for min/max/default
 * values are obtained from the datasheet
 */

ZTEST(bq769x2_functions, test_apply_dis_scp)
{
    int err;

    // default
    bms.ic_conf.dis_sc_limit = 10.0F / shunt_res_mohm; // reg value 0 = 10 mV
    bms.ic_conf.dis_sc_delay_us = (2 - 1) * 15;        // 15 us (reg value 1 = no delay)
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(0, err);
    zassert_equal(10.0F / shunt_res_mohm, bms.ic_conf.dis_sc_limit);
    zassert_equal((2 - 1) * 15, bms.ic_conf.dis_sc_delay_us);
    zassert_equal(0, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9286));
    zassert_equal(2, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9287));

    // min
    bms.ic_conf.dis_sc_delay_us = (1 - 1) * 15; // 15 us (reg value 1 = no delay)
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(0, err);
    zassert_equal((1 - 1) * 15, bms.ic_conf.dis_sc_delay_us);
    zassert_equal(1, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9287));

    // too little
    bms.ic_conf.dis_sc_limit = 5.0F / shunt_res_mohm;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(0, err);
    zassert_equal(10.0F / shunt_res_mohm, bms.ic_conf.dis_sc_limit);
    zassert_equal(0, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9286));

    // max
    bms.ic_conf.dis_sc_limit = 500.0F / shunt_res_mohm; // reg value 015 = 500 mV
    bms.ic_conf.dis_sc_delay_us = (31 - 1) * 15;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(0, err);
    zassert_equal(500.0F / shunt_res_mohm, bms.ic_conf.dis_sc_limit);
    zassert_equal((31 - 1) * 15, bms.ic_conf.dis_sc_delay_us);
    zassert_equal(15, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9286));
    zassert_equal(31, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9287));

    // too much
    bms.ic_conf.dis_sc_limit = 600.0F / shunt_res_mohm;
    bms.ic_conf.dis_sc_delay_us = (32 - 1) * 15;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(0, err);
    zassert_equal(500.0F / shunt_res_mohm, bms.ic_conf.dis_sc_limit);
    zassert_equal((31 - 1) * 15, bms.ic_conf.dis_sc_delay_us);
    zassert_equal(15, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9286));
    zassert_equal(31, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9287));
}

ZTEST(bq769x2_functions, test_apply_chg_ocp)
{
    int err;

    // clear protection enable flag
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9261,
                              bq769x2_emul_get_data_mem(bms_ic_emul, 0x9261) & ~(1U << 4));

    // default
    bms.ic_conf.chg_oc_limit = 2 * 2.0F / shunt_res_mohm;
    bms.ic_conf.chg_oc_delay_ms = lroundf(6.6F + 4 * 3.3F); // 6.6 ms offset
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(0, err);
    zassert_equal(4.0F / shunt_res_mohm, bms.ic_conf.chg_oc_limit);
    zassert_equal(lroundf(6.6F + 4 * 3.3F), bms.ic_conf.chg_oc_delay_ms);
    zassert_equal(2, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9280));
    zassert_equal(4, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9281));

    // min
    bms.ic_conf.chg_oc_delay_ms = 6.6F + 1 * 3.3F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(0, err);
    zassert_equal(10, bms.ic_conf.chg_oc_delay_ms);
    zassert_equal(1, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9281));

    // too little
    bms.ic_conf.chg_oc_limit = 1 * 2.0F / shunt_res_mohm;
    bms.ic_conf.chg_oc_delay_ms = 3.3F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(0, err);
    zassert_equal(4.0F / shunt_res_mohm, bms.ic_conf.chg_oc_limit);
    zassert_equal(10.0F, bms.ic_conf.chg_oc_delay_ms);
    zassert_equal(2, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9280));
    zassert_equal(1, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9281));

    // max
    bms.ic_conf.chg_oc_limit = 62 * 2.0F / shunt_res_mohm;
    bms.ic_conf.chg_oc_delay_ms = 6.6F + 127 * 3.3F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(0, err);
    zassert_equal(62 * 2.0F / shunt_res_mohm, bms.ic_conf.chg_oc_limit);
    zassert_equal(lroundf(6.6F + 127 * 3.3F), bms.ic_conf.chg_oc_delay_ms);
    zassert_equal(62, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9280));
    zassert_equal(127, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9281));

    // too much
    bms.ic_conf.chg_oc_limit = 100 * 2.0F / shunt_res_mohm;
    bms.ic_conf.chg_oc_delay_ms = 6.6F + 150 * 3.3F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(0, err);
    zassert_equal(62 * 2.0F / shunt_res_mohm, bms.ic_conf.chg_oc_limit);
    zassert_equal(lroundf(6.6F + 127 * 3.3F), bms.ic_conf.chg_oc_delay_ms);
    zassert_equal(62, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9280));
    zassert_equal(127, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9281));

    // check if the protection was enabled
    zassert_true(bq769x2_emul_get_data_mem(bms_ic_emul, 0x9261) & (1U << 4));
}

ZTEST(bq769x2_functions, test_apply_dis_ocp)
{
    int err;

    // clear protection enable flag
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9261,
                              bq769x2_emul_get_data_mem(bms_ic_emul, 0x9261) & ~(1U << 5));

    // default
    bms.ic_conf.dis_oc_limit = 4 * 2.0F / shunt_res_mohm;
    bms.ic_conf.dis_oc_delay_ms = 6.6F + 1 * 3.3F; // 6.6 ms offset
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(0, err);
    zassert_equal(8.0F / shunt_res_mohm, bms.ic_conf.dis_oc_limit);
    zassert_equal(10.0F, bms.ic_conf.dis_oc_delay_ms);
    zassert_equal(4, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9282));
    zassert_equal(1, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9283));

    // min
    bms.ic_conf.dis_oc_limit = 2 * 2.0F / shunt_res_mohm;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(0, err);
    zassert_equal(4.0F / shunt_res_mohm, bms.ic_conf.dis_oc_limit);
    zassert_equal(2, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9282));

    // too little
    bms.ic_conf.dis_oc_limit = 1 * 2.0F / shunt_res_mohm;
    bms.ic_conf.dis_oc_delay_ms = 3.3F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(0, err);
    zassert_equal(4.0F / shunt_res_mohm, bms.ic_conf.dis_oc_limit);
    zassert_equal(10.0F, bms.ic_conf.dis_oc_delay_ms);
    zassert_equal(2, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9282));
    zassert_equal(1, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9283));

    // max
    bms.ic_conf.dis_oc_limit = 100 * 2.0F / shunt_res_mohm;
    bms.ic_conf.dis_oc_delay_ms = 6.6F + 127 * 3.3F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(0, err);
    zassert_equal(200.0F / shunt_res_mohm, bms.ic_conf.dis_oc_limit);
    zassert_equal(lroundf(6.6F + 127 * 3.3F), bms.ic_conf.dis_oc_delay_ms);
    zassert_equal(100, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9282));
    zassert_equal(127, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9283));

    // too much
    bms.ic_conf.dis_oc_limit = 120 * 2.0F / shunt_res_mohm;
    bms.ic_conf.dis_oc_delay_ms = 6.6F + 150 * 3.3F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_CURRENT_LIMITS);
    zassert_equal(0, err);
    zassert_equal(200.0F / shunt_res_mohm, bms.ic_conf.dis_oc_limit);
    zassert_equal(lroundf(6.6F + 127 * 3.3F), bms.ic_conf.dis_oc_delay_ms);
    zassert_equal(100, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9282));
    zassert_equal(127, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9283));

    // check if the protection was enabled
    zassert_true(bq769x2_emul_get_data_mem(bms_ic_emul, 0x9261) & (1U << 5));
}

ZTEST(bq769x2_functions, test_apply_cell_uvp)
{
    int err;

    // default
    bms.ic_conf.cell_uv_limit = 50 * 50.6F / 1000.0F;
    bms.ic_conf.cell_uv_reset = 52 * 50.6F / 1000.0F;
    bms.ic_conf.cell_uv_delay_ms = 74 * 3.3F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_VOLTAGE_LIMITS);
    zassert_equal(0, err);
    zassert_equal(50 * 50.6F / 1000.0F, bms.ic_conf.cell_uv_limit);
    zassert_equal(52 * 50.6F / 1000.0F, bms.ic_conf.cell_uv_reset);
    zassert_equal(roundf(74 * 3.3F), bms.ic_conf.cell_uv_delay_ms);
    zassert_equal(50, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9275));
    zassert_equal(74, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9276));
    zassert_equal(0, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9277));
    zassert_equal(2, bq769x2_emul_get_data_mem(bms_ic_emul, 0x927B));

    // min
    bms.ic_conf.cell_uv_limit = 20 * 50.6F / 1000.0F;
    bms.ic_conf.cell_uv_reset = 22 * 50.6F / 1000.0F;
    bms.ic_conf.cell_uv_delay_ms = 1 * 3.3F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_VOLTAGE_LIMITS);
    zassert_equal(0, err);
    zassert_equal(20 * 50.6F / 1000.0F, bms.ic_conf.cell_uv_limit);
    zassert_equal(22 * 50.6F / 1000.0F, bms.ic_conf.cell_uv_reset);
    zassert_equal(roundf(1 * 3.3F), bms.ic_conf.cell_uv_delay_ms);
    zassert_equal(20, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9275));
    zassert_equal(1, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9276));
    zassert_equal(0, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9277));
    zassert_equal(2, bq769x2_emul_get_data_mem(bms_ic_emul, 0x927B));

    // too little
    bms.ic_conf.cell_uv_limit = 19 * 50.6F / 1000.0F;
    bms.ic_conf.cell_uv_reset = 18 * 50.6F / 1000.0F;
    bms.ic_conf.cell_uv_delay_ms = 0.0F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_VOLTAGE_LIMITS);
    zassert_equal(0, err);
    zassert_equal(20 * 50.6F / 1000.0F, bms.ic_conf.cell_uv_limit);
    zassert_equal(22 * 50.6F / 1000.0F, bms.ic_conf.cell_uv_reset);
    zassert_equal(roundf(1 * 3.3F), bms.ic_conf.cell_uv_delay_ms);
    zassert_equal(20, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9275));
    zassert_equal(1, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9276));
    zassert_equal(0, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9277));
    zassert_equal(2, bq769x2_emul_get_data_mem(bms_ic_emul, 0x927B));

    // max
    bms.ic_conf.cell_uv_limit = 90 * 50.6F / 1000.0F;
    bms.ic_conf.cell_uv_reset = 110 * 50.6F / 1000.0F;
    bms.ic_conf.cell_uv_delay_ms = 2047 * 3.3F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_VOLTAGE_LIMITS);
    zassert_equal(0, err);
    zassert_equal(90 * 50.6F / 1000.0F, bms.ic_conf.cell_uv_limit);
    zassert_equal(110 * 50.6F / 1000.0F, bms.ic_conf.cell_uv_reset);
    zassert_equal(roundf(2047 * 3.3F), bms.ic_conf.cell_uv_delay_ms);
    zassert_equal(90, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9275));
    zassert_equal(2047U & 0xFF, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9276));
    zassert_equal(2047U >> 8, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9277));
    zassert_equal(20, bq769x2_emul_get_data_mem(bms_ic_emul, 0x927B));

    // too much
    bms.ic_conf.cell_uv_limit = 91 * 50.6F / 1000.0F;
    bms.ic_conf.cell_uv_reset = 112 * 50.6F / 1000.0F;
    bms.ic_conf.cell_uv_delay_ms = 2048 * 3.3F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_VOLTAGE_LIMITS);
    zassert_equal(0, err);
    zassert_equal(90 * 50.6F / 1000.0F, bms.ic_conf.cell_uv_limit);
    zassert_equal(110 * 50.6F / 1000.0F, bms.ic_conf.cell_uv_reset);
    zassert_equal(roundf(2047 * 3.3F), bms.ic_conf.cell_uv_delay_ms);
    zassert_equal(90, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9275));
    zassert_equal(2047U & 0xFF, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9276));
    zassert_equal(2047U >> 8, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9277));
    zassert_equal(20, bq769x2_emul_get_data_mem(bms_ic_emul, 0x927B));
}

ZTEST(bq769x2_functions, test_apply_cell_ovp)
{
    int err;

    // default
    bms.ic_conf.cell_ov_limit = 86 * 50.6F / 1000.0F;
    bms.ic_conf.cell_ov_reset = 84 * 50.6F / 1000.0F;
    bms.ic_conf.cell_ov_delay_ms = 74 * 3.3F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_VOLTAGE_LIMITS);
    zassert_equal(0, err);
    zassert_equal(86 * 50.6F / 1000.0F, bms.ic_conf.cell_ov_limit);
    zassert_equal(84 * 50.6F / 1000.0F, bms.ic_conf.cell_ov_reset);
    zassert_equal(roundf(74 * 3.3F), bms.ic_conf.cell_ov_delay_ms);
    zassert_equal(86, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9278));
    zassert_equal(74, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9279));
    zassert_equal(0, bq769x2_emul_get_data_mem(bms_ic_emul, 0x927A));
    zassert_equal(2, bq769x2_emul_get_data_mem(bms_ic_emul, 0x927C));

    // min
    bms.ic_conf.cell_ov_limit = 20 * 50.6F / 1000.0F;
    bms.ic_conf.cell_ov_reset = 18 * 50.6F / 1000.0F;
    bms.ic_conf.cell_ov_delay_ms = 1 * 3.3F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_VOLTAGE_LIMITS);
    zassert_equal(0, err);
    zassert_equal(20 * 50.6F / 1000.0F, bms.ic_conf.cell_ov_limit);
    zassert_equal(18 * 50.6F / 1000.0F, bms.ic_conf.cell_ov_reset);
    zassert_equal(roundf(1 * 3.3F), bms.ic_conf.cell_ov_delay_ms);
    zassert_equal(20, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9278));
    zassert_equal(1, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9279));
    zassert_equal(0, bq769x2_emul_get_data_mem(bms_ic_emul, 0x927A));
    zassert_equal(2, bq769x2_emul_get_data_mem(bms_ic_emul, 0x927C));

    // too little
    bms.ic_conf.cell_ov_limit = 19 * 50.6F / 1000.0F;
    bms.ic_conf.cell_ov_reset = 20 * 50.6F / 1000.0F;
    bms.ic_conf.cell_ov_delay_ms = 0.0F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_VOLTAGE_LIMITS);
    zassert_equal(0, err);
    zassert_equal(20 * 50.6F / 1000.0F, bms.ic_conf.cell_ov_limit);
    zassert_equal(18 * 50.6F / 1000.0F, bms.ic_conf.cell_ov_reset);
    zassert_equal(roundf(1 * 3.3F), bms.ic_conf.cell_ov_delay_ms);
    zassert_equal(20, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9278));
    zassert_equal(1, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9279));
    zassert_equal(0, bq769x2_emul_get_data_mem(bms_ic_emul, 0x927A));
    zassert_equal(2, bq769x2_emul_get_data_mem(bms_ic_emul, 0x927C));

    // max
    bms.ic_conf.cell_ov_limit = 110 * 50.6F / 1000.0F;
    bms.ic_conf.cell_ov_reset = 90 * 50.6F / 1000.0F;
    bms.ic_conf.cell_ov_delay_ms = 2047 * 3.3F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_VOLTAGE_LIMITS);
    zassert_equal(0, err);
    zassert_equal(110 * 50.6F / 1000.0F, bms.ic_conf.cell_ov_limit);
    zassert_equal(90 * 50.6F / 1000.0F, bms.ic_conf.cell_ov_reset);
    zassert_equal(roundf(2047 * 3.3F), bms.ic_conf.cell_ov_delay_ms);
    zassert_equal(110, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9278));
    zassert_equal(2047U & 0xFF, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9279));
    zassert_equal(2047U >> 8, bq769x2_emul_get_data_mem(bms_ic_emul, 0x927A));
    zassert_equal(20, bq769x2_emul_get_data_mem(bms_ic_emul, 0x927C));

    // too much
    bms.ic_conf.cell_ov_limit = 111 * 50.6F / 1000.0F;
    bms.ic_conf.cell_ov_reset = 112 * 50.6F / 1000.0F;
    bms.ic_conf.cell_ov_delay_ms = 2048 * 3.3F;
    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_VOLTAGE_LIMITS);
    zassert_equal(0, err);
    zassert_equal(110 * 50.6F / 1000.0F, bms.ic_conf.cell_ov_limit);
    zassert_equal(108 * 50.6F / 1000.0F, bms.ic_conf.cell_ov_reset);
    zassert_equal(roundf(2047 * 3.3F), bms.ic_conf.cell_ov_delay_ms);
    zassert_equal(110, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9278));
    zassert_equal(2047U & 0xFF, bq769x2_emul_get_data_mem(bms_ic_emul, 0x9279));
    zassert_equal(2047U >> 8, bq769x2_emul_get_data_mem(bms_ic_emul, 0x927A));
    zassert_equal(2, bq769x2_emul_get_data_mem(bms_ic_emul, 0x927C));
}

ZTEST(bq769x2_functions, test_apply_temp_limits)
{
    int err;

    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9262, 0);

    // default
    bms.ic_conf.dis_ot_limit = 60;
    bms.ic_conf.dis_ut_limit = 0;
    bms.ic_conf.chg_ot_limit = 55;
    bms.ic_conf.chg_ut_limit = 0;
    bms.ic_conf.temp_limit_hyst = 5;

    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_TEMP_LIMITS);
    zassert_equal(0, err);

    zassert_equal(5, bms.ic_conf.temp_limit_hyst);

    zassert_equal(60, bms.ic_conf.dis_ot_limit);
    zassert_equal(60, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929D));
    zassert_equal(55, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929F));

    zassert_equal(0, bms.ic_conf.dis_ut_limit);
    zassert_equal(0, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92A9));
    zassert_equal(5, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92AB));

    zassert_equal(55, bms.ic_conf.chg_ot_limit);
    zassert_equal(55, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929A));
    zassert_equal(50, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929C));

    zassert_equal(0, bms.ic_conf.chg_ut_limit);
    zassert_equal(0, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92A6));
    zassert_equal(5, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92A8));

    // min
    bms.ic_conf.dis_ot_limit = 0;
    bms.ic_conf.dis_ut_limit = -40;
    bms.ic_conf.chg_ot_limit = 0;
    bms.ic_conf.chg_ut_limit = -40;
    bms.ic_conf.temp_limit_hyst = 1;

    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_TEMP_LIMITS);
    zassert_equal(0, err);

    zassert_equal(1, bms.ic_conf.temp_limit_hyst);

    zassert_equal(0, bms.ic_conf.dis_ot_limit);
    zassert_equal(0, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929D));
    zassert_equal(-1, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929F));

    zassert_equal(-40, bms.ic_conf.dis_ut_limit);
    zassert_equal(-40, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92A9));
    zassert_equal(-39, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92AB));

    zassert_equal(0, bms.ic_conf.chg_ot_limit);
    zassert_equal(0, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929A));
    zassert_equal(-1, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929C));

    zassert_equal(-40, bms.ic_conf.chg_ut_limit);
    zassert_equal(-40, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92A6));
    zassert_equal(-39, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92A8));

    // too little
    bms.ic_conf.dis_ot_limit = -50;
    bms.ic_conf.dis_ut_limit = -50;
    bms.ic_conf.chg_ot_limit = -50;
    bms.ic_conf.chg_ut_limit = -50;
    bms.ic_conf.temp_limit_hyst = 0;

    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_TEMP_LIMITS);
    zassert_equal(-EIO, err); // should fail

    bms.ic_conf.dis_ot_limit = 0;
    bms.ic_conf.dis_ut_limit = -50;
    bms.ic_conf.chg_ot_limit = 0;
    bms.ic_conf.chg_ut_limit = -50;
    bms.ic_conf.temp_limit_hyst = 0;

    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_TEMP_LIMITS);
    zassert_equal(0, err);

    zassert_equal(1, bms.ic_conf.temp_limit_hyst);

    zassert_equal(0, bms.ic_conf.dis_ot_limit);
    zassert_equal(0, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929D));
    zassert_equal(-1, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929F));

    zassert_equal(-40, bms.ic_conf.dis_ut_limit);
    zassert_equal(-40, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92A9));
    zassert_equal(-39, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92AB));

    zassert_equal(0, bms.ic_conf.chg_ot_limit);
    zassert_equal(0, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929A));
    zassert_equal(-1, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929C));

    zassert_equal(-40, bms.ic_conf.chg_ut_limit);
    zassert_equal(-40, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92A6));
    zassert_equal(-39, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92A8));

    // max
    bms.ic_conf.dis_ot_limit = 120;
    bms.ic_conf.dis_ut_limit = 100;
    bms.ic_conf.chg_ot_limit = 120;
    bms.ic_conf.chg_ut_limit = 100;
    bms.ic_conf.temp_limit_hyst = 20;

    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_TEMP_LIMITS);
    zassert_equal(0, err);

    zassert_equal(20, bms.ic_conf.temp_limit_hyst);

    zassert_equal(120, bms.ic_conf.dis_ot_limit);
    zassert_equal(120, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929D));
    zassert_equal(100, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929F));

    zassert_equal(100, bms.ic_conf.dis_ut_limit);
    zassert_equal(100, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92A9));
    zassert_equal(120, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92AB));

    zassert_equal(120, bms.ic_conf.chg_ot_limit);
    zassert_equal(120, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929A));
    zassert_equal(100, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929C));

    zassert_equal(100, bms.ic_conf.chg_ut_limit);
    zassert_equal(100, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92A6));
    zassert_equal(120, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92A8));

    // too much
    bms.ic_conf.dis_ot_limit = 130;
    bms.ic_conf.dis_ut_limit = 100;
    bms.ic_conf.chg_ot_limit = 130;
    bms.ic_conf.chg_ut_limit = 100;
    bms.ic_conf.temp_limit_hyst = 30;

    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_TEMP_LIMITS);
    zassert_equal(0, err);

    zassert_equal(20, bms.ic_conf.temp_limit_hyst);

    zassert_equal(120, bms.ic_conf.dis_ot_limit);
    zassert_equal(120, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929D));
    zassert_equal(100, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929F));

    zassert_equal(100, bms.ic_conf.dis_ut_limit);
    zassert_equal(100, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92A9));
    zassert_equal(120, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92AB));

    zassert_equal(120, bms.ic_conf.chg_ot_limit);
    zassert_equal(120, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929A));
    zassert_equal(100, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x929C));

    zassert_equal(100, bms.ic_conf.chg_ut_limit);
    zassert_equal(100, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92A6));
    zassert_equal(120, (int8_t)bq769x2_emul_get_data_mem(bms_ic_emul, 0x92A8));

    // check if the protection was enabled
    zassert_equal((1U << 0) | (1U << 1) | (1U << 4) | (1U << 5),
                  bq769x2_emul_get_data_mem(bms_ic_emul, 0x9262));
}

static void *bq769x2_setup(void)
{
    common_setup_bms_defaults(&bms);

    return NULL;
}

ZTEST_SUITE(bq769x2_functions, NULL, bq769x2_setup, NULL, NULL, NULL);
