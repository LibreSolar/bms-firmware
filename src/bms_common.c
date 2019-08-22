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

#include "bms.h"
#include "pcb.h"

#include <stdio.h>

void bms_init_config(BmsConfig *conf, enum CellType type, float nominal_capacity)
{
    conf->auto_balancing_enabled = true;
    conf->balancing_min_idle_s = 1800;    // default: 30 minutes
    conf->balancing_voltage_diff_target = 0.01; // 10 mV
    conf->idle_current_threshold = 0.1; // A

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

    conf->dis_sc_limit  = conf->dis_oc_limit * 2;
    conf->dis_sc_delay_us  = 200;

    conf->dis_ut_limit = -20;
    conf->dis_ot_limit = 45;
    conf->chg_ut_limit = 0;
    conf->chg_ot_limit = 45;
    conf->t_limit_hyst = 2;

    conf->shunt_res_mOhm = SHUNT_RESISTOR;

    // TODO
    //conf->ocv = OCV;
    //conf->num_ocv_points = sizeof(OCV)/sizeof(int);

    conf->cell_ov_delay_ms = 2000;
    conf->cell_uv_delay_ms = 2000;

    switch (type)
    {
        case CELL_TYPE_LFP:
            conf->cell_chg_voltage = 3.60;
            conf->balancing_cell_voltage_min = 3.2;
            conf->cell_ov_limit = 3.65;
            conf->cell_ov_limit_hyst = 0.2;
            conf->cell_uv_limit = 2.8;
            conf->cell_uv_limit_hyst = 0.3;
            break;
        case CELL_TYPE_NMC:
            conf->cell_chg_voltage = 4.20;
            conf->balancing_cell_voltage_min = 3.8;
            conf->cell_ov_limit = 4.25;
            conf->cell_ov_limit_hyst = 0.2;
            conf->cell_uv_limit = 3.2;
            conf->cell_uv_limit_hyst = 0.3;
            break;
        case CELL_TYPE_NMC_HV:
            conf->cell_chg_voltage = 4.30;
            conf->balancing_cell_voltage_min = 3.8;
            conf->cell_ov_limit = 4.35;
            conf->cell_ov_limit_hyst = 0.2;
            conf->cell_uv_limit = 3.0;
            conf->cell_uv_limit_hyst = 0.3;
            break;
        case CELL_TYPE_LTO:
            conf->cell_chg_voltage = 2.80;
            conf->balancing_cell_voltage_min = 2.5;
            conf->cell_ov_limit = 2.85;
            conf->cell_ov_limit_hyst = 0.15;
            conf->cell_uv_limit = 1.9;
            conf->cell_uv_limit_hyst = 0.2;
            break;
        case CELL_TYPE_CUSTOM:
            break;
    }
}

void bms_state_machine(BmsConfig *conf, BmsStatus *status)
{
    if (status->error_flags > 0) {
        status->state = BMS_STATE_ERROR;
        printf("Going to state ERROR\n");
    }

    switch(status->state) {
        case BMS_STATE_INIT:
            /* TODO checks */
            status->state = BMS_STATE_IDLE;
            printf("Going to state IDLE\n");
            break;
        case BMS_STATE_IDLE:
            if (bms_dis_allowed(conf, status)) {
                bms_dis_switch(conf, status, true);
                status->state = BMS_STATE_DIS;
                printf("Going to state DIS\n");
            }
            else if (bms_chg_allowed(conf, status)) {
                bms_chg_switch(conf, status, true);
                status->state = BMS_STATE_CHG;
                printf("Going to state CHG\n");
            }
            break;
        case BMS_STATE_CHG:
            if (!bms_chg_allowed(conf, status)) {
                bms_chg_switch(conf, status, false);
                status->state = BMS_STATE_IDLE;
                printf("Going back to state IDLE\n");
            }
            else if (bms_dis_allowed(conf, status)) {
                bms_dis_switch(conf, status, true);
                status->state = BMS_STATE_NORMAL;
                printf("Going to state NORMAL\n");
            }
            break;
        case BMS_STATE_DIS:
            if (!bms_dis_allowed(conf, status)) {
                bms_dis_switch(conf, status, false);
                status->state = BMS_STATE_IDLE;
                printf("Going back to state IDLE\n");
            }
            else if (bms_chg_allowed(conf, status)) {
                bms_chg_switch(conf, status, true);
                status->state = BMS_STATE_NORMAL;
                printf("Going to state NORMAL\n");
            }
            break;
        case BMS_STATE_NORMAL:
            if (!bms_dis_allowed(conf, status)) {
                bms_dis_switch(conf, status, false);
                status->state = BMS_STATE_CHG;
                printf("Going back to state CHG\n");
            }
            else if (!bms_chg_allowed(conf, status)) {
                bms_chg_switch(conf, status, false);
                status->state = BMS_STATE_DIS;
                printf("Going back to state DIS\n");
            }
#ifndef ZEPHYR  // TODO: Zephyr crashes because of time(NULL) call
            else if (bms_balancing_allowed(conf, status)) {
                status->state = BMS_STATE_BALANCING;
                printf("Going to state BALANCING\n");
            }
#endif
            break;
        case BMS_STATE_BALANCING:
            if (!bms_balancing_allowed(conf, status)) {
                status->state = BMS_STATE_NORMAL;
            }
            break;
        case BMS_STATE_ERROR:
            if (status->error_flags == 0) {
                status->state = BMS_STATE_IDLE;
            }
            break;
    }
}

