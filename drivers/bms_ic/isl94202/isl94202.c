/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_isl94202

#include <bms/bms_common.h>
#include <drivers/bms_ic.h>

#include "isl94202_interface.h"
#include "isl94202_priv.h"
#include "isl94202_registers.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> /* for abs() function */
#include <string.h>
#include <time.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>

#include "helper.h"

LOG_MODULE_REGISTER(bms_ic_isl94202, CONFIG_BMS_IC_LOG_LEVEL);

/* Lookup-table for temperatures according to datasheet */
static const float lut_temp_volt[] = { 0.153, 0.295, 0.463, 0.710, 0.755 };
static const float lut_temp_degc[] = { 80, 50, 25, 0, -40 };

static int set_num_cells(const struct device *dev, int num)
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
    // isl94202_write_bytes(dev, ISL94202_MOD_CELL + 1, &cell_reg, 1);
    // k_sleep(30);

    return isl94202_write_bytes(dev, ISL94202_MOD_CELL + 1, &cell_reg, 1);
}

static int isl94202_configure_cell_ovp(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    int err = 0;

    // keeping CPW at the default value of 1 ms
    err += isl94202_write_voltage(dev, ISL94202_OVL_CPW, ic_conf->cell_ov_limit, 1);
    err += isl94202_write_voltage(dev, ISL94202_OVR, ic_conf->cell_ov_reset, 0);
    err +=
        isl94202_write_delay(dev, ISL94202_OVDT, ISL94202_DELAY_MS, ic_conf->cell_ov_delay_ms, 0);

    return err;
}

static int isl94202_configure_cell_uvp(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    int err = 0;

    // keeping LPW at the default value of 1 ms
    err += isl94202_write_voltage(dev, ISL94202_UVL_LPW, ic_conf->cell_uv_limit, 1);
    err += isl94202_write_voltage(dev, ISL94202_UVR, ic_conf->cell_uv_reset, 0);
    err +=
        isl94202_write_delay(dev, ISL94202_UVDT, ISL94202_DELAY_MS, ic_conf->cell_uv_delay_ms, 0);

    return err;
}

static int isl94202_configure_chg_ocp(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    const struct bms_ic_isl94202_config *dev_config = dev->config;

    float actual_limit = isl94202_write_current_limit(
        dev, ISL94202_OCCT_OCC, isl94202_occ_thresholds, ARRAY_SIZE(isl94202_occ_thresholds),
        ic_conf->chg_oc_limit, dev_config->shunt_resistor_uohm / 1000.0F, ISL94202_DELAY_MS,
        ic_conf->chg_oc_delay_ms);

    if (actual_limit > 0) {
        ic_conf->chg_oc_limit = actual_limit;
        return 0;
    }
    else {
        return -1;
    }
}

static int isl94202_configure_dis_ocp(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    const struct bms_ic_isl94202_config *dev_config = dev->config;

    float actual_limit = isl94202_write_current_limit(
        dev, ISL94202_OCDT_OCD, isl94202_ocd_thresholds, ARRAY_SIZE(isl94202_ocd_thresholds),
        ic_conf->dis_oc_limit, dev_config->shunt_resistor_uohm / 1000.0F, ISL94202_DELAY_MS,
        ic_conf->dis_oc_delay_ms);

    if (actual_limit > 0) {
        ic_conf->dis_oc_limit = actual_limit;
        return 0;
    }
    else {
        return -1;
    }
}

static int isl94202_configure_dis_scp(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    const struct bms_ic_isl94202_config *dev_config = dev->config;

    float actual_limit = isl94202_write_current_limit(
        dev, ISL94202_SCDT_SCD, isl94202_dsc_thresholds, ARRAY_SIZE(isl94202_dsc_thresholds),
        ic_conf->dis_sc_limit, dev_config->shunt_resistor_uohm / 1000.0F, ISL94202_DELAY_US,
        ic_conf->dis_sc_delay_us);

    if (actual_limit > 0) {
        ic_conf->dis_sc_limit = actual_limit;
        return 0;
    }
    else {
        return -1;
    }
}

