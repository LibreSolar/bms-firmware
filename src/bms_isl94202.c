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

#include "pcb.h"

#ifdef BMS_ISL94202

#include "bms.h"
#include "isl94202_registers.h"
#include "isl94202_interface.h"
#include "helper.h"

#include "config.h"

#include <stdlib.h>     // for abs() function
#include <stdio.h>
#include <time.h>
#include <string.h>

// Lookup-table for temperatures according to datasheet
static const float lut_temp_volt[] = {0.153, 0.295, 0.463, 0.710, 0.755};
static const float lut_temp_degc[] = {80, 50, 25, 0, -40};

static void set_num_cells(int num)
{
    uint8_t cell_reg = 0;
    switch(num) {
        case 3:
            cell_reg = 0b10000011;
            break;
        case 4:
            cell_reg = 0b11000011;
            break;
        case 5:
            cell_reg = 0b11000111;
            break;
        case 6:
            cell_reg = 0b11100111;
            break;
        case 7:
            cell_reg = 0b11101111;
            break;
        case 8:
            cell_reg = 0b11111111;
            break;
        default:
            break;
    }
    isl94202_write_bytes(ISL94202_MOD_CELL + 1, &cell_reg, 1);
    // ToDo: Double write for certain bytes... move this to _hw.c file
    //k_sleep(30);
    //isl94202_write_bytes(ISL94202_MOD_CELL + 1, &cell_reg, 1);
    //k_sleep(30);
}

void bms_init()
{
    isl94202_init();
    set_num_cells(4);

    uint16_t reg = 0;
    reg |= ISL94202_FC_XT2M_Msk;        // xTemp2 monitoring MOSFETs and not cells
    reg |= ISL94202_FC_CBDC_Msk;        // cell balancing during charging enabled
    isl94202_write_word(ISL94202_FC, reg);
}

void bms_update(BmsConfig *conf, BmsStatus *status)
{
    bms_read_voltages(status);
    bms_read_current(conf, status);
    bms_read_temperatures(conf, status);
    bms_update_error_flags(conf, status);
    bms_apply_balancing(conf, status);
    bms_update_soc(conf, status);
}

void bms_shutdown()
{
    /* ToDo */
}

bool bms_chg_switch(BmsConfig *conf, BmsStatus *status, bool enable)
{
    /* ToDo */
    return 0;
}

bool bms_dis_switch(BmsConfig *conf, BmsStatus *status, bool enable)
{
    /* ToDo */
    return 0;
}

void bms_apply_balancing(BmsConfig *conf, BmsStatus *status)
{
    /* ToDo */
}

float bms_apply_dis_scp(BmsConfig *conf)
{
    return isl94202_write_current_limit(ISL94202_SCDT_SCD,
        DSC_Thresholds, sizeof(DSC_Thresholds)/sizeof(uint16_t),
        conf->dis_sc_limit, conf->shunt_res_mOhm,
        ISL94202_DELAY_US, conf->dis_sc_delay_us);
}

float bms_apply_chg_ocp(BmsConfig *conf)
{
    return isl94202_write_current_limit(ISL94202_OCCT_OCC,
        OCC_Thresholds, sizeof(OCC_Thresholds)/sizeof(uint16_t),
        conf->chg_oc_limit, conf->shunt_res_mOhm,
        ISL94202_DELAY_MS, conf->chg_oc_delay_ms);
}

float bms_apply_dis_ocp(BmsConfig *conf)
{
    return isl94202_write_current_limit(ISL94202_OCDT_OCD,
        OCD_Thresholds, sizeof(OCD_Thresholds)/sizeof(uint16_t),
        conf->dis_oc_limit, conf->shunt_res_mOhm,
        ISL94202_DELAY_MS, conf->dis_oc_delay_ms);
}

int bms_apply_cell_ovp(BmsConfig *conf)
{
    // keeping CPW at the default value of 1 ms
    return isl94202_write_voltage(ISL94202_OVL_CPW, conf->cell_ov_limit, 1)
        && isl94202_write_voltage(ISL94202_OVR, conf->cell_ov_reset, 0)
        && isl94202_write_delay(ISL94202_OVDT, ISL94202_DELAY_MS, conf->cell_ov_delay_ms, 0);
}

int bms_apply_cell_uvp(BmsConfig *conf)
{
    // keeping LPW at the default value of 1 ms
    return isl94202_write_voltage(ISL94202_UVL_LPW, conf->cell_uv_limit, 1)
        && isl94202_write_voltage(ISL94202_UVR, conf->cell_uv_reset, 0)
        && isl94202_write_delay(ISL94202_UVDT, ISL94202_DELAY_MS, conf->cell_uv_delay_ms, 0);
}

