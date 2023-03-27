/*
 * Copyright (c) The ThingSet Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "thingset/can.h"
#include "thingset/sdk.h"

#define CAN_NODE DT_CHOSEN(thingset_can)

#if !DT_NODE_EXISTS(CAN_NODE)
#error "No CAN device chosen."
#endif

static const struct device *can_dev = DEVICE_DT_GET(CAN_NODE);

static struct thingset_can ts_can;

static void thingset_can_thread()
{
    thingset_can_init(&ts_can, can_dev);

    while (1) {
        thingset_can_process(&ts_can);
    }
}

K_THREAD_DEFINE(thingset_can, CONFIG_THINGSET_CAN_THREAD_STACK_SIZE, thingset_can_thread, NULL,
                NULL, NULL, CONFIG_THINGSET_CAN_THREAD_PRIORITY, 0, 1000);
