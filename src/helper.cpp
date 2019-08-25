/* Libre Solar Battery Management System firmware
 * Copyright (c) 2016-2019 Martin Jäger (www.libre.solar)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "helper.h"

#ifdef __MBED__
#include "mbed.h"
#elif defined(ZEPHYR)
#include <zephyr.h>
#endif

#include <time.h>

uint32_t uptime()
{
#ifdef ZEPHYR
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