// using default setting TGain = 0 (GAIN = 2) with 22k resistors
static int isl94202_configure_temp_limits(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    float adc_voltage;

    // Charge over-temperature
    adc_voltage =
        interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc), ic_conf->chg_ot_limit);
    isl94202_write_word(dev, ISL94202_COTS,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_COTS_Msk);

    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc),
                              ic_conf->chg_ot_limit - ic_conf->temp_limit_hyst);
    isl94202_write_word(dev, ISL94202_COTR,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_COTR_Msk);

    // Charge under-temperature
    adc_voltage =
        interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc), ic_conf->chg_ut_limit);
    isl94202_write_word(dev, ISL94202_CUTS,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_CUTS_Msk);

    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc),
                              ic_conf->chg_ut_limit + ic_conf->temp_limit_hyst);
    isl94202_write_word(dev, ISL94202_CUTR,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_CUTR_Msk);

    // Discharge over-temperature
    adc_voltage =
        interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc), ic_conf->dis_ot_limit);
    isl94202_write_word(dev, ISL94202_DOTS,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_DOTS_Msk);

    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc),
                              ic_conf->dis_ot_limit - ic_conf->temp_limit_hyst);
    isl94202_write_word(dev, ISL94202_DOTR,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_DOTR_Msk);

    // Discharge under-temperature
    adc_voltage =
        interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc), ic_conf->dis_ut_limit);
    isl94202_write_word(dev, ISL94202_DUTS,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_DUTS_Msk);

    adc_voltage = interpolate(lut_temp_degc, lut_temp_volt, ARRAY_SIZE(lut_temp_degc),
                              ic_conf->dis_ut_limit + ic_conf->temp_limit_hyst);
    isl94202_write_word(dev, ISL94202_DUTR,
                        (uint16_t)(adc_voltage * 4095 * 2 / 1.8F) & ISL94202_DUTR_Msk);

    return 0;
}

static int isl94202_configure_balancing(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    int err = 0;

    // also apply balancing thresholds here
    err += isl94202_write_voltage(dev, ISL94202_CBMIN, ic_conf->bal_cell_voltage_min, 0);
    err += isl94202_write_voltage(dev, ISL94202_CBMAX, 4.5F, 0); // no upper limit for balancing
    err += isl94202_write_voltage(dev, ISL94202_CBMINDV, ic_conf->bal_cell_voltage_diff, 0);
    err +=
        isl94202_write_voltage(dev, ISL94202_CBMAXDV, 1.0F, 0); // no tight limit for voltage delta

    // EOC condition needs to be set to bal_cell_voltage_min instead of cell_chg_voltage_limit to
    // enable balancing during idle
    err += isl94202_write_voltage(dev, ISL94202_EOC, ic_conf->bal_cell_voltage_min, 0);

    return err;
}

static int bms_ic_isl94202_configure(const struct device *dev, struct bms_ic_conf *ic_conf,
                                     uint32_t flags)
{
    uint32_t actual_flags = 0;
    int err = 0;

    if (flags & BMS_IC_CONF_VOLTAGE_LIMITS) {
        err |= isl94202_configure_cell_ovp(dev, ic_conf);
        err |= isl94202_configure_cell_uvp(dev, ic_conf);
        actual_flags |= BMS_IC_CONF_VOLTAGE_LIMITS;
    }

    if (flags & BMS_IC_CONF_TEMP_LIMITS) {
        err |= isl94202_configure_temp_limits(dev, ic_conf);
        actual_flags |= BMS_IC_CONF_TEMP_LIMITS;
    }

#ifdef CONFIG_BMS_IC_CURRENT_MONITORING
    if (flags & BMS_IC_CONF_CURRENT_LIMITS) {
        err |= isl94202_configure_chg_ocp(dev, ic_conf);
        err |= isl94202_configure_dis_ocp(dev, ic_conf);
        err |= isl94202_configure_dis_scp(dev, ic_conf);
        actual_flags |= BMS_IC_CONF_CURRENT_LIMITS;
    }
#endif /* CONFIG_BMS_IC_CURRENT_MONITORING */

    if (flags & BMS_IC_CONF_BALANCING) {
        err |= isl94202_configure_balancing(dev, ic_conf);
        actual_flags |= BMS_IC_CONF_BALANCING;
    }

    if (err != 0) {
        return -EIO;
    }

    return (flags == actual_flags) ? 0 : -EINVAL;
}

