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

#include "bms.h"

#ifdef ZEPHYR

#include <zephyr.h>
#include <drivers/gpio.h>

#define LED_DIS_PORT DT_ALIAS_LED_RED_GPIOS_CONTROLLER
#define LED_DIS_PIN  DT_ALIAS_LED_RED_GPIOS_PIN

#define LED_CHG_PORT DT_ALIAS_LED_GREEN_GPIOS_CONTROLLER
#define LED_CHG_PIN  DT_ALIAS_LED_GREEN_GPIOS_PIN

#define SLEEP_TIME 	100		// ms

extern BmsConfig bms_conf;
extern BmsStatus bms_status;

void leds_update_thread()
{
	struct device *led_dis_dev;
	led_dis_dev = device_get_binding(LED_DIS_PORT);
	gpio_pin_configure(led_dis_dev, LED_DIS_PIN, GPIO_DIR_OUT);

	struct device *led_chg_dev;
	led_chg_dev = device_get_binding(LED_CHG_PORT);
	gpio_pin_configure(led_chg_dev, LED_CHG_PIN, GPIO_DIR_OUT);

    uint32_t count = 0;

	while (1) {

        // Charging LED control
        if (bms_status.state == BMS_STATE_NORMAL || bms_status.state == BMS_STATE_CHG) {
            if (bms_status.pack_current > bms_conf.bal_idle_current && ((count / 2) % 10) == 0) {
                // not in idle: ____ ____ ____
                gpio_pin_write(led_chg_dev, LED_CHG_PIN, 0);
            }
            else {
                // idle: ______________
                gpio_pin_write(led_chg_dev, LED_CHG_PIN, 1);
            }
        }
        else {
            if (bms_chg_error(&bms_status)) {
                // quick flash
                gpio_pin_write(led_chg_dev, LED_CHG_PIN, count % 2);
            }
            else {
                gpio_pin_write(led_chg_dev, LED_CHG_PIN, 0);
            }
        }

        // Discharging LED control
        if (bms_status.state == BMS_STATE_NORMAL || bms_status.state == BMS_STATE_DIS) {
            if (bms_status.pack_current < -bms_conf.bal_idle_current && ((count / 2) % 10) == 0) {
                // not in idle: ____ ____ ____
                gpio_pin_write(led_dis_dev, LED_DIS_PIN, 0);
            }
            else {
                // idle: ______________
                gpio_pin_write(led_dis_dev, LED_DIS_PIN, 1);
            }
        }
        else {
            if (bms_dis_error(&bms_status)) {
                // quick flash
                gpio_pin_write(led_dis_dev, LED_DIS_PIN, count % 2);
            }
            else {
                gpio_pin_write(led_dis_dev, LED_DIS_PIN, 0);
            }
        }

        count++;
		k_sleep(SLEEP_TIME);
	}
}

#endif /* ZEPHYR */
