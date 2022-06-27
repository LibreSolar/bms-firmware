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

#include <math.h> // log for thermistor calculation
#include <stdio.h>
#include <stdlib.h> // for abs() function
#include <string.h>
#include <time.h>

LOG_MODULE_REGISTER(bq769x0, CONFIG_LOG_DEFAULT_LEVEL);

static int detect_num_cells(Bms *bms)
{
    bms_read_voltages(bms);

    uint16_t vcell_mode = 0;
    for (int i = 0; i < BOARD_NUM_CELLS_MAX; i++) {
        if (bms->status.cell_voltages[i] > 0.5F) {
            vcell_mode |= 1U << i;
        }
    }

    return bq769x2_subcmd_write_u2(BQ769X2_SET_CONF_VCELL_MODE, vcell_mode);
}

int bms_init_hardware(Bms *bms)
{
    int err = 0;

    err = bq769x2_init();
    if (err) {
        return err;
    }

    uint16_t device_number;
    err = bq769x2_subcmd_read_u2(BQ769X2_SUBCMD_DEVICE_NUMBER, &device_number);
    if (err) {
        return err;
    }
    else {
        LOG_INF("detected bq device number: 0x%x", device_number);
    }

    // configure shunt value based on nominal value of VREF2 (could be improved by calibration)
    err = bq769x2_subcmd_write_f4(BQ769X2_CAL_CURR_CC_GAIN, 7.4768F / bms->conf.shunt_res_mOhm);
    if (err) {
        return err;
    }

    // disable automatic turn-on of all MOSFETs
    err = bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_ALL_FETS_OFF);
    if (err) {
        return err;
    }

    // setting FET_EN is required to exit the default FET test mode and enable normal FET control
    MFG_STATUS_Type mfg_status;
    err = bq769x2_subcmd_read_u2(BQ769X2_SUBCMD_MFG_STATUS, &mfg_status.u16);
    if (err) {
        return err;
    }
    else if (mfg_status.FET_EN == 0) {
        // FET_ENABLE subcommand sets FET_EN bit in MFG_STATUS and MFG_STATUS_INIT registers
        err = bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_FET_ENABLE);
        if (err) {
            return err;
        }
    }

    // disable sleep mode to avoid switching off the CHG MOSFET automatically
    err = bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_SLEEP_DISABLE);
    if (err) {
        return err;
    }

    err = detect_num_cells(bms);
    if (err) {
        return err;
    }

    return 0;
}

void bms_update(Bms *bms)
{
    bms_read_voltages(bms);
    bms_read_current(bms);
    bms_soc_update(bms);
    bms_read_temperatures(bms);
    bms_update_error_flags(bms);
}

void bms_state_machine(Bms *bms)
{
    bms_handle_errors(bms);

    if (bms->status.state == BMS_STATE_OFF) {
        if (bms_startup_inhibit()) {
            return;
        }

        if (bms_dis_allowed(bms) || bms_chg_allowed(bms)) {
            // the bq chip will automatically take care of the conditions and only enable the
            // FETs in the allowed directions
            bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_ALL_FETS_ON);
        }
    }

    // FET switching is autonomously handled by the bq chip, so we just read back the status

    FET_STATUS_Type fet_status;
    bq769x2_read_bytes(BQ769X2_CMD_FET_STATUS, &fet_status.byte, 1);

    if (fet_status.CHG_FET && fet_status.DSG_FET) {
        bms->status.state = BMS_STATE_NORMAL;
    }
    else if (fet_status.CHG_FET) {
        bms->status.state = BMS_STATE_CHG;
    }
    else if (fet_status.DSG_FET) {
        bms->status.state = BMS_STATE_DIS;
    }
    else {
        bms->status.state = BMS_STATE_OFF;
    }
}

void bms_set_error_flag(Bms *bms, uint32_t flag, bool value)
{
    // TODO
}

bool bms_startup_inhibit()
{
    // Datasheet: Start-up time max. 4.3 ms
    return k_uptime_get() <= 5;
}

void bms_shutdown()
{
    bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_SHUTDOWN);
}

