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

void bms_init_hardware()
{
    bq769x2_init();

    uint16_t device_number;
    bq769x2_subcmd_read_u2(BQ769X2_SUBCMD_DEVICE_NUMBER, &device_number);
    LOG_INF("detected bq device number: 0x%x", device_number);

    // disable automatic turn-on of all MOSFETs
    bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_ALL_FETS_OFF);

    // setting FET_EN is required to exit the default FET test mode and enable normal FET control
    MFG_STATUS_Type mfg_status;
    bq769x2_subcmd_read_u2(BQ769X2_SUBCMD_MFG_STATUS, &mfg_status.u16);
    if (mfg_status.FET_EN == 0) {
        // FET_ENABLE subcommand sets FET_EN bit in MFG_STATUS and MFG_STATUS_INIT registers
        bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_FET_ENABLE);
    }

    // disable sleep mode to avoid switching off the CHG MOSFET automatically
    bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_SLEEP_DISABLE);
}

void bms_update(BmsConfig *conf, BmsStatus *status)
{
    bms_read_voltages(status);
    bms_read_current(conf, status);
    bms_soc_update(conf, status);
    bms_read_temperatures(conf, status);
    bms_update_error_flags(conf, status);
    bms_apply_balancing(conf, status);
}

void bms_state_machine(BmsConfig *conf, BmsStatus *status)
{
    bms_handle_errors(conf, status);

    if (status->state == BMS_STATE_OFF) {
        if (bms_startup_inhibit()) {
            return;
        }

        if (bms_dis_allowed(status) || bms_chg_allowed(status)) {
            // the bq chip will automatically take care of the conditions and only enable the
            // FETs in the allowed directions
            bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_ALL_FETS_ON);
        }
    }

    // FET switching is autonomously handled by the bq chip, so we just read back the status

    FET_STATUS_Type fet_status;
    bq769x2_read_bytes(BQ769X2_CMD_FET_STATUS, &fet_status.byte, 1);

    if (fet_status.CHG_FET && fet_status.DSG_FET) {
        status->state = BMS_STATE_NORMAL;
    }
    else if (fet_status.CHG_FET) {
        status->state = BMS_STATE_CHG;
    }
    else if (fet_status.DSG_FET) {
        status->state = BMS_STATE_DIS;
    }
    else {
        status->state = BMS_STATE_OFF;
    }
}

void bms_set_error_flag(BmsStatus *status, uint32_t flag, bool value)
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
    // TODO
}

int bms_chg_switch(BmsConfig *conf, BmsStatus *status, bool enable)
{
    // handled by bq769x2 in autonomous mode, manual disable of a FET currently not supported

    return 0;
}

int bms_dis_switch(BmsConfig *conf, BmsStatus *status, bool enable)
{
    // handled by bq769x2 in autonomous mode, manual disable of a FET currently not supported

    return 0;
}

void bms_apply_balancing(BmsConfig *conf, BmsStatus *status)
{
    // TODO
}

int bms_apply_dis_scp(BmsConfig *conf)
{
    // TODO

    return -1;
}

int bms_apply_chg_ocp(BmsConfig *conf)
{
    // TODO

    return -1;
}

int bms_apply_dis_ocp(BmsConfig *conf)
{
    // TODO

    return -1;
}

int bms_apply_cell_uvp(BmsConfig *conf)
{
    int err = 0;

    uint8_t cuv_threshold = lroundf(conf->cell_uv_limit * 1000.0F / 50.6F);
    uint8_t cuv_hyst = lroundf(MAX(conf->cell_uv_reset - conf->cell_uv_limit, 0) * 1000.0F / 50.6F);
    uint16_t cuv_delay = lroundf(conf->cell_uv_delay_ms / 3.3F);

    cuv_threshold = CLAMP(cuv_threshold, 20, 90);
    cuv_hyst = CLAMP(cuv_hyst, 2, 20);
    cuv_delay = CLAMP(cuv_delay, 1, 2047);

    err += bq769x2_subcmd_write_u1(BQ769X2_PROT_CUV_THRESHOLD, cuv_threshold);
    err += bq769x2_subcmd_write_u1(BQ769X2_PROT_CUV_RECOV_HYST, cuv_hyst);
    err += bq769x2_subcmd_write_u2(BQ769X2_PROT_CUV_DELAY, cuv_delay);

    conf->cell_uv_limit = cuv_threshold * 50.6F / 1000.0F;
    conf->cell_uv_reset = (cuv_threshold + cuv_hyst) * 50.6F / 1000.0F;
    conf->cell_uv_delay_ms = cuv_delay * 3.3F;

    return err;
}