static int isl94202_read_voltages(const struct device *dev, struct bms_ic_data *ic_data)
{
    const struct bms_ic_isl94202_config *dev_config = dev->config;
    uint16_t adc_raw = 0;
    int conn_cells = 0;
    int cell_index = 0;
    float sum_voltages = 0;

    int last_cell = find_msb_set(dev_config->used_cell_channels);
    for (int i = 0; i < last_cell; i++) {
        if (dev_config->used_cell_channels & BIT(i)) {
            if (cell_index >= CONFIG_BMS_IC_MAX_CELLS) {
                return -EINVAL;
            }

            isl94202_read_word(dev, ISL94202_CELL1 + i * 2, &adc_raw);
            adc_raw &= 0x0FFF;
            ic_data->cell_voltages[cell_index] = (float)adc_raw * 18 * 800 / 4095 / 3 / 1000;

            if (cell_index == 0) {
                ic_data->cell_voltage_max = ic_data->cell_voltages[cell_index];
                ic_data->cell_voltage_min = ic_data->cell_voltages[cell_index];
            }

            if (ic_data->cell_voltages[cell_index] > 0.5F) {
                conn_cells++;
                sum_voltages += ic_data->cell_voltages[cell_index];
            }

            if (ic_data->cell_voltages[cell_index] > ic_data->cell_voltage_max) {
                ic_data->cell_voltage_max = ic_data->cell_voltages[cell_index];
            }
            if (ic_data->cell_voltages[cell_index] < ic_data->cell_voltage_min
                && ic_data->cell_voltages[cell_index] > 0.5)
            {
                ic_data->cell_voltage_min = ic_data->cell_voltages[cell_index];
            }
            cell_index++;
        }
    }
    ic_data->connected_cells = conn_cells;
    ic_data->cell_voltage_avg = sum_voltages / conn_cells;

    // adc_raw = isl94202_read_word(dev, ISL94202_VBATT) & 0x0FFF;
    // ic_data->total_voltage = (float)adc_raw * 1.8 * 32 / 4095;

    // VBATT based pack voltage seems very inaccurate, so take sum of cell voltages instead
    ic_data->total_voltage = sum_voltages;

    return 0;
}

// using default setting TGain = 0 (GAIN = 2) with 22k resistors
static int isl94202_read_temperatures(const struct device *dev, struct bms_ic_data *ic_data)
{
    uint16_t adc_raw;

    // Internal temperature
    isl94202_read_word(dev, ISL94202_IT, &adc_raw);
    adc_raw &= 0x0FFF;
    ic_data->ic_temp = (float)adc_raw * 1.8 / 4095 * 1000 / 1.8527 - 273.15;

    // External temperature 1
    isl94202_read_word(dev, ISL94202_XT1, &adc_raw);
    adc_raw &= 0x0FFF;
    float adc_v = (float)adc_raw * 1.8 / 4095 / 2;

    ic_data->cell_temp_avg =
        interpolate(lut_temp_volt, lut_temp_degc, ARRAY_SIZE(lut_temp_degc), adc_v);

    // only single battery temperature measurement
    ic_data->cell_temp_min = ic_data->cell_temp_avg;
    ic_data->cell_temp_max = ic_data->cell_temp_avg;

    // External temperature 2 (used for MOSFET temperature sensing)
    isl94202_read_word(dev, ISL94202_XT2, &adc_raw);
    adc_raw &= 0x0FFF;
    adc_v = (float)adc_raw * 1.8 / 4095 / 2;

    ic_data->mosfet_temp =
        interpolate(lut_temp_volt, lut_temp_degc, ARRAY_SIZE(lut_temp_degc), adc_v);

    return 0;
}

