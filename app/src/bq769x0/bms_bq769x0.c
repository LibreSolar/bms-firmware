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

extern int adc_gain;   // factory-calibrated, read out from chip (uV/LSB)
extern int adc_offset; // factory-calibrated, read out from chip (mV)

/**
 * Checks if temperatures are within the limits, otherwise disables CHG/DSG FET
 *
 * This function is necessary as bq769x0 doesn't support temperature protection
 */
void bms_check_cell_temp(Bms *bms);

int bms_init_hardware(Bms *bms)
{
    return bq769x0_init();
}

void bms_update(Bms *bms)
{
    bms_read_voltages(bms);
    bms_read_current(bms);
    bms_soc_update(bms);
    bms_read_temperatures(bms);
    bms_check_cell_temp(bms); // bq769x0 doesn't support temperature settings
    bms_update_error_flags(bms);
    bms_apply_balancing(bms);
}

void bms_set_error_flag(Bms *bms, uint32_t flag, bool value)
{
    // check if error flag changed
    if ((bms->status.error_flags & (1UL << flag)) != ((uint32_t)value << flag)) {
        if (value) {
            bms->status.error_flags |= (1UL << flag);
        }
        else {
            bms->status.error_flags &= ~(1UL << flag);
        }

        LOG_DBG("Error flag %u changed to: %d", flag, value);
    }
}

void bms_check_cell_temp(Bms *bms)
{
    float hyst;

    hyst = (bms->status.error_flags & (1UL << BMS_ERR_CHG_OVERTEMP)) ? bms->conf.t_limit_hyst : 0;
    bool chg_overtemp = bms->status.bat_temp_max > bms->conf.chg_ot_limit - hyst;

    hyst = (bms->status.error_flags & (1UL << BMS_ERR_CHG_UNDERTEMP)) ? bms->conf.t_limit_hyst : 0;
    bool chg_undertemp = bms->status.bat_temp_min < bms->conf.chg_ut_limit + hyst;

    hyst = (bms->status.error_flags & (1UL << BMS_ERR_DIS_OVERTEMP)) ? bms->conf.t_limit_hyst : 0;
    bool dis_overtemp = bms->status.bat_temp_max > bms->conf.dis_ot_limit - hyst;

    hyst = (bms->status.error_flags & (1UL << BMS_ERR_DIS_OVERTEMP)) ? bms->conf.t_limit_hyst : 0;
    bool dis_undertemp = bms->status.bat_temp_min < bms->conf.dis_ut_limit + hyst;

    if (chg_overtemp != (bool)(bms->status.error_flags & (1UL << BMS_ERR_CHG_OVERTEMP))) {
        bms_set_error_flag(bms, BMS_ERR_CHG_OVERTEMP, chg_overtemp);
    }

    if (chg_undertemp != (bool)(bms->status.error_flags & (1UL << BMS_ERR_CHG_UNDERTEMP))) {
        bms_set_error_flag(bms, BMS_ERR_CHG_UNDERTEMP, chg_undertemp);
    }

    if (dis_overtemp != (bool)(bms->status.error_flags & (1UL << BMS_ERR_DIS_OVERTEMP))) {
        bms_set_error_flag(bms, BMS_ERR_DIS_OVERTEMP, dis_overtemp);
    }

    if (dis_undertemp != (bool)(bms->status.error_flags & (1UL << BMS_ERR_DIS_UNDERTEMP))) {
        bms_set_error_flag(bms, BMS_ERR_DIS_UNDERTEMP, dis_undertemp);
    }
}

bool bms_startup_inhibit()
{
    // Datasheet: only 10 ms delay (t_BOOTREADY)
    return k_uptime_get() <= 10;
}

void bms_shutdown()
{
    // puts BMS IC into SHIP mode (i.e. switched off)
    bq769x0_write_byte(BQ769X0_SYS_CTRL1, 0x0);
    bq769x0_write_byte(BQ769X0_SYS_CTRL1, 0x1);
    bq769x0_write_byte(BQ769X0_SYS_CTRL1, 0x2);
}

int bms_chg_switch(Bms *bms, bool enable)
{
    SYS_CTRL2_Type sys_ctrl2;

    sys_ctrl2.byte = bq769x0_read_byte(BQ769X0_SYS_CTRL2);
    sys_ctrl2.CHG_ON = enable;
    bq769x0_write_byte(BQ769X0_SYS_CTRL2, sys_ctrl2.byte);

    return 0;
}

