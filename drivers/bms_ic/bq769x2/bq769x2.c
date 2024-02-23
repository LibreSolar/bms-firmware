/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq769x2_i2c

#include "bq769x2_interface.h"
#include "bq769x2_priv.h"
#include "bq769x2_registers.h"

#include <bms/bms_common.h>
#include <drivers/bms_ic.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>

LOG_MODULE_REGISTER(bms_ic_bq769x2, CONFIG_BMS_IC_LOG_LEVEL);

static int bq769x2_write_bytes_i2c(const struct device *dev, const uint8_t reg_addr,
                                   const uint8_t *data, const size_t num_bytes)
{
    const struct bms_ic_bq769x2_config *config = dev->config;
    uint8_t buf[10] = {
        config->i2c.addr << 1, /* target address for CRC calculation */
        reg_addr,
    };

    if (num_bytes > 4 || num_bytes < 1) {
        return -EINVAL;
    }

    if (config->crc_enabled) {
        /* first CRC includes target address and register address */
        buf[2] = data[0];
        buf[3] = crc8_ccitt(0, buf, 3);

        /* subsequent CRCs only include the data byte */
        for (int i = 1; i < num_bytes; i++) {
            buf[i * 2 + 2] = data[i];
            buf[i * 2 + 3] = crc8_ccitt(0, &data[i], 1);
        }

        return i2c_write_dt(&config->i2c, buf + 1, num_bytes * 2 + 1);
    }
    else {
        memcpy(buf + 2, data, num_bytes);

        return i2c_write_dt(&config->i2c, buf + 1, num_bytes + 1);
    }
}

static int bq769x2_read_bytes_i2c(const struct device *dev, const uint8_t reg_addr, uint8_t *data,
                                  const size_t num_bytes)
{
    const struct bms_ic_bq769x2_config *config = dev->config;

    if (config->crc_enabled) {
        if (num_bytes > BQ769X2_DATA_BUFFER_SIZE || num_bytes < 1) {
            return -EINVAL;
        }

        /*
         * The first CRC is calculated beginning at the first start, so will include the
         * target address, the register address, then the target address with read
         * bit set, then the data byte.
         *
         * We need to reserve a buffer of 2x the maximum data size due to CRC calculation
         * for each individual byte.
         */
        uint8_t buf[3 + BQ769X2_DATA_BUFFER_SIZE * 2] = {
            config->i2c.addr << 1,
            reg_addr,
            (config->i2c.addr << 1) | 1U,
        };
        uint8_t byte, crc_read;
        int err;

        err = i2c_write_read_dt(&config->i2c, &reg_addr, 1, buf + 3, num_bytes * 2);
        if (err != 0) {
            return err;
        }

        /* check CRC of first data byte */
        if (crc8_ccitt(0, buf, 4) == buf[4]) {
            data[0] = buf[3];
        }
        else {
            return -EIO;
        }

        /* check CRCs of subsequent data bytes */
        for (int i = 1; i < num_bytes; i++) {
            byte = buf[2 * i + 3];
            crc_read = buf[2 * i + 4];
            if (crc8_ccitt(0, &byte, 1) == crc_read) {
                data[i] = byte;
            }
            else {
                return -EIO;
            }
        }

        return 0;
    }
    else {
        return i2c_write_read_dt(&config->i2c, &reg_addr, 1, data, num_bytes);
    }
}

static int bq769x2_detect_cells(const struct device *dev)
{
    const struct bms_ic_bq769x2_config *config = dev->config;
    uint8_t conn_cells = 0;
    uint16_t vcell_mode = 0;
    uint16_t voltage = UINT16_MAX; /* init with implausible value */
    uint32_t stack_voltage_calc = 0;
    int err = 0;

    err |= bq769x2_direct_read_i2(dev, BQ769X2_CMD_VOLTAGE_STACK, &voltage);
    uint32_t stack_voltage_meas = voltage * 10; /* unit: 10 mV */

    int last_cell = find_msb_set(config->used_cell_channels);
    for (int i = 0; i < last_cell; i++) {
        if (config->used_cell_channels & BIT(i)) {
            err |= bq769x2_direct_read_i2(dev, BQ769X2_CMD_VOLTAGE_CELL_1 + i * 2, &voltage);
            if (voltage > 500) {
                /* only voltages >500 mV are considered as connected cells */
                vcell_mode |= BIT(i);
                stack_voltage_calc += voltage;
                conn_cells++;
            }
        }
    }

    /*
     * Check for open wires by comparing measured stack voltage with sum of cell voltages.
     * Deviation of +-50 mV per cell are accepted (stack voltage is not very precise if
     * uncalibrated).
     */
    if (!IN_RANGE(stack_voltage_calc, stack_voltage_meas - conn_cells * 50,
                  stack_voltage_meas + conn_cells * 50))
    {
        LOG_ERR("Sum of cell voltages (%u mV) != stack voltage (%u mV)", stack_voltage_calc,
                stack_voltage_meas);
        return -EINVAL;
    }

    err |= bq769x2_datamem_write_u2(dev, BQ769X2_SET_CONF_VCELL_MODE, vcell_mode);

    LOG_INF("Detected %d cells", conn_cells);

    return err == 0 ? 0 : -EIO;
}

