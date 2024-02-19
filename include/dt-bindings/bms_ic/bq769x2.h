/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_BMS_IC_BQ769X2_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_BMS_IC_BQ769X2_H_

/* multi-function pins, which can be used for cell and FET temperature measurement */
#define BQ769X2_PIN_CFETOFF 0
#define BQ769X2_PIN_DFETOFF 1
#define BQ769X2_PIN_ALERT   2
#define BQ769X2_PIN_TS1     3
#define BQ769X2_PIN_TS2     4
#define BQ769X2_PIN_TS3     5
#define BQ769X2_PIN_HDQ     6
#define BQ769X2_PIN_DCHG    7
#define BQ769X2_PIN_DDSG    8

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_BMS_IC_BQ769X2_H_ */
