/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bq769x2_tests.h"

#include "bms.h"
#include "bq769x2/interface.h"

#include "unity.h"

#include <time.h>
#include <stdio.h>

extern BmsConfig bms_conf;
extern BmsStatus bms_status;

// defined in bq769x2_interface_stub
extern uint8_t mem_bq_direct[BQ_DIRECT_MEM_SIZE];
extern uint8_t mem_bq_subcmd[BQ_SUBCMD_MEM_SIZE];

void test_bq769x2_direct_read_u2()
{
    uint16_t u2 = 0;
    int err;

    mem_bq_direct[0] = 0x00;
    mem_bq_direct[1] = 0x00;
    err = bq769x2_direct_read_u2(0, &u2);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL(0, u2);

    mem_bq_direct[0] = 0xFF;
    mem_bq_direct[1] = 0xFF;
    err = bq769x2_direct_read_u2(0, &u2);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL(UINT16_MAX, u2);
}

void test_bq769x2_direct_read_i2()
{
    int16_t i2 = 0;
    int err;

    mem_bq_direct[0] = 0x00;
    mem_bq_direct[1] = 0x00;
    err = bq769x2_direct_read_i2(0, &i2);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL(0, i2);

    mem_bq_direct[0] = 0xFF;
    mem_bq_direct[1] = 0xFF;
    err = bq769x2_direct_read_i2(0, &i2);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL(-1, i2);

    mem_bq_direct[0] = 0xFF;
    mem_bq_direct[1] = 0x7F;
    err = bq769x2_direct_read_i2(0, &i2);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL(INT16_MAX, i2);

    mem_bq_direct[0] = 0x00;
    mem_bq_direct[1] = 0x80;
    err = bq769x2_direct_read_i2(0, &i2);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL(INT16_MIN, i2);
}

void test_bq769x2_subcmd_read_u1()
{
    uint8_t value = 0;

    mem_bq_direct[0x40] = 0xFF;

    mem_bq_direct[0x60] = 0;        // checksum
    mem_bq_direct[0x61] = 4 + 1;    // length

    int err = bq769x2_subcmd_read_u1(0, &value);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL(UINT8_MAX, value);
}

void test_bq769x2_subcmd_read_u2()
{
    uint16_t value = 0;

    mem_bq_direct[0x40] = 0x00;     // LSB
    mem_bq_direct[0x41] = 0xFF;     // MSB

    mem_bq_direct[0x60] = 0;        // checksum
    mem_bq_direct[0x61] = 4 + 2;    // length

    int err = bq769x2_subcmd_read_u2(0, &value);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL(0xFF00, value);
}

void test_bq769x2_subcmd_read_u4()
{
    uint32_t value = 0;

    mem_bq_direct[0x40] = 0xAA;     // LSB
    mem_bq_direct[0x41] = 0xBB;
    mem_bq_direct[0x42] = 0xCC;
    mem_bq_direct[0x43] = 0xDD;     // MSB

    mem_bq_direct[0x60] = (uint8_t)~(0xAA + 0xBB + 0xCC + 0xDD);
    mem_bq_direct[0x61] = 4 + 4;

    int err = bq769x2_subcmd_read_u4(0, &value);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL(0xDDCCBBAA, value);
}

void test_bq769x2_subcmd_read_i1()
{
    int8_t value = 0;

    mem_bq_direct[0x40] = 0x80;

    mem_bq_direct[0x60] = (uint8_t)~0x80;   // checksum
    mem_bq_direct[0x61] = 4 + 1;            // length

    int err = bq769x2_subcmd_read_i1(0, &value);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL(INT8_MIN, value);
}

void test_bq769x2_subcmd_read_i2()
{
    int16_t value = 0;

    mem_bq_direct[0x40] = 0x00;     // LSB
    mem_bq_direct[0x41] = 0x80;     // MSB

    mem_bq_direct[0x60] = (uint8_t)~0x80;   // checksum
    mem_bq_direct[0x61] = 4 + 2;            // length

    int err = bq769x2_subcmd_read_i2(0, &value);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL(INT16_MIN, value);
}