static int bq769x2_configure_cell_ovp(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    int err = 0;

    uint8_t cov_threshold = lroundf(ic_conf->cell_ov_limit * 1000.0F / 50.6F);
    uint8_t cov_hyst =
        lroundf(MAX(ic_conf->cell_ov_limit - ic_conf->cell_ov_reset, 0) * 1000.0F / 50.6F);
    uint16_t cov_delay = lroundf(ic_conf->cell_ov_delay_ms / 3.3F);

    cov_threshold = CLAMP(cov_threshold, 20, 110);
    cov_hyst = CLAMP(cov_hyst, 2, 20);
    cov_delay = CLAMP(cov_delay, 1, 2047);

    err |= bq769x2_datamem_write_u1(dev, BQ769X2_PROT_COV_THRESHOLD, cov_threshold);
    err |= bq769x2_datamem_write_u1(dev, BQ769X2_PROT_COV_RECOV_HYST, cov_hyst);
    err |= bq769x2_datamem_write_u2(dev, BQ769X2_PROT_COV_DELAY, cov_delay);

    ic_conf->cell_ov_limit = cov_threshold * 50.6F / 1000.0F;
    ic_conf->cell_ov_reset = (cov_threshold - cov_hyst) * 50.6F / 1000.0F;
    ic_conf->cell_ov_delay_ms = cov_delay * 3.3F;

    /* COV protection is enabled by default in BQ769X2_SET_PROT_ENABLED_A register. */

    return err == 0 ? 0 : -EIO;
}

static int bq769x2_configure_cell_uvp(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    int err = 0;

    uint8_t cuv_threshold = lroundf(ic_conf->cell_uv_limit * 1000.0F / 50.6F);
    uint8_t cuv_hyst =
        lroundf(MAX(ic_conf->cell_uv_reset - ic_conf->cell_uv_limit, 0) * 1000.0F / 50.6F);
    uint16_t cuv_delay = lroundf(ic_conf->cell_uv_delay_ms / 3.3F);

    cuv_threshold = CLAMP(cuv_threshold, 20, 90);
    cuv_hyst = CLAMP(cuv_hyst, 2, 20);
    cuv_delay = CLAMP(cuv_delay, 1, 2047);

    err |= bq769x2_datamem_write_u1(dev, BQ769X2_PROT_CUV_THRESHOLD, cuv_threshold);
    err |= bq769x2_datamem_write_u1(dev, BQ769X2_PROT_CUV_RECOV_HYST, cuv_hyst);
    err |= bq769x2_datamem_write_u2(dev, BQ769X2_PROT_CUV_DELAY, cuv_delay);

    ic_conf->cell_uv_limit = cuv_threshold * 50.6F / 1000.0F;
    ic_conf->cell_uv_reset = (cuv_threshold + cuv_hyst) * 50.6F / 1000.0F;
    ic_conf->cell_uv_delay_ms = cuv_delay * 3.3F;

    /* CUV protection needs to be enabled, as it is not active by default */
    union bq769x2_reg_safety_a prot_enabled_a;
    err |= bq769x2_datamem_read_u1(dev, BQ769X2_SET_PROT_ENABLED_A, &prot_enabled_a.byte);
    if (!err) {
        prot_enabled_a.CUV = 1;
        err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_PROT_ENABLED_A, prot_enabled_a.byte);
    }

    return err == 0 ? 0 : -EIO;
}

static int bq769x2_configure_temp_limits(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    int err = 0;
    uint8_t hyst = CLAMP(ic_conf->temp_limit_hyst, 1, 20);

    if (ic_conf->dis_ot_limit < 0.0F || ic_conf->chg_ot_limit < 0.0F
        || ic_conf->dis_ot_limit < ic_conf->dis_ut_limit + 20.0F
        || ic_conf->chg_ot_limit < ic_conf->chg_ut_limit + 20.0F)
    {
        return -EINVAL;
    }

    int8_t otc_threshold = CLAMP(ic_conf->chg_ot_limit, -40, 120);
    int8_t otc_recovery = CLAMP(otc_threshold - hyst, -40, 120);
    int8_t otd_threshold = CLAMP(ic_conf->dis_ot_limit, -40, 120);
    int8_t otd_recovery = CLAMP(otd_threshold - hyst, -40, 120);

    int8_t utc_threshold = CLAMP(ic_conf->chg_ut_limit, -40, 120);
    int8_t utc_recovery = CLAMP(utc_threshold + hyst, -40, 120);
    int8_t utd_threshold = CLAMP(ic_conf->dis_ut_limit, -40, 120);
    int8_t utd_recovery = CLAMP(utd_threshold + hyst, -40, 120);

    err |= bq769x2_datamem_write_i1(dev, BQ769X2_PROT_OTC_THRESHOLD, otc_threshold);
    err |= bq769x2_datamem_write_i1(dev, BQ769X2_PROT_OTC_RECOVERY, otc_recovery);
    err |= bq769x2_datamem_write_i1(dev, BQ769X2_PROT_OTD_THRESHOLD, otd_threshold);
    err |= bq769x2_datamem_write_i1(dev, BQ769X2_PROT_OTD_RECOVERY, otd_recovery);

    err |= bq769x2_datamem_write_i1(dev, BQ769X2_PROT_UTC_THRESHOLD, utc_threshold);
    err |= bq769x2_datamem_write_i1(dev, BQ769X2_PROT_UTC_RECOVERY, utc_recovery);
    err |= bq769x2_datamem_write_i1(dev, BQ769X2_PROT_UTD_THRESHOLD, utd_threshold);
    err |= bq769x2_datamem_write_i1(dev, BQ769X2_PROT_UTD_RECOVERY, utd_recovery);

    ic_conf->chg_ot_limit = otc_threshold;
    ic_conf->dis_ot_limit = otd_threshold;
    ic_conf->chg_ut_limit = utc_threshold;
    ic_conf->dis_ut_limit = utd_threshold;
    ic_conf->temp_limit_hyst = hyst;

    /* temperature protection has to be enabled manually */
    union bq769x2_reg_safety_b prot_enabled_b;
    err |= bq769x2_datamem_read_u1(dev, BQ769X2_SET_PROT_ENABLED_B, &prot_enabled_b.byte);
    if (!err) {
        prot_enabled_b.OTC = 1;
        prot_enabled_b.OTD = 1;
        prot_enabled_b.UTC = 1;
        prot_enabled_b.UTD = 1;
        err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_PROT_ENABLED_B, prot_enabled_b.byte);
    }

    return err == 0 ? 0 : -EIO;
}

