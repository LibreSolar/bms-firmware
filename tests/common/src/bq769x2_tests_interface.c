/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/emul.h>
#include <zephyr/ztest.h>

#include "bq769x2_emul.h"
#include "bq769x2_interface.h"
#include <bms/bms.h>

#include "common.h"

#include <stdio.h>
#include <time.h>

static const struct device *bms_ic = DEVICE_DT_GET(DT_ALIAS(bms_ic));
static const struct emul *bms_ic_emul = EMUL_DT_GET(DT_ALIAS(bms_ic));

extern struct bms_context bms;

ZTEST(bq769x2_interface, test_bq769x2_direct_read_u2)
{
    uint16_t u2 = 0;
    int err;

    bq769x2_emul_set_direct_mem(bms_ic_emul, 0, 0x00);
    bq769x2_emul_set_direct_mem(bms_ic_emul, 1, 0x00);
    err = bq769x2_direct_read_u2(bms_ic, 0, &u2);
    zassert_equal(0, err);
    zassert_equal(0, u2);

    bq769x2_emul_set_direct_mem(bms_ic_emul, 0, 0xFF);
    bq769x2_emul_set_direct_mem(bms_ic_emul, 1, 0xFF);
    err = bq769x2_direct_read_u2(bms_ic, 0, &u2);
    zassert_equal(0, err);
    zassert_equal(UINT16_MAX, u2);
}

ZTEST(bq769x2_interface, test_bq769x2_direct_read_i2)
{
    int16_t i2 = 0;
    int err;

    bq769x2_emul_set_direct_mem(bms_ic_emul, 0, 0x00);
    bq769x2_emul_set_direct_mem(bms_ic_emul, 1, 0x00);
    err = bq769x2_direct_read_i2(bms_ic, 0, &i2);
    zassert_equal(0, err);
    zassert_equal(0, i2);

    bq769x2_emul_set_direct_mem(bms_ic_emul, 0, 0xFF);
    bq769x2_emul_set_direct_mem(bms_ic_emul, 1, 0xFF);
    err = bq769x2_direct_read_i2(bms_ic, 0, &i2);
    zassert_equal(0, err);
    zassert_equal(-1, i2);

    bq769x2_emul_set_direct_mem(bms_ic_emul, 0, 0xFF);
    bq769x2_emul_set_direct_mem(bms_ic_emul, 1, 0x7F);
    err = bq769x2_direct_read_i2(bms_ic, 0, &i2);
    zassert_equal(0, err);
    zassert_equal(INT16_MAX, i2);

    bq769x2_emul_set_direct_mem(bms_ic_emul, 0, 0x00);
    bq769x2_emul_set_direct_mem(bms_ic_emul, 1, 0x80);
    err = bq769x2_direct_read_i2(bms_ic, 0, &i2);
    zassert_equal(0, err);
    zassert_equal(INT16_MIN, i2);
}

ZTEST(bq769x2_interface, test_bq769x2_subcmd_cmd_only)
{
    // reset subcommand
    uint8_t subcmd_expected[2] = { 0x12, 0x00 }; // LOWER, UPPER

    // pre-set register
    bq769x2_emul_set_direct_mem(bms_ic_emul, 0x3E, 0xFF);
    bq769x2_emul_set_direct_mem(bms_ic_emul, 0x3F, 0xFF);

    // write subcmd register via API
    int err = bq769x2_subcmd_cmd_only(bms_ic, 0x0012);
    zassert_equal(0, err);

    zassert_equal(subcmd_expected[0], bq769x2_emul_get_direct_mem(bms_ic_emul, 0x3E));
    zassert_equal(subcmd_expected[1], bq769x2_emul_get_direct_mem(bms_ic_emul, 0x3F));
}

ZTEST(bq769x2_interface, test_bq769x2_datamem_read_u1)
{
    uint8_t value = 0;

    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9180, 0xFF);
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9181, 0x00);
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9182, 0x00);
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9183, 0x00);

    uint8_t chk_expected = (uint8_t) ~(0x91 + 0x80 + 0xFF);

    int err = bq769x2_datamem_read_u1(bms_ic, 0x9180, &value);
    zassert_equal(0, err);
    zassert_equal(UINT8_MAX, value);
    zassert_equal(chk_expected, bq769x2_emul_get_direct_mem(bms_ic_emul, 0x60));
}

