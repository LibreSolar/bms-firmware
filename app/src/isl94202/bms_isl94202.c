/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "board.h"

#include "bms.h"
#include "helper.h"
#include "interface.h"
#include "registers.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h> // for abs() function
#include <string.h>
#include <time.h>

LOG_MODULE_REGISTER(isl94202, CONFIG_LOG_DEFAULT_LEVEL);

// Lookup-table for temperatures according to datasheet
static const float lut_temp_volt[] = { 0.153, 0.295, 0.463, 0.710, 0.755 };
static const float lut_temp_degc[] = { 80, 50, 25, 0, -40 };

static void bms_update_balancing(Bms *bms);

static int set_num_cells(int num)
{
    uint8_t cell_reg = 0;
    switch (num) {
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

    // ToDo: Double write for certain bytes (see datasheet 7.1.10) --> move this to _hw.c file
    // k_sleep(30);
    // isl94202_write_bytes(ISL94202_MOD_CELL + 1, &cell_reg, 1);
    // k_sleep(30);

    return isl94202_write_bytes(ISL94202_MOD_CELL + 1, &cell_reg, 1);
}

int bms_init_hardware(Bms *bms)
{
    int err;
    uint8_t reg;

    err = isl94202_init();
    if (err) {
        return err;
    }

    err = set_num_cells(CONFIG_NUM_CELLS_IN_SERIES);
    if (err) {
        return err;
    }

    // xTemp2 monitoring MOSFETs and not cells
    reg = ISL94202_SETUP0_XT2M_Msk;
    err = isl94202_write_bytes(ISL94202_SETUP0, &reg, 1);
    if (err) {
        return err;
    }

    // Enable balancing during charging and EOC conditions
    reg = ISL94202_SETUP1_CBDC_Msk | ISL94202_SETUP1_CB_EOC_Msk;
    err = isl94202_write_bytes(ISL94202_SETUP1, &reg, 1);
    if (err) {
        return err;
    }

    // Enable FET control via microcontroller
    reg = ISL94202_CTRL2_UCFET_Msk;
    err = isl94202_write_bytes(ISL94202_CTRL2, &reg, 1);
    if (err) {
        return err;
    }

    // Remark: Ideal diode control via DFODOV and DFODUV bits of SETUP1 register doesn't have any
    // effect because of FET control via microcontroller.

    return 0;
}

void bms_update(Bms *bms)
{
    bms_read_voltages(bms);
    bms_read_current(bms);
    bms_soc_update(bms);
    bms_read_temperatures(bms);
    bms_update_error_flags(bms);
    bms_update_balancing(bms);
}

bool bms_startup_inhibit()
{
    // Datasheet: 3 seconds wake-up delay from shutdown or initial power-up
    return uptime() < 4;
}

void bms_shutdown(Bms *bms)
{
    uint8_t reg = ISL94202_CTRL3_PDWN_Msk;
    isl94202_write_bytes(ISL94202_CTRL3, &reg, 1);
}

int bms_chg_switch(Bms *bms, bool enable)
{
    uint8_t reg;
    isl94202_read_bytes(ISL94202_CTRL1, &reg, 1);
    if (enable) {
        reg |= ISL94202_CTRL1_CFET_Msk;
    }
    else {
        reg &= ~ISL94202_CTRL1_CFET_Msk;
    }
    return isl94202_write_bytes(ISL94202_CTRL1, &reg, 1);
}

int bms_dis_switch(Bms *bms, bool enable)
{
    uint8_t reg;
    isl94202_read_bytes(ISL94202_CTRL1, &reg, 1);
    if (enable) {
        reg |= ISL94202_CTRL1_DFET_Msk;
    }
    else {
        reg &= ~ISL94202_CTRL1_DFET_Msk;
    }
    return isl94202_write_bytes(ISL94202_CTRL1, &reg, 1);
}

static int bms_apply_balancing_conf(Bms *bms)
{
    int err = 0;

    // also apply balancing thresholds here
    err += isl94202_write_voltage(ISL94202_CBMIN, bms->conf.bal_cell_voltage_min, 0);
    err += isl94202_write_voltage(ISL94202_CBMAX, 4.5F, 0); // no upper limit for balancing
    err += isl94202_write_voltage(ISL94202_CBMINDV, bms->conf.bal_cell_voltage_diff, 0);
    err += isl94202_write_voltage(ISL94202_CBMAXDV, 1.0F, 0); // no tight limit for voltage delta

    // EOC condition needs to be set to bal_cell_voltage_min instead of cell_chg_voltage to enable
    // balancing during idle
    err += isl94202_write_voltage(ISL94202_EOC, bms->conf.bal_cell_voltage_min, 0);

    return err;
}

static void bms_update_balancing(Bms *bms)
{
    uint8_t stat3;
    isl94202_read_bytes(ISL94202_STAT3, &stat3, 1);

    /*
     * System scans for voltage, current and temperature measurements happen in different
     * intervals depending on the mode. Cell balancing should be off during voltage scans.
     *
     * Each scan takes max. 1.7 ms. Choosing 16 ms off-time for voltages to settle.
     */
    if (stat3 & ISL94202_STAT3_INIDLE_Msk) {
        // IDLE mode: Scan every 256 ms
        isl94202_write_delay(ISL94202_CBONT, ISL94202_DELAY_MS, 240, 0);
        isl94202_write_delay(ISL94202_CBOFFT, ISL94202_DELAY_MS, 16, 0);
    }
    else if (stat3 & ISL94202_STAT3_INDOZE_Msk) {
        // DOZE mode: Scan every 512 ms
        isl94202_write_delay(ISL94202_CBONT, ISL94202_DELAY_MS, 496, 0);
        isl94202_write_delay(ISL94202_CBOFFT, ISL94202_DELAY_MS, 16, 0);
    }
    else if (!(stat3 & ISL94202_STAT3_INSLEEP_Msk)) {
        // NORMAL mode: Scan every 32 ms
        isl94202_write_delay(ISL94202_CBONT, ISL94202_DELAY_MS, 16, 0);
        isl94202_write_delay(ISL94202_CBOFFT, ISL94202_DELAY_MS, 16, 0);
    }

    /*
     * Balancing is done automatically, just reading status here (even though the datasheet
     * tells that the CBFC register value cannot be used for indication if a cell is
     * balanced at the moment)
     */

    uint8_t reg = 0;
    isl94202_read_bytes(ISL94202_CBFC, &reg, 1);

    bms->status.balancing_status = reg;
}

static int bms_apply_dis_scp(Bms *bms)
{
    float actual_limit = isl94202_write_current_limit(
        ISL94202_SCDT_SCD, DSC_Thresholds, sizeof(DSC_Thresholds) / sizeof(uint16_t),
        bms->conf.dis_sc_limit, bms->conf.shunt_res_mOhm, ISL94202_DELAY_US,
        bms->conf.dis_sc_delay_us);

    if (actual_limit > 0) {
        bms->conf.dis_sc_limit = actual_limit;
        return 0;
    }
    else {
        return -1;
    }
}

static int bms_apply_chg_ocp(Bms *bms)
{
    float actual_limit = isl94202_write_current_limit(
        ISL94202_OCCT_OCC, OCC_Thresholds, sizeof(OCC_Thresholds) / sizeof(uint16_t),
        bms->conf.chg_oc_limit, bms->conf.shunt_res_mOhm, ISL94202_DELAY_MS,
        bms->conf.chg_oc_delay_ms);

    if (actual_limit > 0) {
        bms->conf.chg_oc_limit = actual_limit;
        return 0;
    }
    else {
        return -1;
    }
}

static int bms_apply_dis_ocp(Bms *bms)
{
    float actual_limit = isl94202_write_current_limit(
        ISL94202_OCDT_OCD, OCD_Thresholds, sizeof(OCD_Thresholds) / sizeof(uint16_t),
        bms->conf.dis_oc_limit, bms->conf.shunt_res_mOhm, ISL94202_DELAY_MS,
        bms->conf.dis_oc_delay_ms);

    if (actual_limit > 0) {
        bms->conf.dis_oc_limit = actual_limit;
        return 0;
    }
    else {
        return -1;
    }
}

static int bms_apply_cell_ovp(Bms *bms)
{
    int err = 0;

    // keeping CPW at the default value of 1 ms
    err += isl94202_write_voltage(ISL94202_OVL_CPW, bms->conf.cell_ov_limit, 1);
    err += isl94202_write_voltage(ISL94202_OVR, bms->conf.cell_ov_reset, 0);
    err += isl94202_write_delay(ISL94202_OVDT, ISL94202_DELAY_MS, bms->conf.cell_ov_delay_ms, 0);

    return err;
}

static int bms_apply_cell_uvp(Bms *bms)
{
    int err = 0;

    // keeping LPW at the default value of 1 ms
    err += isl94202_write_voltage(ISL94202_UVL_LPW, bms->conf.cell_uv_limit, 1);
    err += isl94202_write_voltage(ISL94202_UVR, bms->conf.cell_uv_reset, 0);
    err += isl94202_write_delay(ISL94202_UVDT, ISL94202_DELAY_MS, bms->conf.cell_uv_delay_ms, 0);

    return err;
}

// using default setting TGain = 0 (GAIN = 2) with 22k resistors
static int bms_apply_temp_limits(Bms *bms)
{
    float adc_voltage;

    // Charge over-temperature
    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc),
                              bms->conf.chg_ot_limit);
    isl94202_write_word(ISL94202_COTS,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_COTS_Msk);

    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc),
                              bms->conf.chg_ot_limit - bms->conf.t_limit_hyst);
    isl94202_write_word(ISL94202_COTR,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_COTR_Msk);

    // Charge under-temperature
    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc),
                              bms->conf.chg_ut_limit);
    isl94202_write_word(ISL94202_CUTS,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_CUTS_Msk);

    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc),
                              bms->conf.chg_ut_limit + bms->conf.t_limit_hyst);
    isl94202_write_word(ISL94202_CUTR,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_CUTR_Msk);

    // Discharge over-temperature
    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc),
                              bms->conf.dis_ot_limit);
    isl94202_write_word(ISL94202_DOTS,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_DOTS_Msk);

    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc),
                              bms->conf.dis_ot_limit - bms->conf.t_limit_hyst);
    isl94202_write_word(ISL94202_DOTR,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_DOTR_Msk);

    // Discharge under-temperature
    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc),
                              bms->conf.dis_ut_limit);
    isl94202_write_word(ISL94202_DUTS,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_DUTS_Msk);

    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc),
                              bms->conf.dis_ut_limit + bms->conf.t_limit_hyst);
    isl94202_write_word(ISL94202_DUTR,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_DUTR_Msk);

    return 0;
}