#ifdef CONFIG_BMS_IC_CURRENT_MONITORING

static int bq769x2_configure_chg_ocp(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    const struct bms_ic_bq769x2_config *dev_config = dev->config;
    int err = 0;

    float oc_limit = MIN(ic_conf->chg_oc_limit, dev_config->board_max_current);
    uint8_t oc_threshold = lroundf(oc_limit * dev_config->shunt_resistor_uohm / 2000.0F);
    int16_t oc_delay = lroundf((ic_conf->chg_oc_delay_ms - 6.6F) / 3.3F);

    oc_threshold = CLAMP(oc_threshold, 2, 62);
    oc_delay = CLAMP(oc_delay, 1, 127);

    err |= bq769x2_datamem_write_u1(dev, BQ769X2_PROT_OCC_THRESHOLD, oc_threshold);
    err |= bq769x2_datamem_write_u1(dev, BQ769X2_PROT_OCC_DELAY, oc_delay);

    ic_conf->chg_oc_limit = oc_threshold * 2000.0F / dev_config->shunt_resistor_uohm;
    ic_conf->chg_oc_delay_ms = lroundf(6.6F + oc_delay * 3.3F);

    /* OCC protection needs to be enabled, as it is not active by default */
    union bq769x2_reg_safety_a prot_enabled_a;
    err |= bq769x2_datamem_read_u1(dev, BQ769X2_SET_PROT_ENABLED_A, &prot_enabled_a.byte);
    if (!err) {
        prot_enabled_a.OCC = 1;
        err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_PROT_ENABLED_A, prot_enabled_a.byte);
    }

    return err == 0 ? 0 : -EIO;
}

static int bq769x2_configure_dis_ocp(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    const struct bms_ic_bq769x2_config *dev_config = dev->config;
    int err = 0;

    float oc_limit = MIN(ic_conf->dis_oc_limit, dev_config->board_max_current);
    uint8_t oc_threshold = lroundf(oc_limit * dev_config->shunt_resistor_uohm / 2000.0F);
    int16_t oc_delay = lroundf((ic_conf->dis_oc_delay_ms - 6.6F) / 3.3F);

    oc_threshold = CLAMP(oc_threshold, 2, 100);
    oc_delay = CLAMP(oc_delay, 1, 127);

    err |= bq769x2_datamem_write_u1(dev, BQ769X2_PROT_OCD1_THRESHOLD, oc_threshold);
    err |= bq769x2_datamem_write_u1(dev, BQ769X2_PROT_OCD1_DELAY, oc_delay);

    ic_conf->dis_oc_limit = oc_threshold * 2000.0F / dev_config->shunt_resistor_uohm;
    ic_conf->dis_oc_delay_ms = lroundf(6.6F + oc_delay * 3.3F);

    /* OCD protection needs to be enabled, as it is not active by default */
    union bq769x2_reg_safety_a prot_enabled_a;
    err |= bq769x2_datamem_read_u1(dev, BQ769X2_SET_PROT_ENABLED_A, &prot_enabled_a.byte);
    if (!err) {
        prot_enabled_a.OCD1 = 1;
        err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_PROT_ENABLED_A, prot_enabled_a.byte);
    }

    return err == 0 ? 0 : -EIO;
}

static int bq769x2_configure_dis_scp(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    const struct bms_ic_bq769x2_config *dev_config = dev->config;
    int err = 0;

    uint8_t scp_threshold = 0;
    uint16_t shunt_voltage = ic_conf->dis_sc_limit * dev_config->shunt_resistor_uohm / 1000.0F;
    for (int i = ARRAY_SIZE(bq769x2_scd_thresholds) - 1; i > 0; i--) {
        if (shunt_voltage >= bq769x2_scd_thresholds[i]) {
            scp_threshold = i;
            break;
        }
    }

    uint16_t scp_delay = ic_conf->dis_sc_delay_us / 15.0F + 1;
    scp_delay = CLAMP(scp_delay, 1, 31);

    err |= bq769x2_datamem_write_u1(dev, BQ769X2_PROT_SCD_THRESHOLD, scp_threshold);
    err |= bq769x2_datamem_write_u1(dev, BQ769X2_PROT_SCD_DELAY, scp_delay);

    ic_conf->dis_sc_limit =
        bq769x2_scd_thresholds[scp_threshold] * 1000.0F / dev_config->shunt_resistor_uohm;
    ic_conf->dis_sc_delay_us = (scp_delay - 1) * 15;

    return err == 0 ? 0 : -EIO;
}

#endif /* CONFIG_BMS_IC_CURRENT_MONITORING */

