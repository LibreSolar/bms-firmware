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

#include "leds.h"

#include "bms.h"

#define SLEEP_TIME 	100		// ms

extern BmsConfig bms_conf;
extern BmsStatus bms_status;

#ifdef ZEPHYR

#include <zephyr.h>
#include <drivers/gpio.h>

#define LED_DIS_PORT DT_ALIAS_LED_RED_GPIOS_CONTROLLER
#define LED_DIS_PIN  DT_ALIAS_LED_RED_GPIOS_PIN

#define LED_CHG_PORT DT_ALIAS_LED_GREEN_GPIOS_CONTROLLER
#define LED_CHG_PIN  DT_ALIAS_LED_GREEN_GPIOS_PIN

struct device *led_dis_dev = NULL;
struct device *led_chg_dev = NULL;

void leds_chg_set(bool on)
{
    if (led_chg_dev) {
        leds_chg_set(0);
    }
}

void leds_dis_set(bool on)
{
    if (led_chg_dev) {
        leds_dis_set(0);
    }
}

void leds_update_thread()
{
	led_dis_dev = device_get_binding(LED_DIS_PORT);
	gpio_pin_configure(led_dis_dev, LED_DIS_PIN, GPIO_DIR_OUT);

	led_chg_dev = device_get_binding(LED_CHG_PORT);
	gpio_pin_configure(led_chg_dev, LED_CHG_PIN, GPIO_DIR_OUT);

	while (1) {
        leds_update();
		k_sleep(SLEEP_TIME);
	}
}

#elif defined(__MBED__)

#include "mbed.h"

DigitalOut led_chg(PIN_LED_GREEN);
DigitalOut led_dis(PIN_LED_RED);

void leds_chg_set(bool on)
{
    led_chg = on;
}

void leds_dis_set(bool on)
{
    led_dis = on;
}

#else

void leds_chg_set(bool on) {;}

void leds_dis_set(bool on) {;}

#endif // ZEPHYR / MBED

void leds_update()
{
    static uint32_t count = 0;

    // Charging LED control
    if (bms_status.state == BMS_STATE_NORMAL || bms_status.state == BMS_STATE_CHG) {
        if (bms_status.pack_current > bms_conf.bal_idle_current && ((count / 2) % 10) == 0) {
            // not in idle: ____ ____ ____
            leds_chg_set(0);
        }
        else {
            // idle: ______________
            leds_chg_set(1);
        }
    }
    else {
        if (bms_chg_error(&bms_status)) {
            // quick flash
            leds_chg_set(count % 2);
        }
        else {
            leds_chg_set(0);
        }
    }

    // Discharging LED control
    if (bms_status.state == BMS_STATE_NORMAL || bms_status.state == BMS_STATE_DIS) {
        if (bms_status.pack_current < -bms_conf.bal_idle_current && ((count / 2) % 10) == 0) {
            // not in idle: ____ ____ ____
            leds_dis_set(0);
        }
        else {
            // idle: ______________
            leds_dis_set(1);
        }
    }
    else {
        if (bms_dis_error(&bms_status)) {
            // quick flash
            leds_dis_set(count % 2);
        }
        else {
            leds_dis_set(0);
        }
    }

    count++;
}
