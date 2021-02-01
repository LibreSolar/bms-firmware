/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIT_TEST

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

#define SW_PWR_GPIO DT_ALIAS(sw_pwr)

void main(void)
{
    printf("Booting Libre Solar BMS: %s\n", CONFIG_BOARD);

    data_nodes_init();

    bms_init();
    bms_init_config(&bms_conf, CELL_TYPE_LFP, 45);
    bms_update(&bms_conf, &bms_status);
    bms_reset_soc(&bms_conf, &bms_status, -1);

    const struct device *sw_pwr;
	sw_pwr = device_get_binding(DT_GPIO_LABEL(SW_PWR_GPIO, gpios));
    gpio_pin_configure(sw_pwr, DT_GPIO_PIN(SW_PWR_GPIO, gpios),
        DT_GPIO_FLAGS(SW_PWR_GPIO, gpios) | GPIO_INPUT);

    uint32_t btn_pressed_count = 0;
    while (1) {

        bms_update(&bms_conf, &bms_status);
        bms_state_machine(&bms_conf, &bms_status);

        if (gpio_pin_get(sw_pwr, DT_GPIO_PIN(SW_PWR_GPIO, gpios)) == 1) {
            btn_pressed_count++;
            if (btn_pressed_count > 3) {
                printf("Button pressed for 3s: shutdown...\n");
                bms_shutdown();
                k_sleep(K_MSEC(10000));
            }
        }
        else {
            btn_pressed_count = 0;
        }

        //bms_print_registers();

        k_sleep(K_MSEC(1000));
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
        k_sleep(K_MSEC(1));
    }
}

K_THREAD_DEFINE(ext_thread, 1024, ext_mgr_thread, NULL, NULL, NULL, 6, 0, 1000);

K_THREAD_DEFINE(leds_id, 256, leds_update_thread, NULL, NULL, NULL,	4, 0, 0);

#endif /* UNIT_TEST */
