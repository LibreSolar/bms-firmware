/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
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
 * Possible BMS states
 */
enum BmsState {
    BMS_STATE_OFF,          ///< Off state (charging and discharging disabled)
    BMS_STATE_CHG,          ///< Charging state (discharging disabled)
    BMS_STATE_DIS,          ///< Discharging state (charging disabled)
    BMS_STATE_NORMAL,       ///< Normal operating mode (both charging and discharging enabled)
};


/**
 * Battery cell types
 */
enum CellType {
    CELL_TYPE_CUSTOM = 0,   ///< Custom settings
    CELL_TYPE_LFP,          ///< LiFePO4 Li-ion cells (3.3 V nominal)
    CELL_TYPE_NMC,          ///< NMC/Graphite Li-ion cells (3.7 V nominal)
    CELL_TYPE_NMC_HV,       ///< NMC/Graphite High Voltage Li-ion cells (3.7 V nominal, 4.35 V max)
    CELL_TYPE_LTO           ///< NMC/Titanate (2.4 V nominal)
};

/**
 * BMS configuration values, stored in RAM. The configuration is not automatically applied after
 * values are changed!
 */
typedef struct
{
    /// Effective resistance of the current measurement shunt(s) on the PCB (milli-Ohms)
    float shunt_res_mOhm;

    /// Beta value of the used thermistor. Typical value for Semitec 103AT-5 thermistor: 3435
    uint16_t thermistor_beta;

    /// \brief Pointer to an array containing the Open Circuit Voltage of the cell vs. SOC.
    /// SOC must be equally spaced in descending order (100%, 95%, ..., 5%, 0%)
    float *ocv;
    size_t num_ocv_points;          ///< Number of point in OCV array

    float nominal_capacity_Ah;      ///< Nominal capacity of battery pack (Ah)

    // Current limits
    float dis_sc_limit;             ///< Discharge short circuit limit (A)
    uint32_t dis_sc_delay_us;       ///< Discharge short circuit delay (us)
    float dis_oc_limit;             ///< Discharge over-current limit (A)
    uint32_t dis_oc_delay_ms;       ///< Discharge over-current delay (ms)
    float chg_oc_limit;             ///< Charge over-current limit (A)
    uint32_t chg_oc_delay_ms;       ///< Charge over-current delay (ms)

    // Cell voltage limits
    float cell_chg_voltage;         ///< Cell target charge voltage (V)
    float cell_dis_voltage;         ///< Cell discharge voltage limit (V)
    float cell_ov_limit;            ///< Cell over-voltage limit (V)
    float cell_ov_reset;            ///< Cell over-voltage error reset threshold (V)
    uint32_t cell_ov_delay_ms;      ///< Cell over-voltage delay (ms)
    float cell_uv_limit;            ///< Cell under-voltage limit (V)
    float cell_uv_reset;            ///< Cell under-voltage error reset threshold (V)
    uint32_t cell_uv_delay_ms;      ///< Cell under-voltage delay (ms)

    // Temperature limits (°C)
    float dis_ot_limit;             ///< Discharge over-temperature (DOT) limit (°C)
    float dis_ut_limit;             ///< Discharge under-temperature (DUT) limit (°C)
    float chg_ot_limit;             ///< Charge over-temperature (COT) limit (°C)
    float chg_ut_limit;             ///< Charge under-temperature (CUT) limit (°C)
    float t_limit_hyst;             ///< Temperature limit hysteresis (°C)

    // Balancing settings
    bool auto_balancing_enabled;    ///< Enable automatic balancing
    float bal_cell_voltage_diff;    ///< Balancing cell voltage target difference (V)
    float bal_cell_voltage_min;     ///< Minimum cell voltage to start balancing (V)
    float bal_idle_current;         ///< Current threshold to be considered idle (A)
    uint16_t bal_idle_delay;        ///< Minimum idle duration before balancing (s)
} BmsConfig;

/**
 * Current BMS status including measurements and error flags
 */
typedef struct
{
    uint16_t state;                             ///< Current state of the battery
    bool chg_enable;                            ///< Manual enable/disable setting for charging
    bool dis_enable;                            ///< Manual enable/disable setting for discharging

    uint16_t connected_cells;                   ///< \brief Actual number of cells connected (might
                                                ///< be less than NUM_CELLS_MAX)

    float cell_voltages[NUM_CELLS_MAX];         ///< Single cell voltages (V)
    float cell_voltage_max;                     ///< Maximum cell voltage (V)
    float cell_voltage_min;                     ///< Minimum cell voltage (V)
    float cell_voltage_avg;                     ///< Average cell voltage (V)
    float pack_voltage;                         ///< Battery pack voltage (V)

    float pack_current;                         ///< \brief Battery pack current, charging direction
                                                ///< has positive sign (A)

    float bat_temps[NUM_THERMISTORS_MAX];       ///< Battery temperatures (°C)
    float bat_temp_max;                         ///< Maximum battery temperature (°C)
    float bat_temp_min;                         ///< Minimum battery temperature (°C)
    float bat_temp_avg;                         ///< Average battery temperature (°C)
    float mosfet_temp;                          ///< MOSFET temperature (°C)
    float ic_temp;                              ///< Internal BMS IC temperature (°C)
    float mcu_temp;                             ///< MCU temperature (°C)

    bool full;                                  ///< CV charging to cell_chg_voltage finished
    bool empty;                                 ///< Battery is discharged below cell_dis_voltage

    float soc;                                  ///< Calculated State of Charge (%)

    uint32_t balancing_status;                  ///< holds on/off status of balancing switches
    time_t no_idle_timestamp;                   ///< Stores last time of current > idle threshold

    uint32_t error_flags;                       ///< Bit array for different BmsErrorFlag errors
} BmsStatus;


