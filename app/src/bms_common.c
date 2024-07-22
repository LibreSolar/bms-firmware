/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bms/bms.h>
#include <bms/bms_common.h>

#include "helper.h"

#include <stdio.h>

LOG_MODULE_REGISTER(bms, CONFIG_LOG_DEFAULT_LEVEL);

static const float ocv_lfp[NUM_OCV_POINTS] = {
    3.392F, 3.314F, 3.309F, 3.308F, 3.304F, 3.296F, 3.283F, 3.275F, 3.271F, 3.268F, 3.265F,
    3.264F, 3.262F, 3.252F, 3.240F, 3.226F, 3.213F, 3.190F, 3.177F, 3.132F, 2.833F,
};

static const float ocv_nmc[NUM_OCV_POINTS] = {
    4.198F, 4.135F, 4.089F, 4.056F, 4.026F, 3.993F, 3.962F, 3.924F, 3.883F, 3.858F, 3.838F,
    3.819F, 3.803F, 3.787F, 3.764F, 3.745F, 3.726F, 3.702F, 3.684F, 3.588F, 2.800F,
};

// Source: DOI:10.3390/batteries5010031 (https://www.mdpi.com/2313-0105/5/1/31)
static const float ocv_lto[NUM_OCV_POINTS] = {
    2.800F, 2.513F, 2.458F, 2.415F, 2.376F, 2.340F, 2.309F, 2.282F, 2.259F, 2.240F, 2.224F,
    2.210F, 2.198F, 2.187F, 2.177F, 2.166F, 2.154F, 2.141F, 2.125F, 2.105F, 2.000F,
};

static const float soc_default[NUM_OCV_POINTS] = {
    100.0F, 95.0F, 90.0F, 85.0F, 80.0F, 85.0F, 70.0F, 65.0F, 60.0F, 55.0F, 50.0F,
    45.0F,  40.0F, 35.0F, 30.0F, 25.0F, 20.0F, 15.0F, 10.0F, 5.0F,  0.0F,
};

// actually used OCV vs. SOC curve
float ocv_points[NUM_OCV_POINTS] = { 0 };
float soc_points[NUM_OCV_POINTS] = { 0 };

void bms_init_config(struct bms_context *bms, enum bms_cell_type type, float nominal_capacity_Ah)
{
    bms->nominal_capacity_Ah = nominal_capacity_Ah;

    bms->chg_enable = true;
    bms->dis_enable = true;

    bms->ic_conf.bal_idle_delay = 1800;         // default: 30 minutes
    bms->ic_conf.bal_idle_current = 0.1F;       // A
    bms->ic_conf.bal_cell_voltage_diff = 0.01F; // 10 mV

#ifdef CONFIG_BMS_IC_CURRENT_MONITORING
    /* 1C should be safe for all batteries */
    bms->ic_conf.dis_oc_limit = bms->nominal_capacity_Ah;
    bms->ic_conf.chg_oc_limit = bms->nominal_capacity_Ah;

    bms->ic_conf.dis_oc_delay_ms = 320;
    bms->ic_conf.chg_oc_delay_ms = 320;

    bms->ic_conf.dis_sc_limit = bms->ic_conf.dis_oc_limit * 2;
    bms->ic_conf.dis_sc_delay_us = 200;
#endif /* CONFIG_BMS_IC_CURRENT_MONITORING */

    bms->ic_conf.dis_ut_limit = -20;
    bms->ic_conf.dis_ot_limit = 45;
    bms->ic_conf.chg_ut_limit = 0;
    bms->ic_conf.chg_ot_limit = 45;
    bms->ic_conf.temp_limit_hyst = 5;

    bms->ic_conf.cell_ov_delay_ms = 2000;
    bms->ic_conf.cell_uv_delay_ms = 2000;

    bms->ocv_points = ocv_points;
    bms->soc_points = soc_points;

    memcpy(soc_points, soc_default, sizeof(soc_points));

    switch (type) {
        case CELL_TYPE_LFP:
            bms->ic_conf.cell_ov_limit = 3.80F;
            bms->ic_conf.cell_chg_voltage_limit = 3.55F;
            bms->ic_conf.cell_ov_reset = 3.40F;
            bms->ic_conf.bal_cell_voltage_min = 3.30F;
            bms->ic_conf.cell_uv_reset = 3.10F;
            bms->ic_conf.cell_dis_voltage_limit = 2.80F;
            /*
             * most cells survive even 2.0V, but we should keep some margin for further
             * self-discharge
             */
            bms->ic_conf.cell_uv_limit = 2.50F;
            memcpy(ocv_points, ocv_lfp, sizeof(ocv_points));
            break;
        case CELL_TYPE_NMC:
            bms->ic_conf.cell_ov_limit = 4.25F;
            bms->ic_conf.cell_chg_voltage_limit = 4.20F;
            bms->ic_conf.cell_ov_reset = 4.05F;
            bms->ic_conf.bal_cell_voltage_min = 3.80F;
            bms->ic_conf.cell_uv_reset = 3.50F;
            bms->ic_conf.cell_dis_voltage_limit = 3.20F;
            bms->ic_conf.cell_uv_limit = 3.00F;
            memcpy(ocv_points, ocv_nmc, sizeof(ocv_points));
            break;
        case CELL_TYPE_LTO:
            bms->ic_conf.cell_ov_limit = 2.85F;
            bms->ic_conf.cell_chg_voltage_limit = 2.80F;
            bms->ic_conf.cell_ov_reset = 2.70F;
            bms->ic_conf.bal_cell_voltage_min = 2.50F;
            bms->ic_conf.cell_uv_reset = 2.10F;
            bms->ic_conf.cell_dis_voltage_limit = 2.00F;
            bms->ic_conf.cell_uv_limit = 1.90F;
            memcpy(ocv_points, ocv_lto, sizeof(ocv_points));
            break;
    }

    /* trigger alert for all possible errors by default */
    bms->ic_conf.alert_mask = BMS_ERR_ALL;
}

