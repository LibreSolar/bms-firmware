/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_BMS_BMS_H_
#define ZEPHYR_BMS_BMS_H_

/**
 * @file
 * @brief Battery Management System (BMS) high-level API
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <drivers/bms_ic.h>

#include "bms_common.h"

#include <stdbool.h>
#include <stdint.h>

/* fixed number of OCV vs. SOC points */
#define NUM_OCV_POINTS 21

/**
 * Possible BMS states
 */
enum bms_state
{
    BMS_STATE_OFF,      ///< Off state (charging and discharging disabled)
    BMS_STATE_CHG,      ///< Charging state (discharging disabled)
    BMS_STATE_DIS,      ///< Discharging state (charging disabled)
    BMS_STATE_NORMAL,   ///< Normal operating mode (both charging and discharging enabled)
    BMS_STATE_SHUTDOWN, ///< BMS starting shutdown sequence
};

/**
 * Battery cell types
 */
enum bms_cell_type
{
    CELL_TYPE_LFP, ///< LiFePO4 Li-ion cells (3.3 V nominal)
    CELL_TYPE_NMC, ///< NMC/Graphite Li-ion cells (3.7 V nominal)
    CELL_TYPE_LTO, ///< NMC/Titanate (2.4 V nominal)
};

/**
 * Battery Management System context information
 */
struct bms_context
{
    /** Current state of the battery */
    enum bms_state state;

    /** Manual enable/disable setting for charging */
    bool chg_enable;
    /** Manual enable/disable setting for discharging */
    bool dis_enable;

    /** CV charging to cell_chg_voltage_limit finished */
    bool full;
    /** Battery is discharged below cell_dis_voltage_limit */
    bool empty;

    /** Calculated State of Charge (%) */
    float soc;

    /** Nominal capacity of battery pack (Ah) */
    float nominal_capacity_Ah;

    /**
     * Pointer to an array containing the Open Circuit Voltage of the cell vs. SOC. The array
     * must be spaced the same as the soc_points.
     */
    float *ocv_points;

    /** Pointer to an array containing the State of Charge points for the OCV. */
    float *soc_points;

    /** BMS IC device pointer */
    const struct device *ic_dev;

    /** BMS IC configuration applied during start-up. */
    struct bms_ic_conf ic_conf;

    /** BMS IC data read from the device. */
    struct bms_ic_data ic_data;
};

/**
 * Initialization of struct bms_ic_conf with typical default values for the given cell type.
 *
 * @param bms Pointer to BMS object.
 * @param type One of enum CellType (defined as int so that it can be set via Kconfig).
 * @param capacity_Ah Nominal capacity of the battery pack.
 */
void bms_init_config(struct bms_context *bms, enum bms_cell_type type, float capacity_Ah);

/**
 * Main BMS state machine
 *
 * @param bms Pointer to BMS object.
 */
void bms_state_machine(struct bms_context *bms);

/**
 * Switch off MOSFETs and go into the shutdown state
 *
 * @param bms Pointer to BMS object.
 */
void bms_shutdown(struct bms_context *bms);

/**
 * Update SOC based on most recent current measurement
 *
 * Function should be called each time after a new current measurement was obtained.
 *
 * @param bms Pointer to BMS object.
 */
void bms_soc_update(struct bms_context *bms);

/**
 * Reset SOC to specified value or calculate based on average cell open circuit voltage
 *
 * @param bms Pointer to BMS object.
 * @param percent 0-100 %, -1 for calculation based on OCV
 */
void bms_soc_reset(struct bms_context *bms, int percent);

/**
 * Charging error flags check
 *
 * @returns true if any charging error flag is set
 */
bool bms_chg_error(uint32_t error_flags);

/**
 * Discharging error flags check
 *
 * @returns true if any discharging error flag is set
 */
bool bms_dis_error(uint32_t error_flags);

/**
 * Check if charging is allowed
 *
 * @returns true if no charging error flags are set
 */
bool bms_chg_allowed(struct bms_context *bms);

/**
 * Check if discharging is allowed
 *
 * @param bms Pointer to BMS object.
 *
 * @returns true if no discharging error flags are set
 */
bool bms_dis_allowed(struct bms_context *bms);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_BMS_BMS_H_ */