// using default setting TGain = 0 (GAIN = 2) with 22k resistors
void bms_read_temperatures(Bms *bms)
{
    uint16_t adc_raw;

    // Internal temperature
    isl94202_read_word(ISL94202_IT, &adc_raw);
    adc_raw &= 0x0FFF;
    bms->status.ic_temp = (float)adc_raw * 1.8 / 4095 * 1000 / 1.8527 - 273.15;

    // External temperature 1
    isl94202_read_word(ISL94202_XT1, &adc_raw);
    adc_raw &= 0x0FFF;
    float adc_v = (float)adc_raw * 1.8 / 4095 / 2;

    bms->status.bat_temp_avg =
        interpolate(lut_temp_volt, lut_temp_degc, ARRAY_SIZE(lut_temp_degc), adc_v);

    // only single battery temperature measurement
    bms->status.bat_temp_min = bms->status.bat_temp_avg;
    bms->status.bat_temp_max = bms->status.bat_temp_avg;

    // External temperature 2 (used for MOSFET temperature sensing)
    isl94202_read_word(ISL94202_XT2, &adc_raw);
    adc_raw &= 0x0FFF;
    adc_v = (float)adc_raw * 1.8 / 4095 / 2;

    bms->status.mosfet_temp =
        interpolate(lut_temp_volt, lut_temp_degc, ARRAY_SIZE(lut_temp_degc), adc_v);
}