int bms_chg_switch(Bms *bms, bool enable)
{
    // handled by bq769x2 in autonomous mode, manual disable of a FET currently not supported

    return 0;
}

int bms_dis_switch(Bms *bms, bool enable)
{
    // handled by bq769x2 in autonomous mode, manual disable of a FET currently not supported

    return 0;
}

int bms_apply_balancing_conf(Bms *bms)
{
    // TODO

    return 0;
}

int bms_apply_dis_scp(Bms *bms)
{
    int err = 0;

    uint8_t scp_threshold = 0;
    uint16_t shunt_voltage = bms->conf.dis_sc_limit * bms->conf.shunt_res_mOhm;
    for (int i = ARRAY_SIZE(SCD_threshold_setting) - 1; i > 0; i--) {
        if (shunt_voltage >= SCD_threshold_setting[i]) {
            scp_threshold = i;
            break;
        }
    }

    uint16_t scp_delay = bms->conf.dis_sc_delay_us / 15.0F + 1;
    scp_delay = CLAMP(scp_delay, 1, 31);

    err += bq769x2_subcmd_write_u1(BQ769X2_PROT_SCD_THRESHOLD, scp_threshold);
    err += bq769x2_subcmd_write_u1(BQ769X2_PROT_SCD_DELAY, scp_delay);

    bms->conf.dis_sc_limit = SCD_threshold_setting[scp_threshold] / bms->conf.shunt_res_mOhm;
    bms->conf.dis_sc_delay_us = (scp_delay - 1) * 15;

    return err;
}

int bms_apply_chg_ocp(Bms *bms)
{
    int err = 0;

    uint8_t oc_threshold = lroundf(bms->conf.chg_oc_limit * bms->conf.shunt_res_mOhm / 2.0F);
    int16_t oc_delay = lroundf((bms->conf.chg_oc_delay_ms - 6.6F) / 3.3F);

    oc_threshold = CLAMP(oc_threshold, 2, 62);
    oc_delay = CLAMP(oc_delay, 1, 127);

    err += bq769x2_subcmd_write_u1(BQ769X2_PROT_OCC_THRESHOLD, oc_threshold);
    err += bq769x2_subcmd_write_u1(BQ769X2_PROT_OCC_DELAY, oc_delay);

    bms->conf.chg_oc_limit = oc_threshold * 2.0F / bms->conf.shunt_res_mOhm;
    bms->conf.chg_oc_delay_ms = lroundf(6.6F + oc_delay * 3.3F);

    // OCC protection needs to be enabled, as it is not active by default
    SAFETY_STATUS_A_Type prot_enabled_a;
    err += bq769x2_subcmd_read_u1(BQ769X2_SET_PROT_ENABLED_A, &prot_enabled_a.byte);
    if (!err) {
        prot_enabled_a.OCC = 1;
        err += bq769x2_subcmd_write_u1(BQ769X2_SET_PROT_ENABLED_A, prot_enabled_a.byte);
    }

    return err;
}

int bms_apply_dis_ocp(Bms *bms)
{
    int err = 0;

    uint8_t oc_threshold = lroundf(bms->conf.dis_oc_limit * bms->conf.shunt_res_mOhm / 2.0F);
    int16_t oc_delay = lroundf((bms->conf.dis_oc_delay_ms - 6.6F) / 3.3F);

    oc_threshold = CLAMP(oc_threshold, 2, 100);
    oc_delay = CLAMP(oc_delay, 1, 127);

    err += bq769x2_subcmd_write_u1(BQ769X2_PROT_OCD1_THRESHOLD, oc_threshold);
    err += bq769x2_subcmd_write_u1(BQ769X2_PROT_OCD1_DELAY, oc_delay);

    bms->conf.dis_oc_limit = oc_threshold * 2.0F / bms->conf.shunt_res_mOhm;
    bms->conf.dis_oc_delay_ms = lroundf(6.6F + oc_delay * 3.3F);

    // OCD protection needs to be enabled, as it is not active by default
    SAFETY_STATUS_A_Type prot_enabled_a;
    err += bq769x2_subcmd_read_u1(BQ769X2_SET_PROT_ENABLED_A, &prot_enabled_a.byte);
    if (!err) {
        prot_enabled_a.OCD1 = 1;
        err += bq769x2_subcmd_write_u1(BQ769X2_SET_PROT_ENABLED_A, prot_enabled_a.byte);
    }

    return err;
}

