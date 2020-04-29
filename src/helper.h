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

#ifndef HELPER_H
#define HELPER_H

/** @file
 *
 * @brief
 * General helper functions
 */

#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ZEPHYR__
/* below macros are taken from Zephyr util.h */

/* Evaluates to 0 if cond is true-ish; compile error otherwise */
#define ZERO_OR_COMPILE_ERROR(cond) ((int) sizeof(char[1 - 2 * !(cond)]) - 1)

/* Evaluates to 0 if array is an array; compile error if not array (e.g.
 * pointer)
 */
#define IS_ARRAY(array) \
	ZERO_OR_COMPILE_ERROR( \
		!__builtin_types_compatible_p(__typeof__(array), \
					      __typeof__(&(array)[0])))

/* Evaluates to number of elements in an array; compile error if not
 * an array (e.g. pointer)
 */
#define ARRAY_SIZE(array) \
	((unsigned long) (IS_ARRAY(array) + \
		(sizeof(array) / sizeof((array)[0]))))

#endif

/**
 * Interpolation in a look-up table. Values of a must be monotonically increasing/decreasing
 *
 * @returns interpolated value of array b at position value_a
 */
float interpolate(const float a[], const float b[], size_t size, float value_a);

/**
 * Framework-independent system uptime
 *
 * @returns seconds since the system booted
 */
uint32_t uptime();

#ifdef __cplusplus
}
#endif

#endif /* HELPER_H */