static int bq769x2_configure_balancing(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    struct bms_ic_bq769x2_data *dev_data = dev->data;
    int err = 0;

    /*
     * The bq769x2 differentiates between charging and relaxed balancing. We apply
     * the same setpoints for both mechanisms.
     */
    int16_t cell_voltage_min = ic_conf->bal_cell_voltage_min * 1000.0F;
    err |= bq769x2_datamem_write_i2(dev, BQ769X2_SET_CBAL_CHG_MIN_CELL_V, cell_voltage_min);
    err |= bq769x2_datamem_write_i2(dev, BQ769X2_SET_CBAL_RLX_MIN_CELL_V, cell_voltage_min);

    int8_t cell_voltage_delta = ic_conf->bal_cell_voltage_diff * 1000.0F;
    err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_CBAL_CHG_MIN_DELTA, cell_voltage_delta);
    err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_CBAL_CHG_STOP_DELTA, cell_voltage_delta);
    err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_CBAL_RLX_MIN_DELTA, cell_voltage_delta);
    err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_CBAL_RLX_STOP_DELTA, cell_voltage_delta);

    /* same temperature limits as for normal discharging */
    int8_t utd_threshold = CLAMP(ic_conf->dis_ut_limit, -40, 120);
    int8_t otd_threshold = CLAMP(ic_conf->dis_ot_limit, -40, 120);
    err |= bq769x2_datamem_write_i1(dev, BQ769X2_SET_CBAL_MIN_CELL_TEMP, utd_threshold);
    err |= bq769x2_datamem_write_i1(dev, BQ769X2_SET_CBAL_MAX_CELL_TEMP, otd_threshold);

    /* relaxed status is defined based on global idle current thresholds */
    int16_t idle_current_threshold = ic_conf->bal_idle_current * 1000.0F;
    err |= bq769x2_datamem_write_i2(dev, BQ769X2_SET_DSG_CURR_TH, idle_current_threshold);
    err |= bq769x2_datamem_write_i2(dev, BQ769X2_SET_CHG_CURR_TH, idle_current_threshold);

    /* allow balancing of up to 4 cells (instead of only 1 by default) */
    err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_CBAL_MAX_CELLS, 4);

    if (ic_conf->auto_balancing) {
        /* enable CB_RLX and CB_CHG */
        err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_CBAL_CONF, 0x03);
    }
    else {
        err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_CBAL_CONF, 0x00);
    }
    dev_data->auto_balancing = ic_conf->auto_balancing;

    ic_conf->bal_cell_voltage_min = ic_conf->bal_cell_voltage_min;
    ic_conf->bal_cell_voltage_diff = ic_conf->bal_cell_voltage_diff;
    ic_conf->bal_idle_current = ic_conf->bal_idle_current;

    return err == 0 ? 0 : -EIO;
}

static int bq769x2_configure_alerts(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    int err = 0;

    union bq769x2_reg_alarm_sf_alert_mask_a sf_alert_mask_a = { 0 };
    union bq769x2_reg_alarm_sf_alert_mask_b sf_alert_mask_b = { 0 };

    ic_conf->alert_mask = 0;

    sf_alert_mask_a.CUV = !!(ic_conf->alert_mask & BMS_ERR_CELL_UNDERVOLTAGE);
    sf_alert_mask_a.COV = !!(ic_conf->alert_mask & BMS_ERR_CELL_OVERVOLTAGE);
    sf_alert_mask_a.SCD = !!(ic_conf->alert_mask & BMS_ERR_SHORT_CIRCUIT);
    sf_alert_mask_a.OCD1 = !!(ic_conf->alert_mask & BMS_ERR_DIS_OVERCURRENT);
    sf_alert_mask_a.OCC = !!(ic_conf->alert_mask & BMS_ERR_CHG_OVERCURRENT);

    ic_conf->alert_mask |= (ic_conf->alert_mask & BMS_ERR_CELL_UNDERVOLTAGE);
    ic_conf->alert_mask |= (ic_conf->alert_mask & BMS_ERR_CELL_OVERVOLTAGE);
    ic_conf->alert_mask |= (ic_conf->alert_mask & BMS_ERR_SHORT_CIRCUIT);
    ic_conf->alert_mask |= (ic_conf->alert_mask & BMS_ERR_DIS_OVERCURRENT);
    ic_conf->alert_mask |= (ic_conf->alert_mask & BMS_ERR_CHG_OVERCURRENT);

    err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_ALARM_SF_ALERT_MASK_A, sf_alert_mask_a.byte);

    sf_alert_mask_b.UTD = !!(ic_conf->alert_mask & BMS_ERR_DIS_UNDERTEMP);
    sf_alert_mask_b.OTD = !!(ic_conf->alert_mask & BMS_ERR_DIS_OVERTEMP);
    sf_alert_mask_b.UTC = !!(ic_conf->alert_mask & BMS_ERR_CHG_UNDERTEMP);
    sf_alert_mask_b.OTC = !!(ic_conf->alert_mask & BMS_ERR_CHG_OVERTEMP);
    sf_alert_mask_b.OTINT = !!(ic_conf->alert_mask & BMS_ERR_INT_OVERTEMP);
    sf_alert_mask_b.OTF = !!(ic_conf->alert_mask & BMS_ERR_FET_OVERTEMP);

    ic_conf->alert_mask |= (ic_conf->alert_mask & BMS_ERR_DIS_UNDERTEMP);
    ic_conf->alert_mask |= (ic_conf->alert_mask & BMS_ERR_DIS_OVERTEMP);
    ic_conf->alert_mask |= (ic_conf->alert_mask & BMS_ERR_CHG_UNDERTEMP);
    ic_conf->alert_mask |= (ic_conf->alert_mask & BMS_ERR_CHG_OVERTEMP);
    ic_conf->alert_mask |= (ic_conf->alert_mask & BMS_ERR_INT_OVERTEMP);
    ic_conf->alert_mask |= (ic_conf->alert_mask & BMS_ERR_FET_OVERTEMP);

    err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_ALARM_SF_ALERT_MASK_B, sf_alert_mask_b.byte);

    /* enable alarm (triggering of ALERT pin) for SF alert masks configured above */
    err |= bq769x2_datamem_write_u2(dev, BQ769X2_SET_ALARM_DEFAULT_MASK, 0x1000);

    return err == 0 ? 0 : -EIO;
}