bool bms_chg_allowed(BmsConfig *conf, BmsStatus *status)
{
    int errors = 0;
    for (int thermistor = 0; thermistor < NUM_THERMISTORS_MAX; thermistor++) {
        errors += (status->temperatures[thermistor] > conf->chg_ot_limit ||
            status->temperatures[thermistor] < conf->chg_ut_limit);
    }
    errors += status->cell_voltages[status->id_cell_voltage_max] > conf->cell_ov_limit;
    return errors == 0;
}

bool bms_dis_allowed(BmsConfig *conf, BmsStatus *status)
{
    int errors = 0;
    for (int thermistor = 0; thermistor < NUM_THERMISTORS_MAX; thermistor++) {
        errors += (status->temperatures[thermistor] > conf->dis_ot_limit ||
            status->temperatures[thermistor] < conf->dis_ut_limit);
    }
    errors += status->cell_voltages[status->id_cell_voltage_min] < conf->cell_uv_limit;
    return errors == 0;
}

bool bms_balancing_allowed(BmsConfig *conf, BmsStatus *status)
{
    int idle_sec = time(NULL) - status->no_idle_timestamp;
    float voltage_diff = status->cell_voltages[status->id_cell_voltage_max] -
        status->cell_voltages[status->id_cell_voltage_min];

    return idle_sec >= conf->balancing_min_idle_s &&
        status->cell_voltages[status->id_cell_voltage_max] > conf->balancing_cell_voltage_min &&
        voltage_diff > conf->balancing_voltage_diff_target;
}

void bms_update_soc(BmsConfig *conf, BmsStatus *status)
{
    status->soc = status->coulomb_counter_mAs / (conf->nominal_capacity_Ah * 360); // %
}

void bms_reset_soc(BmsConfig *conf, BmsStatus *status, int percent)
{
    if (percent <= 100 && percent >= 0)
    {
        status->coulomb_counter_mAs = conf->nominal_capacity_Ah * 3.6e6F * (percent / 100.0);
    }
    else  // reset based on OCV
    {
        printf("NumCells: %d, voltage: %.2f V\n", status->connected_cells, status->pack_voltage);
        int voltage = status->pack_voltage / status->connected_cells;

        status->coulomb_counter_mAs = 0;  // initialize with totally depleted battery (0% SOC)

        for (unsigned int i = 0; i < conf->num_ocv_points; i++)
        {
            if (conf->ocv[i] <= voltage) {
                if (i == 0) {
                    status->coulomb_counter_mAs = conf->nominal_capacity_Ah * 3.6e6F;  // 100% full
                }
                else {
                    // interpolate between OCV[i] and OCV[i-1]
                    status->coulomb_counter_mAs = conf->nominal_capacity_Ah * 3.6e6F / (conf->num_ocv_points - 1.0) *
                    (conf->num_ocv_points - 1.0 - i + ((float)voltage - conf->ocv[i])/(conf->ocv[i-1] - conf->ocv[i]));
                }
                return;
            }
        }
    }
}
