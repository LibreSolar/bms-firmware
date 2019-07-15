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
#include "uext.h"
#include "output_can.h"
#include "output_serial.h"
#include "data_objects.h"

#include "bms.h"

//----------------------------------------------------------------------------
// global variables

Serial serial(PIN_SWD_TX, PIN_SWD_RX, "serial");
CAN can(PIN_CAN_RX, PIN_CAN_TX, 250000);  // 250 kHz

bms_t bms;

DigitalOut led_green(PIN_LED_GREEN);
DigitalOut led_red(PIN_LED_RED);
DigitalOut can_disable(PIN_CAN_STB);
#ifdef PIN_PCHG_EN
DigitalOut pchg_enable(PIN_PCHG_EN);    // precharge capacitors on the bus
#endif

DigitalIn button(PIN_SW_POWER);
Timer btnTimer;

#ifdef PIN_V_BAT
AnalogIn v_bat(PIN_V_BAT);
#endif
AnalogIn v_ext(PIN_V_EXT);

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

        bms_update(&bms);

        // called once per second
        if (time(NULL) - last_second >= 1) {
            last_second = time(NULL);

            // shutdown BMS if button is pressed > 3 seconds
            if (button == 1) {
                btnTimer.start();
                if (btnTimer.read() > 3) {
                    bms_shutdown(&bms);
                }
            } else {
                btnTimer.stop();
                btnTimer.reset();
            }

            fflush(stdout);

            update_measurements();

            uext_process_1s();

            //bms_print_registers();

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

    uext_init();

    // TXFP: Transmit FIFO priority driven by request order (chronologically)
    // NART: No automatic retransmission
    CAN1->MCR |= CAN_MCR_TXFP | CAN_MCR_NART;

    // ToDo: Ensure that these settings are set even in case of initial communication error
    bms_temperature_limits(&bms, -20, 45, 0, 45);
    bms_set_shunt_res(&bms, SHUNT_RESISTOR);
    bms_dis_sc_limit(&bms, 35000, 200);  // delay in us
    bms_chg_oc_limit(&bms, 25000, 200);  // delay in ms
    bms_dis_oc_limit(&bms, 20000, 320); // delay in ms
    bms_cell_uv_limit(&bms, 2800, 2); // delay in s
    bms_cell_ov_limit(&bms, 3650, 2);  // delay in s

    bms_set_ocv(&bms, OCV, sizeof(OCV)/sizeof(int));
    bms_set_battery_capacity(&bms, 45000);  // mAh

    bms_update(&bms);   // get voltage and temperature measurements before switching on

    bms_balancing_thresholds(&bms, 10, 3200, 10);  // minIdleTime_min, minCellV_mV, maxVoltageDiff_mV
    bms_set_idle_current_threshold(&bms, 100);
    bms_auto_balancing(&bms, true);

    // TODO: watch voltage rise before stopping pre-charge
#ifdef PIN_PCHG_EN
    pchg_enable = 1;
    wait(2);
    pchg_enable = 0;
#endif

    bms_update(&bms);
    bms_reset_soc(&bms);
    bms_dis_switch(&bms, true);
    bms_chg_switch(&bms, true);

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
        sum_adc_readings += v_ext.read_u16();
    }
    load_voltage = (sum_adc_readings / ADC_AVG_SAMPLES) * 110 / 10 * vcc / 0xFFFF;

    battery_voltage = bms_pack_voltage(&bms);
    battery_current = bms_pack_current(&bms);
    cell_voltages[0] = bms_cell_voltage(&bms, 1);
    cell_voltages[1] = bms_cell_voltage(&bms, 2);
    cell_voltages[2] = bms_cell_voltage(&bms, 3);
    cell_voltages[3] = bms_cell_voltage(&bms, 4);
    cell_voltages[4] = bms_cell_voltage(&bms, 5);
    temperatures[0] = bms_get_temp_degC(&bms, 1);
    SOC = bms_get_soc(&bms);
}