int bms_dis_switch(Bms *bms, bool enable)
{
    SYS_CTRL2_Type sys_ctrl2;

    sys_ctrl2.byte = bq769x0_read_byte(BQ769X0_SYS_CTRL2);
    sys_ctrl2.DSG_ON = enable;
    bq769x0_write_byte(BQ769X0_SYS_CTRL2, sys_ctrl2.byte);

    return 0;
}

void bms_apply_balancing(Bms *bms)
{
    long idle_secs = uptime() - bms->status.no_idle_timestamp;
    int num_sections = BOARD_NUM_CELLS_MAX / 5;

    // check for millisecond-timer overflow
    if (idle_secs < 0) {
        bms->status.no_idle_timestamp = 0;
        idle_secs = uptime();
    }

    // check if balancing allowed
    if (idle_secs >= bms->conf.bal_idle_delay
        && bms->status.cell_voltage_max > bms->conf.bal_cell_voltage_min
        && (bms->status.cell_voltage_max - bms->status.cell_voltage_min)
               > bms->conf.bal_cell_voltage_diff)
    {
        bms->status.balancing_status = 0; // current status will be set in following loop

        int balancing_flags;
        int balancing_flags_target;

        for (int section = 0; section < num_sections; section++) {
            // find cells which should be balanced and sort them by voltage descending
            int cell_list[5];
            int cell_counter = 0;
            for (int i = 0; i < 5; i++) {
                if ((bms->status.cell_voltages[section * 5 + i] - bms->status.cell_voltage_min)
                    > bms->conf.bal_cell_voltage_diff)
                {
                    int j = cell_counter;
                    while (j > 0
                           && bms->status.cell_voltages[section * 5 + cell_list[j - 1]]
                                  < bms->status.cell_voltages[section * 5 + i])
                    {
                        cell_list[j] = cell_list[j - 1];
                        j--;
                    }
                    cell_list[j] = i;
                    cell_counter++;
                }
            }

            balancing_flags = 0;
            for (int i = 0; i < cell_counter; i++) {
                // try to enable balancing of current cell
                balancing_flags_target = balancing_flags | (1 << cell_list[i]);

                // check if attempting to balance adjacent cells
                bool adjacent_cell_collision = ((balancing_flags_target << 1) & balancing_flags)
                                               || ((balancing_flags << 1) & balancing_flags_target);

                if (adjacent_cell_collision == false) {
                    balancing_flags = balancing_flags_target;
                }
            }

            LOG_DBG("Setting CELLBAL%d register to: %s", section + 1, byte2bitstr(balancing_flags));

            bms->status.balancing_status |= balancing_flags << section * 5;

            // set balancing register for this section
            bq769x0_write_byte(BQ769X0_CELLBAL1 + section, balancing_flags);

        } // section loop
    }
    else if (bms->status.balancing_status > 0) {
        // clear all CELLBAL registers
        for (int section = 0; section < num_sections; section++) {
            LOG_DBG("Clearing Register CELLBAL%d\n", section + 1);
            bq769x0_write_byte(BQ769X0_CELLBAL1 + section, 0x0);
        }

        bms->status.balancing_status = 0;
    }
}

int bms_apply_dis_scp(Bms *bms)
{
    PROTECT1_Type protect1;

    // only RSNS = 1 considered
    protect1.RSNS = 1;

    protect1.SCD_THRESH = 0;
    for (int i = ARRAY_SIZE(SCD_threshold_setting) - 1; i > 0; i--) {
        if (bms->conf.dis_sc_limit * bms->conf.shunt_res_mOhm >= SCD_threshold_setting[i]) {
            protect1.SCD_THRESH = i;
            break;
        }
    }

    protect1.SCD_DELAY = 0;
    for (int i = ARRAY_SIZE(SCD_delay_setting) - 1; i > 0; i--) {
        if (bms->conf.dis_sc_delay_us >= SCD_delay_setting[i]) {
            protect1.SCD_DELAY = i;
            break;
        }
    }

    bq769x0_write_byte(BQ769X0_PROTECT1, protect1.byte);

    // store actually configured values
    bms->conf.dis_sc_limit = SCD_threshold_setting[protect1.SCD_THRESH] / bms->conf.shunt_res_mOhm;
    bms->conf.dis_sc_delay_us = SCD_delay_setting[protect1.SCD_DELAY];

    return 0;
}