static int bq769x2_configure_voltage_regs(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    int err = 0;

    uint8_t reg12_config;
    err = bq769x2_datamem_read_u1(dev, BQ769X2_SET_CONF_REG12, &reg12_config);
    if (err != 0) {
        return -EIO;
    }

    /* clear REG2_EN and REG1_EN bits and keep voltage setting untouched */
    reg12_config &= ~0x11;

    if (ic_conf->vregs_enable & BIT(1)) {
        reg12_config |= 0x01; /* REG1_EN */
        ic_conf->vregs_enable |= BIT(1);
    }

    if (ic_conf->vregs_enable & BIT(2)) {
        reg12_config |= 0x10; /* REG2_EN */
        ic_conf->vregs_enable |= BIT(2);
    }

    err = bq769x2_datamem_write_u1(dev, BQ769X2_SET_CONF_REG12, reg12_config);

    return err == 0 ? 0 : -EIO;
}

static int bq769x2_init_config(const struct device *dev)
{
    const struct bms_ic_bq769x2_config *config = dev->config;
    int err = 0;

    /* Shunt value based on nominal value of VREF2 (could be improved by calibration) */
    err |= bq769x2_datamem_write_f4(dev, BQ769X2_CAL_CURR_CC_GAIN,
                                    7568.4F / config->shunt_resistor_uohm);

    /* Set resolution for CC2 current to 10 mA and stack/pack voltage to 10 mV */
    err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_CONF_DA, 0x06);

    /* Disable automatic turn-on of all MOSFETs */
    err |= bq769x2_subcmd_cmd_only(dev, BQ769X2_SUBCMD_ALL_FETS_OFF);

    /*
     * Setting FET_EN is required to exit the default FET test mode and enable normal FET
     * control
     */
    union bq769x2_reg_mfg_status mfg_status;
    err |= bq769x2_subcmd_read_u2(dev, BQ769X2_SUBCMD_MFG_STATUS, &mfg_status.u16);
    if (!err && mfg_status.FET_EN == 0) {
        /*
         * FET_ENABLE subcommand sets FET_EN bit in MFG_STATUS and MFG_STATUS_INIT
         * registers
         */
        err |= bq769x2_subcmd_cmd_only(dev, BQ769X2_SUBCMD_FET_ENABLE);
    }

    /* Disable sleep mode to avoid switching off the CHG MOSFET automatically */
    err |= bq769x2_datamem_write_u2(dev, BQ769X2_SET_CONF_POWER, 0x2882);

    /* Set ideal diode threshold to 500 mA (default was 50 mA) */
    err |= bq769x2_datamem_write_i2(dev, BQ769X2_SET_PROT_BODY_DIODE_TH, 500);

    if (config->auto_pdsg) {
        /* Enable automatic pre-discharge before switching on DSG FETs */
        err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_FET_OPTIONS, 0x1D);

        /*
         * Disable pre-discharge timeout (DSG FETs are only turned on based on bus voltage).
         * PDSG voltage settings in BQ769X2_SET_FET_PDSG_STOP_DV are kept at default 500 mV.
         */
        err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_FET_PDSG_TIMEOUT, 0);
    }
    else {
        /* Disable automatic pre-discharge before switching on DSG FETs */
        err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_FET_OPTIONS, 0x0D);
    }

    /* Configure multi-function pins (e.g. thermistor inputs) */
    for (int i = 0; i < ARRAY_SIZE(config->pin_config); i++) {
        /* pin_config array is structured same as the config in memory */
        err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_CONF_CFETOFF + i, config->pin_config[i]);
    }

    /* Configure REG1 and REG2 voltages and default enable/disable setting */
    err |= bq769x2_datamem_write_u1(dev, BQ769X2_SET_CONF_REG12, config->reg12_config);

    err |= bq769x2_detect_cells(dev);

    return err == 0 ? 0 : -EIO;
}

static int bms_ic_bq769x2_configure(const struct device *dev, struct bms_ic_conf *ic_conf,
                                    uint32_t flags)
{
    uint32_t actual_flags = 0;
    int err = 0;

    err |= bq769x2_config_update_mode(dev, true);

    if (flags & BMS_IC_CONF_VOLTAGE_LIMITS) {
        err |= bq769x2_configure_cell_ovp(dev, ic_conf);
        err |= bq769x2_configure_cell_uvp(dev, ic_conf);
        actual_flags |= BMS_IC_CONF_VOLTAGE_LIMITS;
    }

    if (flags & BMS_IC_CONF_TEMP_LIMITS) {
        err |= bq769x2_configure_temp_limits(dev, ic_conf);
        actual_flags |= BMS_IC_CONF_TEMP_LIMITS;
    }

#ifdef CONFIG_BMS_IC_CURRENT_MONITORING
    if (flags & BMS_IC_CONF_CURRENT_LIMITS) {
        err |= bq769x2_configure_chg_ocp(dev, ic_conf);
        err |= bq769x2_configure_dis_ocp(dev, ic_conf);
        err |= bq769x2_configure_dis_scp(dev, ic_conf);
        actual_flags |= BMS_IC_CONF_CURRENT_LIMITS;
    }
#endif /* CONFIG_BMS_IC_CURRENT_MONITORING */

    if (flags & BMS_IC_CONF_BALANCING) {
        err |= bq769x2_configure_balancing(dev, ic_conf);
        actual_flags |= BMS_IC_CONF_BALANCING;
    }

    if (flags & BMS_IC_CONF_ALERTS) {
        err |= bq769x2_configure_alerts(dev, ic_conf);
        actual_flags |= BMS_IC_CONF_ALERTS;
    }

    if (flags & BMS_IC_CONF_VOLTAGE_REGS) {
        err |= bq769x2_configure_voltage_regs(dev, ic_conf);
        actual_flags |= BMS_IC_CONF_VOLTAGE_REGS;
    }

    err |= bq769x2_config_update_mode(dev, false);

    if (err != 0) {
        return -EIO;
    }

    return (actual_flags != 0) ? actual_flags : -ENOTSUP;
}

