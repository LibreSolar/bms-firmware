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

#if defined(__MBED__) && !defined(UNIT_TEST)

#include "mbed.h"
#include "config.h"     // select hardware version
#include "pcb.h"
#include "ext/ext.h"
#include "thingset.h"

#include "bms.h"
#include "leds.h"

//----------------------------------------------------------------------------
// global variables

Serial serial(PIN_SWD_TX, PIN_SWD_RX, "serial");

BmsConfig bms_conf;
BmsStatus bms_status;
extern ThingSet ts;

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

float load_voltage;

int balancingStatus = 0;
bool blinkOn = false;

//----------------------------------------------------------------------------
// function prototypes

void setup();
void update_measurements();

void toggleBlink();     // to blink cell voltages on display during balancing

//----------------------------------------------------------------------------
int main()
{
    time_t last_second = time(NULL);

    setup();

    tick1.attach(&toggleBlink, 0.2);
    tick3.attach(&leds_update, 0.1);

    while(1) {

        bms_update(&bms_conf, &bms_status);

        ext_mgr.process_asap();

        // called once per second
        if (time(NULL) - last_second >= 1) {
            last_second = time(NULL);

            // shutdown BMS if button is pressed > 3 seconds
            if (button == 1) {
                btnTimer.start();
                if (btnTimer.read() > 3) {
                    bms_shutdown();
                }
            } else {
                btnTimer.stop();
                btnTimer.reset();
            }

            fflush(stdout);

            update_measurements();
            bms_state_machine(&bms_conf, &bms_status);

            ext_mgr.process_1s();

            //bms_print_registers();
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

    // ToDo: Ensure that below settings are set even in case of communication error

    bms_init();
//    bms_init_config(&bms_conf, CELL_TYPE_LFP, 45);
    bms_init_config(&bms_conf, CELL_TYPE_CUSTOM, 200); //(xsider)
//    bms_init_config(&bms_conf, CELL_TYPE_LiFeYPo, 160); //(xsider)


    // apply config to AFE
    bms_apply_dis_scp(&bms_conf);
    bms_apply_dis_ocp(&bms_conf);
    bms_apply_chg_ocp(&bms_conf);
    bms_apply_cell_ovp(&bms_conf);
    bms_apply_cell_uvp(&bms_conf);
    bms_apply_temp_limits(&bms_conf);

    bms_conf.shunt_res_mOhm = SHUNT_RESISTOR;

    bms_update(&bms_conf, &bms_status);   // get voltage and temperature measurements before switching on
    bms_apply_balancing(&bms_conf, &bms_status);

    // TODO: watch voltage rise before stopping pre-charge
#ifdef PIN_PCHG_EN
    pchg_enable = 1;
    wait(2);
    pchg_enable = 0;
#endif

    bms_update(&bms_conf, &bms_status);
    bms_reset_soc(&bms_conf, &bms_status, -1);
    bms_dis_switch(&bms_conf, &bms_status, true);
    bms_chg_switch(&bms_conf, &bms_status, true);

    update_measurements();

    // initialize all extensions and external communication interfaces
    ext_mgr.enable_all();
}

//----------------------------------------------------------------------------
void toggleBlink()
{
    blinkOn = !blinkOn;
}

//----------------------------------------------------------------------------
void update_measurements(void)
{
    const float vcc = 3.3;
    unsigned long sum_adc_readings = 0;
    for (int i = 0; i < ADC_AVG_SAMPLES; i++) {
        sum_adc_readings += v_ext.read_u16();
    }
    load_voltage = (sum_adc_readings / ADC_AVG_SAMPLES) * GAIN_PACK_VOLTAGE * vcc / 0xFFFF;

    bms_update(&bms_conf, &bms_status);
}

#endif /* UNIT_TEST */