int bms_apply_chg_ocp(Bms *bms)
{
    // ToDo: Software protection for charge overcurrent

    return -1;
}

int bms_apply_dis_ocp(Bms *bms)
{
    PROTECT2_Type protect2;

    // Remark: RSNS must be set to 1 in PROTECT1 register

    protect2.OCD_THRESH = 0;
    for (int i = ARRAY_SIZE(OCD_threshold_setting) - 1; i > 0; i--) {
        if (bms->conf.dis_oc_limit * bms->conf.shunt_res_mOhm >= OCD_threshold_setting[i]) {
            protect2.OCD_THRESH = i;
            break;
        }
    }

    protect2.OCD_DELAY = 0;
    for (int i = ARRAY_SIZE(OCD_delay_setting) - 1; i > 0; i--) {
        if (bms->conf.dis_oc_delay_ms >= OCD_delay_setting[i]) {
            protect2.OCD_DELAY = i;
            break;
        }
    }

    bq769x0_write_byte(BQ769X0_PROTECT2, protect2.byte);

    // store actually configured values
    bms->conf.dis_oc_limit = OCD_threshold_setting[protect2.OCD_THRESH] / bms->conf.shunt_res_mOhm;
    bms->conf.dis_oc_delay_ms = OCD_delay_setting[protect2.OCD_DELAY];

    return 0;
}

int bms_apply_cell_uvp(Bms *bms)
{
    PROTECT3_Type protect3;
    int uv_trip = 0;

    protect3.byte = bq769x0_read_byte(BQ769X0_PROTECT3);

    uv_trip =
        ((((long)(bms->conf.cell_uv_limit * 1000) - adc_offset) * 1000 / adc_gain) >> 4) & 0x00FF;
    uv_trip += 1; // always round up for lower cell voltage
    bq769x0_write_byte(BQ769X0_UV_TRIP, uv_trip);

    protect3.UV_DELAY = 0;
    for (int i = ARRAY_SIZE(UV_delay_setting) - 1; i > 0; i--) {
        if (bms->conf.cell_uv_delay_ms >= UV_delay_setting[i]) {
            protect3.UV_DELAY = i;
            break;
        }
    }

    bq769x0_write_byte(BQ769X0_PROTECT3, protect3.byte);

    // store actually configured values
    bms->conf.cell_uv_limit = ((long)1 << 12 | uv_trip << 4) * adc_gain / 1000 + adc_offset;
    bms->conf.cell_uv_delay_ms = UV_delay_setting[protect3.UV_DELAY];

    return 0;
}

int bms_apply_cell_ovp(Bms *bms)
{
    PROTECT3_Type protect3;
    int ov_trip = 0;

    protect3.byte = bq769x0_read_byte(BQ769X0_PROTECT3);

    ov_trip =
        ((((long)(bms->conf.cell_ov_limit * 1000) - adc_offset) * 1000 / adc_gain) >> 4) & 0x00FF;
    bq769x0_write_byte(BQ769X0_OV_TRIP, ov_trip);

    protect3.OV_DELAY = 0;
    for (int i = ARRAY_SIZE(OV_delay_setting) - 1; i > 0; i--) {
        if (bms->conf.cell_ov_delay_ms >= OV_delay_setting[i]) {
            protect3.OV_DELAY = i;
            break;
        }
    }

    bq769x0_write_byte(BQ769X0_PROTECT3, protect3.byte);

    // store actually configured values

    bms->conf.cell_ov_limit = ((long)1 << 13 | ov_trip << 4) * adc_gain / 1000 + adc_offset;
    bms->conf.cell_ov_delay_ms = OV_delay_setting[protect3.OV_DELAY];

    return 0;
}

int bms_apply_temp_limits(Bms *bms)
{
    // bq769x0 don't support temperature limits --> has to be solved in software

    return 0;
}

