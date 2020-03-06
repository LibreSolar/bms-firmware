/*
 * Copyright (c) 2019 Martin Jäger / Libre Solar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIT_TEST

#include "config.h"

#if CONFIG_EXT_OLED_DISPLAY     // otherwise don't compile code to reduce firmware size

#include "ext/ext.h"

#include <math.h>
#include <stdio.h>

#if defined(__ZEPHYR__)
#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#endif

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

const unsigned char bmp_load [] = {
    0x20, 0x22, 0x04, 0x70, 0x88, 0x8B, 0x88, 0x70, 0x04, 0x22, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x07, 0x04, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char bmp_arrow_right [] = {
    0x41, 0x63, 0x36, 0x1C
};

const unsigned char bmp_pv_panel [] = {
    0x60, 0x98, 0x86, 0xC9, 0x31, 0x19, 0x96, 0x62, 0x32, 0x2C, 0xC4, 0x64, 0x98, 0x08, 0xC8, 0x30,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04, 0x04, 0x03, 0x00, 0x00
};

const unsigned char bmp_disconnected [] = {
    0x08, 0x08, 0x08, 0x08, 0x00, 0x41, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x41, 0x00, 0x08, 0x08,
    0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#if defined(__MBED__)
I2C i2c_dev(PIN_UEXT_SDA, PIN_UEXT_SCL);
OledSSD1306 oled(i2c_dev);
#elif defined(__ZEPHYR__)
const char i2c_dev[] = DT_ALIAS_I2C_UEXT_LABEL;
OledSSD1306 oled(i2c_dev);
#endif

void ExtOled::enable()
{
#if defined(PIN_UEXT_DIS) && defined(__MBED__)
    DigitalOut uext_dis(PIN_UEXT_DIS);
    uext_dis = 0;
#endif

#ifdef DT_SWITCH_UEXT_GPIOS_CONTROLLER
    struct device *dev_uext_dis = device_get_binding(DT_SWITCH_UEXT_GPIOS_CONTROLLER);
    gpio_pin_configure(dev_uext_dis, DT_SWITCH_UEXT_GPIOS_PIN,
        DT_SWITCH_UEXT_GPIOS_FLAGS | GPIO_OUTPUT_ACTIVE);
#endif

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
