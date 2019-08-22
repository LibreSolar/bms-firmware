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

#define LED_RED_PORT DT_ALIAS_LED_RED_GPIOS_CONTROLLER
#define LED_RED_PIN  DT_ALIAS_LED_RED_GPIOS_PIN

#define LED_GREEN_PORT DT_ALIAS_LED_GREEN_GPIOS_CONTROLLER
#define LED_GREEN_PIN  DT_ALIAS_LED_GREEN_GPIOS_PIN

#define SLEEP_TIME 	100		// ms

extern BmsConfig bms_conf;
extern BmsStatus bms_status;

void leds_update_thread()
{
	struct device *led_red;
	led_red = device_get_binding(LED_RED_PORT);
	gpio_pin_configure(led_red, LED_RED_PIN, GPIO_DIR_OUT);

	struct device *led_green;
	led_green = device_get_binding(LED_GREEN_PORT);
	gpio_pin_configure(led_red, LED_GREEN_PIN, GPIO_DIR_OUT);

    uint32_t count = 0;

	while (1) {
        switch (bms_status.state) {
            case BMS_STATE_ERROR:
                // flash
                gpio_pin_write(led_red, LED_RED_PIN, (count / 2) % 2);
                gpio_pin_write(led_red, LED_GREEN_PIN, (count / 2) % 2);
                break;
            case BMS_STATE_CHG:
                gpio_pin_write(led_red, LED_RED_PIN, 0);
                gpio_pin_write(led_red, LED_GREEN_PIN, 1);
                break;
            case BMS_STATE_DIS:
                gpio_pin_write(led_red, LED_RED_PIN, 1);
                gpio_pin_write(led_red, LED_GREEN_PIN, 0);
                break;
            case BMS_STATE_NORMAL:
            case BMS_STATE_BALANCING:
                gpio_pin_write(led_red, LED_RED_PIN, 1);
                gpio_pin_write(led_red, LED_GREEN_PIN, 1);
                break;
            case BMS_STATE_IDLE:
                // blink slowly
                gpio_pin_write(led_red, LED_RED_PIN, (count / 10) % 2);
                gpio_pin_write(led_red, LED_GREEN_PIN, (count / 10) % 2);
                break;
        }
        count++;
		k_sleep(SLEEP_TIME);
	}
}

#endif /* ZEPHYR */
