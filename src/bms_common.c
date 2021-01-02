/* Libre Solar Battery Management System firmware
 * Copyright (c) 2016-2019 Martin Jäger (www.libre.solar)
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

#include "bms.h"
#include "pcb.h"
#include "helper.h"

#include <stdio.h>

float ocv_lfp[] = { // 100, 95, ..., 0 %
  3.392, 3.314, 3.309, 3.308, 3.304, 3.296, 3.283, 3.275, 3.271, 3.268, 3.265,
  3.264, 3.262, 3.252, 3.240, 3.226, 3.213, 3.190, 3.177, 3.132, 2.833
};

static float ocv_custom[] = { // 100, 95, ..., 0 %
  4.200, 4.140, 4.080, 4.020, 3.960, 3.900, 3.840, 3.780, 3.720, 3.660, 3.600,
  3.540, 3.480, 3.420, 3.360, 3.300, 3.240, 3.180, 3.120, 3.060, 3.000
}; //(xsider)


void bms_init_config(BmsConfig *conf, enum CellType type, float nominal_capacity)
{
    conf->auto_balancing_enabled = true;
    conf->bal_idle_delay = 1800;            // default: 30 minutes
    conf->bal_idle_current = 0.1F;          // A
    conf->bal_cell_voltage_diff = 0.01F;    // 10 mV

    conf->thermistor_beta = 3435;  // typical value for Semitec 103AT-5 thermistor

    conf->nominal_capacity_Ah = nominal_capacity;

    if (conf->nominal_capacity_Ah < PCB_MAX_CURRENT) {
        // 1C should be safe for all batteries
        conf->dis_oc_limit = conf->nominal_capacity_Ah;
        conf->chg_oc_limit = conf->nominal_capacity_Ah;
    }
    else {
        conf->dis_oc_limit = PCB_MAX_CURRENT;
        conf->chg_oc_limit = PCB_MAX_CURRENT;
    }
    conf->dis_oc_delay_ms  = 320;
    conf->chg_oc_delay_ms  = 320;

    conf->dis_sc_limit = conf->dis_oc_limit * 2;
    conf->dis_sc_delay_us  = 200;

    conf->dis_ut_limit = -20;
    conf->dis_ot_limit =  45;
    conf->chg_ut_limit = -10;
    conf->chg_ot_limit =  45;
    conf->t_limit_hyst =   2;

    conf->shunt_res_mOhm = SHUNT_RESISTOR;

    conf->cell_ov_delay_ms = 2000;
    conf->cell_uv_delay_ms = 2000;

    switch (type)
    {
        case CELL_TYPE_LFP:
            conf->cell_ov_limit         = 3.65F;
            conf->cell_chg_voltage      = 3.60F;
            conf->cell_ov_reset         = 3.45F;
            conf->bal_cell_voltage_min  = 3.20F;
            conf->cell_uv_reset         = 3.10F;
            conf->cell_dis_voltage      = 3.00F;
            conf->cell_uv_limit         = 2.80F;
            conf->ocv = ocv_lfp;
            conf->num_ocv_points = sizeof(ocv_lfp)/sizeof(float);
            break;
        case CELL_TYPE_NMC:
            conf->cell_ov_limit         = 4.25F;
            conf->cell_chg_voltage      = 4.20F;
            conf->cell_ov_reset         = 4.05F;
            conf->bal_cell_voltage_min  = 3.80F;
            conf->cell_uv_reset         = 3.50F;
            conf->cell_dis_voltage      = 3.20F;
            conf->cell_uv_limit         = 3.00F;
            break;
        case CELL_TYPE_NMC_HV:
            conf->cell_ov_limit         = 4.35F;
            conf->cell_chg_voltage      = 4.30F;
            conf->cell_ov_reset         = 4.15F;
            conf->bal_cell_voltage_min  = 3.80F;
            conf->cell_uv_reset         = 3.50F;
            conf->cell_dis_voltage      = 3.20F;
            conf->cell_uv_limit         = 3.00F;
            break;
        case CELL_TYPE_LTO:
            conf->cell_ov_limit         = 2.85F;
            conf->cell_chg_voltage      = 2.80F;
            conf->cell_ov_reset         = 2.70F;
            conf->bal_cell_voltage_min  = 2.50F;
            conf->cell_uv_reset         = 2.10F;
            conf->cell_dis_voltage      = 2.00F;
            conf->cell_uv_limit         = 1.90F;
            break;
        case CELL_TYPE_CUSTOM:
            conf->cell_ov_limit         = 4.00F;
            conf->cell_chg_voltage      = 3.90F;
            conf->cell_ov_reset         = 3.80F;
            conf->bal_cell_voltage_min  = 3.60F;
            conf->cell_uv_reset         = 3.30F;
            conf->cell_dis_voltage      = 3.10F;
            conf->cell_uv_limit         = 2.90F;
            conf->ocv = ocv_custom;
            conf->num_ocv_points = sizeof(ocv_custom)/sizeof(float);
            break;
        case CELL_TYPE_LiFeYPo:
            conf->cell_ov_limit         = 3.90F;
            conf->cell_chg_voltage      = 3.80F;
            conf->cell_ov_reset         = 3.70F;
            conf->bal_cell_voltage_min  = 3.20F;
            conf->cell_uv_reset         = 3.10F;
            conf->cell_dis_voltage      = 3.00F;
            conf->cell_uv_limit         = 2.90F;
             break;

    }
}

void bms_state_machine(BmsConfig *conf, BmsStatus *status)
{
    bms_handle_errors(conf, status);

    switch(status->state) {
        case BMS_STATE_OFF:
            if (!bms_dis_error(status) && !status->empty) {
                bms_dis_switch(conf, status, true);
                status->state = BMS_STATE_DIS;
                printf("Going to state DIS\n");
            }
            else if (!bms_chg_error(status) && !status->full) {
                bms_chg_switch(conf, status, true);
                status->state = BMS_STATE_CHG;
                printf("Going to state CHG\n");
            }
            break;
        case BMS_STATE_CHG:
            if (bms_chg_error(status) || status->full) {
                bms_chg_switch(conf, status, false);
                status->state = BMS_STATE_OFF;
                printf("Going back to state OFF\n");
            }
            else if (!bms_dis_error(status) && !status->empty) {
                bms_dis_switch(conf, status, true);
                status->state = BMS_STATE_NORMAL;
                printf("Going to state NORMAL\n");
            }
            break;
        case BMS_STATE_DIS:
            if (bms_dis_error(status) || status->empty) {
                bms_dis_switch(conf, status, false);
                status->state = BMS_STATE_OFF;
                printf("Going back to state OFF\n");
            }
            else if (!bms_chg_error(status) && !status->full) {
                bms_chg_switch(conf, status, true);
                status->state = BMS_STATE_NORMAL;
                printf("Going to state NORMAL\n");
            }
            break;
        case BMS_STATE_NORMAL:
            if (bms_dis_error(status) || status->empty) {
                bms_dis_switch(conf, status, false);
                status->state = BMS_STATE_CHG;
                printf("Going back to state CHG\n");
            }
            else if (bms_chg_error(status) || status->full) {
                bms_chg_switch(conf, status, false);
                status->state = BMS_STATE_DIS;
                printf("Going back to state DIS\n");
            }
            break;
    }
}

bool bms_chg_error(BmsStatus *status)
{
    return (status->error_flags & (1U << BMS_ERR_CELL_OVERVOLTAGE))
        || (status->error_flags & (1U << BMS_ERR_CHG_OVERCURRENT))
        || (status->error_flags & (1U << BMS_ERR_OPEN_WIRE))
        || (status->error_flags & (1U << BMS_ERR_CHG_UNDERTEMP))
        || (status->error_flags & (1U << BMS_ERR_CHG_OVERTEMP))
        || (status->error_flags & (1U << BMS_ERR_INT_OVERTEMP))
        || (status->error_flags & (1U << BMS_ERR_CELL_FAILURE));
}

bool bms_dis_error(BmsStatus *status)
{
    return (status->error_flags & (1U << BMS_ERR_CELL_UNDERVOLTAGE))
        || (status->error_flags & (1U << BMS_ERR_SHORT_CIRCUIT))
        || (status->error_flags & (1U << BMS_ERR_DIS_OVERCURRENT))
        || (status->error_flags & (1U << BMS_ERR_OPEN_WIRE))
        || (status->error_flags & (1U << BMS_ERR_DIS_UNDERTEMP))
        || (status->error_flags & (1U << BMS_ERR_DIS_OVERTEMP))
        || (status->error_flags & (1U << BMS_ERR_INT_OVERTEMP))
        || (status->error_flags & (1U << BMS_ERR_CELL_FAILURE));
}

bool bms_balancing_allowed(BmsConfig *conf, BmsStatus *status)
{
    int idle_sec = uptime() - status->no_idle_timestamp;
    float voltage_diff = status->cell_voltage_max -
        status->cell_voltage_min;

    return idle_sec >= conf->bal_idle_delay &&
        status->cell_voltage_max > conf->bal_cell_voltage_min &&
        voltage_diff > conf->bal_cell_voltage_diff;
}

void bms_reset_soc(BmsConfig *conf, BmsStatus *status, int percent)
{
    if (percent <= 100 && percent >= 0)
    {
        status->coulomb_counter_mAs = conf->nominal_capacity_Ah * 3.6e4F * percent;
    }
    else  // reset based on OCV
    {
        status->coulomb_counter_mAs = 0;  // initialize with totally depleted battery (0% SOC)

        for (unsigned int i = 0; i < conf->num_ocv_points; i++)
        {
            if (conf->ocv[i] <= status->cell_voltage_avg) {
                if (i == 0) {
                    status->coulomb_counter_mAs = conf->nominal_capacity_Ah * 3.6e6F;  // 100% full
                }
                else {
                    // interpolate between OCV[i] and OCV[i-1]
                    status->coulomb_counter_mAs = conf->nominal_capacity_Ah * 3.6e6F / (conf->num_ocv_points - 1.0) *
                    (conf->num_ocv_points - 1.0 - i + (status->cell_voltage_avg - conf->ocv[i])/(conf->ocv[i-1] - conf->ocv[i]));
                }
                return;
            }
        }
    }
}