void test_bq769x2_subcmd_read_i4()
{
    int32_t value = 0;

    mem_bq_direct[0x40] = 0x00;     // LSB
    mem_bq_direct[0x41] = 0x00;
    mem_bq_direct[0x42] = 0x00;
    mem_bq_direct[0x43] = 0x80;     // MSB

    mem_bq_direct[0x60] = (uint8_t)~0x80;   // checksum
    mem_bq_direct[0x61] = 4 + 4;            // length

    int err = bq769x2_subcmd_read_i4(0, &value);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL(INT32_MIN, value);
}

void test_bq769x2_subcmd_read_f4()
{
    float value = 0.0F;

    mem_bq_direct[0x40] = 0xB6;     // LSB
    mem_bq_direct[0x41] = 0xF3;
    mem_bq_direct[0x42] = 0x9D;
    mem_bq_direct[0x43] = 0x3F;     // MSB

    mem_bq_direct[0x60] = (uint8_t)~(0xB6 + 0xF3 + 0x9D + 0x3F);
    mem_bq_direct[0x61] = 4 + 4;

    int err = bq769x2_subcmd_read_f4(0, &value);
    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_EQUAL_FLOAT(1.234F, value);
}

void test_bq769x2_subcmd_write_u1()
{
    uint8_t chk_len_expected[2];
    uint8_t data_expected[4];

    data_expected[0x0] = 0xFF;

    chk_len_expected[0x0] = 0;        // checksum
    chk_len_expected[0x1] = 4 + 1;    // length

    int err = bq769x2_subcmd_write_u1(0, UINT8_MAX);
    TEST_ASSERT_EQUAL(0, err);

    TEST_ASSERT_EQUAL_HEX8_ARRAY(chk_len_expected, mem_bq_direct + 0x60, 2);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(data_expected, mem_bq_direct + 0x40, 1);
}

void test_bq769x2_subcmd_write_u2()
{
    uint8_t chk_len_expected[2];
    uint8_t data_expected[4];

    data_expected[0x0] = 0x00;     // LSB
    data_expected[0x1] = 0xFF;     // MSB

    chk_len_expected[0x0] = 0;        // checksum
    chk_len_expected[0x1] = 4 + 2;    // length

    int err = bq769x2_subcmd_write_u2(0, 0xFF00);
    TEST_ASSERT_EQUAL(0, err);

    TEST_ASSERT_EQUAL_HEX8_ARRAY(chk_len_expected, mem_bq_direct + 0x60, 2);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(data_expected, mem_bq_direct + 0x40, 2);
}

void test_bq769x2_subcmd_write_u4()
{
    uint8_t chk_len_expected[2];
    uint8_t data_expected[4];

    data_expected[0x0] = 0xAA;     // LSB
    data_expected[0x1] = 0xBB;
    data_expected[0x2] = 0xCC;
    data_expected[0x3] = 0xDD;     // MSB

    chk_len_expected[0x0] = (uint8_t)~(0xAA + 0xBB + 0xCC + 0xDD);
    chk_len_expected[0x1] = 4 + 4;

    int err = bq769x2_subcmd_write_u4(0, 0xDDCCBBAA);
    TEST_ASSERT_EQUAL(0, err);

    TEST_ASSERT_EQUAL_HEX8_ARRAY(chk_len_expected, mem_bq_direct + 0x60, 2);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(data_expected, mem_bq_direct + 0x40, 4);
}

void test_bq769x2_subcmd_write_i1()
{
    uint8_t chk_len_expected[2];
    uint8_t data_expected[4];

    data_expected[0x0] = 0x80;

    chk_len_expected[0x0] = (uint8_t)~0x80;   // checksum
    chk_len_expected[0x1] = 4 + 1;            // length

    int err = bq769x2_subcmd_write_i1(0, INT8_MIN);
    TEST_ASSERT_EQUAL(0, err);

    TEST_ASSERT_EQUAL_HEX8_ARRAY(chk_len_expected, mem_bq_direct + 0x60, 2);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(data_expected, mem_bq_direct + 0x40, 1);
}

