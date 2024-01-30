/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "helper.h"

#include <stdio.h>

#include <zephyr/ztest.h>

ZTEST(is_empty, test_empty)
{
    float a[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    zassert_true(is_empty((uint8_t *)a, sizeof(a)));

#if __SIZEOF_POINTER__ == 4
    uint8_t b[] = { 0, 0, 0 };
#elif __SIZEOF_POINTER__ == 8
    uint8_t b[] = { 0, 0, 0, 0, 0, 0, 0 };
#endif
    zassert_true(is_empty((uint8_t *)b, sizeof(b)));
}

ZTEST(is_empty, test_filled_last_element)
{
    float a[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0001 };
    zassert_false(is_empty((uint8_t *)a, sizeof(a)));

#if __SIZEOF_POINTER__ == 4
    uint8_t b[] = { 0, 0, 1 };
#elif __SIZEOF_POINTER__ == 8
    uint8_t b[] = { 0, 0, 0, 0, 0, 0, 1 };
#endif
    zassert_false(is_empty((uint8_t *)b, sizeof(b)));
}

ZTEST(is_empty, test_filled_first_element)
{
    float a[] = { 0.0001, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    zassert_false(is_empty((uint8_t *)a, sizeof(a)));

#if __SIZEOF_POINTER__ == 4
    uint8_t b[] = { 1, 0, 0 };
#elif __SIZEOF_POINTER__ == 8
    uint8_t b[] = { 1, 0, 0, 0, 0, 0, 0 };
#endif
    zassert_false(is_empty((uint8_t *)b, sizeof(b)));
}

ZTEST(is_empty, test_unaligned_access)
{

    uint8_t a[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    zassert_true(is_empty(((uint8_t *)a) + 1, sizeof(a) - 1));

    uint8_t b[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    zassert_false(is_empty(((uint8_t *)b) + 1, sizeof(b) - 1));
}

ZTEST_SUITE(is_empty, NULL, NULL, NULL, NULL, NULL);
