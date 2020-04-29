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

#if defined(__ZEPHYR__) && !defined(UNIT_TEST)

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <stdio.h>

#include "bms.h"
#include "ext/ext.h"
#include "leds.h"
#include "thingset.h"
#include "helper.h"
#include "data_nodes.h"

BmsConfig bms_conf;
BmsStatus bms_status;
extern ThingSet ts;

// preliminary (necessary in uext_oled for compatibility with mbed)
float load_voltage;
bool blinkOn = false;

void main(void)
{
    printf("Booting Libre Solar BMS: %s\n", CONFIG_BOARD);

    data_nodes_init();

    bms_init();
    bms_init_config(&bms_conf, CELL_TYPE_LFP, 45);
    bms_update(&bms_conf, &bms_status);
    bms_reset_soc(&bms_conf, &bms_status, -1);

    struct device *sw_pwr;
	sw_pwr = device_get_binding(DT_ALIAS_SW_PWR_GPIOS_CONTROLLER);
    gpio_pin_configure(sw_pwr, DT_ALIAS_SW_PWR_GPIOS_PIN, DT_ALIAS_SW_PWR_GPIOS_FLAGS | GPIO_INPUT);

    uint32_t btn_pressed_count = 0;
    while (1) {

        bms_update(&bms_conf, &bms_status);
        bms_state_machine(&bms_conf, &bms_status);

        if (gpio_pin_get(sw_pwr, DT_ALIAS_SW_PWR_GPIOS_PIN) == 1) {
            btn_pressed_count++;
            if (btn_pressed_count > 3) {
                printf("Button pressed for 3s: shutdown...\n");
                bms_shutdown();
                k_sleep(10000);
            }
        }
        else {
            btn_pressed_count = 0;
        }

        //bms_print_registers();

        k_sleep(1000);
    }
}

void ext_mgr_thread()
{
    // initialize all extensions and external communication interfaces
    ext_mgr.enable_all();

    uint32_t last_call = 0;
    while (1) {
        uint32_t now = k_uptime_get() / 1000;
        ext_mgr.process_asap();     // approx. every millisecond
        if (now >= last_call + 1) {
            last_call = now;
            ext_mgr.process_1s();
        }
        k_sleep(1);
    }
}

K_THREAD_DEFINE(ext_thread, 1024, ext_mgr_thread, NULL, NULL, NULL, 6, 0, 1000);

K_THREAD_DEFINE(leds_id, 256, leds_update_thread, NULL, NULL, NULL,	4, 0, K_NO_WAIT);

#endif /* __ZEPHYR__ */
