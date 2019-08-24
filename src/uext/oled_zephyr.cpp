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

#include "bms.h"
#include "config.h"

#ifdef OLED_ENABLED

#define DISPLAY_DRIVER		"SSD1306"

static struct device *dev;

extern BmsStatus bms_status;

void uext_init()
{
	dev = device_get_binding(DISPLAY_DRIVER);
	if (dev == NULL) {
		printf("Device not found\n");
		return;
	}

	display_set_pixel_format(dev, PIXEL_FORMAT_MONO10);
	display_blanking_off(dev);

	cfb_framebuffer_init(dev);
	cfb_framebuffer_clear(dev, true);
	cfb_framebuffer_invert(dev);
	cfb_framebuffer_set_font(dev, 0);	// first and smallest font
}

void uext_process_1s()
{
	char buf[30];
	cfb_framebuffer_clear(dev, false);
	int width = cfb_get_display_parameter(dev, CFB_DISPLAY_WIDTH);

	snprintf(buf, sizeof(buf), "%2.1fV %3.2fA", bms_status.pack_voltage, bms_status.pack_current);
	cfb_print(dev, buf, 0, 0);

	snprintf(buf, sizeof(buf), "%3.0fC SOC%3d%%S:%d E:0x%X", bms_status.bat_temp_avg, bms_status.soc,
		bms_status.state, bms_status.error_flags);
	cfb_print(dev, buf, 1 * width, 0);

	cfb_framebuffer_finalize(dev);
}

#endif /* OLED_ENABLED */

#endif /* UNIT_TEST */