ZTEST(bq769x2_interface, test_bq769x2_datamem_read_u2)
{
    uint16_t value = 0;

    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9180, 0x00); // LSB
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9181, 0xFF); // MSB
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9182, 0x00);
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9183, 0x00);

    uint8_t chk_expected = (uint8_t) ~(0x91 + 0x80 + 0x00 + 0xFF);

    int err = bq769x2_datamem_read_u2(bms_ic, 0x9180, &value);
    zassert_equal(0, err);
    zassert_equal(0xFF00, value);
    zassert_equal(chk_expected, bq769x2_emul_get_direct_mem(bms_ic_emul, 0x60));
}

ZTEST(bq769x2_interface, test_bq769x2_datamem_read_i1)
{
    int8_t value = 0;

    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9180, 0x80);
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9181, 0x00);
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9182, 0x00);
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9183, 0x00);

    uint8_t chk_expected = (uint8_t) ~(0x91 + 0x80 + 0x80);

    int err = bq769x2_datamem_read_i1(bms_ic, 0x9180, &value);
    zassert_equal(0, err);
    zassert_equal(INT8_MIN, value);
    zassert_equal(chk_expected, bq769x2_emul_get_direct_mem(bms_ic_emul, 0x60));
}

ZTEST(bq769x2_interface, test_bq769x2_datamem_read_i2)
{
    int16_t value = 0;

    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9180, 0x00); // LSB
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9181, 0x80); // MSB
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9182, 0x00);
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9183, 0x00);

    uint8_t chk_expected = (uint8_t) ~(0x91 + 0x80 + 0x00 + 0x80);

    int err = bq769x2_datamem_read_i2(bms_ic, 0x9180, &value);
    zassert_equal(0, err);
    zassert_equal(INT16_MIN, value);
    zassert_equal(chk_expected, bq769x2_emul_get_direct_mem(bms_ic_emul, 0x60));
}

ZTEST(bq769x2_interface, test_bq769x2_datamem_read_f4)
{
    float value = 0.0F;

    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9180, 0xB6); // LSB
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9181, 0xF3);
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9182, 0x9D);
    bq769x2_emul_set_data_mem(bms_ic_emul, 0x9183, 0x3F); // MSB

    uint8_t chk_expected = (uint8_t) ~(0x91 + 0x80 + 0xB6 + 0xF3 + 0x9D + 0x3F);

    int err = bq769x2_datamem_read_f4(bms_ic, 0x9180, &value);
    zassert_equal(0, err);
    zassert_equal(1.234F, value);
    zassert_equal(chk_expected, bq769x2_emul_get_direct_mem(bms_ic_emul, 0x60));
}

ZTEST(bq769x2_interface, test_bq769x2_datamem_write_u1)
{
    uint8_t data_expected[] = { 0xFF };
    uint8_t chk_expected = (uint8_t) ~(0x91 + 0x80 + 0xFF);
    uint8_t len_expected = 4 + sizeof(data_expected);

    bq769x2_config_update_mode(bms_ic, true);

    int err = bq769x2_datamem_write_u1(bms_ic, 0x9180, UINT8_MAX);
    zassert_equal(0, err);

    zassert_equal(data_expected[0], bq769x2_emul_get_direct_mem(bms_ic_emul, 0x40));
    zassert_equal(chk_expected, bq769x2_emul_get_direct_mem(bms_ic_emul, 0x60));
    zassert_equal(len_expected, bq769x2_emul_get_direct_mem(bms_ic_emul, 0x61));

    bq769x2_config_update_mode(bms_ic, false);
}

