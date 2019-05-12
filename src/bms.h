/* Battery management system based on bq769x0 for ARM mbed
 * Copyright (c) 2015-2018 Martin Jäger (www.libre.solar)
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

#ifndef BMS_H
#define BMS_H

#include "mbed.h"

#define MAX_NUMBER_OF_CELLS 15
#define MAX_NUMBER_OF_THERMISTORS 3
#define NUM_OCV_POINTS 21

// IC type/size
#define bq76920 1
#define bq76930 2
#define bq76940 3

// output information to serial console for debugging
#define BMS_DEBUG 1

class BMS {

public:
    // initialization, status update and shutdown
    BMS(I2C& bqI2C, PinName alertPin, int bqType = bq76930, int bqI2CAddress = 0x08, bool crc = true);
    int check_status();  // returns 0 if everything is OK
    void update(void);
    void boot(PinName bootPin);
    void shutdown(void);

    // charging/discharging MOSFET control
    bool chg_switch(bool enable);
    bool dis_switch(bool enable);

    // hardware settings
    void set_shunt_res(float res_mOhm);
    void set_thermistor_beta(int beta_K);

    void reset_soc(int percent = -1);    // 0-100 %, -1 for automatic reset based on OCV
    void set_battery_capacity(long capacity_mAh);
    void set_ocv(int voltageVsSOC[NUM_OCV_POINTS]);

    int get_num_cells_max(void);
    int get_connected_cells(void);

    // limit settings (for battery protection)
    void temperature_limits(int min_dis_degC, int max_dis_degC, int min_chg_degC, int max_chg_degC, int hysteresis_degC = 2);    // °C
    long dis_sc_limit(long current_mA, int delay_us = 70);
    long chg_oc_limit(long current_mA, int delay_ms = 8);
    long dis_oc_limit(long current_mA, int delay_ms = 8);
    int cell_uv_limit(int voltage_mV, int delay_s = 1);
    int cell_ov_limit(int voltage_mV, int delay_s = 1);

    // balancing settings
    void balancing_thresholds(int idleTime_min = 30, int absVoltage_mV = 3400, int voltageDifference_mV = 20);
    void set_idle_current_threshold(int current_mA);

    // automatic balancing when battery is within balancing thresholds
    void auto_balancing(bool enable);

    // battery status
    int  pack_current(void);
    int  pack_voltage(void);
    int  cell_voltage(int idCell);    // from 1 to 15
    int  cell_voltage_min(void);
    int  cell_voltage_max(void);
    //int  cell_voltage_avg(void);
    float get_temp_degC(int channel = 1);
    float get_temp_degF(int channel = 1);
    float get_soc(void);
    int get_balancing_status(void);

    // interrupt handling (not to be called manually!)
    void set_alert_interrupt_flag(void);

    #if BMS_DEBUG
    void print_registers(void);
    #endif

private:

    // Variables

    I2C& _i2c;
    Timer _timer;
    InterruptIn _alertInterrupt;

    int I2CAddress;
    int bq_type;
    bool crc_enabled;

    float shunt_res_mOhm;
    int thermistor_beta;  // typical value for Semitec 103AT-5 thermistor: 3435
    int *OCV;  // Open Circuit Voltage of cell for SOC 100%, 95%, ..., 5%, 0%

    int num_cells_max;                      // number of cells allowed by IC
    int connected_cells;                     // actual number of cells connected
    int cell_voltages[MAX_NUMBER_OF_CELLS];          // mV
    int id_cell_voltage_max;
    int id_cell_voltage_min;
    long battery_voltage;                                // mV
    long battery_current;                                // mA
    int temperatures[MAX_NUMBER_OF_THERMISTORS];    // °C/10

    long nominal_capacity;    // mAs, nominal capacity of battery pack, max. 1193 Ah possible
    long coulomb_counter;     // mAs (= milli Coulombs) for current integration

    // Current limits (mA)
    //long charge_current_limit_max;
    //long discharge_current_limit_max;
    int idle_current_threshold; // mA

    // temperature limits (°C/10)
    int chg_temp_limit_min;
    int dis_temp_limit_min;
    int chg_temp_limit_max;
    int dis_temp_limit_max;
    int temp_limit_hysteresis;

    // cell voltage limits (mV)
    int cell_voltage_limit_max;
    int cell_voltage_limit_min;
    int balancing_cell_voltage_min;     // mV
    int balancing_voltage_diff_target;   // mV

    int error_status;
    bool auto_balancing_enabled;
    unsigned int balancing_status;     // holds on/off status of balancing switches
    int balancing_min_idle_s;
    unsigned long idle_timestamp;

    unsigned int sec_since_error_counter;
    unsigned long interrupt_timestamp;

    bool chg_temp_error_flag;
    bool dis_temp_error_flag;

    // Methods

    bool determine_address_and_crc(void);

    void update_voltages(void);
    void update_current(void);
    void update_temperatures(void);

    void update_balancing_switches(void);

    void check_cell_temp(void);

    int  read_register(int address);
    void write_register(int address, int data);

};

#endif // BMS_H
