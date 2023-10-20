/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_BMS_IC_BMS_IC_BQ769X2_EMUL_H_
#define DRIVERS_BMS_IC_BMS_IC_BQ769X2_EMUL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint8_t bq769x2_emul_get_direct_mem(const struct emul *em, uint8_t addr);

void bq769x2_emul_set_direct_mem(const struct emul *em, uint8_t addr, uint8_t byte);

uint8_t bq769x2_emul_get_data_mem(const struct emul *em, uint16_t addr);

void bq769x2_emul_set_data_mem(const struct emul *em, uint16_t addr, uint8_t byte);

#ifdef __cplusplus
}
#endif

#endif // DRIVERS_BMS_IC_BMS_IC_BQ769X2_EMUL_H_
