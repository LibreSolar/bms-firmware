/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "leds.h"

#include "bms.h"

#define SLEEP_TIME K_MSEC(100)

extern Bms bms;

#ifndef UNIT_TEST

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define LED_DIS_NODE DT_ALIAS(led_red)
#define LED_CHG_NODE DT_ALIAS(led_green)

static const struct gpio_dt_spec led_dis = GPIO_DT_SPEC_GET(LED_DIS_NODE, gpios);
static const struct gpio_dt_spec led_chg = GPIO_DT_SPEC_GET(LED_CHG_NODE, gpios);

void leds_chg_set(bool on)
{
    if (led_chg.port) {
        gpio_pin_set_dt(&led_chg, on);
    }
}

void leds_dis_set(bool on)
{
    if (led_dis.port) {
        gpio_pin_set_dt(&led_dis, on);
    }
}

void leds_update_thread()
{
    if (!device_is_ready(led_chg.port) || !device_is_ready(led_dis.port)) {
        return;
    }

    gpio_pin_configure_dt(&led_dis, GPIO_OUTPUT);

    gpio_pin_configure_dt(&led_chg, GPIO_OUTPUT);

    while (1) {
        leds_update();
        k_sleep(SLEEP_TIME);
    }
}

void leds_update()
{
    static uint32_t count = 0;

    // Charging LED control
    if (bms.status.state == BMS_STATE_NORMAL || bms.status.state == BMS_STATE_CHG) {
        if (bms.status.pack_current > bms.conf.bal_idle_current && ((count / 2) % 10) == 0) {
            // not in idle: ____ ____ ____
            leds_chg_set(0);
        }
        else {
            // idle: ______________
            leds_chg_set(1);
        }
    }
    else {
        if (bms_chg_error(bms.status.error_flags)) {
            // quick flash
            leds_chg_set(count % 2);
        }
        else if (((count / 2) % 10) == 0) {
            // off without error: _    _    _
            leds_chg_set(1);
        }
        else {
            leds_chg_set(0);
        }
    }

    // Discharging LED control
    if (bms.status.state == BMS_STATE_NORMAL || bms.status.state == BMS_STATE_DIS) {
        if (bms.status.pack_current < -bms.conf.bal_idle_current && ((count / 2) % 10) == 0) {
            // not in idle: ____ ____ ____
            leds_dis_set(0);
        }
        else {
            // idle: ______________
            leds_dis_set(1);
        }
    }
    else {
        if (bms_dis_error(bms.status.error_flags)) {
            // quick flash
            leds_dis_set(count % 2);
        }
        else if (((count / 2) % 10) == 0) {
            // off without error: _    _    _
            leds_dis_set(1);
        }
        else {
            leds_dis_set(0);
        }
    }

    count++;
}

K_THREAD_DEFINE(leds, 256, leds_update_thread, NULL, NULL, NULL, 4, 0, 0);

#else

void leds_chg_set(bool on) { ; }

void leds_dis_set(bool on) { ; }

#endif // UNIT_TEST
