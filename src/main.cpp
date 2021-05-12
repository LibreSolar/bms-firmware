/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIT_TEST

#include <zephyr.h>
#include <stdio.h>

#include "bms.h"
#include "button.h"
#include "eeprom.h"
#include "leds.h"
#include "thingset.h"
#include "helper.h"
#include "data_nodes.h"

BmsConfig bms_conf;
BmsStatus bms_status;
extern ThingSet ts;

void main(void)
{
    printf("Hardware: Libre Solar %s (%s)\n",
        DT_PROP(DT_PATH(pcb), type), DT_PROP(DT_PATH(pcb), version_str));
    printf("Firmware: %s\n", FIRMWARE_VERSION_ID);

    bms_init_hardware();
    bms_init_status(&bms_status);
    bms_init_config(&bms_conf, CONFIG_CELL_TYPE, CONFIG_BAT_CAPACITY_AH);

    // read custom configuration from EEPROM
    data_nodes_init();

    bms_apply_cell_ovp(&bms_conf);
    bms_apply_cell_uvp(&bms_conf);

    bms_apply_dis_scp(&bms_conf);
    bms_apply_dis_ocp(&bms_conf);
    bms_apply_chg_ocp(&bms_conf);

    bms_update(&bms_conf, &bms_status);
    bms_reset_soc(&bms_conf, &bms_status, -1);

    button_init();

    int64_t t_start = k_uptime_get();
    while (true) {

        bms_update(&bms_conf, &bms_status);
        bms_state_machine(&bms_conf, &bms_status);

        if (button_pressed_for_3s()) {
            printf("Button pressed for 3s: shutdown...\n");
            bms_shutdown();
            k_sleep(K_MSEC(10000));
        }

        //bms_print_registers();

        eeprom_update();

        t_start += 100;
        k_sleep(K_TIMEOUT_ABS_MS(t_start));
    }
}

#endif /* UNIT_TEST */
