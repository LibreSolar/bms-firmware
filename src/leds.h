/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LEDS_H_
#define LEDS_H_

#include <stdbool.h>

/**
 * LED control depending on BMS state, should be called every 100 ms
 *
 * Red LED
 * - Discharging finished (empty)           (off)
 * - Discharging allowed (current < idle)   _______________
 * - Discharging active (current >= idle)   ____ ____ ____
 * - Discharging error (UV/OT/UT/OC/SC)     _ _ _ _ _ _ _ _
 *
 * Green LED
 * - Charging finished (full)               (off)
 * - Charging allowed (current < idle)      _______________
 * - Charging active (current >= idle)      ____ ____ ____
 * - Charging error (OV/OT/UT/OC)           _ _ _ _ _ _ _ _
 *
 */
void leds_update();

/**
 * Thread that calls leds_update() in regular interval
 */
void leds_update_thread();

/**
 * Set green charging LED on or off
 */
void leds_chg_set(bool on);

/**
 * Set red discharging LED on or off
 */
void leds_dis_set(bool on);

#endif // LEDS_H_
