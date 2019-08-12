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

/** @file
 *
 * @brief
 * Battery Management System (BMS) module for different analog frontends
 */

#include "pcb.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// output information to serial console for debugging
#define BMS_DEBUG 1

/**
 * BMS configuration values, stored in RAM. The configuration is not automatically applied after
 * values are changed!
 */
typedef struct
{
    /// Effective resistance of the current measurement shunt(s) on the PCB (micro-Ohms)
    float shunt_res_mOhm;

    /// Beta value of the used thermistor. Typical value for Semitec 103AT-5 thermistor: 3435
    uint16_t thermistor_beta;

    /// \brief Pointer to an array containing the Open Circuit Voltage of the cell vs. SOC.
    /// SOC must be equally spaced in descending order (100%, 95%, ..., 5%, 0%)
    uint16_t *ocv;
    size_t num_ocv_points;          ///< Number of point in OCV array

    uint32_t nominal_capacity;      ///< Nominal capacity of battery pack (mAh)

    // Current limits (mA)
    uint32_t dis_sc_limit_mA;       ///< Discharge short circuit limit (mA)
    uint32_t dis_sc_delay_us;       ///< Discharge short circuit delay (us)
    uint32_t dis_oc_limit_mA;       ///< Discharge over-current limit (mA)
    uint32_t dis_oc_delay_ms;       ///< Discharge over-current delay (ms)
    uint32_t chg_oc_limit_mA;       ///< Charge over-current limit (mA)
    uint32_t chg_oc_delay_ms;       ///< Charge over-current delay (ms)

    // Cell voltage limits (mV)
    uint32_t cell_ov_limit_mV;      ///< Cell over-voltage limit (mV)
    uint32_t cell_ov_delay_ms;      ///< Cell over-voltage delay (ms)
    uint32_t cell_uv_limit_mV;      ///< Cell under-voltage limit (mV)
    uint32_t cell_uv_delay_ms;      ///< Cell under-voltage delay (ms)

    // Temperature limits (°C)
    int16_t dis_ot_limit;           ///< Discharge over-temperature (DOT) limit (°C)
    int16_t dis_ut_limit;           ///< Discharge under-temperature (DUT) limit (°C)
    int16_t chg_ot_limit;           ///< Charge over-temperature (COT) limit (°C)
    int16_t chg_ut_limit;           ///< Charge under-temperature (CUT) limit (°C)
    int16_t t_limit_hyst;           ///< Temperature limit hysteresis (°C)

    // Balancing settings
    bool auto_balancing_enabled;            ///< Enable automatic balancing
    uint16_t balancing_voltage_diff_target; ///< Balancing voltage difference target (mV)
    uint16_t balancing_cell_voltage_min;    ///< Minimum voltage to start balancing (mV)
    uint16_t balancing_min_idle_s;          ///< Minimum idle duration before balancing (s)
    uint16_t idle_current_threshold;        ///< Current threshold to be considered idle (mA)
} BmsConfig;


typedef struct
{
    uint16_t connected_cells;                   ///< \brief Actual number of cells connected (might
                                                ///< be less than NUM_CELLS_MAX)
    uint16_t cell_voltages[NUM_CELLS_MAX];      ///< Single cell voltages (mV)
    uint16_t id_cell_voltage_max;               ///< ID of cell with maximum voltage
    uint16_t id_cell_voltage_min;               ///< ID of cell with minimum voltage
    uint32_t pack_voltage;                      ///< Battery pack voltage (mV)
    int32_t pack_current;                       ///< Battery pack current (mA)
    int16_t temperatures[NUM_THERMISTORS_MAX];  ///< Temperatures (°C/10)

    uint16_t soc;                               ///< Calculated state of charge (SOC)
    uint32_t coulomb_counter;                   ///< Current integration (mAs = milli Coulombs)

    uint32_t balancing_status;                  ///< holds on/off status of balancing switches
    time_t no_idle_timestamp;                   ///< Stores last time of current > idle threshold

    uint32_t error_flags;                       ///< Bit array for different BmsErrorFlag errors
} BmsStatus;