ZTEST(bq769x2_interface, test_bq769x2_datamem_write_u2)
{
    uint8_t data_expected[] = { 0x00, 0xFF };
    uint8_t chk_expected = (uint8_t) ~(0x91 + 0x80 + 0xFF);
    uint8_t len_expected = 4 + sizeof(data_expected);

    bq769x2_config_update_mode(bms_ic, true);

    int err = bq769x2_datamem_write_u2(bms_ic, 0x9180, 0xFF00);
    zassert_equal(0, err);

    zassert_equal(data_expected[0], bq769x2_emul_get_direct_mem(bms_ic_emul, 0x40));
    zassert_equal(data_expected[1], bq769x2_emul_get_direct_mem(bms_ic_emul, 0x41));
    zassert_equal(chk_expected, bq769x2_emul_get_direct_mem(bms_ic_emul, 0x60));
    zassert_equal(len_expected, bq769x2_emul_get_direct_mem(bms_ic_emul, 0x61));

    bq769x2_config_update_mode(bms_ic, false);
}

ZTEST(bq769x2_interface, test_bq769x2_datamem_write_i1)
{
    uint8_t data_expected[] = { 0x80 };
    uint8_t chk_expected = (uint8_t) ~(0x91 + 0x80 + 0x80);
    uint8_t len_expected = 4 + sizeof(data_expected);

    bq769x2_config_update_mode(bms_ic, true);

    int err = bq769x2_datamem_write_i1(bms_ic, 0x9180, INT8_MIN);
    zassert_equal(0, err);

    zassert_equal(data_expected[0], bq769x2_emul_get_direct_mem(bms_ic_emul, 0x40));
    zassert_equal(chk_expected, bq769x2_emul_get_direct_mem(bms_ic_emul, 0x60));
    zassert_equal(len_expected, bq769x2_emul_get_direct_mem(bms_ic_emul, 0x61));

    bq769x2_config_update_mode(bms_ic, false);
}

ZTEST(bq769x2_interface, test_bq769x2_datamem_write_i2)
{
    uint8_t data_expected[] = { 0x00, 0x80 };
    uint8_t chk_expected = (uint8_t) ~(0x91 + 0x80 + 0x00 + 0x80);
    uint8_t len_expected = 4 + sizeof(data_expected);

    bq769x2_config_update_mode(bms_ic, true);

    int err = bq769x2_datamem_write_i2(bms_ic, 0x9180, INT16_MIN);
    zassert_equal(0, err);

    zassert_equal(data_expected[0], bq769x2_emul_get_direct_mem(bms_ic_emul, 0x40));
    zassert_equal(data_expected[1], bq769x2_emul_get_direct_mem(bms_ic_emul, 0x41));
    zassert_equal(chk_expected, bq769x2_emul_get_direct_mem(bms_ic_emul, 0x60));
    zassert_equal(len_expected, bq769x2_emul_get_direct_mem(bms_ic_emul, 0x61));

    bq769x2_config_update_mode(bms_ic, false);
}

ZTEST(bq769x2_interface, test_bq769x2_datamem_write_f4)
{
    uint8_t data_expected[] = { 0xB6, 0xF3, 0x9D, 0x3F };
    uint8_t chk_expected = (uint8_t) ~(0x91 + 0x80 + 0xB6 + 0xF3 + 0x9D + 0x3F);
    uint8_t len_expected = 4 + sizeof(data_expected);

    bq769x2_config_update_mode(bms_ic, true);

    int err = bq769x2_datamem_write_f4(bms_ic, 0x9180, 1.234F);
    zassert_equal(0, err);

    zassert_equal(data_expected[0], bq769x2_emul_get_direct_mem(bms_ic_emul, 0x40));
    zassert_equal(data_expected[1], bq769x2_emul_get_direct_mem(bms_ic_emul, 0x41));
    zassert_equal(data_expected[2], bq769x2_emul_get_direct_mem(bms_ic_emul, 0x42));
    zassert_equal(data_expected[3], bq769x2_emul_get_direct_mem(bms_ic_emul, 0x43));
    zassert_equal(chk_expected, bq769x2_emul_get_direct_mem(bms_ic_emul, 0x60));
    zassert_equal(len_expected, bq769x2_emul_get_direct_mem(bms_ic_emul, 0x61));

    bq769x2_config_update_mode(bms_ic, false);
}

static void *bq769x2_setup(void)
{
    common_setup_bms_defaults(&bms);

    return NULL;
}

ZTEST_SUITE(bq769x2_interface, NULL, bq769x2_setup, NULL, NULL, NULL);
