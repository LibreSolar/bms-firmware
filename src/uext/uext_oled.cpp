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

#ifndef UNIT_TEST

#include "config.h"

#ifdef OLED_ENABLED     // otherwise don't compile code to reduce firmware size

#include "pcb.h"

#include "Adafruit_SSD1306.h" // Library for SSD1306 OLED-Display, 128x64

#include "bms.h"
#include "data_objects.h"

I2C i2c(PIN_UEXT_SDA, PIN_UEXT_SCL);
Adafruit_SSD1306_I2c oled(i2c, PB_2, 0x78, 64, 128);

extern bool blinkOn;

extern BMS bms;
extern Serial serial;

extern int battery_voltage;
extern int battery_current;
extern int load_voltage;
extern int cell_voltages[15];      // max. number of cells
extern float temperatures[3];
extern float SOC;

void uext_init() {;}    // not needed

void uext_process_1s()    // OLED SSD1306
{
    int balancingStatus = bms_get_balancing_status(&bms);

    i2c.frequency(400000);
    oled.clearDisplay();

    oled.setTextCursor(0, 0);
    oled.printf("%.2f V", bms_pack_voltage(&bms)/1000.0);
    oled.setTextCursor(64, 0);
    oled.printf("%.2f A", bms_pack_current(&bms)/1000.0);

    oled.setTextCursor(0, 8);
    oled.printf("T:%.1f C", bms_get_temp_degC(&bms, 1));
    oled.setTextCursor(64, 8);
    oled.printf("SOC:%.2f", bms_get_soc(&bms));

    oled.setTextCursor(0, 16);
    oled.printf("Load: %.2fV", load_voltage/1000.0);

    for (int i = 0; i < NUM_CELLS_MAX; i++) {
        if (blinkOn || !(balancingStatus & (1 << i))) {
            oled.setTextCursor((i % 2 == 0) ? 0 : 64, 24 + (i / 2) * 8);
            oled.printf("%d:%.3f V", i+1, bms_cell_voltage(&bms, i+1)/1000.0);
        }
    }

    oled.display();
}

#endif /* OLED_ENABLED */

#endif /* UNIT_TEST */
