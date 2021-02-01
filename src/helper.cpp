/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "helper.h"

#ifndef UNIT_TEST
#include <zephyr.h>
#endif

#include <time.h>

uint32_t uptime()
{
#ifdef __ZEPHYR__
    return k_uptime_get() / 1000;
#else
    return time(NULL);
#endif
}

float interpolate(const float a[], const float b[], size_t size, float value_a)
{
    if (a[0] < a[size - 1]) {
        for (unsigned int i = 0; i < size; i++) {
            if (value_a <= a[i]) {
                if (i == 0) {
                    return b[0];    // value_a smaller than first element
                }
                else {
                    return b[i-1] + (b[i] - b[i-1]) * (value_a - a[i-1]) / (a[i] - a[i-1]);
                }
            }
        }
        return b[size - 1];         // value_a larger than last element
    }
    else {
        for (unsigned int i = 0; i < size; i++) {
            if (value_a >= a[i]) {
                if (i == 0) {
                    return b[0];    // value_a smaller than first element
                }
                else {
                    return b[i-1] + (b[i] - b[i-1]) * (value_a - a[i-1]) / (a[i] - a[i-1]);
                }
            }
        }
        return b[size - 1];         // value_a larger than last element
    }
}