// using default setting TGain = 0 (GAIN = 2) with 22k resistors
int bms_apply_temp_limits(BmsConfig *conf)
{
    float adc_voltage;

    // Charge over-temperature
    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt,
        sizeof(lut_temp_degc)/sizeof(float), conf->chg_ot_limit);
    isl94202_write_word(ISL94202_COTS, (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_COTS_Msk);

    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt,
        sizeof(lut_temp_degc)/sizeof(float), conf->chg_ot_limit - conf->t_limit_hyst);
    isl94202_write_word(ISL94202_COTR, (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_COTR_Msk);

    // Charge under-temperature
    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt,
        sizeof(lut_temp_degc)/sizeof(float), conf->chg_ut_limit);
    isl94202_write_word(ISL94202_CUTS, (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_CUTS_Msk);

    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt,
        sizeof(lut_temp_degc)/sizeof(float), conf->chg_ut_limit + conf->t_limit_hyst);
    isl94202_write_word(ISL94202_CUTR, (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_CUTR_Msk);

    // Discharge over-temperature
    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt,
        sizeof(lut_temp_degc)/sizeof(float), conf->dis_ot_limit);
    isl94202_write_word(ISL94202_DOTS, (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_DOTS_Msk);

    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt,
        sizeof(lut_temp_degc)/sizeof(float), conf->dis_ot_limit - conf->t_limit_hyst);
    isl94202_write_word(ISL94202_DOTR, (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_DOTR_Msk);

    // Discharge under-temperature
    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt,
        sizeof(lut_temp_degc)/sizeof(float), conf->dis_ut_limit);
    isl94202_write_word(ISL94202_DUTS, (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_DUTS_Msk);

    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt,
        sizeof(lut_temp_degc)/sizeof(float), conf->dis_ut_limit + conf->t_limit_hyst);
    isl94202_write_word(ISL94202_DUTR, (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_DUTR_Msk);

    return 1;
}

// using default setting TGain = 0 (GAIN = 2) with 22k resistors
void bms_read_temperatures(BmsConfig *conf, BmsStatus *status)
{
    uint16_t adc_raw;

    // Internal temperature
    adc_raw = isl94202_read_word(ISL94202_IT) & 0x0FFF;
    status->ic_temp = (float)adc_raw * 1.8 / 4095 * 1000 / 1.8527 - 273.15;

    // External temperature 1
    adc_raw = isl94202_read_word(ISL94202_XT1) & 0x0FFF;
    float adc_v = (float)adc_raw * 1.8 / 4095 / 2;

    status->bat_temp_avg = interpolate(lut_temp_volt, lut_temp_degc,
        sizeof(lut_temp_degc)/sizeof(float), adc_v);

    // only single battery temperature measurement
    status->bat_temp_min = status->bat_temp_avg;
    status->bat_temp_max = status->bat_temp_avg;

    // External temperature 2 (used for MOSFET temperature sensing)
    adc_raw = isl94202_read_word(ISL94202_XT2) & 0x0FFF;
    adc_v = (float)adc_raw * 1.8 / 4095 / 2;

    status->mosfet_temp = interpolate(lut_temp_volt, lut_temp_degc,
        sizeof(lut_temp_degc)/sizeof(float), adc_v);
}

void bms_read_current(BmsConfig *conf, BmsStatus *status)
{
    uint8_t buf[2];

    // gain
    isl94202_read_bytes(ISL94202_CG, buf, 1);
    uint8_t gain_reg = (buf[0] & ISL94202_CG_Msk) >> ISL94202_CG_Pos;
    int gain = gain_reg < 3 ? ISL94202_Current_Gain[gain_reg] : 500;

    // direction / sign
    int sign = 0;
    isl94202_read_bytes(ISL94202_STAT2, buf, 1);
    sign += (buf[0] & ISL94202_STAT2_CHING_Msk) >> ISL94202_STAT2_CHING_Pos;
    isl94202_read_bytes(ISL94202_STAT2, buf, 1);
    sign -= (buf[0] & ISL94202_STAT2_DCHING_Msk) >> ISL94202_STAT2_DCHING_Pos;

    // ADC value
    uint16_t adc_raw = isl94202_read_word(ISL94202_ISNS) & 0x0FFF;

    status->pack_current = (float)(sign * adc_raw * 1800) / 4095 / gain / conf->shunt_res_mOhm;
}