void bms_read_temperatures(Bms *bms)
{
    float tmp = 0;
    int adc_raw = 0;
    int vtsx = 0;
    unsigned long rts = 0;

    // calculate R_thermistor according to bq769x0 datasheet
    adc_raw = (bq769x0_read_byte(BQ769X0_TS1_HI_BYTE) & 0b00111111) << 8
              | bq769x0_read_byte(BQ769X0_TS1_LO_BYTE);
    vtsx = adc_raw * 0.382;                 // mV
    rts = 10000.0 * vtsx / (3300.0 - vtsx); // Ohm

    // Temperature calculation using Beta equation
    // - According to bq769x0 datasheet, only 10k thermistors should be used
    // - 25°C reference temperature for Beta equation assumed
    tmp = 1.0 / (1.0 / (273.15 + 25) + 1.0 / bms->conf.thermistor_beta * log(rts / 10000.0)); // K
    bms->status.bat_temps[0] = tmp - 273.15;
    bms->status.bat_temp_min = bms->status.bat_temps[0];
    bms->status.bat_temp_max = bms->status.bat_temps[0];
    int num_temps = 1;
    float sum_temps = bms->status.bat_temps[0];

    if (BOARD_NUM_THERMISTORS_MAX >= 2) { // bq76930 or bq76940
        adc_raw = (bq769x0_read_byte(BQ769X0_TS2_HI_BYTE) & 0b00111111) << 8
                  | bq769x0_read_byte(BQ769X0_TS2_LO_BYTE);
        vtsx = adc_raw * 0.382;                 // mV
        rts = 10000.0 * vtsx / (3300.0 - vtsx); // Ohm
        tmp =
            1.0 / (1.0 / (273.15 + 25) + 1.0 / bms->conf.thermistor_beta * log(rts / 10000.0)); // K
        bms->status.bat_temps[1] = tmp - 273.15;

        if (bms->status.bat_temps[1] < bms->status.bat_temp_min) {
            bms->status.bat_temp_min = bms->status.bat_temps[1];
        }
        if (bms->status.bat_temps[1] > bms->status.bat_temp_max) {
            bms->status.bat_temp_max = bms->status.bat_temps[1];
        }
        num_temps++;
        sum_temps += bms->status.bat_temps[1];
    }

    if (BOARD_NUM_THERMISTORS_MAX == 3) { // bq76940
        adc_raw = (bq769x0_read_byte(BQ769X0_TS3_HI_BYTE) & 0b00111111) << 8
                  | bq769x0_read_byte(BQ769X0_TS3_LO_BYTE);
        vtsx = adc_raw * 0.382;                 // mV
        rts = 10000.0 * vtsx / (3300.0 - vtsx); // Ohm
        tmp =
            1.0 / (1.0 / (273.15 + 25) + 1.0 / bms->conf.thermistor_beta * log(rts / 10000.0)); // K
        bms->status.bat_temps[2] = tmp - 273.15;

        if (bms->status.bat_temps[2] < bms->status.bat_temp_min) {
            bms->status.bat_temp_min = bms->status.bat_temps[2];
        }
        if (bms->status.bat_temps[2] > bms->status.bat_temp_max) {
            bms->status.bat_temp_max = bms->status.bat_temps[2];
        }
        num_temps++;
        sum_temps += bms->status.bat_temps[2];
    }
    bms->status.bat_temp_avg = sum_temps / num_temps;
}

void bms_read_current(Bms *bms)
{
    SYS_STAT_Type sys_stat;
    sys_stat.byte = bq769x0_read_byte(BQ769X0_SYS_STAT);

    // check if new current reading available
    if (sys_stat.CC_READY == 1) {
        int adc_raw = bq769x0_read_word(BQ769X0_CC_HI_BYTE);
        if (adc_raw < 0) {
            LOG_ERR("Error reading current measurement");
            return;
        }

        int32_t pack_current_mA = (int16_t)adc_raw * 8.44 / bms->conf.shunt_res_mOhm;

        // remove noise around 0 A
        if (pack_current_mA > -10 && pack_current_mA < 10) {
            pack_current_mA = 0;
        }

        bms->status.pack_current = pack_current_mA / 1000.0;

        // reset no_idle_timestamp
        if (fabs(bms->status.pack_current) > bms->conf.bal_idle_current) {
            bms->status.no_idle_timestamp = uptime();
        }

        // no error occured which caused alert
        if (!(sys_stat.byte & BQ769X0_SYS_STAT_ERROR_MASK)) {
            bq769x0_alert_flag_reset();
        }

        bq769x0_write_byte(BQ769X0_SYS_STAT, BQ769X0_SYS_STAT_CC_READY); // clear CC ready flag
    }
}

