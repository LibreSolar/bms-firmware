/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bms.h"
#include "board.h"
#include "helper.h"

#include <stdio.h>

LOG_MODULE_REGISTER(bms, CONFIG_LOG_DEFAULT_LEVEL);

static float ocv_lfp[OCV_POINTS] = { 3.392F, 3.314F, 3.309F, 3.308F, 3.304F, 3.296F, 3.283F,
                                     3.275F, 3.271F, 3.268F, 3.265F, 3.264F, 3.262F, 3.252F,
                                     3.240F, 3.226F, 3.213F, 3.190F, 3.177F, 3.132F, 2.833F };

static float ocv_nmc[OCV_POINTS] = { 4.198F, 4.135F, 4.089F, 4.056F, 4.026F, 3.993F, 3.962F,
                                     3.924F, 3.883F, 3.858F, 3.838F, 3.819F, 3.803F, 3.787F,
                                     3.764F, 3.745F, 3.726F, 3.702F, 3.684F, 3.588F, 2.800F };

void bms_init_status(Bms *bms)
{
    bms->status.chg_enable = true;
    bms->status.dis_enable = true;
}

void bms_init_config(Bms *bms, int type, float nominal_capacity)
{
    bms->conf.bal_idle_delay = 1800;         // default: 30 minutes
    bms->conf.bal_idle_current = 0.1F;       // A
    bms->conf.bal_cell_voltage_diff = 0.01F; // 10 mV

    bms->conf.thermistor_beta = 3435; // typical value for Semitec 103AT-5 thermistor

    bms->conf.nominal_capacity_Ah = nominal_capacity;

    if (bms->conf.nominal_capacity_Ah < BOARD_MAX_CURRENT) {
        // 1C should be safe for all batteries
        bms->conf.dis_oc_limit = bms->conf.nominal_capacity_Ah;
        bms->conf.chg_oc_limit = bms->conf.nominal_capacity_Ah;
    }
    else {
        bms->conf.dis_oc_limit = BOARD_MAX_CURRENT;
        bms->conf.chg_oc_limit = BOARD_MAX_CURRENT;
    }
    bms->conf.dis_oc_delay_ms = 320;
    bms->conf.chg_oc_delay_ms = 320;

    bms->conf.dis_sc_limit = bms->conf.dis_oc_limit * 2;
    bms->conf.dis_sc_delay_us = 200;

    bms->conf.dis_ut_limit = -20;
    bms->conf.dis_ot_limit = 45;
    bms->conf.chg_ut_limit = 0;
    bms->conf.chg_ot_limit = 45;
    bms->conf.t_limit_hyst = 5;

    bms->conf.shunt_res_mOhm = BOARD_SHUNT_RESISTOR;

    bms->conf.cell_ov_delay_ms = 2000;
    bms->conf.cell_uv_delay_ms = 2000;

    switch (type) {
        case CELL_TYPE_LFP:
            bms->conf.cell_ov_limit = 3.80F;
            bms->conf.cell_chg_voltage = 3.55F;
            bms->conf.cell_ov_reset = 3.40F;
            bms->conf.bal_cell_voltage_min = 3.30F;
            bms->conf.cell_uv_reset = 3.10F;
            bms->conf.cell_dis_voltage = 2.80F;
            bms->conf.cell_uv_limit = 2.50F; // most cells survive even 2.0V, but we should
                                             // keep some margin for further self-discharge
            bms->conf.ocv = ocv_lfp;
            break;
        case CELL_TYPE_NMC:
            bms->conf.cell_ov_limit = 4.25F;
            bms->conf.cell_chg_voltage = 4.20F;
            bms->conf.cell_ov_reset = 4.05F;
            bms->conf.bal_cell_voltage_min = 3.80F;
            bms->conf.cell_uv_reset = 3.50F;
            bms->conf.cell_dis_voltage = 3.20F;
            bms->conf.cell_uv_limit = 3.00F;
            bms->conf.ocv = ocv_nmc;
            break;
        case CELL_TYPE_NMC_HV:
            bms->conf.cell_ov_limit = 4.35F;
            bms->conf.cell_chg_voltage = 4.30F;
            bms->conf.cell_ov_reset = 4.15F;
            bms->conf.bal_cell_voltage_min = 3.80F;
            bms->conf.cell_uv_reset = 3.50F;
            bms->conf.cell_dis_voltage = 3.20F;
            bms->conf.cell_uv_limit = 3.00F;
            // ToDo: Use typical OCV curve for NMC_HV cells
            bms->conf.ocv = NULL;
            break;
        case CELL_TYPE_LTO:
            bms->conf.cell_ov_limit = 2.85F;
            bms->conf.cell_chg_voltage = 2.80F;
            bms->conf.cell_ov_reset = 2.70F;
            bms->conf.bal_cell_voltage_min = 2.50F;
            bms->conf.cell_uv_reset = 2.10F;
            bms->conf.cell_dis_voltage = 2.00F;
            bms->conf.cell_uv_limit = 1.90F;
            // ToDo: Use typical OCV curve for LTO cells
            bms->conf.ocv = NULL;
            break;
        case CELL_TYPE_CUSTOM:
            break;
    }
}