#ifdef CONFIG_BMS_IC_CURRENT_MONITORING

static int isl94202_read_current(const struct device *dev, struct bms_ic_data *ic_data)
{
    const struct bms_ic_isl94202_config *dev_config = dev->config;
    uint8_t buf[2];

    // gain
    isl94202_read_bytes(dev, ISL94202_CTRL0, buf, 1);
    uint8_t gain_reg = (buf[0] & ISL94202_CTRL0_CG_Msk) >> ISL94202_CTRL0_CG_Pos;
    int gain = gain_reg < 3 ? isl94202_current_gains[gain_reg] : 500;

    // direction / sign
    int sign = 0;
    isl94202_read_bytes(dev, ISL94202_STAT2, buf, 1);
    sign += (buf[0] & ISL94202_STAT2_CHING_Msk) >> ISL94202_STAT2_CHING_Pos;
    sign -= (buf[0] & ISL94202_STAT2_DCHING_Msk) >> ISL94202_STAT2_DCHING_Pos;

    // ADC value
    uint16_t adc_raw;
    isl94202_read_word(dev, ISL94202_ISNS, &adc_raw);
    adc_raw &= 0x0FFF;

    ic_data->current =
        (float)(sign * adc_raw * 1800) / (4095 * gain * dev_config->shunt_resistor_uohm) * 1000;

    return 0;
}

#endif /* CONFIG_BMS_IC_CURRENT_MONITORING */

static int isl94202_read_balancing(const struct device *dev, struct bms_ic_data *ic_data)
{
    /*
     * Balancing is done automatically, just reading status here (even though the datasheet
     * tells that the CBFC register value cannot be used for indication if a cell is
     * balanced at the moment)
     */

    uint8_t reg = 0;
    isl94202_read_bytes(dev, ISL94202_CBFC, &reg, 1);

    ic_data->balancing_status = reg;

    return 0;
}

static int isl94202_read_error_flags(const struct device *dev, struct bms_ic_data *ic_data)
{
    struct bms_ic_isl94202_data *dev_data = dev->data;
    uint32_t error_flags = 0;
    uint8_t stat[2];
    uint8_t ctrl1;

    isl94202_read_bytes(dev, ISL94202_STAT0, stat, 2);
    isl94202_read_bytes(dev, ISL94202_CTRL1, &ctrl1, 1);

    if (stat[0] & ISL94202_STAT0_UVF_Msk)
        error_flags |= BMS_ERR_CELL_UNDERVOLTAGE;
    if (stat[0] & ISL94202_STAT0_OVF_Msk)
        error_flags |= BMS_ERR_CELL_OVERVOLTAGE;
    if (stat[1] & ISL94202_STAT1_DSCF_Msk)
        error_flags |= BMS_ERR_SHORT_CIRCUIT;
    if (stat[1] & ISL94202_STAT1_DOCF_Msk)
        error_flags |= BMS_ERR_DIS_OVERCURRENT;
    if (stat[1] & ISL94202_STAT1_COCF_Msk)
        error_flags |= BMS_ERR_CHG_OVERCURRENT;
    if (stat[1] & ISL94202_STAT1_OPENF_Msk)
        error_flags |= BMS_ERR_OPEN_WIRE;
    if (stat[0] & ISL94202_STAT0_DUTF_Msk)
        error_flags |= BMS_ERR_DIS_UNDERTEMP;
    if (stat[0] & ISL94202_STAT0_DOTF_Msk)
        error_flags |= BMS_ERR_DIS_OVERTEMP;
    if (stat[0] & ISL94202_STAT0_CUTF_Msk)
        error_flags |= BMS_ERR_CHG_UNDERTEMP;
    if (stat[0] & ISL94202_STAT0_COTF_Msk)
        error_flags |= BMS_ERR_CHG_OVERTEMP;
    if (stat[1] & ISL94202_STAT1_IOTF_Msk)
        error_flags |= BMS_ERR_INT_OVERTEMP;
    if (stat[1] & ISL94202_STAT1_CELLF_Msk)
        error_flags |= BMS_ERR_CELL_FAILURE;

    if (!(ctrl1 & ISL94202_CTRL1_DFET_Msk) && (dev_data->fet_state & BMS_SWITCH_DIS)) {
        error_flags |= BMS_ERR_DIS_OFF;
    }

    if (!(ctrl1 & ISL94202_CTRL1_CFET_Msk) && (dev_data->fet_state & BMS_SWITCH_CHG)) {
        error_flags |= BMS_ERR_CHG_OFF;
    }

    ic_data->error_flags = error_flags;

    return 0;
}