int bms_apply_cell_uvp(Bms *bms)
{
    int err = 0;

    uint8_t cuv_threshold = lroundf(bms->conf.cell_uv_limit * 1000.0F / 50.6F);
    uint8_t cuv_hyst =
        lroundf(MAX(bms->conf.cell_uv_reset - bms->conf.cell_uv_limit, 0) * 1000.0F / 50.6F);
    uint16_t cuv_delay = lroundf(bms->conf.cell_uv_delay_ms / 3.3F);

    cuv_threshold = CLAMP(cuv_threshold, 20, 90);
    cuv_hyst = CLAMP(cuv_hyst, 2, 20);
    cuv_delay = CLAMP(cuv_delay, 1, 2047);

    err += bq769x2_subcmd_write_u1(BQ769X2_PROT_CUV_THRESHOLD, cuv_threshold);
    err += bq769x2_subcmd_write_u1(BQ769X2_PROT_CUV_RECOV_HYST, cuv_hyst);
    err += bq769x2_subcmd_write_u2(BQ769X2_PROT_CUV_DELAY, cuv_delay);

    bms->conf.cell_uv_limit = cuv_threshold * 50.6F / 1000.0F;
    bms->conf.cell_uv_reset = (cuv_threshold + cuv_hyst) * 50.6F / 1000.0F;
    bms->conf.cell_uv_delay_ms = cuv_delay * 3.3F;

    // CUV protection needs to be enabled, as it is not active by default
    SAFETY_STATUS_A_Type prot_enabled_a;
    err += bq769x2_subcmd_read_u1(BQ769X2_SET_PROT_ENABLED_A, &prot_enabled_a.byte);
    if (!err) {
        prot_enabled_a.CUV = 1;
        err += bq769x2_subcmd_write_u1(BQ769X2_SET_PROT_ENABLED_A, prot_enabled_a.byte);
    }

    return err;
}

int bms_apply_cell_ovp(Bms *bms)
{
    int err = 0;

    uint8_t cov_threshold = lroundf(bms->conf.cell_ov_limit * 1000.0F / 50.6F);
    uint8_t cov_hyst =
        lroundf(MAX(bms->conf.cell_ov_limit - bms->conf.cell_ov_reset, 0) * 1000.0F / 50.6F);
    uint16_t cov_delay = lroundf(bms->conf.cell_ov_delay_ms / 3.3F);

    cov_threshold = CLAMP(cov_threshold, 20, 110);
    cov_hyst = CLAMP(cov_hyst, 2, 20);
    cov_delay = CLAMP(cov_delay, 1, 2047);

    err += bq769x2_subcmd_write_u1(BQ769X2_PROT_COV_THRESHOLD, cov_threshold);
    err += bq769x2_subcmd_write_u1(BQ769X2_PROT_COV_RECOV_HYST, cov_hyst);
    err += bq769x2_subcmd_write_u2(BQ769X2_PROT_COV_DELAY, cov_delay);

    bms->conf.cell_ov_limit = cov_threshold * 50.6F / 1000.0F;
    bms->conf.cell_ov_reset = (cov_threshold - cov_hyst) * 50.6F / 1000.0F;
    bms->conf.cell_ov_delay_ms = cov_delay * 3.3F;

    // COV protection is enabled by default in BQ769X2_SET_PROT_ENABLED_A register.

    return err;
}

