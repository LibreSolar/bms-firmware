/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIT_TEST

#if CONFIG_EXT_OLED_DISPLAY     // otherwise don't compile code to reduce firmware size

#include "ext/ext.h"

#include <math.h>
#include <stdio.h>

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>

#include "bms.h"
#include "pcb.h"
#include "oled_ssd1306.h"

extern bool blinkOn;
extern BmsStatus bms_status;
extern float load_voltage;

// implement specific extension inherited from ExtInterface
class ExtOled: public ExtInterface
{
public:
    ExtOled() {};
    void enable();
    void process_1s();
};

static ExtOled ext_oled;    // local instance, will self register itself

OledSSD1306 oled(DT_LABEL(DT_ALIAS(i2c_uext)));

void ExtOled::enable()
{
#ifdef OLED_BRIGHTNESS
    oled.init(OLED_BRIGHTNESS);
#else
    oled.init();        // use default (lowest brightness)
#endif
}

void ExtOled::process_1s()
{
    char buf[30];
    unsigned int len;

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
    len = snprintf(buf, sizeof(buf), "SOC:%d", bms_status.soc);
    oled.writeString(buf, len);

    oled.setTextCursor(0, 16);
    len = snprintf(buf, sizeof(buf), "Load: %.2fV", load_voltage);
    oled.writeString(buf, len);

    for (int i = 0; i < NUM_CELLS_MAX; i++) {
        if (blinkOn || !(bms_status.balancing_status & (1 << i))) {
            oled.setTextCursor((i % 2 == 0) ? 0 : 64, 24 + (i / 2) * 8);
            len = snprintf(buf, sizeof(buf), "%d:%.3f V", i+1, bms_status.cell_voltages[i]);
            oled.writeString(buf, len);
        }
    }

    oled.display();
}

#endif /* CONFIG_EXT_OLED_DISPLAY */

#endif /* UNIT_TEST */
