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

#include "pcb.h"

#include "Adafruit_SSD1306.h" // Library for SSD1306 OLED-Display, 128x64

#include "bms.h"

I2C i2c(PIN_UEXT_SDA, PIN_UEXT_SCL);
Adafruit_SSD1306_I2c oled(i2c, PB_2, 0x78, 64, 128);

extern bool blinkOn;

extern BmsStatus bms_status;

extern float load_voltage;

void uext_init() {;}    // not needed

void uext_process_1s()    // OLED SSD1306
{
    i2c.frequency(400000);
    oled.clearDisplay();

    oled.setTextCursor(0, 0);
    oled.printf("%.2f V", bms_status.pack_voltage/1000.0);
    oled.setTextCursor(64, 0);
    oled.printf("%.2f A", bms_status.pack_current/1000.0);

    oled.setTextCursor(0, 8);
    oled.printf("T:%.1f C", bms_status.bat_temp_avg);
    oled.setTextCursor(64, 8);
    oled.printf("SOC:%.2f", (float)bms_status.soc / 100);

    oled.setTextCursor(0, 16);
    oled.printf("Load: %.2fV", load_voltage/1000.0);

    for (int i = 0; i < NUM_CELLS_MAX; i++) {
        if (blinkOn || !(bms_status.balancing_status & (1 << i))) {
            oled.setTextCursor((i % 2 == 0) ? 0 : 64, 24 + (i / 2) * 8);
            oled.printf("%d:%.3f V", i+1, bms_status.cell_voltages[i]);
        }
    }

    oled.display();
}

#endif /* OLED_ENABLED */

#endif /* UNIT_TEST */
