/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIT_TEST

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <stdio.h>

#include "bms.h"
#include "button.h"
#include "data_objects.h"
#include "helper.h"
#include "leds.h"
#include "thingset.h"

LOG_MODULE_REGISTER(bms_main, CONFIG_LOG_DEFAULT_LEVEL);

Bms bms;

int main(void)
{
    printf("Hardware: Libre Solar %s (%s)\n", DT_PROP(DT_PATH(pcb), type),
           DT_PROP(DT_PATH(pcb), version_str));
    printf("Firmware: %s\n", FIRMWARE_VERSION_ID);

    while (bms_init_hardware(&bms) != 0) {
        LOG_ERR("BMS hardware initialization failed, retrying in 10s");
        k_sleep(K_MSEC(10000));
    }

    bms_configure(&bms);

    bms_update(&bms);
    bms_soc_reset(&bms, -1);

    button_init();

    int64_t t_start = k_uptime_get();
    while (true) {

        bms_update(&bms);
        bms_state_machine(&bms);

        if (button_pressed_for_3s()) {
            printf("Button pressed for 3s: shutdown...\n");
            bms_shutdown(&bms);
            k_sleep(K_MSEC(10000));
        }

        // bms_print_registers();

        t_start += 100;
        k_sleep(K_TIMEOUT_ABS_MS(t_start));
    }

    return 0;
}

static int init_config(void)
{
    bms_init_status(&bms);
    bms_init_config(&bms, CONFIG_CELL_TYPE, CONFIG_BAT_CAPACITY_AH);

    return 0;
}

/*
 * The default configuration must be initialized before the ThingSet storage backend reads data
 * from EEPROM or flash (with THINGSET_INIT_PRIORITY_STORAGE = 30).
 */
SYS_INIT(init_config, APPLICATION, 0);

#endif /* UNIT_TEST */
