/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tests.h"

#include "helper.h"

#include <time.h>
#include <stdio.h>

void test_interpolate_increasing()
{
    float a[] = {1, 2, 3, 4, 5};
    float b[] = {2, 4, 6, 8, 10};

    float value_b = interpolate(a, b, sizeof(a)/sizeof(float), 1.75);
    TEST_ASSERT_EQUAL_FLOAT(3.5, value_b);

    value_b = interpolate(a, b, sizeof(a)/sizeof(float), -1);
    TEST_ASSERT_EQUAL_FLOAT(2.0, value_b);

    value_b = interpolate(a, b, sizeof(a)/sizeof(float), 6);
    TEST_ASSERT_EQUAL_FLOAT(10.0, value_b);
}

void test_interpolate_decreasing()
{
    float a[] = {5, 4, 3, 2, 1};
    float b[] = {2, 4, 6, 8, 10};

    float value_b = interpolate(a, b, sizeof(a)/sizeof(float), 1.75);
    TEST_ASSERT_EQUAL_FLOAT(8.5, value_b);

    value_b = interpolate(a, b, sizeof(a)/sizeof(float), -1);
    TEST_ASSERT_EQUAL_FLOAT(10.0, value_b);

    value_b = interpolate(a, b, sizeof(a)/sizeof(float), 6);
    TEST_ASSERT_EQUAL_FLOAT(2.0, value_b);
}

void helper_tests()
{
    UNITY_BEGIN();

    RUN_TEST(test_interpolate_increasing);
    RUN_TEST(test_interpolate_decreasing);

    UNITY_END();
}
