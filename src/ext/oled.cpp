/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_UEXT_OLED_DISPLAY

#include <math.h>
#include <stdio.h>

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>

#include "bms.h"
#include "pcb.h"
#include "oled_ssd1306.h"

extern BmsStatus bms_status;

OledSSD1306 oled(DT_LABEL(DT_ALIAS(i2c_uext)));

static bool blink_on = false;

void oled_update()
{
    char buf[30];
    unsigned int len;

    blink_on = !blink_on;

    oled.clear();

    oled.setTextCursor(0, 0);
    len = snprintf(buf, sizeof(buf), "%.2f V", bms_status.pack_voltage);
    oled.writeString(buf, len);

    oled.setTextCursor(64, 0);
    len = snprintf(buf, sizeof(buf), "%.2f A", bms_status.pack_current);
    oled.writeString(buf, len);

    oled.setTextCursor(0, 8);
    len = snprintf(buf, sizeof(buf), "T:%.1f C", bms_status.bat_temp_avg);
    oled.writeString(buf, len);

    oled.setTextCursor(64, 8);
    len = snprintf(buf, sizeof(buf), "SOC:%.0f", bms_status.soc);
    oled.writeString(buf, len);

    //oled.setTextCursor(0, 16);
    //len = snprintf(buf, sizeof(buf), "Load: %.2fV", load_voltage);
    //oled.writeString(buf, len);

    for (int i = 0; i < NUM_CELLS_MAX; i++) {
        if (blink_on || !(bms_status.balancing_status & (1 << i))) {
            oled.setTextCursor((i % 2 == 0) ? 0 : 64, 24 + (i / 2) * 8);
            len = snprintf(buf, sizeof(buf), "%d:%.3f V", i+1, bms_status.cell_voltages[i]);
            oled.writeString(buf, len);
        }
    }

    oled.display();
}

void oled_thread()
{
    oled.init(CONFIG_UEXT_OLED_BRIGHTNESS);

    while (true) {
        oled_update();
        k_sleep(K_MSEC(1000));
    }
}

K_THREAD_DEFINE(oled_thread_id, 1024, oled_thread, NULL, NULL, NULL, 6, 0, 1000);

#endif /* CONFIG_UEXT_OLED_DISPLAY */