static int bq769x2_read_cell_voltages(const struct device *dev, struct bms_ic_data *ic_data)
{
    const struct bms_ic_bq769x2_config *dev_config = dev->config;
    int16_t voltage = 0;
    uint8_t conn_cells = 0;
    int cell_index = 0;
    float sum_voltages = 0;
    float v_max = 0, v_min = 10;
    int err = 0;

    int last_cell = find_msb_set(dev_config->used_cell_channels);
    for (int i = 0; i < last_cell; i++) {
        if (dev_config->used_cell_channels & BIT(i)) {
            if (cell_index >= CONFIG_BMS_IC_MAX_CELLS) {
                return -EINVAL;
            }

            err |= bq769x2_direct_read_i2(dev, BQ769X2_CMD_VOLTAGE_CELL_1 + i * 2, &voltage);
            ic_data->cell_voltages[cell_index] = voltage * 1e-3F; // unit: 1 mV

            if (ic_data->cell_voltages[cell_index] > 0.5F) {
                conn_cells++;
                sum_voltages += ic_data->cell_voltages[cell_index];
            }
            if (ic_data->cell_voltages[cell_index] > v_max) {
                v_max = ic_data->cell_voltages[cell_index];
            }
            if (ic_data->cell_voltages[cell_index] < v_min
                && ic_data->cell_voltages[cell_index] > 0.5F)
            {
                v_min = ic_data->cell_voltages[cell_index];
            }
            cell_index++;
        }
    }

    ic_data->connected_cells = conn_cells;
    ic_data->cell_voltage_avg = sum_voltages / conn_cells;
    ic_data->cell_voltage_min = v_min;
    ic_data->cell_voltage_max = v_max;

    return err == 0 ? 0 : -EIO;
}

static int bq769x2_read_total_voltages(const struct device *dev, struct bms_ic_data *ic_data)
{
    int16_t voltage = 0;
    int err;

    err = bq769x2_direct_read_i2(dev, BQ769X2_CMD_VOLTAGE_STACK, &voltage);
    ic_data->total_voltage = voltage * 1e-2F; /* unit: 10 mV */

#ifdef CONFIG_BMS_IC_SWITCHES
    err |= bq769x2_direct_read_i2(dev, BQ769X2_CMD_VOLTAGE_PACK, &voltage);
    ic_data->external_voltage = voltage * 1e-2F; /* unit: 10 mV */
#endif

    return err == 0 ? 0 : -EIO;
}

static int bq769x2_read_temperatures(const struct device *dev, struct bms_ic_data *ic_data)
{
    const struct bms_ic_bq769x2_config *config = dev->config;
    int16_t temp = 0; /* unit: 0.1 K */
    float sum_temps = 0;
    float temp_max = -1000, temp_min = 1000;
    int err = 0;

    for (int i = 0; i < config->num_cell_temps; i++) {
        /*
         * The pins are numbered in the same order as the registers and each value uses
         * two bytes. CFETOFF is the first temperature sensor register, which is used to
         * calculate the offsets for the following sensors.
         */
        err |= bq769x2_direct_read_i2(
            dev, BQ769X2_CMD_TEMP_CFETOFF + config->cell_temp_pins[i] * 2U, &temp);
        ic_data->cell_temps[i] = (temp * 0.1F) - 273.15F;
        sum_temps += ic_data->cell_temps[i];
        if (ic_data->cell_temps[i] > temp_max) {
            temp_max = ic_data->cell_temps[i];
        }
        if (ic_data->cell_temps[i] < temp_min) {
            temp_min = ic_data->cell_temps[i];
        }
    }

    ic_data->used_thermistors = config->num_cell_temps;
    ic_data->cell_temp_avg = sum_temps / config->num_cell_temps;
    ic_data->cell_temp_min = temp_min;
    ic_data->cell_temp_max = temp_max;

    err |= bq769x2_direct_read_i2(dev, BQ769X2_CMD_TEMP_INT, &temp);
    ic_data->ic_temp = (temp * 0.1F) - 273.15F;

#ifdef CONFIG_BMS_IC_SWITCHES
    /* Read MOSFET temperature if a pin was defined in Devicetree */
    if (config->fet_temp_pin < ARRAY_SIZE(config->pin_config)) {
        err |= bq769x2_direct_read_i2(dev, BQ769X2_CMD_TEMP_CFETOFF + config->fet_temp_pin * 2U,
                                      &temp);
        ic_data->mosfet_temp = (temp * 0.1F) - 273.15F;
    }
#endif

    return err == 0 ? 0 : -EIO;
}

#ifdef CONFIG_BMS_IC_CURRENT_MONITORING

static int bq769x2_read_current(const struct device *dev, struct bms_ic_data *ic_data)
{
    int16_t current = 0;
    int err;

    err = bq769x2_direct_read_i2(dev, BQ769X2_CMD_CURRENT_CC2, &current);
    ic_data->current = current * 1e-2F;

    return err;
}

#endif /* CONFIG_BMS_IC_CURRENT_MONITORING */

static int bq769x2_read_balancing(const struct device *dev, struct bms_ic_data *ic_data)
{
    uint16_t balancing_status;
    int err;

    err = bq769x2_subcmd_read_u2(dev, BQ769X2_SUBCMD_CB_ACTIVE_CELLS, &balancing_status);

    if (!err) {
        ic_data->balancing_status = balancing_status;
    }

    return err;
}