__weak void bms_state_machine(Bms *bms)
{
    bms_handle_errors(bms);

    switch (bms->status.state) {
        case BMS_STATE_OFF:
            if (bms_startup_inhibit()) {
                return;
            }

            if (bms_dis_allowed(bms)) {
                bms_dis_switch(bms, true);
                bms->status.state = BMS_STATE_DIS;
                LOG_INF("OFF -> DIS (error flags: 0x%08x)\n", bms->status.error_flags);
            }
            else if (bms_chg_allowed(bms)) {
                bms_chg_switch(bms, true);
                bms->status.state = BMS_STATE_CHG;
                LOG_INF("OFF -> CHG (error flags: 0x%08x)\n", bms->status.error_flags);
            }
            break;
        case BMS_STATE_CHG:
            if (!bms_chg_allowed(bms)) {
                bms_chg_switch(bms, false);
                bms_dis_switch(bms, false); // if on because of ideal diode control
                bms->status.state = BMS_STATE_OFF;
                LOG_INF("CHG -> OFF (error flags: 0x%08x)\n", bms->status.error_flags);
            }
            else if (bms_dis_allowed(bms)) {
                bms_dis_switch(bms, true);
                bms->status.state = BMS_STATE_NORMAL;
                LOG_INF("CHG -> NORMAL (error flags: 0x%08x)\n", bms->status.error_flags);
            }
#ifndef CONFIG_BQ769X2 // bq769x2 has built-in ideal diode control
            else {
                // ideal diode control for discharge MOSFET (with hysteresis)
                if (bms->status.pack_current > 0.5F) {
                    bms_dis_switch(bms, true);
                }
                else if (bms->status.pack_current < 0.1F) {
                    bms_dis_switch(bms, false);
                }
            }
#endif
            break;
        case BMS_STATE_DIS:
            if (!bms_dis_allowed(bms)) {
                bms_dis_switch(bms, false);
                bms_chg_switch(bms, false); // if on because of ideal diode control
                bms->status.state = BMS_STATE_OFF;
                LOG_INF("DIS -> OFF (error flags: 0x%08x)\n", bms->status.error_flags);
            }
            else if (bms_chg_allowed(bms)) {
                bms_chg_switch(bms, true);
                bms->status.state = BMS_STATE_NORMAL;
                LOG_INF("DIS -> NORMAL (error flags: 0x%08x)\n", bms->status.error_flags);
            }
#ifndef CONFIG_BQ769X2 // bq769x2 has built-in ideal diode control
            else {
                // ideal diode control for charge MOSFET (with hysteresis)
                if (bms->status.pack_current < -0.5F) {
                    bms_chg_switch(bms, true);
                }
                else if (bms->status.pack_current > -0.1F) {
                    bms_chg_switch(bms, false);
                }
            }
#endif
            break;
        case BMS_STATE_NORMAL:
            if (!bms_dis_allowed(bms)) {
                bms_dis_switch(bms, false);
                bms->status.state = BMS_STATE_CHG;
                LOG_INF("NORMAL -> CHG (error flags: 0x%08x)\n", bms->status.error_flags);
            }
            else if (!bms_chg_allowed(bms)) {
                bms_chg_switch(bms, false);
                bms->status.state = BMS_STATE_DIS;
                LOG_INF("NORMAL -> DIS (error flags: 0x%08x)\n", bms->status.error_flags);
            }
            break;
    }
}

bool bms_chg_error(uint32_t error_flags)
{
    return (error_flags & (1U << BMS_ERR_CELL_OVERVOLTAGE))
           || (error_flags & (1U << BMS_ERR_CHG_OVERCURRENT))
           || (error_flags & (1U << BMS_ERR_OPEN_WIRE))
           || (error_flags & (1U << BMS_ERR_CHG_UNDERTEMP))
           || (error_flags & (1U << BMS_ERR_CHG_OVERTEMP))
           || (error_flags & (1U << BMS_ERR_INT_OVERTEMP))
           || (error_flags & (1U << BMS_ERR_CELL_FAILURE))
           || (error_flags & (1U << BMS_ERR_CHG_OFF));
}

bool bms_dis_error(uint32_t error_flags)
{
    return (error_flags & (1U << BMS_ERR_CELL_UNDERVOLTAGE))
           || (error_flags & (1U << BMS_ERR_SHORT_CIRCUIT))
           || (error_flags & (1U << BMS_ERR_DIS_OVERCURRENT))
           || (error_flags & (1U << BMS_ERR_OPEN_WIRE))
           || (error_flags & (1U << BMS_ERR_DIS_UNDERTEMP))
           || (error_flags & (1U << BMS_ERR_DIS_OVERTEMP))
           || (error_flags & (1U << BMS_ERR_INT_OVERTEMP))
           || (error_flags & (1U << BMS_ERR_CELL_FAILURE))
           || (error_flags & (1U << BMS_ERR_DIS_OFF));
}

bool bms_chg_allowed(Bms *bms)
{
    return !bms_chg_error(bms->status.error_flags & ~(1U << BMS_ERR_CHG_OFF)) && !bms->status.full
           && bms->status.chg_enable;
}

bool bms_dis_allowed(Bms *bms)
{
    return !bms_dis_error(bms->status.error_flags & ~(1U << BMS_ERR_DIS_OFF)) && !bms->status.empty
           && bms->status.dis_enable;
}

bool bms_balancing_allowed(Bms *bms)
{
    int idle_sec = uptime() - bms->status.no_idle_timestamp;
    float voltage_diff = bms->status.cell_voltage_max - bms->status.cell_voltage_min;

    return idle_sec >= bms->conf.bal_idle_delay
           && bms->status.cell_voltage_max > bms->conf.bal_cell_voltage_min
           && voltage_diff > bms->conf.bal_cell_voltage_diff;
}
