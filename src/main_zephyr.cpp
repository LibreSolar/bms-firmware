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

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <stdio.h>

#include "uext.h"
#include "bms.h"

#define LED_RED_PORT DT_ALIAS_LED_RED_GPIOS_CONTROLLER
#define LED_RED_PIN  DT_ALIAS_LED_RED_GPIOS_PIN

#define LED_GREEN_PORT DT_ALIAS_LED_GREEN_GPIOS_CONTROLLER
#define LED_GREEN_PIN  DT_ALIAS_LED_GREEN_GPIOS_PIN

#define SLEEP_TIME 	1000		// 1000 ms = 1 s

BmsConfig bms_conf;
BmsStatus bms_status;

void main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD);

	uext_init();

	bms_init();
	//bms_init_config(&bms_conf);

	int cnt = 0;
	struct device *led_red;
	led_red = device_get_binding(LED_RED_PORT);
	gpio_pin_configure(led_red, LED_RED_PIN, GPIO_DIR_OUT);

	while (1) {
		// Set pin to HIGH/LOW every 1 second
		gpio_pin_write(led_red, LED_RED_PIN, cnt % 2);
		cnt++;

		bms_read_voltages(&bms_status);
		printf("Cell voltages: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f\n",
			bms_status.cell_voltages[0], bms_status.cell_voltages[1],
			bms_status.cell_voltages[2], bms_status.cell_voltages[3],
			bms_status.cell_voltages[4], bms_status.cell_voltages[5],
			bms_status.cell_voltages[6], bms_status.cell_voltages[7]);

		//bms_print_registers();

		k_sleep(SLEEP_TIME);
	}
}