int bms_apply_temp_limits(Bms *bms)
{
    int err = 0;
    uint8_t hyst = CLAMP(bms->conf.t_limit_hyst, 1, 20);

    if (bms->conf.dis_ot_limit < 0.0F || bms->conf.chg_ot_limit < 0.0F
        || bms->conf.dis_ot_limit < bms->conf.dis_ut_limit + 20.0F
        || bms->conf.chg_ot_limit < bms->conf.chg_ut_limit + 20.0F)
    {
        // values not plausible
        return -1;
    }

    int8_t otc_threshold = CLAMP(bms->conf.chg_ot_limit, -40, 120);
    int8_t otc_recovery = CLAMP(otc_threshold - hyst, -40, 120);
    int8_t otd_threshold = CLAMP(bms->conf.dis_ot_limit, -40, 120);
    int8_t otd_recovery = CLAMP(otd_threshold - hyst, -40, 120);

    int8_t utc_threshold = CLAMP(bms->conf.chg_ut_limit, -40, 120);
    int8_t utc_recovery = CLAMP(utc_threshold + hyst, -40, 120);
    int8_t utd_threshold = CLAMP(bms->conf.dis_ut_limit, -40, 120);
    int8_t utd_recovery = CLAMP(utd_threshold + hyst, -40, 120);

    err += bq769x2_subcmd_write_i1(BQ769X2_PROT_OTC_THRESHOLD, otc_threshold);
    err += bq769x2_subcmd_write_i1(BQ769X2_PROT_OTC_RECOVERY, otc_recovery);
    err += bq769x2_subcmd_write_i1(BQ769X2_PROT_OTD_THRESHOLD, otd_threshold);
    err += bq769x2_subcmd_write_i1(BQ769X2_PROT_OTD_RECOVERY, otd_recovery);

    err += bq769x2_subcmd_write_i1(BQ769X2_PROT_UTC_THRESHOLD, utc_threshold);
    err += bq769x2_subcmd_write_i1(BQ769X2_PROT_UTC_RECOVERY, utc_recovery);
    err += bq769x2_subcmd_write_i1(BQ769X2_PROT_UTD_THRESHOLD, utd_threshold);
    err += bq769x2_subcmd_write_i1(BQ769X2_PROT_UTD_RECOVERY, utd_recovery);

    bms->conf.chg_ot_limit = otc_threshold;
    bms->conf.dis_ot_limit = otd_threshold;
    bms->conf.chg_ut_limit = utc_threshold;
    bms->conf.dis_ut_limit = utd_threshold;
    bms->conf.t_limit_hyst = hyst;

    // temperature protection has to be enabled manually
    SAFETY_STATUS_B_Type prot_enabled_b;
    err += bq769x2_subcmd_read_u1(BQ769X2_SET_PROT_ENABLED_B, &prot_enabled_b.byte);
    if (!err) {
        prot_enabled_b.OTC = 1;
        prot_enabled_b.OTD = 1;
        prot_enabled_b.UTC = 1;
        prot_enabled_b.UTD = 1;
        err += bq769x2_subcmd_write_u1(BQ769X2_SET_PROT_ENABLED_B, prot_enabled_b.byte);
    }

    return err;
}

void bms_read_temperatures(Bms *bms)
{
    int16_t temp = 0; // unit: 0.1 K

    /* by default, only TS1 is configured as cell temperature sensor */
    bq769x2_direct_read_i2(BQ769X2_CMD_TEMP_TS1, &temp);
    bms->status.bat_temp_avg = (temp * 0.1F) - 273.15F;
    bms->status.bat_temp_min = bms->status.bat_temp_avg;
    bms->status.bat_temp_max = bms->status.bat_temp_avg;

    bq769x2_direct_read_i2(BQ769X2_CMD_TEMP_INT, &temp);
    bms->status.ic_temp = (temp * 0.1F) - 273.15F;
}

void bms_read_current(Bms *bms)
{
    int16_t current = 0;

    bq769x2_direct_read_i2(BQ769X2_CMD_CURRENT_CC2, &current);
    bms->status.pack_current = current * 1e-3F;
}