/**
 * BMS error flags
 */
enum BmsErrorFlag {
    BMS_ERR_CELL_UNDERVOLTAGE = 0,      ///< Cell undervoltage flag
    BMS_ERR_CELL_OVERVOLTAGE = 1,       ///< Cell undervoltage flag
    BMS_ERR_SHORT_CIRCUIT = 2,          ///< Pack short circuit (discharge direction)
    BMS_ERR_DIS_OVERCURRENT = 3,        ///< Pack overcurrent (discharge direction)
    BMS_ERR_CHG_OVERCURRENT = 4,        ///< Pack overcurrent (charge direction)
    BMS_ERR_OPEN_WIRE = 5,              ///< Cell open wire
    BMS_ERR_DIS_UNDERTEMP = 6,          ///< Temperature below discharge minimum limit
    BMS_ERR_DIS_OVERTEMP = 7,           ///< Temperature above discharge maximum limit
    BMS_ERR_CHG_UNDERTEMP = 8,          ///< Temperature below charge maximum limit
    BMS_ERR_CHG_OVERTEMP = 9,           ///< Temperature above charge maximum limit
    BMS_ERR_INT_OVERTEMP = 10,          ///< Internal temperature above limit (e.g. MOSFETs or IC)
    BMS_ERR_CELL_FAILURE = 11           ///< Cell failure (too high voltage difference)
};

/**
 * Initialization of BMS incl. setup of communication. This function does not yet set any config.
 */
void bms_init_hardware();

/**
 * Initialization of BmsStatus with suitable default values.
 */
void bms_init_status(BmsStatus *status);

/**
 * Initialization of BmsConfig with typical default values for the given cell type.
 *
 * Type must be defined as int, as enum CellType cannot be set via Kconfig.
 */
void bms_init_config(BmsConfig *conf, int type, float nominal_capacity);

/**
 * Main BMS state machine
 */
void bms_state_machine(BmsConfig *conf, BmsStatus *status);

/**
 * Update measurements and check for errors before calling the state machine
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
 * Charging error flags check
 *
 * @returns true if any charging error flag is set
 */
bool bms_chg_error(BmsStatus *status);

/**
 * Discharging error flags check
 *
 * @returns true if any discharging error flag is set
 */
bool bms_dis_error(BmsStatus *status);

/**
 * Check if charging is allowed
 *
 * @returns true if no charging error flags are set
 */
bool bms_chg_allowed(BmsStatus *status);

/**
 * Check if discharging is allowed
 *
 * @returns true if no discharging error flags are set
 */
bool bms_dis_allowed(BmsStatus *status);

/**
 * Balancing limits check
 *
 * @returns if balancing is allowed
 */
bool bms_balancing_allowed(BmsConfig *conf, BmsStatus *status);

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
float bms_apply_dis_scp(BmsConfig *conf);

/**
 * Apply discharge overcurrent protection (OCP) threshold and delay.
 *
 * @param conf BMS configuration containing the limit settings
 * @returns applied current threshold value or 0 in case of error
 */
float bms_apply_dis_ocp(BmsConfig *conf);

/**
 * Apply charge overcurrent protection (OCP) threshold and delay.
 *
 * @param conf BMS configuration containing the limit settings
 * @returns applied current threshold value or 0 in case of error
 */
float bms_apply_chg_ocp(BmsConfig *conf);

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
 * Reads all cell voltages to array cell_voltages[NUM_CELLS], updates battery_voltage and updates
 * ids of cells with min/max voltage
 */
void bms_read_voltages(BmsStatus *status);

/**
 * Reads pack current and updates coloumb counter and SOC
 */
void bms_read_current(BmsConfig *conf, BmsStatus *status);

/**
 * Reads all temperature sensors
 */
void bms_read_temperatures(BmsConfig *conf, BmsStatus *status);

/**
 * Reads error flags from IC or updates them based on measurements
 */
void bms_update_error_flags(BmsConfig *conf, BmsStatus *status);

/**
 * Tries to handle / resolve errors
 */
void bms_handle_errors(BmsConfig *conf, BmsStatus *status);

#if BMS_DEBUG

/**
 * Print passed BMS IC register
 */
void bms_print_register(uint8_t addr);

/**
 * Print all BMS IC registers
 */
void bms_print_registers();

#endif

#ifdef __cplusplus
}
#endif

#endif // BMS_H
