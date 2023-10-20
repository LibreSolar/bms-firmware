/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <stdio.h>

#include "button.h"
#include "data_objects.h"
#include "helper.h"
#include "leds.h"
#include "thingset.h"
#include <bms/bms.h>

LOG_MODULE_REGISTER(bms_main, CONFIG_LOG_DEFAULT_LEVEL);

const struct device *bms_ic = DEVICE_DT_GET(DT_ALIAS(bms_ic));

struct bms_context bms;

int main(void)
{
    int err;

    LOG_INF("Hardware: Libre Solar %s (%s)", DT_PROP(DT_PATH(pcb), type),
            DT_PROP(DT_PATH(pcb), version_str));
    LOG_INF("Firmware: %s", FIRMWARE_VERSION_ID);

    if (!device_is_ready(bms_ic)) {
        LOG_ERR("BMS IC not ready");
        return -ENODEV;
    }

    bms_ic_assign_data(bms_ic, &bms.ic_data);

    err = bms_ic_set_mode(bms_ic, BMS_IC_MODE_ACTIVE);
    if (err != 0) {
        LOG_ERR("Failed to activate BMS IC: %d", err);
    }

    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_ALL);
    if (err != 0) {
        LOG_ERR("Failed to configure BMS IC: %d", err);
    }

    err = bms_ic_read_data(bms_ic, BMS_IC_DATA_CELL_VOLTAGES);
    if (err != 0) {
        LOG_ERR("Failed to read data from BMS IC: %d", err);
    }

    bms_soc_reset(&bms, -1);

    button_init();

    int64_t t_start = k_uptime_get();
    while (true) {
        err = bms_ic_read_data(bms_ic, BMS_IC_DATA_ALL);
        if (err != 0) {
            LOG_ERR("Failed to read data from BMS IC: %d", err);
        }

        bms_state_machine(&bms);

        if (button_pressed_for_3s()) {
            LOG_WRN("Button pressed for 3s: shutdown...");
            bms_ic_set_mode(bms_ic, BMS_IC_MODE_OFF);
            k_sleep(K_MSEC(10000));
        }

        t_start += CONFIG_BMS_IC_POLLING_INTERVAL_MS;
        k_sleep(K_TIMEOUT_ABS_MS(t_start));
    }

    return 0;
}

static int init_config(void)
{
    bms_init_config(&bms, (enum bms_cell_type)CONFIG_CELL_TYPE, CONFIG_BAT_CAPACITY_AH);

    return 0;
}

/*
 * The default configuration must be initialized before the ThingSet storage backend reads data
 * from EEPROM or flash (with THINGSET_INIT_PRIORITY_STORAGE = 30).
 */
SYS_INIT(init_config, APPLICATION, 0);