static int bq769x2_read_error_flags(const struct device *dev, struct bms_ic_data *ic_data)
{
    union bq769x2_reg_safety_a safety_status_a;
    union bq769x2_reg_safety_b safety_status_b;
    union bq769x2_reg_safety_c safety_status_c;
    uint32_t error_flags = 0;
    int err;

    /*
     * Safety alert: immediately set if a fault condition occured
     * Safety fault (status registers): only set if alert persists for specified time
     */

    err = bq769x2_direct_read_u1(dev, BQ769X2_CMD_SAFETY_STATUS_A, &safety_status_a.byte);
    err |= bq769x2_direct_read_u1(dev, BQ769X2_CMD_SAFETY_STATUS_B, &safety_status_b.byte);
    err |= bq769x2_direct_read_u1(dev, BQ769X2_CMD_SAFETY_STATUS_C, &safety_status_c.byte);
    if (err) {
        return -EIO;
    }

    error_flags |= (safety_status_a.CUV * UINT32_MAX) & BMS_ERR_CELL_UNDERVOLTAGE;
    error_flags |= (safety_status_a.COV * UINT32_MAX) & BMS_ERR_CELL_OVERVOLTAGE;
    error_flags |= (safety_status_a.SCD * UINT32_MAX) & BMS_ERR_SHORT_CIRCUIT;
    error_flags |= (safety_status_a.OCD1 * UINT32_MAX) & BMS_ERR_DIS_OVERCURRENT;
    error_flags |= (safety_status_a.OCC * UINT32_MAX) & BMS_ERR_CHG_OVERCURRENT;
    error_flags |= (safety_status_b.UTD * UINT32_MAX) & BMS_ERR_DIS_UNDERTEMP;
    error_flags |= (safety_status_b.OTD * UINT32_MAX) & BMS_ERR_DIS_OVERTEMP;
    error_flags |= (safety_status_b.UTC * UINT32_MAX) & BMS_ERR_CHG_UNDERTEMP;
    error_flags |= (safety_status_b.OTC * UINT32_MAX) & BMS_ERR_CHG_OVERTEMP;
    error_flags |= (safety_status_b.OTINT * UINT32_MAX) & BMS_ERR_INT_OVERTEMP;
    error_flags |= (safety_status_b.OTF * UINT32_MAX) & BMS_ERR_FET_OVERTEMP;

    ic_data->error_flags = error_flags;

    return 0;
}

static int bms_ic_bq769x2_read_data(const struct device *dev, uint32_t flags)
{
    struct bms_ic_bq769x2_data *dev_data = dev->data;
    struct bms_ic_data *ic_data = dev_data->ic_data;
    uint32_t actual_flags = 0;
    int err = 0;

    if (ic_data == NULL) {
        return -ENOMEM;
    }

    if (flags & BMS_IC_DATA_CELL_VOLTAGES) {
        err |= bq769x2_read_cell_voltages(dev, ic_data);
        actual_flags |= BMS_IC_DATA_CELL_VOLTAGES;
    }

    if (flags & BMS_IC_DATA_PACK_VOLTAGES) {
        err |= bq769x2_read_total_voltages(dev, ic_data);
        actual_flags |= BMS_IC_DATA_PACK_VOLTAGES;
    }

    if (flags & BMS_IC_DATA_TEMPERATURES) {
        err |= bq769x2_read_temperatures(dev, ic_data);
        actual_flags |= BMS_IC_DATA_TEMPERATURES;
    }

#ifdef CONFIG_BMS_IC_CURRENT_MONITORING
    if (flags & BMS_IC_DATA_CURRENT) {
        err |= bq769x2_read_current(dev, ic_data);
        actual_flags |= BMS_IC_DATA_CURRENT;
    }
#endif /* CONFIG_BMS_IC_CURRENT_MONITORING */

    if (flags & BMS_IC_DATA_BALANCING) {
        err |= bq769x2_read_balancing(dev, ic_data);
        actual_flags |= BMS_IC_DATA_BALANCING;
    }

    if (flags & BMS_IC_DATA_ERROR_FLAGS) {
        err |= bq769x2_read_error_flags(dev, ic_data);
        actual_flags |= BMS_IC_DATA_ERROR_FLAGS;
    }

    if (err != 0) {
        return -EIO;
    }

    return (flags == actual_flags) ? 0 : -EINVAL;
}

static void bms_ic_bq769x2_assign_data(const struct device *dev, struct bms_ic_data *ic_data)
{
    struct bms_ic_bq769x2_data *dev_data = dev->data;

    dev_data->ic_data = ic_data;
}

#ifdef CONFIG_BMS_IC_SWITCHES

static int bms_ic_bq769x2_set_switches(const struct device *dev, uint8_t switches, bool enabled)
{
    union bq769x2_reg_fet_status fet_status;
    int err;

    err = bq769x2_direct_read_u1(dev, BQ769X2_CMD_FET_STATUS, &fet_status.byte);
    if (err != 0) {
        return err;
    }

    /* only lower 4 bytes relevant for FET_CONTROL */
    fet_status.byte &= 0x0F;

    if (switches & BMS_SWITCH_CHG) {
        fet_status.CHG_FET = enabled ? 1 : 0;
    }
    if (switches & BMS_SWITCH_DIS) {
        fet_status.DSG_FET = enabled ? 1 : 0;
    }
    if (switches & BMS_SWITCH_PDSG) {
        fet_status.PDSG_FET = enabled ? 1 : 0;
    }
    if (switches & BMS_SWITCH_PCHG) {
        fet_status.PCHG_FET = enabled ? 1 : 0;
    }

    err = bq769x2_subcmd_write_u1(dev, BQ769X2_SUBCMD_FET_CONTROL, fet_status.byte);

    if (enabled) {
        err |= bq769x2_subcmd_cmd_only(dev, BQ769X2_SUBCMD_ALL_FETS_ON);
    }

    return err == 0 ? 0 : -EIO;
}

#endif /* CONFIG_BMS_IC_SWITCHES */