static int bms_ic_isl94202_read_data(const struct device *dev, uint32_t flags)
{
    struct bms_ic_isl94202_data *dev_data = dev->data;
    struct bms_ic_data *ic_data = dev_data->ic_data;
    uint32_t actual_flags = 0;
    int err = 0;

    if (ic_data == NULL) {
        return -ENOMEM;
    }

    if (flags & (BMS_IC_DATA_CELL_VOLTAGES | BMS_IC_DATA_PACK_VOLTAGES)) {
        err |= isl94202_read_voltages(dev, ic_data);
        actual_flags |= ((BMS_IC_DATA_CELL_VOLTAGES | BMS_IC_DATA_PACK_VOLTAGES) & flags);
    }

    if (flags & BMS_IC_DATA_TEMPERATURES) {
        err |= isl94202_read_temperatures(dev, ic_data);
        actual_flags |= BMS_IC_DATA_TEMPERATURES;
    }

#ifdef CONFIG_BMS_IC_CURRENT_MONITORING
    if (flags & BMS_IC_DATA_CURRENT) {
        err |= isl94202_read_current(dev, ic_data);
        actual_flags |= BMS_IC_DATA_CURRENT;
    }
#endif /* CONFIG_BMS_IC_CURRENT_MONITORING */

    if (flags & BMS_IC_DATA_BALANCING) {
        err |= isl94202_read_balancing(dev, ic_data);
        actual_flags |= BMS_IC_DATA_BALANCING;
    }

    if (flags & BMS_IC_DATA_ERROR_FLAGS) {
        err |= isl94202_read_error_flags(dev, ic_data);
        actual_flags |= BMS_IC_DATA_ERROR_FLAGS;
    }

    if (err != 0) {
        return -EIO;
    }

    return (flags == actual_flags) ? 0 : -EINVAL;
}

static void bms_ic_isl94202_assign_data(const struct device *dev, struct bms_ic_data *ic_data)
{
    struct bms_ic_isl94202_data *dev_data = dev->data;

    dev_data->ic_data = ic_data;
}

#ifdef CONFIG_BMS_IC_SWITCHES

