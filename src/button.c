/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "button.h"

#ifndef UNIT_TEST

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>

#define BTN_GPIO    DT_ALIAS(sw_pwr)
#define BTN_CTLR    DT_GPIO_CTLR(BTN_GPIO, gpios)
#define BTN_PIN     DT_GPIO_PIN(BTN_GPIO, gpios)
#define BTN_FLAGS   DT_GPIO_FLAGS(BTN_GPIO, gpios)

static const struct device *btn_dev = DEVICE_DT_GET(BTN_CTLR);
static struct gpio_callback btn_cb_data;
static int64_t time_pressed;

static void button_pressed_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    time_pressed = k_uptime_get();
}

void button_init()
{
    if (!device_is_ready(btn_dev)) {
        printk("Button device not found\n");
        return;
    }

    gpio_pin_configure(btn_dev, BTN_PIN, BTN_FLAGS | GPIO_INPUT);
    gpio_pin_interrupt_configure(btn_dev, BTN_PIN, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&btn_cb_data, button_pressed_cb, BIT(BTN_PIN));
    gpio_add_callback(btn_dev, &btn_cb_data);
}

bool button_pressed_for_3s()
{
    return gpio_pin_get(btn_dev, BTN_PIN) == 1 &&
        (k_uptime_get() - time_pressed) > 3000;
}

#endif /* UNIT_TEST */