/**
 * BMS error flags
 */
enum BmsErrorFlag {
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

/**
 * Initialization of BMS incl. setup of communication. This function does not yet set any config.
 */
void bms_init();

/**
 * Initialization of BmsConfig with typical default values.
 */
void bms_init_config(BmsConfig *conf);

/**
 * Fast function to check if BMS has an error
 *
 * @returns 0 if everything is OK
 */
int bms_quickcheck(BmsConfig *conf, BmsStatus *status);

/**
 * Update and check important measurements
 *
 * Should be called at least once every 250 ms to get correct coulomb counting
 */
void bms_update(BmsConfig *conf, BmsStatus *status);

/**
 * Shut down BMS IC and entire PCB power supply
 */
void bms_shutdown();

/**
 * Enable/disable charge MOSFET
 */
bool bms_chg_switch(BmsConfig *conf, BmsStatus *status, bool enable);

/**
 * Enable/disable discharge MOSFET
 */
bool bms_dis_switch(BmsConfig *conf, BmsStatus *status, bool enable);

/**
 * SOC reset to specified value or calculation based on average cell open circuit voltage
 *
 * @param percent 0-100 %, -1 for calculation based on OCV
 */
void bms_reset_soc(BmsConfig *conf, BmsStatus *status, int percent);

/**
 * Apply charge/discharge temperature limits.
 *
 * @param conf BMS configuration containing the limit settings
 * @returns 1 for success or 0 in case of error
 */
int bms_apply_temp_limits(BmsConfig *conf);

/**
 * Apply discharge short circuit protection (SCP) current threshold and delay.
 *
 * @param conf BMS configuration containing the limit settings
 * @returns applied current threshold value or 0 in case of error
 */
int bms_apply_dis_scp(BmsConfig *conf);

/**
 * Apply discharge overcurrent protection (OCP) threshold and delay.
 *
 * @param conf BMS configuration containing the limit settings
 * @returns applied current threshold value or 0 in case of error
 */
int bms_apply_dis_ocp(BmsConfig *conf);

/**
 * Apply charge overcurrent protection (OCP) threshold and delay.
 *
 * @param conf BMS configuration containing the limit settings
 * @returns applied current threshold value or 0 in case of error
 */
int bms_apply_chg_ocp(BmsConfig *conf);

/**
 * Apply cell undervoltage protection (UVP) threshold and delay.
 *
 * @param conf BMS configuration containing the limit settings
 * @returns 1 for success or 0 in case of error
 */
int bms_apply_cell_uvp(BmsConfig *conf);

/**
 * Apply cell overvoltage protection (OVP) threshold and delay.
 *
 * @param conf BMS configuration containing the limit settings
 * @returns 1 for success or 0 in case of error
 */
int bms_apply_cell_ovp(BmsConfig *conf);

/**
 * Set balancing registers if balancing is allowed (i.e. sufficient idle time + voltage)
 */
void bms_apply_balancing(BmsConfig *conf, BmsStatus *status);

/**
 * Reads all cell voltages to array cell_voltages[NUM_CELLS] and updates battery_voltage
 */
void bms_read_voltages(BmsStatus *status);

/**
 * Reads pack current and updates coloumb counter
 */
void bms_read_current(BmsConfig *conf, BmsStatus *status);

/**
 * Reads all temperature sensors
 */
void bms_read_temperatures(BmsConfig *conf, BmsStatus *status);

/**
 * Calculates new SOC value based on coloumb counter
 */
void bms_update_soc(BmsConfig *conf, BmsStatus *status);

#if BMS_DEBUG
/**
 * Print all BMS IC registers
 */
void bms_print_registers();
#endif

#ifdef __cplusplus
}
#endif

#endif // BMS_H