void bms_read_voltages(Bms *bms)
{
    int adc_raw = 0;
    int conn_cells = 0;
    float sum_voltages = 0;
    float v_max = 0, v_min = 10;

    for (int i = 0; i < BOARD_NUM_CELLS_MAX; i++) {
        adc_raw = bq769x0_read_word(BQ769X0_VC1_HI_BYTE + i * 2) & 0x3FFF;
        bms->status.cell_voltages[i] = (adc_raw * adc_gain * 1e-3F + adc_offset) * 1e-3F;

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

    // read battery pack voltage
    adc_raw = bq769x0_read_word(BQ769X0_BAT_HI_BYTE);
    bms->status.pack_voltage =
        (4.0F * adc_gain * adc_raw * 1e-3F + bms->status.connected_cells * adc_offset) * 1e-3F;
}

void bms_update_error_flags(Bms *bms)
{
    SYS_STAT_Type sys_stat;
    sys_stat.byte = bq769x0_read_byte(BQ769X0_SYS_STAT);

    uint32_t error_flags_temp = 0;
    if (sys_stat.UV)
        error_flags_temp |= 1U << BMS_ERR_CELL_UNDERVOLTAGE;
    if (sys_stat.OV)
        error_flags_temp |= 1U << BMS_ERR_CELL_OVERVOLTAGE;
    if (sys_stat.SCD)
        error_flags_temp |= 1U << BMS_ERR_SHORT_CIRCUIT;
    if (sys_stat.OCD)
        error_flags_temp |= 1U << BMS_ERR_DIS_OVERCURRENT;

    if (bms->status.pack_current > bms->conf.chg_oc_limit) {
        // ToDo: consider bms->conf.chg_oc_delay
        error_flags_temp |= 1U << BMS_ERR_CHG_OVERCURRENT;
    }

    if (bms->status.bat_temp_max > bms->conf.chg_ot_limit
        || (bms->status.error_flags & (1UL << BMS_ERR_CHG_OVERTEMP)))
    {
        error_flags_temp |= 1U << BMS_ERR_CHG_OVERTEMP;
    }

    if (bms->status.bat_temp_min < bms->conf.chg_ut_limit
        || (bms->status.error_flags & (1UL << BMS_ERR_CHG_UNDERTEMP)))
    {
        error_flags_temp |= 1U << BMS_ERR_CHG_UNDERTEMP;
    }

    if (bms->status.bat_temp_max > bms->conf.dis_ot_limit
        || (bms->status.error_flags & (1UL << BMS_ERR_DIS_OVERTEMP)))
    {
        error_flags_temp |= 1U << BMS_ERR_DIS_OVERTEMP;
    }

    if (bms->status.bat_temp_min < bms->conf.dis_ut_limit
        || (bms->status.error_flags & (1UL << BMS_ERR_DIS_UNDERTEMP)))
    {
        error_flags_temp |= 1U << BMS_ERR_DIS_UNDERTEMP;
    }

    bms->status.error_flags = error_flags_temp;
}

void bms_handle_errors(Bms *bms)
{
    static uint16_t error_status = 0;
    static uint32_t sec_since_error = 0;

    // ToDo: Handle also temperature and chg errors (incl. temp hysteresis)
    SYS_STAT_Type sys_stat;
    sys_stat.byte = bq769x0_read_byte(BQ769X0_SYS_STAT);
    error_status = sys_stat.byte;

    if (!bq769x0_alert_flag() && error_status == 0) {
        return;
    }
    else if (sys_stat.byte & BQ769X0_SYS_STAT_ERROR_MASK) {

        if (bq769x0_alert_flag() == true) {
            sec_since_error = 0;
        }

        unsigned int sec_since_interrupt = uptime() - bq769x0_alert_timestamp();

        if (abs((long)(sec_since_interrupt - sec_since_error)) > 2) {
            sec_since_error = sec_since_interrupt;
        }

        // called only once per second
        if (sec_since_interrupt >= sec_since_error) {
            if (sys_stat.DEVICE_XREADY) {
                // datasheet recommendation: try to clear after waiting a few seconds
                if (sec_since_error % 3 == 0) {
                    LOG_DBG("Attempting to clear XR error");
                    bq769x0_write_byte(BQ769X0_SYS_STAT, BQ769X0_SYS_STAT_DEVICE_XREADY);
                    bms_chg_switch(bms, true);
                    bms_dis_switch(bms, true);
                }
            }
            if (sys_stat.OVRD_ALERT) {
                if (sec_since_error % 10 == 0) {
                    LOG_DBG("Attempting to clear Alert error");
                    bq769x0_write_byte(BQ769X0_SYS_STAT, BQ769X0_SYS_STAT_OVRD_ALERT);
                    bms_chg_switch(bms, true);
                    bms_dis_switch(bms, true);
                }
            }
            if (sys_stat.UV) {
                bms_read_voltages(bms);
                if (bms->status.cell_voltage_min > bms->conf.cell_uv_reset) {
                    LOG_DBG("Attempting to clear UV error");
                    bq769x0_write_byte(BQ769X0_SYS_STAT, BQ769X0_SYS_STAT_UV);
                    bms_dis_switch(bms, true);
                }
            }
            if (sys_stat.OV) {
                bms_read_voltages(bms);
                if (bms->status.cell_voltage_max < bms->conf.cell_ov_reset) {
                    LOG_DBG("Attempting to clear OV error");
                    bq769x0_write_byte(BQ769X0_SYS_STAT, BQ769X0_SYS_STAT_OV);
                    bms_chg_switch(bms, true);
                }
            }
            if (sys_stat.SCD) {
                if (sec_since_error % 60 == 0) {
                    LOG_DBG("Attempting to clear SCD error");
                    bq769x0_write_byte(BQ769X0_SYS_STAT, BQ769X0_SYS_STAT_SCD);
                    bms_dis_switch(bms, true);
                }
            }
            if (sys_stat.OCD) {
                if (sec_since_error % 60 == 0) {
                    LOG_DBG("Attempting to clear OCD error");
                    bq769x0_write_byte(BQ769X0_SYS_STAT, BQ769X0_SYS_STAT_OCD);
                    bms_dis_switch(bms, true);
                }
            }
            sec_since_error++;
        }
    }
    else {
        error_status = 0;
    }
}

void bms_print_register(uint16_t addr)
{
    uint8_t reg = bq769x0_read_byte((uint8_t)addr);
    printf("0x%.2X: 0x%.2X = %s\n", addr, reg, byte2bitstr(reg));
}

void bms_print_registers()
{
    printf("0x00 SYS_STAT:  %s\n", byte2bitstr(bq769x0_read_byte(BQ769X0_SYS_STAT)));
    printf("0x01 CELLBAL1:  %s\n", byte2bitstr(bq769x0_read_byte(BQ769X0_CELLBAL1)));
    printf("0x04 SYS_CTRL1: %s\n", byte2bitstr(bq769x0_read_byte(BQ769X0_SYS_CTRL1)));
    printf("0x05 SYS_CTRL2: %s\n", byte2bitstr(bq769x0_read_byte(BQ769X0_SYS_CTRL2)));
    printf("0x06 PROTECT1:  %s\n", byte2bitstr(bq769x0_read_byte(BQ769X0_PROTECT1)));
    printf("0x07 PROTECT2:  %s\n", byte2bitstr(bq769x0_read_byte(BQ769X0_PROTECT2)));
    printf("0x08 PROTECT3:  %s\n", byte2bitstr(bq769x0_read_byte(BQ769X0_PROTECT3)));
    printf("0x09 OV_TRIP:   %s\n", byte2bitstr(bq769x0_read_byte(BQ769X0_OV_TRIP)));
    printf("0x0A UV_TRIP:   %s\n", byte2bitstr(bq769x0_read_byte(BQ769X0_UV_TRIP)));
    printf("0x0B CC_CFG:    %s\n", byte2bitstr(bq769x0_read_byte(BQ769X0_CC_CFG)));
    printf("0x32 CC_HI:     %s\n", byte2bitstr(bq769x0_read_byte(BQ769X0_CC_HI_BYTE)));
    printf("0x33 CC_LO:     %s\n", byte2bitstr(bq769x0_read_byte(BQ769X0_CC_LO_BYTE)));
    /*
    printf("0x50 BQ769X0_ADCGAIN1:  %s\n", byte2bitstr(bq769x0_read_byte(BQ769X0_ADCGAIN1)));
    printf("0x51 BQ769X0_ADCOFFSET: %s\n", byte2bitstr(bq769x0_read_byte(BQ769X0_ADCOFFSET)));
    printf("0x59 BQ769X0_ADCGAIN2:  %s\n", byte2bitstr(bq769x0_read_byte(BQ769X0_ADCGAIN2)));
    */
}
