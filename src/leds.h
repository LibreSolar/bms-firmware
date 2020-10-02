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

#ifndef LEDS_H_
#define LEDS_H_

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