void test_bq769x2_subcmd_write_i2()
{
    uint8_t chk_len_expected[2];
    uint8_t data_expected[4];

    data_expected[0x0] = 0x00;     // LSB
    data_expected[0x1] = 0x80;     // MSB

    chk_len_expected[0x0] = (uint8_t)~0x80;   // checksum
    chk_len_expected[0x1] = 4 + 2;            // length

    int err = bq769x2_subcmd_write_i2(0, INT16_MIN);
    TEST_ASSERT_EQUAL(0, err);

    TEST_ASSERT_EQUAL_HEX8_ARRAY(chk_len_expected, mem_bq_direct + 0x60, 2);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(data_expected, mem_bq_direct + 0x40, 2);
}

void test_bq769x2_subcmd_write_i4()
{
    uint8_t chk_len_expected[2];
    uint8_t data_expected[4];

    data_expected[0x0] = 0x00;     // LSB
    data_expected[0x1] = 0x00;
    data_expected[0x2] = 0x00;
    data_expected[0x3] = 0x80;     // MSB

    chk_len_expected[0x0] = (uint8_t)~0x80;   // checksum
    chk_len_expected[0x1] = 4 + 4;            // length

    int err = bq769x2_subcmd_write_i4(0, INT32_MIN);
    TEST_ASSERT_EQUAL(0, err);

    TEST_ASSERT_EQUAL_HEX8_ARRAY(chk_len_expected, mem_bq_direct + 0x60, 2);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(data_expected, mem_bq_direct + 0x40, 4);
}

void test_bq769x2_subcmd_write_f4()
{
    uint8_t chk_len_expected[2];
    uint8_t data_expected[4];

    data_expected[0x0] = 0xB6;     // LSB
    data_expected[0x1] = 0xF3;
    data_expected[0x2] = 0x9D;
    data_expected[0x3] = 0x3F;     // MSB

    chk_len_expected[0x0] = (uint8_t)~(0xB6 + 0xF3 + 0x9D + 0x3F);
    chk_len_expected[0x1] = 4 + 4;

    int err = bq769x2_subcmd_write_f4(0, 1.234F);
    TEST_ASSERT_EQUAL(0, err);

    TEST_ASSERT_EQUAL_HEX8_ARRAY(chk_len_expected, mem_bq_direct + 0x60, 2);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(data_expected, mem_bq_direct + 0x40, 4);
}

int bq769x2_tests()
{
    UNITY_BEGIN();

    RUN_TEST(test_bq769x2_direct_read_u2);
    RUN_TEST(test_bq769x2_direct_read_i2);

    RUN_TEST(test_bq769x2_subcmd_read_u1);
    RUN_TEST(test_bq769x2_subcmd_read_u2);
    RUN_TEST(test_bq769x2_subcmd_read_u4);

    RUN_TEST(test_bq769x2_subcmd_read_i1);
    RUN_TEST(test_bq769x2_subcmd_read_i2);
    RUN_TEST(test_bq769x2_subcmd_read_i4);

    RUN_TEST(test_bq769x2_subcmd_read_f4);

    RUN_TEST(test_bq769x2_subcmd_write_u1);
    RUN_TEST(test_bq769x2_subcmd_write_u2);
    RUN_TEST(test_bq769x2_subcmd_write_u4);

    RUN_TEST(test_bq769x2_subcmd_write_i1);
    RUN_TEST(test_bq769x2_subcmd_write_i2);
    RUN_TEST(test_bq769x2_subcmd_write_i4);

    RUN_TEST(test_bq769x2_subcmd_write_f4);

    return UNITY_END();
}
