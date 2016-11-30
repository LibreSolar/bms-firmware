 /* LibreSolar MPPT charge controller firmware
  * Copyright (c) 2016 Martin Jäger (www.libre.solar)
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

#include "mbed.h"
#include "DogLCD.h"
#include "font_6x8.h"
#include "font_8x16.h"
#include "bq769x0.h"    // Library for Texas Instruments bq76920 battery management IC

#define DISPLAY_ENABLED
#define ADC_AVG_SAMPLES 8       // number of ADC values to read for averaging
#define BMS_I2C_ADDRESS 0x08

int OCV[NUM_OCV_POINTS] = { // 100, 95, ..., 0 %
  3392, 3314, 3309, 3308, 3304, 3296, 3283, 3275, 3271, 3268, 3265,
  3264, 3262, 3252, 3240, 3226, 3213, 3190, 3177, 3132, 2833
};

int balancingStatus = 0;
bool blinkOn = false;

//----------------------------------------------------------------------------
// global variables

#ifdef DISPLAY_ENABLED
SPI spi(PB_5, NC, PB_3);
DogLCD lcd(spi, PA_1, PA_3, PA_2); //  spi, cs, a0, reset
#endif

Serial serial(PB_10, PB_11, "serial");

I2C i2c_bq(PB_14, PB_13);
bq769x0 BMS(i2c_bq, PB_12);    // battery management system object

DigitalOut led1(PA_9);
DigitalOut led2(PA_10);

DigitalIn button(PB_15);
Timer btnTimer;

AnalogIn v_bat(PA_4);
AnalogIn v_load(PA_5);

Ticker tick1, tick2;

int battery_voltage;    // mV
int load_voltage;       // mV

//----------------------------------------------------------------------------
// function prototypes

void setup();
void update_measurements();
void toggleBlink();

#ifdef DISPLAY_ENABLED
void update_screen();
#endif

//----------------------------------------------------------------------------
int main()
{
    led1 = 1;
    led2 = 1;

    setup();

    #ifdef DISPLAY_ENABLED
    tick1.attach(&update_screen, 0.5);
    #endif

    tick2.attach(&toggleBlink, 0.5);

    while(1) {

        // shutdown BMS if button is pressed > 3 seconds
        if (button == 1) {
            btnTimer.start();
            if (btnTimer.read() > 3) {
                BMS.shutdown();
            }
        } else {
            btnTimer.stop();
            btnTimer.reset();
        }

        fflush(stdout);

        /*
        // test communication
        char buf[10];
        buf[0] = 0x0B;
        i2c_bq.write(BMS_I2C_ADDRESS << 1, buf, 1);
        i2c_bq.read(BMS_I2C_ADDRESS << 1, buf, 1);
        serial.printf("buf: 0x%x\n", buf[0]);
        */

        BMS.update();
        update_measurements();

        //BMS.printRegisters();

        wait(0.2);

        led2 = !led2;
    }
}

//----------------------------------------------------------------------------
void setup()
{
    serial.baud(115200);
    serial.printf("\nSerial interface started...\n");

    // retarget stdout to serial
    freopen("/serial", "w", stdout);

#ifdef DISPLAY_ENABLED
    lcd.init();
    lcd.view(VIEW_TOP);
#endif

    // ToDo: Ensure that these settings are set even in case of initial communication error
    BMS.setTemperatureLimits(-20, 45, 0, 45);
    BMS.setShuntResistorValue(1);
    BMS.setShortCircuitProtection(14000, 200);  // delay in us
    BMS.setOvercurrentChargeProtection(8000, 200);  // delay in ms
    BMS.setOvercurrentDischargeProtection(8000, 320); // delay in ms
    BMS.setCellUndervoltageProtection(2800, 2); // delay in s
    BMS.setCellOvervoltageProtection(3650, 2);  // delay in s

    BMS.setOCV(OCV);
    BMS.setBatteryCapacity(45000);  // mAh

    BMS.update();   // get voltage and temperature measurements before switching on
    BMS.resetSOC();

    BMS.setBalancingThresholds(1, 3200, 10);  // minIdleTime_min, minCellV_mV, maxVoltageDiff_mV
    BMS.setIdleCurrentThreshold(100);
    BMS.enableAutoBalancing();

    BMS.enableDischarging();
    BMS.enableCharging();

    update_measurements();
}