static int bms_ic_isl94202_set_switches(const struct device *dev, uint8_t switches, bool enabled)
{
    struct bms_ic_isl94202_data *dev_data = dev->data;
    uint8_t reg;

    if ((switches & (BMS_SWITCH_CHG | BMS_SWITCH_DIS)) != switches) {
        return -EINVAL;
    }

    isl94202_read_bytes(dev, ISL94202_CTRL1, &reg, 1);

    if (switches & BMS_SWITCH_CHG) {
        if (enabled) {
            reg |= ISL94202_CTRL1_CFET_Msk;
            dev_data->fet_state |= BMS_SWITCH_CHG;
        }
        else {
            reg &= ~ISL94202_CTRL1_CFET_Msk;
            dev_data->fet_state &= ~BMS_SWITCH_CHG;
        }
    }
    if (switches & BMS_SWITCH_DIS) {
        if (enabled) {
            reg |= ISL94202_CTRL1_DFET_Msk;
            dev_data->fet_state |= BMS_SWITCH_DIS;
        }
        else {
            reg &= ~ISL94202_CTRL1_DFET_Msk;
            dev_data->fet_state &= ~BMS_SWITCH_DIS;
        }
    }

    return isl94202_write_bytes(dev, ISL94202_CTRL1, &reg, 1);
}

#endif /* CONFIG_BMS_IC_SWITCHES */

static void isl94202_print_register(const struct device *dev, uint16_t addr)
{
    uint8_t reg;

    isl94202_read_bytes(dev, (uint8_t)addr, &reg, 1);

    LOG_INF("0x%.2X: 0x%.2X = %s", addr, reg, byte2bitstr(reg));
}

static int bms_ic_isl94202_debug_print_mem(const struct device *dev)
{
    LOG_INF("EEPROM content: ------------------");
    for (int i = 0; i < 0x4C; i++) {
        isl94202_print_register(dev, i);
    }
    LOG_INF("RAM content: ------------------");
    for (int i = 0x80; i <= 0xAB; i++) {
        isl94202_print_register(dev, i);
    }

    return 0;
}

static int bms_ic_isl94202_balance(const struct device *dev, uint32_t cells)
{
    int err = 0;

    if (cells == BMS_IC_BALANCING_OFF) {
    }
    else if (cells == BMS_IC_BALANCING_AUTO) {
        uint8_t stat3;
        isl94202_read_bytes(dev, ISL94202_STAT3, &stat3, 1);

        /*
         * System scans for voltage, current and temperature measurements happen in different
         * intervals depending on the mode. Cell balancing should be off during voltage scans.
         *
         * Each scan takes max. 1.7 ms. Choosing 16 ms off-time for voltages to settle.
         */
        if (stat3 & ISL94202_STAT3_INIDLE_Msk) {
            /* IDLE mode: Scan every 256 ms */
            isl94202_write_delay(dev, ISL94202_CBONT, ISL94202_DELAY_MS, 240, 0);
            isl94202_write_delay(dev, ISL94202_CBOFFT, ISL94202_DELAY_MS, 16, 0);
        }
        else if (stat3 & ISL94202_STAT3_INDOZE_Msk) {
            /* DOZE mode: Scan every 512 ms */
            isl94202_write_delay(dev, ISL94202_CBONT, ISL94202_DELAY_MS, 496, 0);
            isl94202_write_delay(dev, ISL94202_CBOFFT, ISL94202_DELAY_MS, 16, 0);
        }
        else if (!(stat3 & ISL94202_STAT3_INSLEEP_Msk)) {
            /* NORMAL mode: Scan every 32 ms */
            isl94202_write_delay(dev, ISL94202_CBONT, ISL94202_DELAY_MS, 16, 0);
            isl94202_write_delay(dev, ISL94202_CBOFFT, ISL94202_DELAY_MS, 16, 0);
        }
    }
    else {
        return -ENOTSUP;
    }

    return err;
}

static int bms_ic_isl94202_set_mode(const struct device *dev, enum bms_ic_mode mode)
{
    uint8_t reg;

    switch (mode) {
        case BMS_IC_MODE_OFF:
            reg = ISL94202_CTRL3_PDWN_Msk;
            isl94202_write_bytes(dev, ISL94202_CTRL3, &reg, 1);
            return 0;
        default:
            return -ENOTSUP;
    }
}