static int bms_ic_bq769x2_balance(const struct device *dev, uint32_t cells)
{
    struct bms_ic_bq769x2_data *dev_data = dev->data;

    if (dev_data->auto_balancing) {
        return -EBUSY;
    }

    if (((cells << 1) & cells) || ((cells >> 1) & cells)) {
        /* balancing of adjacent cells not allowed */
        return -EINVAL;
    }

    /* ToDo: Consider bq chip number and gaps in CB_ACTIVE_CELLS */
    return bq769x2_subcmd_write_u2(dev, BQ769X2_SUBCMD_CB_ACTIVE_CELLS, (uint16_t)cells);
}

static int bq769x2_activate(const struct device *dev)
{
    uint16_t device_number;
    int err = 0;

    while (bq769x2_subcmd_read_u2(dev, BQ769X2_SUBCMD_DEVICE_NUMBER, &device_number) != 0) {
        /*
         * For some boards (e.g. with ESP32-C3) the first attempt to configure the IC may fail
         * because of glitches in the I2C communication after MCU reset. Retry multiple times.
         */
        LOG_ERR("Failed to read from BMS IC with error %d, retrying in 10s", err);
        k_sleep(K_MSEC(10000));
    }

    err |= bq769x2_config_update_mode(dev, true);

    err |= bq769x2_init_config(dev);
    if (err != 0) {
        return -EIO;
    }

    err |= bq769x2_config_update_mode(dev, false);

    LOG_INF("Activated BMS IC 0x%x", device_number);

    return err == 0 ? 0 : -EIO;
}

static int bms_ic_bq769x2_set_mode(const struct device *dev, enum bms_ic_mode mode)
{
    switch (mode) {
        case BMS_IC_MODE_ACTIVE:
            return bq769x2_activate(dev);
        case BMS_IC_MODE_OFF:
            return bq769x2_subcmd_cmd_only(dev, BQ769X2_SUBCMD_SHUTDOWN);
        default:
            return -ENOTSUP;
    }
}

static int bq769x2_init(const struct device *dev)
{
    const struct bms_ic_bq769x2_config *config = dev->config;

    if (!i2c_is_ready_dt(&config->i2c)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }

    /* Datasheet: Start-up time max. 4.3 ms */
    k_sleep(K_TIMEOUT_ABS_MS(5));

    return 0;
}

static const struct bms_ic_driver_api bq769x2_driver_api = {
    .configure = bms_ic_bq769x2_configure,
    .assign_data = bms_ic_bq769x2_assign_data,
    .read_data = bms_ic_bq769x2_read_data,
#ifdef CONFIG_BMS_IC_SWITCHES
    .set_switches = bms_ic_bq769x2_set_switches,
#endif
    .balance = bms_ic_bq769x2_balance,
    .set_mode = bms_ic_bq769x2_set_mode,
};

#define BQ769X2_ASSERT_CURRENT_MONITORING_PROP_GREATER_ZERO(index, prop) \
    BUILD_ASSERT(COND_CODE_0(IS_ENABLED(CONFIG_BMS_IC_CURRENT_MONITORING), (1), \
                             (DT_INST_PROP_OR(index, prop, 0) > 0)), \
                 "Devicetree properties shunt-resistor-uohm and board-max-current " \
                 "must be greater than 0 for CONFIG_BMS_IC_CURRENT_MONITORING=y")

#define BQ769X2_INIT(index) \
    static struct bms_ic_bq769x2_data bq769x2_data_##index = { 0 }; \
    BQ769X2_ASSERT_CURRENT_MONITORING_PROP_GREATER_ZERO(index, shunt_resistor_uohm); \
    BQ769X2_ASSERT_CURRENT_MONITORING_PROP_GREATER_ZERO(index, board_max_current); \
    static const struct bms_ic_bq769x2_config bq769x2_config_##index = {                   \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
		.alert_gpio = GPIO_DT_SPEC_INST_GET(index, alert_gpios),                           \
		.shunt_resistor_uohm = DT_INST_PROP_OR(index, shunt_resistor_uohm, 1000),          \
		.board_max_current = DT_INST_PROP_OR(index, board_max_current, 0),                 \
		.used_cell_channels = DT_INST_PROP(index, used_cell_channels),                     \
		.pin_config =                                                                      \
			{                                                                              \
				DT_INST_PROP_OR(index, cfetoff_pin_config, 0),                             \
				DT_INST_PROP_OR(index, dfetoff_pin_config, 0),                             \
				DT_INST_PROP_OR(index, alert_pin_config, 0),                               \
				DT_INST_PROP_OR(index, ts1_pin_config, 0x07),                              \
				DT_INST_PROP_OR(index, ts2_pin_config, 0),                                 \
				DT_INST_PROP_OR(index, ts3_pin_config, 0),                                 \
				DT_INST_PROP_OR(index, hdq_pin_config, 0),                                 \
				DT_INST_PROP_OR(index, dchg_pin_config, 0),                                \
				DT_INST_PROP_OR(index, ddsg_pin_config, 0),                                \
			},                                                                             \
		.cell_temp_pins = DT_INST_PROP(index, cell_temp_pins),                             \
		.num_cell_temps = DT_INST_PROP_LEN(index, cell_temp_pins),                         \
		.fet_temp_pin = DT_INST_PROP_OR(index, fet_temp_pin, UINT8_MAX),                   \
		.crc_enabled = DT_INST_PROP(index, crc_enabled),                                   \
		.auto_pdsg = DT_INST_PROP(index, auto_pdsg),                                       \
		.reg12_config = DT_INST_PROP(index, reg12_config),                                 \
		.write_bytes = bq769x2_write_bytes_i2c,                                            \
		.read_bytes = bq769x2_read_bytes_i2c,                                              \
	}; \
    DEVICE_DT_INST_DEFINE(index, &bq769x2_init, NULL, &bq769x2_data_##index, \
                          &bq769x2_config_##index, POST_KERNEL, CONFIG_BMS_IC_INIT_PRIORITY, \
                          &bq769x2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ769X2_INIT)
