/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "helper.h"

#include <stdio.h>

#include <zephyr/ztest.h>

ZTEST(interpolation, test_increasing)
{
    float a[] = { 1, 2, 3, 4, 5 };
    float b[] = { 2, 4, 6, 8, 10 };

    float value_b = interpolate(a, b, sizeof(a) / sizeof(float), 1.75);
    zassert_equal(3.5F, value_b);

    value_b = interpolate(a, b, sizeof(a) / sizeof(float), -1);
    zassert_equal(2.0F, value_b);

    value_b = interpolate(a, b, sizeof(a) / sizeof(float), 6);
    zassert_equal(10.0F, value_b);
}

ZTEST(interpolation, test_decreasing)
{
    float a[] = { 5, 4, 3, 2, 1 };
    float b[] = { 2, 4, 6, 8, 10 };

    float value_b = interpolate(a, b, sizeof(a) / sizeof(float), 1.75);
    zassert_equal(8.5F, value_b);

    value_b = interpolate(a, b, sizeof(a) / sizeof(float), -1);
    zassert_equal(10.0F, value_b);

    value_b = interpolate(a, b, sizeof(a) / sizeof(float), 6);
    zassert_equal(2.0F, value_b);
}

ZTEST_SUITE(interpolation, NULL, NULL, NULL, NULL, NULL);