void bms_read_voltages(BmsStatus *status)
{
    uint16_t adc_raw = 0;
    int conn_cells = 0;
    float sum_voltages = 0;

    for (int i = 0; i < NUM_CELLS_MAX; i++) {
        adc_raw = isl94202_read_word(ISL94202_CELL1 + i*2) & 0x0FFF;
        status->cell_voltages[i] = (float)adc_raw * 18 * 800 / 4095 / 3 / 1000;

        if (i == 0) {
            status->cell_voltage_max = status->cell_voltages[i];
            status->cell_voltage_min = status->cell_voltages[i];
        }

        if (status->cell_voltages[i] > 0.5F) {
            conn_cells++;
            sum_voltages += status->cell_voltages[i];
        }

        if (status->cell_voltages[i] > status->cell_voltage_max) {
            status->cell_voltage_max = status->cell_voltages[i];
        }
        if (status->cell_voltages[i] < status->cell_voltage_min && status->cell_voltages[i] > 0.5) {
            status->cell_voltage_min = status->cell_voltages[i];
        }
    }
    status->connected_cells = conn_cells;
    status->cell_voltage_avg = sum_voltages / conn_cells;

    adc_raw = isl94202_read_word(ISL94202_VBATT) & 0x0FFF;
    status->pack_voltage = (float)adc_raw * 1.8 * 32 / 4095;
}

void bms_update_error_flags(BmsConfig *conf, BmsStatus *status)
{
    uint8_t buf[2];
    isl94202_read_bytes(ISL94202_STAT1, buf, 2);
    uint16_t stat1 = buf[0] + (buf[1] << 8);

    status->error_flags = 0;
    if (stat1 & ISL94202_STAT1_UV_Msk)      status->error_flags |= 1U << BMS_ERR_CELL_UNDERVOLTAGE;
    if (stat1 & ISL94202_STAT1_OV_Msk)      status->error_flags |= 1U << BMS_ERR_CELL_OVERVOLTAGE;
    if (stat1 & ISL94202_STAT1_DSC_Msk)     status->error_flags |= 1U << BMS_ERR_SHORT_CIRCUIT;
    if (stat1 & ISL94202_STAT1_DOC_Msk)     status->error_flags |= 1U << BMS_ERR_DIS_OVERCURRENT;
    if (stat1 & ISL94202_STAT1_COC_Msk)     status->error_flags |= 1U << BMS_ERR_CHG_OVERCURRENT;
    if (stat1 & ISL94202_STAT1_OPEN_Msk)    status->error_flags |= 1U << BMS_ERR_OPEN_WIRE;
    if (stat1 & ISL94202_STAT1_DUT_Msk)     status->error_flags |= 1U << BMS_ERR_DIS_UNDERTEMP;
    if (stat1 & ISL94202_STAT1_DOT_Msk)     status->error_flags |= 1U << BMS_ERR_DIS_OVERTEMP;
    if (stat1 & ISL94202_STAT1_CUT_Msk)     status->error_flags |= 1U << BMS_ERR_CHG_UNDERTEMP;
    if (stat1 & ISL94202_STAT1_COT_Msk)     status->error_flags |= 1U << BMS_ERR_CHG_OVERTEMP;
    if (stat1 & ISL94202_STAT1_IOT_Msk)     status->error_flags |= 1U << BMS_ERR_INT_OVERTEMP;
    if (stat1 & ISL94202_STAT1_CELLF_Msk)   status->error_flags |= 1U << BMS_ERR_CELL_FAILURE;
}

void bms_handle_errors(BmsConfig *conf, BmsStatus *status)
{
    // Nothing to do. ISL94202 handles errors automatically
}

#if BMS_DEBUG

static const char *byte2bitstr(uint8_t b)
{
    static char str[9];
    str[0] = '\0';
    for (int z = 128; z > 0; z >>= 1) {
        strcat(str, ((b & z) == z) ? "1" : "0");
    }
    return str;
}

void bms_print_registers()
{
    printf("EEPROM content: ------------------\n");
    for (int i = 0; i < 0x4C; i += 2) {
        uint8_t buf[2];
        isl94202_read_bytes(i, buf, sizeof(buf));
        printf("0x%.2X: 0x%.2X%.2X = ", i, buf[1], buf[0]);
        printf("%s ", byte2bitstr(buf[1]));
        printf("%s\n", byte2bitstr(buf[0]));

    }
    printf("RAM content: ------------------\n");
    for (int i = 0x80; i <= 0xAA; i += 2) {
        uint8_t buf[2];
        isl94202_read_bytes(i, buf, sizeof(buf));
        printf("0x%.2X: 0x%.2X%.2X = ", i, buf[1], buf[0]);
        printf("%s ", byte2bitstr(buf[1]));
        printf("%s\n", byte2bitstr(buf[0]));

    }
}

#endif

#endif // BMS_ISL94202