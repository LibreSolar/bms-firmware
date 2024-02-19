/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_BMS_IC_BMS_IC_ISL94202_EMUL_H_
#define DRIVERS_BMS_IC_BMS_IC_ISL94202_EMUL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint8_t isl94202_emul_get_byte(const struct emul *em, uint8_t addr);

void isl94202_emul_set_byte(const struct emul *em, uint8_t addr, uint8_t byte);

uint16_t isl94202_emul_get_word(const struct emul *em, uint8_t addr);

void isl94202_emul_set_word(const struct emul *em, uint8_t addr, uint16_t word);

void isl94202_emul_set_mem_defaults(const struct emul *em);

#ifdef __cplusplus
}
#endif

#endif // DRIVERS_BMS_IC_BMS_IC_ISL94202_EMUL_H_
