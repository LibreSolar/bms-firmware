/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "oled_font.h"

#include <stdio.h>

#include <bms/bms.h>

#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(oled, CONFIG_LOG_DEFAULT_LEVEL);

extern struct bms_context bms;

const struct device *oled_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static bool blink_on = false;

void oled_overview_screen()
{
    static char buf[30];
    unsigned int len;

    cfb_framebuffer_clear(oled_dev, false);

    cfb_print(oled_dev, "Libre Solar", 0, 0);
    cfb_print(oled_dev, DT_PROP(DT_PATH(pcb), type), 0, 12);

    len = snprintf(buf, sizeof(buf), "%.2fV", bms.ic_data.total_voltage);
    cfb_print(oled_dev, buf, 0, 28);

    len = snprintf(buf, sizeof(buf), "%.1fA", bms.ic_data.current);
    cfb_print(oled_dev, buf, 64, 28);

    len = snprintf(buf, sizeof(buf), "T:%.1f", bms.ic_data.cell_temp_avg);
    cfb_print(oled_dev, buf, 0, 40);

    len = snprintf(buf, sizeof(buf), "SOC:%.0f", bms.soc);
    cfb_print(oled_dev, buf, 64, 40);

    len = snprintf(buf, sizeof(buf), "Err:0x%X", bms.ic_data.error_flags);
    cfb_print(oled_dev, buf, 0, 52);

    cfb_framebuffer_finalize(oled_dev);
}

void oled_cell_voltages_screen(int offset)
{
    static char buf[30];
    unsigned int len;

    cfb_framebuffer_clear(oled_dev, false);

    cfb_print(oled_dev, "Cell Voltages", 0, 0);

    for (int i = offset; i < CONFIG_BMS_IC_MAX_CELLS; i++) {
        if (blink_on || !(bms.ic_data.balancing_status & (1 << i))) {
            len = snprintf(buf, sizeof(buf), "%d:%.2f", i + 1, bms.ic_data.cell_voltages[i]);
            cfb_print(oled_dev, buf, (i % 2 == 0) ? 0 : 64, 16 + (i / 2) * 12);
        }
    }

    cfb_framebuffer_finalize(oled_dev);
}

void oled_thread()
{
    uint16_t rows;
    uint8_t ppt;

    if (!device_is_ready(oled_dev)) {
        LOG_ERR("OLED device %s not ready", oled_dev->name);
        return;
    }

    if (display_set_pixel_format(oled_dev, PIXEL_FORMAT_MONO10) != 0) {
        LOG_ERR("Failed to set required pixel format");
        return;
    }

    LOG_DBG("Initialized OLED %s", oled_dev->name);

    if (cfb_framebuffer_init(oled_dev)) {
        LOG_ERR("Framebuffer initialization failed!");
        return;
    }

    cfb_framebuffer_clear(oled_dev, true);
    cfb_framebuffer_invert(oled_dev);

    display_blanking_off(oled_dev);

    rows = cfb_get_display_parameter(oled_dev, CFB_DISPLAY_ROWS);
    ppt = cfb_get_display_parameter(oled_dev, CFB_DISPLAY_PPT);

    /* set first and only font */
    cfb_framebuffer_set_font(oled_dev, 0);

    LOG_DBG("x_res %d, y_res %d, ppt %d, rows %d, cols %d",
            cfb_get_display_parameter(oled_dev, CFB_DISPLAY_WIDTH),
            cfb_get_display_parameter(oled_dev, CFB_DISPLAY_HEIGH), ppt, rows,
            cfb_get_display_parameter(oled_dev, CFB_DISPLAY_COLS));

    while (true) {
        blink_on = !blink_on;

        oled_overview_screen();
        k_sleep(K_SECONDS(3));

        oled_cell_voltages_screen(0);
        k_sleep(K_SECONDS(3));

        if (CONFIG_BMS_IC_MAX_CELLS > 8) {
            oled_cell_voltages_screen(8);
            k_sleep(K_SECONDS(3));
        }
    }
}

K_THREAD_DEFINE(oled_thread_id, 1024, oled_thread, NULL, NULL, NULL, 0, 0, 1000);
