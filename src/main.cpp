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

#include "mbed.h"
#include "config.h"     // select hardware version
#include "pcb.h"
#include "output.h"
#include "output_can.h"
#include "data_objects.h"

#include "bms.h"

//----------------------------------------------------------------------------
// global variables

Serial serial(PIN_SWD_TX, PIN_SWD_RX, "serial");
CAN can(PIN_CAN_RX, PIN_CAN_TX, 250000);  // 250 kHz

BMS bms;

DigitalOut led_green(PIN_LED_GREEN);
DigitalOut led_red(PIN_LED_RED);
DigitalOut can_disable(PIN_CAN_STB);
DigitalOut pchg_enable(PIN_PCHG_EN);    // precharge capacitors on the bus

DigitalIn button(PIN_SW_POWER);
Timer btnTimer;

AnalogIn v_bat(PIN_V_BAT);
AnalogIn v_load(PIN_V_LOAD);

Ticker tick1, tick2, tick3;

int battery_voltage;
int battery_current;
int load_voltage;
int cell_voltages[15];      // max. number of cells
float temperatures[3];
float SOC;

int balancingStatus = 0;
bool blinkOn = false;

int OCV[] = { // 100, 95, ..., 0 %
  3392, 3314, 3309, 3308, 3304, 3296, 3283, 3275, 3271, 3268, 3265,
  3264, 3262, 3252, 3240, 3226, 3213, 3190, 3177, 3132, 2833
};

//----------------------------------------------------------------------------
// function prototypes

void setup();
void update_measurements();

void toggleBlink();     // to blink cell voltages on display during balancing

//----------------------------------------------------------------------------
int main()
{
    led_green = 1;
    led_red = 1;
    can_disable = 0;
    time_t last_second = time(NULL);

    setup();

    tick1.attach(&toggleBlink, 0.2);
    tick2.attach(&can_send_data, 1);

    while(1) {

        can_process_outbox();
        can_process_inbox();

        bms.update();

        // called once per second
        if (time(NULL) - last_second >= 1) {
            last_second = time(NULL);

            // shutdown BMS if button is pressed > 3 seconds
            if (button == 1) {
                btnTimer.start();
                if (btnTimer.read() > 3) {
                    bms.shutdown();
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

            update_measurements();

            //BMS.print_registers();
            output_oled();
            //output_doglcd();

            led_green = !led_green;
        }

        sleep();    // wake-up by ticker interrupts
    }
}

//----------------------------------------------------------------------------
void setup()
{
    //serial.baud(9600);
    serial.baud(115200);
    serial.printf("\nSerial interface started. Time: %d\n", (unsigned int)time(NULL));
    freopen("/serial", "w", stdout);    // retarget stdout to serial

    can.attach(&can_receive);
    can.mode(CAN::Normal);

    // not needed if doglcd not used. remove if other device connected to UEXT SPI port
    init_doglcd();

    // TXFP: Transmit FIFO priority driven by request order (chronologically)
    // NART: No automatic retransmission
    CAN1->MCR |= CAN_MCR_TXFP | CAN_MCR_NART;

    // ToDo: Ensure that these settings are set even in case of initial communication error
    bms.temperature_limits(-20, 45, 0, 45);
    bms.set_shunt_res(SHUNT_RESISTOR);
    bms.dis_sc_limit(35000, 200);  // delay in us
    bms.chg_oc_limit(25000, 200);  // delay in ms
    bms.dis_oc_limit(20000, 320); // delay in ms
    bms.cell_uv_limit(2800, 2); // delay in s
    bms.cell_ov_limit(3650, 2);  // delay in s

    bms.set_ocv(OCV, sizeof(OCV)/sizeof(int));
    bms.set_battery_capacity(45000);  // mAh

    bms.update();   // get voltage and temperature measurements before switching on

    bms.balancing_thresholds(10, 3200, 10);  // minIdleTime_min, minCellV_mV, maxVoltageDiff_mV
    bms.set_idle_current_threshold(100);
    bms.auto_balancing(true);

    // TODO: watch voltage rise before stopping pre-charge
    pchg_enable = 1;
    wait(2);
    pchg_enable = 0;

    bms.update();
    bms.reset_soc();
    bms.dis_switch(true);
    bms.chg_switch(true);

    update_measurements();
}

//----------------------------------------------------------------------------
void toggleBlink()
{
    blinkOn = !blinkOn;
}

//----------------------------------------------------------------------------
void update_measurements(void)
{
    const int vcc = 3300;     // mV
    unsigned long sum_adc_readings;

/*
    // battery voltage divider o-- 100k --o-- 5.6k --o
    sum_adc_readings = 0;
    for (int i = 0; i < ADC_AVG_SAMPLES; i++) {
        sum_adc_readings += v_bat.read_u16();
    }
    battery_voltage = (sum_adc_readings / ADC_AVG_SAMPLES) * 110 / 10 * vcc / 0xFFFF;
*/
    // load voltage divider o-- 100k --o-- 5.6k --o
    sum_adc_readings = 0;
    for (int i = 0; i < ADC_AVG_SAMPLES; i++) {
        sum_adc_readings += v_load.read_u16();
    }
    load_voltage = (sum_adc_readings / ADC_AVG_SAMPLES) * 110 / 10 * vcc / 0xFFFF;

    battery_voltage = bms.pack_voltage();
    battery_current = bms.pack_current();
    cell_voltages[0] = bms.cell_voltage(1);
    cell_voltages[1] = bms.cell_voltage(2);
    cell_voltages[2] = bms.cell_voltage(3);
    cell_voltages[3] = bms.cell_voltage(4);
    cell_voltages[4] = bms.cell_voltage(5);
    temperatures[0] = bms.get_temp_degC(1);
    SOC = bms.get_soc();
}
