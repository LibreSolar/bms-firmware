/* LibreSolar Battery Management System firmware
 * Copyright (c) 2016-2018 Martin Jäger (www.libre.solar)
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

#include "config.h"

#include "Adafruit_SSD1306.h" // Library for SSD1306 OLED-Display, 128x64
#include "DogLCD.h"
#include "font_6x8.h"
#include "font_8x16.h"

#include "bq769x0.h"
#include "data_objects.h"

Serial serial_uext(PIN_UEXT_TX, PIN_UEXT_RX, "serial_uext");

I2C i2c(PIN_UEXT_SDA, PIN_UEXT_SCL);
Adafruit_SSD1306_I2c oled(i2c, PB_2, 0x78, 64, 128);

SPI spi(PIN_UEXT_MOSI, NC, PIN_UEXT_SCK);
DogLCD lcd(spi, PIN_UEXT_SSEL, PIN_UEXT_RX, PIN_UEXT_TX); //  spi, cs, a0, reset

extern bool blinkOn;

extern bq769x0 BMS;
extern Serial serial;

extern int battery_voltage;
extern int battery_current;
extern int load_voltage;
extern int cell_voltages[15];      // max. number of cells
extern float temperatures[3];
extern float SOC;

void output_serial()
{
    serial.printf("|");
    for (int i = 1; i <= 5; i++) {
        serial.printf("%i|", BMS.getCellVoltage(i));
    }
    serial.printf("%i|", BMS.getBatteryVoltage());
    serial.printf("%i|", BMS.getBatteryCurrent());
    serial.printf("%.2f|", BMS.getSOC());
    serial.printf("%.1f|", BMS.getTemperatureDegC(1));
    //serial.printf("%.1f|", BMS.getTemperatureDegC(2));
    //serial.printf("%.1f|", BMS.getTemperatureDegC(3));
    serial.printf("%i|", load_voltage);
    serial.printf("%i|", BMS.getBalancingStatus());

    serial.printf(" \n");
}

void init_doglcd()
{
    lcd.init();
    lcd.view(VIEW_TOP);
}

void output_doglcd()    // EA DOG
{
    char str[20];

    int balancingStatus = BMS.getBalancingStatus();

    lcd.clear_screen();

    sprintf(str, "%.2fV", BMS.getBatteryVoltage()/1000.0);
    lcd.string(0,0,font_8x16, str);

    sprintf(str, "%.2fA", BMS.getBatteryCurrent()/1000.0);
    lcd.string(7*8,0,font_8x16, str);

    sprintf(str, "T:%.1f", BMS.getTemperatureDegC(1));
    lcd.string(0,2,font_6x8, str);

    sprintf(str, "SOC:%.2f", BMS.getSOC());
    lcd.string(6*7,2,font_6x8, str);

    sprintf(str, "Load: %.2fV", load_voltage/1000.0);
    lcd.string(0,3,font_6x8, str);

    for (int i = 0; i < BMS.getNumberOfCells(); i++) {
        if (blinkOn || !(balancingStatus & (1 << i))) {
            sprintf(str, "%d:%.3fV", i+1, BMS.getCellVoltage(i+1)/1000.0);
            lcd.string((i % 2 == 0) ? 0 : 51, 4 + (i / 2), font_6x8, str);
        }
    }
}

void output_oled()    // OLED SSD1306
{
    int balancingStatus = BMS.getBalancingStatus();

    i2c.frequency(400000);
    oled.clearDisplay();

    oled.setTextCursor(0, 0);
    oled.printf("%.2f V", BMS.getBatteryVoltage()/1000.0);
    oled.setTextCursor(64, 0);
    oled.printf("%.2f A", BMS.getBatteryCurrent()/1000.0);

    oled.setTextCursor(0, 8);
    oled.printf("T:%.1f C", BMS.getTemperatureDegC(1));
    oled.setTextCursor(64, 8);
    oled.printf("SOC:%.2f", BMS.getSOC());

    oled.setTextCursor(0, 16);
    oled.printf("Load: %.2fV", load_voltage/1000.0);

    for (int i = 0; i < BMS.getNumberOfCells(); i++) {
        if (blinkOn || !(balancingStatus & (1 << i))) {
            oled.setTextCursor((i % 2 == 0) ? 0 : 64, 24 + (i / 2) * 8);
            oled.printf("%d:%.3f V", i+1, BMS.getCellVoltage(i+1)/1000.0);
        }
    }

    oled.display();
}
