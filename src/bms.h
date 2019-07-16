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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// output information to serial console for debugging
#define BMS_DEBUG 1


typedef struct
{
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
} BMS;

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

/** Initialization of BMS
 */
void bms_init(BMS *bms);

/** Fast function to check if BMS has an error
 *
 * @returns 0 if everything is OK
 */
int bms_check_status(BMS *bms);

/** Update and check important measurements
 *
 * Should be called at least once every 250 ms to get correct coulomb counting
 */
void bms_update(BMS *bms);

/** Shut down BMS IC and entire PCB power supply
 */
void bms_shutdown(BMS *bms);

/** Enable/disable charge MOSFET
 */
bool bms_chg_switch(BMS *bms, bool enable);

/** Enable/disable discharge MOSFET
 */
bool bms_dis_switch(BMS *bms, bool enable);

// hardware settings
void bms_set_shunt_res(BMS *bms, float res_mOhm);
void bms_set_thermistor_beta(BMS *bms, int beta_K);

/** SOC calculation based on average cell open circuit voltage
 *
 * @param percent 0-100 %, -1 for automatic reset based on OCV
 */
void bms_reset_soc(BMS *bms, int percent);
void bms_set_battery_capacity(BMS *bms, long capacity_mAh);
void bms_set_ocv(BMS *bms, int *voltage_vs_soc, size_t num_points);

int bms_get_connected_cells(BMS *bms);

// limit settings (for battery protection)
void bms_temperature_limits(BMS *bms, int min_dis_degC, int max_dis_degC, int min_chg_degC, int max_chg_degC, int hysteresis_degC);    // °C
long bms_dis_sc_limit(BMS *bms, long current_mA, int delay_us);
long bms_chg_oc_limit(BMS *bms, long current_mA, int delay_ms);
long bms_dis_oc_limit(BMS *bms, long current_mA, int delay_ms);
int bms_cell_uv_limit(BMS *bms, int voltage_mV, int delay_s);
int bms_cell_ov_limit(BMS *bms, int voltage_mV, int delay_s);

// balancing settings
void bms_balancing_thresholds(BMS *bms, int idle_time_min, int abs_voltage_mV, int voltage_difference_mV);
void bms_set_idle_current_threshold(BMS *bms, int current_mA);

// automatic balancing when battery is within balancing thresholds
void bms_auto_balancing(BMS *bms, bool enable);

// battery status
int  bms_pack_current(BMS *bms);
int  bms_pack_voltage(BMS *bms);
int  bms_cell_voltage(BMS *bms, int idCell);    // from 1 to 15
int  bms_cell_voltage_min(BMS *bms);
int  bms_cell_voltage_max(BMS *bms);
//int  cell_voltage_avg(void);
float bms_get_temp_degC(BMS *bms, int channel);
float bms_get_temp_degF(BMS *bms, int channel);
float bms_get_soc(BMS *bms);
int bms_get_balancing_status(BMS *bms);

#if BMS_DEBUG
void bms_print_registers();
#endif

//private:

/** Reads all cell voltages to array cell_voltages[NUM_CELLS] and updates battery_voltage
 */
void bms_update_voltages(BMS *bms);

/** Reads pack current
 */
void bms_update_current(BMS *bms);

/** Reads all temperature sensors
 */
void bms_update_temperatures(BMS *bms);

/** Sets balancing registers if balancing is allowed (i.e. sufficient idle time + voltage)
 */
void bms_update_balancing_switches(BMS *bms);

/** Checks if temperatures are within the limits, otherwise disables CHG/DSG FET
 */
void bms_check_cell_temp(BMS *bms);

#ifdef __cplusplus
}
#endif

#endif // BMS_H