void bms_read_current(Bms *bms)
{
    uint8_t buf[2];

    // gain
    isl94202_read_bytes(ISL94202_CTRL0, buf, 1);
    uint8_t gain_reg = (buf[0] & ISL94202_CTRL0_CG_Msk) >> ISL94202_CTRL0_CG_Pos;
    int gain = gain_reg < 3 ? ISL94202_Current_Gain[gain_reg] : 500;

    // direction / sign
    int sign = 0;
    isl94202_read_bytes(ISL94202_STAT2, buf, 1);
    sign += (buf[0] & ISL94202_STAT2_CHING_Msk) >> ISL94202_STAT2_CHING_Pos;
    sign -= (buf[0] & ISL94202_STAT2_DCHING_Msk) >> ISL94202_STAT2_DCHING_Pos;

    // ADC value
    uint16_t adc_raw;
    isl94202_read_word(ISL94202_ISNS, &adc_raw);
    adc_raw &= 0x0FFF;

    bms->status.pack_current =
        (float)(sign * adc_raw * 1800) / 4095 / gain / bms->conf.shunt_res_mOhm;
}

void bms_read_voltages(Bms *bms)
{
    uint16_t adc_raw = 0;
    int conn_cells = 0;
    float sum_voltages = 0;

    for (int i = 0; i < BOARD_NUM_CELLS_MAX; i++) {
        isl94202_read_word(ISL94202_CELL1 + i * 2, &adc_raw);
        adc_raw &= 0x0FFF;
        bms->status.cell_voltages[i] = (float)adc_raw * 18 * 800 / 4095 / 3 / 1000;

        if (i == 0) {
            bms->status.cell_voltage_max = bms->status.cell_voltages[i];
            bms->status.cell_voltage_min = bms->status.cell_voltages[i];
        }

        if (bms->status.cell_voltages[i] > 0.5F) {
            conn_cells++;
            sum_voltages += bms->status.cell_voltages[i];
        }

        if (bms->status.cell_voltages[i] > bms->status.cell_voltage_max) {
            bms->status.cell_voltage_max = bms->status.cell_voltages[i];
        }
        if (bms->status.cell_voltages[i] < bms->status.cell_voltage_min
            && bms->status.cell_voltages[i] > 0.5)
        {
            bms->status.cell_voltage_min = bms->status.cell_voltages[i];
        }
    }
    bms->status.connected_cells = conn_cells;
    bms->status.cell_voltage_avg = sum_voltages / conn_cells;

    // adc_raw = isl94202_read_word(ISL94202_VBATT) & 0x0FFF;
    // bms->status.pack_voltage = (float)adc_raw * 1.8 * 32 / 4095;

    // VBATT based pack voltage seems very inaccurate, so take sum of cell voltages instead
    bms->status.pack_voltage = sum_voltages;
}

