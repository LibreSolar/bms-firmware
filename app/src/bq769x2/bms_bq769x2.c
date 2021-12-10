/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "board.h"

#include "bms.h"
#include "registers.h"
#include "interface.h"
#include "helper.h"

#include <math.h>       // log for thermistor calculation
#include <stdlib.h>     // for abs() function
#include <stdio.h>
#include <time.h>
#include <string.h>

void bms_init_hardware()
{
    bq769x2_init();
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

bool bms_chg_switch(BmsConfig *conf, BmsStatus *status, bool enable)
{
    // TODO

    return 0;
}

bool bms_dis_switch(BmsConfig *conf, BmsStatus *status, bool enable)
{
    // TODO

    return 0;
}

void bms_apply_balancing(BmsConfig *conf, BmsStatus *status)
{
    // TODO
}

float bms_apply_dis_scp(BmsConfig *conf)
{
    // TODO

    return 0;
}

float bms_apply_chg_ocp(BmsConfig *conf)
{
    // TODO

    return 0;
}

float bms_apply_dis_ocp(BmsConfig *conf)
{
    // TODO

    return 0;
}

int bms_apply_cell_uvp(BmsConfig *conf)
{
    // TODO

    return 0;
}

int bms_apply_cell_ovp(BmsConfig *conf)
{
    // TODO

    return 0;
}

int bms_apply_temp_limits(BmsConfig *bms)
{
    // TODO

    return 0;
}

void bms_read_temperatures(BmsConfig *conf, BmsStatus *status)
{
    int16_t temp = 0;   // unit: 0.1 K

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
        bq769x2_direct_read_i2(BQ769X2_CMD_VOLTAGE_CELL_1 + i*2, &voltage);
        status->cell_voltages[i] = voltage * 1e-3F;     // unit: 1 mV

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
    status->pack_voltage = voltage * 1e-2F;      // unit: 10 mV
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
    //error_flags |= ? << BMS_ERR_OPEN_WIRE;
    error_flags |= stat_b.UTD << BMS_ERR_DIS_UNDERTEMP;
    error_flags |= stat_b.OTD << BMS_ERR_DIS_OVERTEMP;
    error_flags |= stat_b.UTC << BMS_ERR_CHG_UNDERTEMP;
    error_flags |= stat_b.OTC << BMS_ERR_CHG_OVERTEMP;
    error_flags |= stat_b.OTINT << BMS_ERR_INT_OVERTEMP;
    error_flags |= stat_b.OTF << BMS_ERR_FET_OVERTEMP;
    //error_flags |= 1U << BMS_ERR_CELL_FAILURE;

    if (!fet_status.DSG_FET &&
        (status->state == BMS_STATE_DIS || status->state == BMS_STATE_NORMAL))
    {
        error_flags |= 1U << BMS_ERR_DIS_OFF;
    }

    if (!fet_status.CHG_FET &&
        (status->state == BMS_STATE_CHG || status->state == BMS_STATE_NORMAL))
    {
        error_flags |= 1U << BMS_ERR_CHG_OFF;
    }

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
    // TODO
}
