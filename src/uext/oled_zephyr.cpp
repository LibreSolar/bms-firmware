/* Libre Solar Battery Management System firmware
 * Copyright (c) 2016-2019 Martin Jäger (www.libre.solar)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UNIT_TEST

#include "uext.h"

#include <zephyr.h>
#include <drivers/i2c.h>
#include <display/cfb.h>

#include <stdio.h>

#include "config.h"

#ifdef OLED_ENABLED

#define DISPLAY_DRIVER		"SSD1306"

static struct device *dev;

void uext_init()
{
	u16_t rows;
	u8_t ppt;
	u8_t font_width;
	u8_t font_height;

	dev = device_get_binding(DISPLAY_DRIVER);

	if (dev == NULL) {
		printf("Device not found\n");
		return;
	}

	if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO10) != 0) {
		printf("Failed to set required pixel format\n");
		return;
	}

	printf("initialized %s\n", DISPLAY_DRIVER);

	if (cfb_framebuffer_init(dev)) {
		printf("Framebuffer initialization failed!\n");
		return;
	}

	cfb_framebuffer_clear(dev, true);
	cfb_framebuffer_invert(dev);

	display_blanking_off(dev);

	rows = cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS);
	ppt = cfb_get_display_parameter(dev, CFB_DISPLAY_PPT);

	for (int idx = 0; idx < 42; idx++) {
		if (cfb_get_font_size(dev, idx, &font_width, &font_height)) {
			break;
		}
		cfb_framebuffer_set_font(dev, idx);
		printf("font width %d, font height %d\n",
		       font_width, font_height);
	}

	printf("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n",
	       cfb_get_display_parameter(dev, CFB_DISPLAY_WIDTH),
	       cfb_get_display_parameter(dev, CFB_DISPLAY_HEIGH),
	       ppt,
	       rows,
	       cfb_get_display_parameter(dev, CFB_DISPLAY_COLS));

	cfb_framebuffer_clear(dev, false);
	if (cfb_print(dev,
				"Hello world!\"§$%&/()=",
				0, 1 * ppt)) {
		printf("Failed to print a string\n");
	}

	cfb_framebuffer_finalize(dev);
}

void uext_process_1s()
{
    /*
    int balancingStatus = bms.get_balancing_status();

    i2c.frequency(400000);
    oled.clearDisplay();

    oled.setTextCursor(0, 0);
    oled.printf("%.2f V", bms.pack_voltage()/1000.0);
    oled.setTextCursor(64, 0);
    oled.printf("%.2f A", bms.pack_current()/1000.0);

    oled.setTextCursor(0, 8);
    oled.printf("T:%.1f C", bms.get_temp_degC(1));
    oled.setTextCursor(64, 8);
    oled.printf("SOC:%.2f", bms.get_soc());

    oled.setTextCursor(0, 16);
    oled.printf("Load: %.2fV", load_voltage/1000.0);

    for (int i = 0; i < NUM_CELLS_MAX; i++) {
        if (blinkOn || !(balancingStatus & (1 << i))) {
            oled.setTextCursor((i % 2 == 0) ? 0 : 64, 24 + (i / 2) * 8);
            oled.printf("%d:%.3f V", i+1, bms.cell_voltage(i+1)/1000.0);
        }
    }

    oled.display();
    */
}

#endif /* OLED_ENABLED */

#endif /* UNIT_TEST */