void toggleBlink()
{
    blinkOn = !blinkOn;
}

#ifdef DISPLAY_ENABLED
void update_screen(void)
{
    char str[20];

    balancingStatus = BMS.getBalancingStatus();

    lcd.clear_screen();

    sprintf(str, "%.2fV", BMS.getBatteryVoltage()/1000.0);
    lcd.string(0,0,font_8x16, str);

    sprintf(str, "%.2fA", BMS.getBatteryCurrent()/1000.0);
    lcd.string(7*8,0,font_8x16, str);

    sprintf(str, "T:%.1f", BMS.getTemperatureDegC(1));
    lcd.string(0,2,font_6x8, str);

    sprintf(str, "SOC:%.2f", BMS.getSOC());
    lcd.string(6*7,2,font_6x8, str);

    sprintf(str, "DC bus: %.2fV", battery_voltage/1000.0);
    lcd.string(0,3,font_6x8, str);

    if (blinkOn || !(balancingStatus & (1 << 0))) {
        sprintf(str, "1:%.3fV", BMS.getCellVoltage(1)/1000.0);
        lcd.string(0,4,font_6x8, str);
    }

    if (blinkOn || !(balancingStatus & (1 << 1))) {
        sprintf(str, "2:%.3fV", BMS.getCellVoltage(2)/1000.0);
        lcd.string(51,4,font_6x8, str);
    }

    if (blinkOn || !(balancingStatus & (1 << 2))) {
        sprintf(str, "3:%.3fV", BMS.getCellVoltage(3)/1000.0);
        lcd.string(0,5,font_6x8, str);
    }

    if (blinkOn || !(balancingStatus & (1 << 4))) {
        sprintf(str, "4:%.3fV", BMS.getCellVoltage(5)/1000.0);
        lcd.string(51,5,font_6x8, str);
    }

    if (blinkOn || !(balancingStatus & (1 << 5))) {
        sprintf(str, "5:%.3fV", BMS.getCellVoltage(6)/1000.0);
        lcd.string(0,6,font_6x8, str);
    }

    if (blinkOn || !(balancingStatus & (1 << 6))) {
        sprintf(str, "6:%.3fV", BMS.getCellVoltage(7)/1000.0);
        lcd.string(51,6,font_6x8, str);
    }

    if (blinkOn || !(balancingStatus & (1 << 7))) {
        sprintf(str, "7:%.3fV", BMS.getCellVoltage(8)/1000.0);
        lcd.string(0,7,font_6x8, str);
    }

    if (blinkOn || !(balancingStatus & (1 << 9))) {
        sprintf(str, "8:%.3fV", BMS.getCellVoltage(10)/1000.0);
        lcd.string(51,7,font_6x8, str);
    }
}
#endif // DISPLAY_ENABLED

//----------------------------------------------------------------------------
void update_measurements(void)
{
    const int vcc = 3300;     // mV
    unsigned long sum_adc_readings;

    // battery voltage divider o-- 100k --o-- 5.6k --o
    sum_adc_readings = 0;
    for (int i = 0; i < ADC_AVG_SAMPLES; i++) {
        sum_adc_readings += v_bat.read_u16();
    }
    battery_voltage = (sum_adc_readings / ADC_AVG_SAMPLES) * 1056 / 56 * vcc / 0xFFFF;

    // load voltage divider o-- 100k --o-- 5.6k --o
    sum_adc_readings = 0;
    for (int i = 0; i < ADC_AVG_SAMPLES; i++) {
        sum_adc_readings += v_load.read_u16();
    }
    load_voltage = (sum_adc_readings / ADC_AVG_SAMPLES) * 1056 / 56 * vcc / 0xFFFF;
}