__weak void bms_state_machine(struct bms_context *bms)
{
    switch (bms->state) {
        case BMS_STATE_OFF:
            if (bms_dis_allowed(bms)) {
                bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_DIS, true);
                bms->state = BMS_STATE_DIS;
                LOG_INF("OFF -> DIS (error flags: 0x%08x)", bms->ic_data.error_flags);
            }
            else if (bms_chg_allowed(bms)) {
                bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_CHG, true);
                bms->state = BMS_STATE_CHG;
                LOG_INF("OFF -> CHG (error flags: 0x%08x)", bms->ic_data.error_flags);
            }
            break;
        case BMS_STATE_CHG:
            if (!bms_chg_allowed(bms)) {
                bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_CHG, false);
                /* DIS switch may be on on because of ideal diode control */
                bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_DIS, false);
                bms->state = BMS_STATE_OFF;
                LOG_INF("CHG -> OFF (error flags: 0x%08x)", bms->ic_data.error_flags);
            }
            else if (bms_dis_allowed(bms)) {
                bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_DIS, true);
                bms->state = BMS_STATE_NORMAL;
                LOG_INF("CHG -> NORMAL (error flags: 0x%08x)", bms->ic_data.error_flags);
            }
#ifndef CONFIG_BMS_IC_BQ769X2 /* bq769x2 has built-in ideal diode control */
            else {
                /* ideal diode control for discharge MOSFET (with hysteresis) */
                if (bms->ic_data.current > 0.5F) {
                    bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_DIS, true);
                }
                else if (bms->ic_data.current < 0.1F) {
                    bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_DIS, false);
                }
            }
#endif
            break;
        case BMS_STATE_DIS:
            if (!bms_dis_allowed(bms)) {
                bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_DIS, false);
                /* CHG_FET may be on because of ideal diode control */
                bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_CHG, false);
                bms->state = BMS_STATE_OFF;
                LOG_INF("DIS -> OFF (error flags: 0x%08x)", bms->ic_data.error_flags);
            }
            else if (bms_chg_allowed(bms)) {
                bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_CHG, true);
                bms->state = BMS_STATE_NORMAL;
                LOG_INF("DIS -> NORMAL (error flags: 0x%08x)", bms->ic_data.error_flags);
            }
#ifndef CONFIG_BMS_IC_BQ769X2 /* bq769x2 has built-in ideal diode control */
            else {
                /* ideal diode control for charge MOSFET (with hysteresis) */
                if (bms->ic_data.current < -0.5F) {
                    bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_CHG, true);
                }
                else if (bms->ic_data.current > -0.1F) {
                    bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_CHG, false);
                }
            }
#endif
            break;
        case BMS_STATE_NORMAL:
            if (!bms_dis_allowed(bms)) {
                bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_DIS, false);
                bms->state = BMS_STATE_CHG;
                LOG_INF("NORMAL -> CHG (error flags: 0x%08x)", bms->ic_data.error_flags);
            }
            else if (!bms_chg_allowed(bms)) {
                bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_CHG, false);
                bms->state = BMS_STATE_DIS;
                LOG_INF("NORMAL -> DIS (error flags: 0x%08x)", bms->ic_data.error_flags);
            }
            break;
        case BMS_STATE_SHUTDOWN:
            /* do nothing and wait until shutdown is completed */
            break;
    }
}

void bms_shutdown(struct bms_context *bms)
{
    bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_DIS, false);
    bms_ic_set_switches(bms->ic_dev, BMS_SWITCH_CHG, false);
    bms->state = BMS_STATE_SHUTDOWN;
}

bool bms_chg_error(uint32_t error_flags)
{
    return error_flags
           & (BMS_ERR_CELL_OVERVOLTAGE | BMS_ERR_CHG_OVERCURRENT | BMS_ERR_OPEN_WIRE
              | BMS_ERR_CHG_UNDERTEMP | BMS_ERR_CHG_OVERTEMP | BMS_ERR_INT_OVERTEMP
              | BMS_ERR_CELL_FAILURE | BMS_ERR_CHG_OFF);
}

bool bms_dis_error(uint32_t error_flags)
{
    return error_flags
           & (BMS_ERR_CELL_UNDERVOLTAGE | BMS_ERR_SHORT_CIRCUIT | BMS_ERR_DIS_OVERCURRENT
              | BMS_ERR_OPEN_WIRE | BMS_ERR_DIS_UNDERTEMP | BMS_ERR_DIS_OVERTEMP
              | BMS_ERR_INT_OVERTEMP | BMS_ERR_CELL_FAILURE | BMS_ERR_DIS_OFF);
}

bool bms_chg_allowed(struct bms_context *bms)
{
    return !bms_chg_error(bms->ic_data.error_flags & ~BMS_ERR_CHG_OFF) && !bms->full
           && bms->chg_enable;
}

bool bms_dis_allowed(struct bms_context *bms)
{
    return !bms_dis_error(bms->ic_data.error_flags & ~BMS_ERR_DIS_OFF) && !bms->empty
           && bms->dis_enable;
}
