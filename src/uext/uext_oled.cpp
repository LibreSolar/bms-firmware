/* LibreSolar Battery Management System firmware
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

#if defined(__MBED__) && !defined(UNIT_TEST)

#include "config.h"

#ifdef OLED_ENABLED     // otherwise don't compile code to reduce firmware size

#include "oled_ssd1306.h"

#include "pcb.h"
#include "bms.h"

I2C i2c(PIN_UEXT_SDA, PIN_UEXT_SCL);
OledSSD1306 oled(i2c);

extern bool blinkOn;

extern BmsStatus bms_status;

extern float load_voltage;

void uext_init()
{
#ifdef OLED_BRIGHTNESS
    oled.init(OLED_BRIGHTNESS);
#else
    oled.init();        // use default (lowest brightness)
#endif
}

void uext_process_1s()    // OLED SSD1306
{
    char buf[30];
    unsigned int len;

    i2c.frequency(400000);
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

#endif /* OLED_ENABLED */

#endif /* UNIT_TEST */
