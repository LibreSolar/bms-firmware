/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @file
 *
 * @brief
 * Functions to handle on/off switch
 */

/**
 * Initialize button and configure interrupts
 */
void button_init();

/**
 * Check if button was pressed for 3 seconds
 *
 * \return true if pressed for at least 3 seconds
 */
bool button_pressed_for_3s();

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H_ */