void bms_update_error_flags(Bms *bms)
{
    uint32_t error_flags = 0;
    uint8_t stat[2];
    uint8_t ctrl1;

    isl94202_read_bytes(ISL94202_STAT0, stat, 2);
    isl94202_read_bytes(ISL94202_CTRL1, &ctrl1, 1);

    if (stat[0] & ISL94202_STAT0_UVF_Msk)
        error_flags |= 1U << BMS_ERR_CELL_UNDERVOLTAGE;
    if (stat[0] & ISL94202_STAT0_OVF_Msk)
        error_flags |= 1U << BMS_ERR_CELL_OVERVOLTAGE;
    if (stat[1] & ISL94202_STAT1_DSCF_Msk)
        error_flags |= 1U << BMS_ERR_SHORT_CIRCUIT;
    if (stat[1] & ISL94202_STAT1_DOCF_Msk)
        error_flags |= 1U << BMS_ERR_DIS_OVERCURRENT;
    if (stat[1] & ISL94202_STAT1_COCF_Msk)
        error_flags |= 1U << BMS_ERR_CHG_OVERCURRENT;
    if (stat[1] & ISL94202_STAT1_OPENF_Msk)
        error_flags |= 1U << BMS_ERR_OPEN_WIRE;
    if (stat[0] & ISL94202_STAT0_DUTF_Msk)
        error_flags |= 1U << BMS_ERR_DIS_UNDERTEMP;
    if (stat[0] & ISL94202_STAT0_DOTF_Msk)
        error_flags |= 1U << BMS_ERR_DIS_OVERTEMP;
    if (stat[0] & ISL94202_STAT0_CUTF_Msk)
        error_flags |= 1U << BMS_ERR_CHG_UNDERTEMP;
    if (stat[0] & ISL94202_STAT0_COTF_Msk)
        error_flags |= 1U << BMS_ERR_CHG_OVERTEMP;
    if (stat[1] & ISL94202_STAT1_IOTF_Msk)
        error_flags |= 1U << BMS_ERR_INT_OVERTEMP;
    if (stat[1] & ISL94202_STAT1_CELLF_Msk)
        error_flags |= 1U << BMS_ERR_CELL_FAILURE;

    if (!(ctrl1 & ISL94202_CTRL1_DFET_Msk)
        && (bms->status.state == BMS_STATE_DIS || bms->status.state == BMS_STATE_NORMAL))
    {
        error_flags |= 1U << BMS_ERR_DIS_OFF;
    }

    if (!(ctrl1 & ISL94202_CTRL1_CFET_Msk)
        && (bms->status.state == BMS_STATE_CHG || bms->status.state == BMS_STATE_NORMAL))
    {
        error_flags |= 1U << BMS_ERR_CHG_OFF;
    }

    bms->status.error_flags = error_flags;
}

void bms_handle_errors(Bms *bms)
{
    // Nothing to do. ISL94202 handles errors automatically
}

void bms_print_register(uint16_t addr)
{
    uint8_t reg;
    isl94202_read_bytes((uint8_t)addr, &reg, 1);
    LOG_INF("0x%.2X: 0x%.2X = %s", addr, reg, byte2bitstr(reg));
}

void bms_print_registers()
{
    LOG_INF("EEPROM content: ------------------");
    for (int i = 0; i < 0x4C; i++) {
        bms_print_register(i);
    }
    LOG_INF("RAM content: ------------------");
    for (int i = 0x80; i <= 0xAB; i++) {
        bms_print_register(i);
    }
}

int bms_configure(Bms *bms)
{
    int err = 0;

    err += bms_apply_cell_ovp(bms);
    err += bms_apply_cell_uvp(bms);

    err += bms_apply_dis_scp(bms);
    err += bms_apply_dis_ocp(bms);
    err += bms_apply_chg_ocp(bms);

    err += bms_apply_temp_limits(bms);
    err += bms_apply_balancing_conf(bms);

    return err;
}