static int isl94202_init(const struct device *dev)
{
    const struct bms_ic_isl94202_config *dev_config = dev->config;
    uint8_t reg;
    int err;

    if (!i2c_is_ready_dt(&dev_config->i2c)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }
    if (!gpio_is_ready_dt(&dev_config->i2c_pullup)) {
        LOG_ERR("I2C pull-up GPIO not ready");
        return -ENODEV;
    }

    /* Datasheet: 3 seconds wake-up delay from shutdown or initial power-up */
    k_sleep(K_SECONDS(3));

    /* activate pull-up at I2C SDA and SCL */
    gpio_pin_configure_dt(&dev_config->i2c_pullup, GPIO_OUTPUT_ACTIVE);

    err = set_num_cells(dev, CONFIG_NUM_CELLS_IN_SERIES);
    if (err) {
        return err;
    }

    // xTemp2 monitoring MOSFETs and not cells
    reg = ISL94202_SETUP0_XT2M_Msk;
    err = isl94202_write_bytes(dev, ISL94202_SETUP0, &reg, 1);
    if (err) {
        return err;
    }

    // Enable balancing during charging and EOC conditions
    reg = ISL94202_SETUP1_CBDC_Msk | ISL94202_SETUP1_CB_EOC_Msk;
    err = isl94202_write_bytes(dev, ISL94202_SETUP1, &reg, 1);
    if (err) {
        return err;
    }

    // Enable FET control via microcontroller
    reg = ISL94202_CTRL2_UCFET_Msk;
    err = isl94202_write_bytes(dev, ISL94202_CTRL2, &reg, 1);
    if (err) {
        return err;
    }

    // Remark: Ideal diode control via DFODOV and DFODUV bits of SETUP1 register doesn't have any
    // effect because of FET control via microcontroller.

    return 0;
}

static const struct bms_ic_driver_api isl94202_driver_api = {
    .configure = bms_ic_isl94202_configure,
    .assign_data = bms_ic_isl94202_assign_data,
    .read_data = bms_ic_isl94202_read_data,
#ifdef CONFIG_BMS_IC_SWITCHES
    .set_switches = bms_ic_isl94202_set_switches,
#endif
    .balance = bms_ic_isl94202_balance,
    .set_mode = bms_ic_isl94202_set_mode,
    .debug_print_mem = bms_ic_isl94202_debug_print_mem,
};

#define ISL94202_ASSERT_CURRENT_MONITORING_PROP_GREATER_ZERO(index, prop) \
    BUILD_ASSERT(COND_CODE_0(IS_ENABLED(CONFIG_BMS_IC_CURRENT_MONITORING), (1), \
                             (DT_INST_PROP_OR(index, prop, 0) > 0)), \
                 "Devicetree properties shunt-resistor-uohm and board-max-current " \
                 "must be greater than 0 for CONFIG_BMS_IC_CURRENT_MONITORING=y")

#define ISL94202_INIT(index) \
    static struct bms_ic_isl94202_data isl94202_data_##index = { 0 }; \
    ISL94202_ASSERT_CURRENT_MONITORING_PROP_GREATER_ZERO(index, shunt_resistor_uohm); \
    ISL94202_ASSERT_CURRENT_MONITORING_PROP_GREATER_ZERO(index, board_max_current); \
    static const struct bms_ic_isl94202_config isl94202_config_##index = { \
        .i2c = I2C_DT_SPEC_INST_GET(index), \
        .i2c_pullup = GPIO_DT_SPEC_INST_GET(index, pull_up_gpios), \
        .shunt_resistor_uohm = DT_INST_PROP_OR(index, shunt_resistor_uohm, 1000), \
        .board_max_current = DT_INST_PROP_OR(index, board_max_current, 0), \
        .used_cell_channels = DT_INST_PROP(index, used_cell_channels), \
    }; \
    DEVICE_DT_INST_DEFINE(index, &isl94202_init, NULL, &isl94202_data_##index, \
                          &isl94202_config_##index, POST_KERNEL, CONFIG_BMS_IC_INIT_PRIORITY, \
                          &isl94202_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ISL94202_INIT)
