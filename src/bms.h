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

#include "pcb.h"

#include <stdint.h>
#include <stddef.h>

// output information to serial console for debugging
#define BMS_DEBUG 1


/** BMS error flags
 */
enum error_flag_t {
    ERR_CELL_UNDERVOLTAGE,  ///< Cell undervoltage flag
    ERR_CELL_OVERVOLTAGE,   ///< Cell undervoltage flag
    ERR_SHORT_CIRCUIT,      ///< Pack short circuit (discharge direction)
    ERR_DIS_OVERCURRENT,    ///< Pack overcurrent (discharge direction)
    ERR_CHG_OVERCURRENT,    ///< Pack overcurrent (charge direction)
    ERR_OPEN_WIRE,          ///< Cell open wire
    ERR_DIS_UNDERTEMP,      ///< Temperature below discharge minimum limit
    ERR_DIS_OVERTEMP,       ///< Temperature above discharge maximum limit
    ERR_CHG_UNDERTEMP,      ///< Temperature below charge maximum limit
    ERR_CHG_OVERTEMP,       ///< Temperature above charge maximum limit
    ERR_INT_OVERTEMP,       ///< Internal temperature above limit (e.g. MOSFETs or IC)
    ERR_CELL_FAILURE        ///< Cell failure (too high voltage difference)
};

class BMS {

public:
    /** Initialization of BMS
     */
    BMS();

    /** Fast function to check if BMS has an error
     *
     * @returns 0 if everything is OK
     */
    int check_status();

    /** Update and check important measurements
     *
     * Should be called at least once every 250 ms to get correct coulomb counting
     */
    void update(void);

    /** Shut down BMS IC and entire PCB power supply
     */
    void shutdown(void);

    /** Enable/disable charge MOSFET
     */
    bool chg_switch(bool enable);

    /** Enable/disable discharge MOSFET
     */
    bool dis_switch(bool enable);

    // hardware settings
    void set_shunt_res(float res_mOhm);
    void set_thermistor_beta(int beta_K);

    /** SOC calculation based on average cell open circuit voltage
     *
     * @param percent 0-100 %, -1 for automatic reset based on OCV
     */
    void reset_soc(int percent = -1);
    void set_battery_capacity(long capacity_mAh);
    void set_ocv(int *voltage_vs_soc, size_t num_points);

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

    #if BMS_DEBUG
    void print_registers(void);
    #endif

private:

    float shunt_res_mOhm;
    int thermistor_beta;                    ///< typical value for Semitec 103AT-5 thermistor: 3435

    int *OCV;                               ///< Open Circuit Voltage of cell for SOC 100%, 95%, ..., 5%, 0%
    size_t num_ocv_points;                  ///< Number of point in OCV array

    int connected_cells;                    ///< Actual number of cells connected (might be less than NUM_CELLS_MAX)
    int cell_voltages[NUM_CELLS_MAX];       ///< Single cell voltages (mV)
    int id_cell_voltage_max;                ///< ID of cell with maximum voltage
    int id_cell_voltage_min;                ///< ID of cell with minimum voltage
    long battery_voltage;                   ///< Battery pack voltage (mV)
    long battery_current;                   ///< Battery pack current (mA)
    int temperatures[NUM_THERMISTORS_MAX];  ///< Temperatures (°C/10)

    long nominal_capacity;                  ///< Nominal capacity of battery pack (mA, max. 1193 Ah possible)
    long coulomb_counter;                   ///< Current integration (mAs = milli Coulombs)

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

    bool chg_temp_error_flag;
    bool dis_temp_error_flag;

    uint32_t error_flags;       ///< New variable to store all different errors

    /** Reads all cell voltages to array cell_voltages[NUM_CELLS] and updates battery_voltage
     */
    void update_voltages(void);

    /** Reads pack current
     */
    void update_current(void);

    /** Reads all temperature sensors
     */
    void update_temperatures(void);

    /** Sets balancing registers if balancing is allowed (i.e. sufficient idle time + voltage)
     */
    void update_balancing_switches(void);

    /** Checks if temperatures are within the limits, otherwise disables CHG/DSG FET
     */
    void check_cell_temp(void);
};

#endif // BMS_H