int bms_apply_cell_ovp(BmsConfig *conf)
{
    int err = 0;

    uint8_t cov_threshold = lroundf(conf->cell_ov_limit * 1000.0F / 50.6F);
    uint8_t cov_hyst = lroundf(MAX(conf->cell_ov_limit - conf->cell_ov_reset, 0) * 1000.0F / 50.6F);
    uint16_t cov_delay = lroundf(conf->cell_ov_delay_ms / 3.3F);

    cov_threshold = CLAMP(cov_threshold, 20, 110);
    cov_hyst = CLAMP(cov_hyst, 2, 20);
    cov_delay = CLAMP(cov_delay, 1, 2047);

    err += bq769x2_subcmd_write_u1(BQ769X2_PROT_COV_THRESHOLD, cov_threshold);
    err += bq769x2_subcmd_write_u1(BQ769X2_PROT_COV_RECOV_HYST, cov_hyst);
    err += bq769x2_subcmd_write_u2(BQ769X2_PROT_COV_DELAY, cov_delay);

    conf->cell_ov_limit = cov_threshold * 50.6F / 1000.0F;
    conf->cell_ov_reset = (cov_threshold - cov_hyst) * 50.6F / 1000.0F;
    conf->cell_ov_delay_ms = cov_delay * 3.3F;

    return err;
}

int bms_apply_temp_limits(BmsConfig *bms)
{
    // TODO

    return 0;
}

void bms_read_temperatures(BmsConfig *conf, BmsStatus *status)
{
    int16_t temp = 0; // unit: 0.1 K

    /* by default, only TS1 is configured as cell temperature sensor */
    bq769x2_direct_read_i2(BQ769X2_CMD_TEMP_TS1, &temp);
    status->bat_temp_avg = (temp * 0.1F) - 273.15F;
    status->bat_temp_min = status->bat_temp_avg;
    status->bat_temp_max = status->bat_temp_avg;

    bq769x2_direct_read_i2(BQ769X2_CMD_TEMP_INT, &temp);
    status->ic_temp = (temp * 0.1F) - 273.15F;
}

void bms_read_current(BmsConfig *conf, BmsStatus *status)
{
    int16_t current = 0;

    bq769x2_direct_read_i2(BQ769X2_CMD_CURRENT_CC2, &current);
    status->pack_current = current * 1e-3F;
}

void bms_read_voltages(BmsStatus *status)
{
    int16_t voltage = 0;
    int conn_cells = 0;
    float sum_voltages = 0;
    float v_max = 0, v_min = 10;

    for (int i = 0; i < BOARD_NUM_CELLS_MAX; i++) {
        bq769x2_direct_read_i2(BQ769X2_CMD_VOLTAGE_CELL_1 + i * 2, &voltage);
        status->cell_voltages[i] = voltage * 1e-3F; // unit: 1 mV

        if (status->cell_voltages[i] > 0.5F) {
            conn_cells++;
            sum_voltages += status->cell_voltages[i];
        }
        if (status->cell_voltages[i] > v_max) {
            v_max = status->cell_voltages[i];
        }
        if (status->cell_voltages[i] < v_min && status->cell_voltages[i] > 0.5F) {
            v_min = status->cell_voltages[i];
        }
    }
    status->connected_cells = conn_cells;
    status->cell_voltage_avg = sum_voltages / conn_cells;
    status->cell_voltage_min = v_min;
    status->cell_voltage_max = v_max;

    bq769x2_direct_read_i2(BQ769X2_CMD_VOLTAGE_STACK, &voltage);
    status->pack_voltage = voltage * 1e-2F; // unit: 10 mV
}

void bms_update_error_flags(BmsConfig *conf, BmsStatus *status)
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

    status->error_flags = error_flags;
}

void bms_handle_errors(BmsConfig *conf, BmsStatus *status)
{
    // TODO
}

void bms_print_register(uint8_t addr)
{
    uint8_t reg;
    bq769x2_read_bytes(addr, &reg, 1);
    printf("0x%.2X: 0x%.2X = %s\n", addr, reg, byte2bitstr(reg));
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