void bms_read_voltages(Bms *bms)
{
    int16_t voltage = 0;
    int conn_cells = 0;
    float sum_voltages = 0;
    float v_max = 0, v_min = 10;

    for (int i = 0; i < BOARD_NUM_CELLS_MAX; i++) {
        bq769x2_direct_read_i2(BQ769X2_CMD_VOLTAGE_CELL_1 + i * 2, &voltage);
        bms->status.cell_voltages[i] = voltage * 1e-3F; // unit: 1 mV

        if (bms->status.cell_voltages[i] > 0.5F) {
            conn_cells++;
            sum_voltages += bms->status.cell_voltages[i];
        }
        if (bms->status.cell_voltages[i] > v_max) {
            v_max = bms->status.cell_voltages[i];
        }
        if (bms->status.cell_voltages[i] < v_min && bms->status.cell_voltages[i] > 0.5F) {
            v_min = bms->status.cell_voltages[i];
        }
    }
    bms->status.connected_cells = conn_cells;
    bms->status.cell_voltage_avg = sum_voltages / conn_cells;
    bms->status.cell_voltage_min = v_min;
    bms->status.cell_voltage_max = v_max;

    bq769x2_direct_read_i2(BQ769X2_CMD_VOLTAGE_STACK, &voltage);
    bms->status.pack_voltage = voltage * 1e-2F; // unit: 10 mV
}

void bms_update_error_flags(Bms *bms)
{
    uint32_t error_flags = 0;
    SAFETY_STATUS_A_Type stat_a;
    SAFETY_STATUS_B_Type stat_b;
    SAFETY_STATUS_C_Type stat_c;
    FET_STATUS_Type fet_status;

    // safety alert: immediately set if a fault condition occured
    // safety fault (status registers): only set if alert persists for specified time

    bq769x2_read_bytes(BQ769X2_CMD_SAFETY_STATUS_A, &stat_a.byte, 1);
    bq769x2_read_bytes(BQ769X2_CMD_SAFETY_STATUS_B, &stat_b.byte, 1);
    bq769x2_read_bytes(BQ769X2_CMD_SAFETY_STATUS_C, &stat_c.byte, 1);
    bq769x2_read_bytes(BQ769X2_CMD_FET_STATUS, &fet_status.byte, 1);

    error_flags |= stat_a.CUV << BMS_ERR_CELL_UNDERVOLTAGE;
    error_flags |= stat_a.COV << BMS_ERR_CELL_OVERVOLTAGE;
    error_flags |= stat_a.SCD << BMS_ERR_SHORT_CIRCUIT;
    error_flags |= stat_a.OCD1 << BMS_ERR_DIS_OVERCURRENT;
    error_flags |= stat_a.OCC << BMS_ERR_CHG_OVERCURRENT;
    // error_flags |= ? << BMS_ERR_OPEN_WIRE;
    error_flags |= stat_b.UTD << BMS_ERR_DIS_UNDERTEMP;
    error_flags |= stat_b.OTD << BMS_ERR_DIS_OVERTEMP;
    error_flags |= stat_b.UTC << BMS_ERR_CHG_UNDERTEMP;
    error_flags |= stat_b.OTC << BMS_ERR_CHG_OVERTEMP;
    error_flags |= stat_b.OTINT << BMS_ERR_INT_OVERTEMP;
    error_flags |= stat_b.OTF << BMS_ERR_FET_OVERTEMP;
    // error_flags |= 1U << BMS_ERR_CELL_FAILURE;

    bms->status.error_flags = error_flags;
}

void bms_handle_errors(Bms *bms)
{
    // TODO
}

void bms_print_register(uint16_t addr)
{
    uint8_t value;
    if (addr < 0x7F) {
        bq769x2_read_bytes(addr, &value, 1);
    }
    else {
        bq769x2_subcmd_read_u1(addr, &value);
    }
    printf("0x%.2X: 0x%.2X = %s\n", addr, value, byte2bitstr(value));
}

void bms_print_registers()
{
    printf("Status: ------------------\n");
    bms_print_register(BQ769X2_CMD_SAFETY_STATUS_A);
    bms_print_register(BQ769X2_CMD_SAFETY_STATUS_B);
    bms_print_register(BQ769X2_CMD_SAFETY_STATUS_C);
    bms_print_register(BQ769X2_CMD_BATTERY_STATUS);
    bms_print_register(BQ769X2_CMD_BATTERY_STATUS + 1);
    bms_print_register(BQ769X2_CMD_ALARM_STATUS);
    bms_print_register(BQ769X2_CMD_ALARM_STATUS + 1);
    bms_print_register(BQ769X2_CMD_FET_STATUS);
}
