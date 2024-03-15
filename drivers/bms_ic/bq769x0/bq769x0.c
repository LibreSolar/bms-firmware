/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq769x0

#include "bq769x0_registers.h"

#include <bms/bms_common.h>
#include <drivers/bms_ic.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> /* for abs() function */
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>

LOG_MODULE_REGISTER(bms_ic_bq769x0, CONFIG_BMS_IC_LOG_LEVEL);

#define BQ769X0_READ_MAX_ATTEMPTS (10)

/* read-only driver configuration */
struct bms_ic_bq769x0_config
{
    struct i2c_dt_spec i2c;
    struct gpio_dt_spec alert_gpio;
    struct gpio_dt_spec bus_pchg_gpio;
    uint32_t shunt_resistor_uohm;
    uint32_t board_max_current;
    uint16_t used_cell_channels;
    uint16_t thermistor_beta;
    uint8_t num_sections;
};

/* driver run-time data */
struct bms_ic_bq769x0_data
{
    struct bms_ic_data *ic_data;
    const struct device *dev;
    struct k_work_delayable alert_work;
    struct k_work_delayable balancing_work;
    /** ADC gain, factory-calibrated, read out from chip (uV/LSB) */
    int adc_gain;
    /** ADC offset, factory-calibrated, read out from chip (mV) */
    int adc_offset;
    struct gpio_callback alert_cb;
    /** Cached data from struct bms_ic_conf required for handling errors etc. in sofware */
    struct
    {
        /* Cell voltage limits */
        float cell_ov_reset;
        float cell_uv_reset;

        /* Cell temperature limits */
        float dis_ot_limit;
        float dis_ut_limit;
        float chg_ot_limit;
        float chg_ut_limit;
        float temp_limit_hyst;

#ifdef CONFIG_BMS_IC_CURRENT_MONITORING
        /* Current limits */
        float chg_oc_limit;
        uint32_t chg_oc_delay_ms;
#endif

        /* Balancing settings */
        float bal_cell_voltage_diff;
        float bal_cell_voltage_min;
        float bal_idle_current;
        uint16_t bal_idle_delay;
        bool auto_balancing;

        uint32_t alert_mask;
    } ic_conf;
    int64_t active_timestamp;
    union bq769x0_sys_stat sys_stat_prev;
    int error_seconds_counter;
    uint32_t balancing_status;
    bool crc_enabled;
};

static int bq769x0_set_balancing_switches(const struct device *dev, uint32_t cells);

/*
 * The bq769x0 drives the ALERT pin high if the SYS_STAT register contains
 * a new value (either new CC reading or an error)
 */
static void bq769x0_alert_isr(const struct device *port, struct gpio_callback *cb,
                              gpio_port_pins_t pins)
{
    struct bms_ic_bq769x0_data *data = CONTAINER_OF(cb, struct bms_ic_bq769x0_data, alert_cb);

    k_work_schedule(&data->alert_work, K_NO_WAIT);
}

static int bq769x0_write_byte(const struct device *dev, uint8_t reg_addr, uint8_t data)
{
    const struct bms_ic_bq769x0_config *dev_config = dev->config;
    const struct bms_ic_bq769x0_data *dev_data = dev->data;

    uint8_t buf[4] = {
        dev_config->i2c.addr << 1, /* target address for CRC calculation */
        reg_addr,
        data,
    };

    if (dev_data->crc_enabled) {
        buf[3] = crc8_ccitt(0, buf, 3);
        return i2c_write_dt(&dev_config->i2c, buf + 1, 3);
    }
    else {
        return i2c_write_dt(&dev_config->i2c, buf + 1, 2);
    }
}

static int bq769x0_read_bytes(const struct device *dev, uint8_t reg_addr, uint8_t *data,
                              size_t num_bytes)
{
    const struct bms_ic_bq769x0_config *dev_config = dev->config;
    const struct bms_ic_bq769x0_data *dev_data = dev->data;
    uint8_t buf[5] = {
        (dev_config->i2c.addr << 1) | 1U, /* target address for CRC calculation */
    };
    int err;

    if (num_bytes < 1 || num_bytes > 2) {
        return -EINVAL;
    }

    if (dev_data->crc_enabled) {
        for (int attempts = 1; attempts <= BQ769X0_READ_MAX_ATTEMPTS; attempts++) {
            err = i2c_write_read_dt(&dev_config->i2c, &reg_addr, 1, buf + 1, num_bytes * 2);
            if (err != 0) {
                return err;
            }

            /*
             * First CRC includes target address (incl. R/W bit) and data byte, subsequent CRCs
             * only consider data.
             */
            if (crc8_ccitt(0, buf, 2) == buf[2]) {
                data[0] = buf[1];
                if (num_bytes == 1) {
                    return 0;
                }
                else if (crc8_ccitt(0, buf + 3, 1) == buf[4]) {
                    data[1] = buf[3];
                    return 0;
                }
            }
        }

        LOG_ERR("Failed to read 0x%02X after %d attempts", reg_addr, BQ769X0_READ_MAX_ATTEMPTS);
        return -EIO;
    }
    else {
        return i2c_write_read_dt(&dev_config->i2c, &reg_addr, 1, data, num_bytes);
    }
}

static inline int bq769x0_read_byte(const struct device *dev, uint8_t reg_addr, uint8_t *byte)
{
    return bq769x0_read_bytes(dev, reg_addr, byte, sizeof(uint8_t));
}

static inline int bq769x0_read_word(const struct device *dev, uint8_t reg_addr, uint16_t *word)
{
    uint8_t buf[2];
    int err;

    err = bq769x0_read_bytes(dev, reg_addr, buf, sizeof(buf));
    if (err == 0) {
        *word = buf[0] << 8 | buf[1];
    }

    return err;
}

static int bq769x0_detect_crc(const struct device *dev)
{
    struct bms_ic_bq769x0_data *dev_data = dev->data;
    uint8_t cc_cfg = 0;
    int err = 0;

    dev_data->crc_enabled = true;
    err |= bq769x0_write_byte(dev, BQ769X0_CC_CFG, 0x19);
    err |= bq769x0_read_byte(dev, BQ769X0_CC_CFG, &cc_cfg);
    if (err == 0 && cc_cfg == 0x19) {
        return 0;
    }

    dev_data->crc_enabled = false;
    err |= bq769x0_write_byte(dev, BQ769X0_CC_CFG, 0x19);
    err |= bq769x0_read_byte(dev, BQ769X0_CC_CFG, &cc_cfg);
    if (err == 0 && cc_cfg == 0x19) {
        return 0;
    }

    return -EIO;
}

static int bq769x0_configure_cell_ovp(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    struct bms_ic_bq769x0_data *dev_data = dev->data;
    union bq769x0_protect3 protect3;
    int ov_trip = 0;
    int err;

    err = bq769x0_read_byte(dev, BQ769X0_PROTECT3, &protect3.byte);
    if (err != 0) {
        return err;
    }

    ov_trip = ((((long)(ic_conf->cell_ov_limit * 1000) - dev_data->adc_offset) * 1000
                / dev_data->adc_gain)
               >> 4)
              & 0x00FF;
    err = bq769x0_write_byte(dev, BQ769X0_OV_TRIP, ov_trip);
    if (err != 0) {
        return err;
    }

    protect3.OV_DELAY = 0;
    for (int i = ARRAY_SIZE(bq769x0_ov_delays) - 1; i > 0; i--) {
        if (ic_conf->cell_ov_delay_ms >= bq769x0_ov_delays[i]) {
            protect3.OV_DELAY = i;
            break;
        }
    }

    err = bq769x0_write_byte(dev, BQ769X0_PROTECT3, protect3.byte);
    if (err != 0) {
        return err;
    }

    /* store actually configured values */
    ic_conf->cell_ov_limit =
        (1U << 13 | ov_trip << 4) * dev_data->adc_gain / 1000 + dev_data->adc_offset;
    ic_conf->cell_ov_delay_ms = bq769x0_ov_delays[protect3.OV_DELAY];

    return 0;
}

static int bq769x0_configure_cell_uvp(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    struct bms_ic_bq769x0_data *dev_data = dev->data;
    union bq769x0_protect3 protect3;
    int uv_trip = 0;
    int err;

    err = bq769x0_read_byte(dev, BQ769X0_PROTECT3, &protect3.byte);
    if (err != 0) {
        return err;
    }

    uv_trip = ((((long)(ic_conf->cell_uv_limit * 1000) - dev_data->adc_offset) * 1000
                / dev_data->adc_gain)
               >> 4)
              & 0x00FF;
    uv_trip += 1; /* always round up for lower cell voltage */
    err = bq769x0_write_byte(dev, BQ769X0_UV_TRIP, uv_trip);
    if (err != 0) {
        return err;
    }

    protect3.UV_DELAY = 0;
    for (int i = ARRAY_SIZE(bq769x0_uv_delays) - 1; i > 0; i--) {
        if (ic_conf->cell_uv_delay_ms >= bq769x0_uv_delays[i]) {
            protect3.UV_DELAY = i;
            break;
        }
    }

    err = bq769x0_write_byte(dev, BQ769X0_PROTECT3, protect3.byte);
    if (err != 0) {
        return err;
    }

    /* store actually configured values */
    ic_conf->cell_uv_limit =
        (1U << 12 | uv_trip << 4) * dev_data->adc_gain / 1000 + dev_data->adc_offset;
    ic_conf->cell_uv_delay_ms = bq769x0_uv_delays[protect3.UV_DELAY];

    return 0;
}

static int bq769x0_configure_temp_limits(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    struct bms_ic_bq769x0_data *dev_data = dev->data;

    dev_data->ic_conf.dis_ot_limit = ic_conf->dis_ot_limit;
    dev_data->ic_conf.dis_ut_limit = ic_conf->dis_ut_limit;
    dev_data->ic_conf.chg_ot_limit = ic_conf->chg_ot_limit;
    dev_data->ic_conf.chg_ut_limit = ic_conf->chg_ut_limit;
    dev_data->ic_conf.temp_limit_hyst = ic_conf->temp_limit_hyst;

    return 0;
}

#ifdef CONFIG_BMS_IC_CURRENT_MONITORING

static int bq769x0_configure_chg_ocp(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    struct bms_ic_bq769x0_data *dev_data = dev->data;

    dev_data->ic_conf.chg_oc_limit = ic_conf->chg_oc_limit;
    dev_data->ic_conf.chg_oc_delay_ms = ic_conf->chg_oc_delay_ms;

    return 0;
}

static int bq769x0_configure_dis_ocp(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    const struct bms_ic_bq769x0_config *dev_config = dev->config;
    union bq769x0_protect2 protect2;
    int err;

    /* Remark: RSNS must be set to 1 in PROTECT1 register */

    protect2.OCD_THRESH = 0;
    for (int i = ARRAY_SIZE(bq769x0_ocd_thresholds) - 1; i > 0; i--) {
        if (ic_conf->dis_oc_limit * (dev_config->shunt_resistor_uohm / 1000.0F)
            >= bq769x0_ocd_thresholds[i])
        {
            protect2.OCD_THRESH = i;
            break;
        }
    }

    protect2.OCD_DELAY = 0;
    for (int i = ARRAY_SIZE(bq769x0_ocd_delays) - 1; i > 0; i--) {
        if (ic_conf->dis_oc_delay_ms >= bq769x0_ocd_delays[i]) {
            protect2.OCD_DELAY = i;
            break;
        }
    }

    err = bq769x0_write_byte(dev, BQ769X0_PROTECT2, protect2.byte);
    if (err != 0) {
        return err;
    }

    /* store actually configured values */
    ic_conf->dis_oc_limit =
        bq769x0_ocd_thresholds[protect2.OCD_THRESH] / (dev_config->shunt_resistor_uohm / 1000.0F);
    ic_conf->dis_oc_delay_ms = bq769x0_ocd_delays[protect2.OCD_DELAY];

    return 0;
}

static int bq769x0_configure_dis_scp(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    const struct bms_ic_bq769x0_config *dev_config = dev->config;
    union bq769x0_protect1 protect1;
    int err;

    /* only RSNS = 1 considered */
    protect1.RSNS = 1;

    protect1.SCD_THRESH = 0;
    for (int i = ARRAY_SIZE(bq769x0_scd_thresholds) - 1; i > 0; i--) {
        if (ic_conf->dis_sc_limit * (dev_config->shunt_resistor_uohm / 1000.0F)
            >= bq769x0_scd_thresholds[i])
        {
            protect1.SCD_THRESH = i;
            break;
        }
    }

    protect1.SCD_DELAY = 0;
    for (int i = ARRAY_SIZE(bq769x0_scd_delays) - 1; i > 0; i--) {
        if (ic_conf->dis_sc_delay_us >= bq769x0_scd_delays[i]) {
            protect1.SCD_DELAY = i;
            break;
        }
    }

    err = bq769x0_write_byte(dev, BQ769X0_PROTECT1, protect1.byte);
    if (err != 0) {
        return err;
    }

    /* store actually configured values */
    ic_conf->dis_sc_limit =
        bq769x0_scd_thresholds[protect1.SCD_THRESH] / (dev_config->shunt_resistor_uohm / 1000.0F);
    ic_conf->dis_sc_delay_us = bq769x0_scd_delays[protect1.SCD_DELAY];

    return 0;
}

#endif /* CONFIG_BMS_IC_CURRENT_MONITORING */

static int bq769x0_configure_balancing(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    struct bms_ic_bq769x0_data *dev_data = dev->data;
    struct k_work_sync work_sync;

    dev_data->ic_conf.bal_cell_voltage_diff = ic_conf->bal_cell_voltage_diff;
    dev_data->ic_conf.bal_cell_voltage_min = ic_conf->bal_cell_voltage_min;
    dev_data->ic_conf.bal_idle_current = ic_conf->bal_idle_current;
    dev_data->ic_conf.bal_idle_delay = ic_conf->bal_idle_delay;
    dev_data->ic_conf.auto_balancing = ic_conf->auto_balancing;

    if (ic_conf->auto_balancing) {
        k_work_schedule(&dev_data->balancing_work, K_NO_WAIT);
        return 0;
    }
    else {
        k_work_cancel_delayable_sync(&dev_data->balancing_work, &work_sync);
        return bq769x0_set_balancing_switches(dev, 0x0);
    }
}

static int bq769x0_configure_alerts(const struct device *dev, struct bms_ic_conf *ic_conf)
{
    struct bms_ic_bq769x0_data *dev_data = dev->data;

    dev_data->ic_conf.alert_mask = ic_conf->alert_mask;

    return 0;
}

static int bms_ic_bq769x0_configure(const struct device *dev, struct bms_ic_conf *ic_conf,
                                    uint32_t flags)
{
    uint32_t actual_flags = 0;
    int err = 0;

    if (flags & BMS_IC_CONF_VOLTAGE_LIMITS) {
        err |= bq769x0_configure_cell_ovp(dev, ic_conf);
        err |= bq769x0_configure_cell_uvp(dev, ic_conf);
        actual_flags |= BMS_IC_CONF_VOLTAGE_LIMITS;
    }

    if (flags & BMS_IC_CONF_TEMP_LIMITS) {
        err |= bq769x0_configure_temp_limits(dev, ic_conf);
        actual_flags |= BMS_IC_CONF_TEMP_LIMITS;
    }

#ifdef CONFIG_BMS_IC_CURRENT_MONITORING
    if (flags & BMS_IC_CONF_CURRENT_LIMITS) {
        err |= bq769x0_configure_chg_ocp(dev, ic_conf);
        err |= bq769x0_configure_dis_ocp(dev, ic_conf);
        err |= bq769x0_configure_dis_scp(dev, ic_conf);
        actual_flags |= BMS_IC_CONF_CURRENT_LIMITS;
    }
#endif /* CONFIG_BMS_IC_CURRENT_MONITORING */

    if (flags & BMS_IC_CONF_BALANCING) {
        err |= bq769x0_configure_balancing(dev, ic_conf);
        actual_flags |= BMS_IC_CONF_BALANCING;
    }

    if (flags & BMS_IC_CONF_ALERTS) {
        err |= bq769x0_configure_alerts(dev, ic_conf);
        actual_flags |= BMS_IC_CONF_ALERTS;
    }

    if (err != 0) {
        return -EIO;
    }

    return (actual_flags != 0) ? actual_flags : -ENOTSUP;
}

static int bq769x0_read_cell_voltages(const struct device *dev, struct bms_ic_data *ic_data)
{
    const struct bms_ic_bq769x0_config *dev_config = dev->config;
    struct bms_ic_bq769x0_data *dev_data = dev->data;
    uint16_t adc_raw;
    int conn_cells = 0;
    float sum_voltages = 0;
    float v_max = 0, v_min = 10;
    int err;

    for (int i = 0; i < dev_config->num_sections * 5; i++) {
        err = bq769x0_read_word(dev, BQ769X0_VC1_HI_BYTE + i * 2, &adc_raw);
        if (err != 0) {
            return err;
        }

        adc_raw &= 0x3FFF;
        ic_data->cell_voltages[i] =
            (adc_raw * dev_data->adc_gain * 1e-3F + dev_data->adc_offset) * 1e-3F;

        if (ic_data->cell_voltages[i] > 0.5F) {
            conn_cells++;
            sum_voltages += ic_data->cell_voltages[i];
        }
        if (ic_data->cell_voltages[i] > v_max) {
            v_max = ic_data->cell_voltages[i];
        }
        if (ic_data->cell_voltages[i] < v_min && ic_data->cell_voltages[i] > 0.5F) {
            v_min = ic_data->cell_voltages[i];
        }
    }
    ic_data->connected_cells = conn_cells;
    ic_data->cell_voltage_avg = sum_voltages / conn_cells;
    ic_data->cell_voltage_min = v_min;
    ic_data->cell_voltage_max = v_max;

    return 0;
}

static int bq769x0_read_total_voltages(const struct device *dev, struct bms_ic_data *ic_data)
{
    struct bms_ic_bq769x0_data *dev_data = dev->data;
    uint16_t adc_raw;
    int err;

    err = bq769x0_read_word(dev, BQ769X0_BAT_HI_BYTE, &adc_raw);
    if (err != 0) {
        return err;
    }

    ic_data->total_voltage = (4.0F * dev_data->adc_gain * adc_raw * 1e-3F
                              + ic_data->connected_cells * dev_data->adc_offset)
                             * 1e-3F;

    return 0;
}

static int bq769x0_read_temperatures(const struct device *dev, struct bms_ic_data *ic_data)
{
    const struct bms_ic_bq769x0_config *dev_config = dev->config;
    float tmp = 0;
    uint16_t adc_raw = 0;
    int vtsx = 0;
    unsigned long rts = 0;
    float sum_temps = 0;
    int num_temps = 0;
    int err;

    for (int i = 0; i < dev_config->num_sections && i < CONFIG_BMS_IC_MAX_THERMISTORS; i++) {
        /* calculate R_thermistor according to bq769x0 datasheet */
        err = bq769x0_read_word(dev, BQ769X0_TS1_HI_BYTE + i * 2, &adc_raw);
        if (err != 0) {
            return err;
        }

        adc_raw &= 0x3FFF;
        vtsx = adc_raw * 0.382F;                 /* mV */
        rts = 10000.0F * vtsx / (3300.0 - vtsx); /* Ohm */

        /*
         * Temperature calculation using Beta equation
         * - According to bq769x0 datasheet, only 10k thermistors should be used
         * - 25°C reference temperature for Beta equation assumed
         */
        tmp = 1.0F
              / (1.0F / (273.15F + 25) + 1.0F / dev_config->thermistor_beta * log(rts / 10000.0F));
        ic_data->cell_temps[i] = tmp - 273.15F;
        if (i == 0) {
            ic_data->cell_temp_min = ic_data->cell_temps[i];
            ic_data->cell_temp_max = ic_data->cell_temps[i];
        }
        else {
            if (ic_data->cell_temps[i] < ic_data->cell_temp_min) {
                ic_data->cell_temp_min = ic_data->cell_temps[i];
            }
            if (ic_data->cell_temps[i] > ic_data->cell_temp_max) {
                ic_data->cell_temp_max = ic_data->cell_temps[i];
            }
        }
        num_temps++;
        sum_temps += ic_data->cell_temps[i];
    }
    ic_data->cell_temp_avg = sum_temps / num_temps;

    return 0;
}

#ifdef CONFIG_BMS_IC_CURRENT_MONITORING

static int bq769x0_read_current(const struct device *dev, struct bms_ic_data *ic_data)
{
    const struct bms_ic_bq769x0_config *dev_config = dev->config;
    struct bms_ic_bq769x0_data *dev_data = dev->data;
    uint16_t adc_raw;

    int err = bq769x0_read_word(dev, BQ769X0_CC_HI_BYTE, &adc_raw);
    if (err != 0) {
        LOG_ERR("Error reading current measurement");
        return err;
    }

    int32_t current_mA = (int16_t)adc_raw * 8.44F / (dev_config->shunt_resistor_uohm / 1000.0F);

    /* remove noise around 0 A */
    if (current_mA > -10 && current_mA < 10) {
        current_mA = 0;
    }

    ic_data->current = current_mA / 1000.0;

    /* reset active timestamp */
    if (fabs(ic_data->current) > dev_data->ic_conf.bal_idle_current) {
        dev_data->active_timestamp = k_uptime_get();
    }

    return 0;
}

#endif /* CONFIG_BMS_IC_CURRENT_MONITORING */

static int bq769x0_read_error_flags(const struct device *dev, struct bms_ic_data *ic_data)
{
    const struct bms_ic_bq769x0_data *dev_data = dev->data;
    union bq769x0_sys_stat sys_stat;
    uint32_t error_flags = 0;
    float hyst;

    int err = bq769x0_read_byte(dev, BQ769X0_SYS_STAT, &sys_stat.byte);
    if (err != 0) {
        return err;
    }

    error_flags |= (sys_stat.UV * UINT32_MAX) & BMS_ERR_CELL_UNDERVOLTAGE;
    error_flags |= (sys_stat.OV * UINT32_MAX) & BMS_ERR_CELL_OVERVOLTAGE;
    error_flags |= (sys_stat.SCD * UINT32_MAX) & BMS_ERR_SHORT_CIRCUIT;
    error_flags |= (sys_stat.OCD * UINT32_MAX) & BMS_ERR_DIS_OVERCURRENT;

    if (ic_data->current > dev_data->ic_conf.chg_oc_limit) {
        /* ToDo: consider ic_conf->chg_oc_delay */
        error_flags |= BMS_ERR_CHG_OVERCURRENT;
    }

    hyst = (ic_data->error_flags & BMS_ERR_CHG_OVERTEMP) ? dev_data->ic_conf.temp_limit_hyst : 0;
    if (ic_data->cell_temp_max > dev_data->ic_conf.chg_ot_limit - hyst) {
        error_flags |= BMS_ERR_CHG_OVERTEMP;
    }

    hyst = (ic_data->error_flags & BMS_ERR_CHG_UNDERTEMP) ? dev_data->ic_conf.temp_limit_hyst : 0;
    if (ic_data->cell_temp_min < dev_data->ic_conf.chg_ut_limit + hyst) {
        error_flags |= BMS_ERR_CHG_UNDERTEMP;
    }

    hyst = (ic_data->error_flags & BMS_ERR_DIS_OVERTEMP) ? dev_data->ic_conf.temp_limit_hyst : 0;
    if (ic_data->cell_temp_max > dev_data->ic_conf.dis_ot_limit - hyst) {
        error_flags |= BMS_ERR_DIS_OVERTEMP;
    }

    hyst = (ic_data->error_flags & BMS_ERR_DIS_UNDERTEMP) ? dev_data->ic_conf.temp_limit_hyst : 0;
    if (ic_data->cell_temp_min < dev_data->ic_conf.dis_ut_limit + hyst) {
        error_flags |= BMS_ERR_DIS_UNDERTEMP;
    }

    ic_data->error_flags = error_flags;

    return 0;
}

static void bq769x0_alert_handler(struct k_work *work)
{
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct bms_ic_bq769x0_data *dev_data =
        CONTAINER_OF(dwork, struct bms_ic_bq769x0_data, alert_work);
    const struct device *dev = dev_data->dev;
    struct bms_ic_data *ic_data = dev_data->ic_data;
    int err;

    /* ToDo: Handle also temperature and chg errors (incl. temp hysteresis) */

    union bq769x0_sys_stat sys_stat;
    err = bq769x0_read_byte(dev, BQ769X0_SYS_STAT, &sys_stat.byte);
    if (err != 0) {
        return;
    }

    /* get new current reading if available */
    if (sys_stat.CC_READY == 1) {
        bq769x0_read_current(dev, ic_data);
        err = bq769x0_write_byte(dev, BQ769X0_SYS_STAT, BQ769X0_SYS_STAT_CC_READY);
        if (err != 0) {
            LOG_ERR("Failed to clear CC_READY flag");
        }
    }

    /* handle potential errors */
    if ((sys_stat.byte & BQ769X0_SYS_STAT_ERROR_MASK) != 0) {
        if (dev_data->error_seconds_counter < 0) {
            dev_data->error_seconds_counter = 0;
        }

        err = 0;

        if (sys_stat.DEVICE_XREADY) {
            /* datasheet recommendation: try to clear after waiting a few seconds */
            if (dev_data->error_seconds_counter % 3 == 0) {
                LOG_DBG("Attempting to clear XR error");
                err |= bq769x0_write_byte(dev, BQ769X0_SYS_STAT, BQ769X0_SYS_STAT_DEVICE_XREADY);
            }
        }
        if (sys_stat.OVRD_ALERT) {
            if (dev_data->error_seconds_counter % 10 == 0) {
                LOG_DBG("Attempting to clear Alert error");
                err |= bq769x0_write_byte(dev, BQ769X0_SYS_STAT, BQ769X0_SYS_STAT_OVRD_ALERT);
            }
        }
        if (sys_stat.UV) {
            bms_ic_read_data(dev, BMS_IC_DATA_CELL_VOLTAGES);
            if (ic_data->cell_voltage_min > dev_data->ic_conf.cell_uv_reset) {
                LOG_DBG("Attempting to clear UV error");
                err |= bq769x0_write_byte(dev, BQ769X0_SYS_STAT, BQ769X0_SYS_STAT_UV);
            }
        }
        if (sys_stat.OV) {
            bms_ic_read_data(dev, BMS_IC_DATA_CELL_VOLTAGES);
            if (ic_data->cell_voltage_max < dev_data->ic_conf.cell_ov_reset) {
                LOG_DBG("Attempting to clear OV error");
                err |= bq769x0_write_byte(dev, BQ769X0_SYS_STAT, BQ769X0_SYS_STAT_OV);
            }
        }
        if (sys_stat.SCD) {
            if (dev_data->error_seconds_counter % 60 == 0) {
                LOG_DBG("Attempting to clear SCD error");
                err |= bq769x0_write_byte(dev, BQ769X0_SYS_STAT, BQ769X0_SYS_STAT_SCD);
            }
        }
        if (sys_stat.OCD) {
            if (dev_data->error_seconds_counter % 60 == 0) {
                LOG_DBG("Attempting to clear OCD error");
                err |= bq769x0_write_byte(dev, BQ769X0_SYS_STAT, BQ769X0_SYS_STAT_OCD);
            }
        }

        if (err != 0) {
            LOG_ERR("Attempts to clear error flags failed");
        }

        dev_data->error_seconds_counter++;
        k_work_reschedule(dwork, K_SECONDS(1));
    }
    else {
        dev_data->error_seconds_counter = -1;
    }
}

static int bms_ic_bq769x0_read_data(const struct device *dev, uint32_t flags)
{
    struct bms_ic_bq769x0_data *dev_data = dev->data;
    struct bms_ic_data *ic_data = dev_data->ic_data;
    uint32_t actual_flags = 0;
    int err = 0;

    if (flags & BMS_IC_DATA_CELL_VOLTAGES) {
        err |= bq769x0_read_cell_voltages(dev, ic_data);
        actual_flags |= BMS_IC_DATA_CELL_VOLTAGES;
    }

    if (flags & BMS_IC_DATA_PACK_VOLTAGES) {
        err |= bq769x0_read_total_voltages(dev, ic_data);
        actual_flags |= BMS_IC_DATA_PACK_VOLTAGES;
    }

    if (flags & BMS_IC_DATA_TEMPERATURES) {
        err |= bq769x0_read_temperatures(dev, ic_data);
        actual_flags |= BMS_IC_DATA_TEMPERATURES;
    }

#ifdef CONFIG_BMS_IC_CURRENT_MONITORING
    if (flags & BMS_IC_DATA_CURRENT) {
        err |= bq769x0_read_current(dev, ic_data);
        actual_flags |= BMS_IC_DATA_CURRENT;
    }
#endif /* CONFIG_BMS_IC_CURRENT_MONITORING */

    if (flags & BMS_IC_DATA_BALANCING) {
        ic_data->balancing_status = dev_data->balancing_status;
        actual_flags |= BMS_IC_DATA_BALANCING;
    }

    if (flags & BMS_IC_DATA_ERROR_FLAGS) {
        err |= bq769x0_read_error_flags(dev, ic_data);
        actual_flags |= BMS_IC_DATA_ERROR_FLAGS;
    }

    if (err != 0) {
        return -EIO;
    }

    return (flags == actual_flags) ? 0 : -EINVAL;
}

static void bms_ic_bq769x0_assign_data(const struct device *dev, struct bms_ic_data *ic_data)
{
    struct bms_ic_bq769x0_data *dev_data = dev->data;

    dev_data->ic_data = ic_data;
}

#ifdef CONFIG_BMS_IC_SWITCHES

static int bms_ic_bq769x0_set_switches(const struct device *dev, uint8_t switches, bool enabled)
{
    union bq769x0_sys_ctrl2 sys_ctrl2;
    int err;

    if ((switches & (BMS_SWITCH_CHG | BMS_SWITCH_DIS)) != switches) {
        return -EINVAL;
    }

    err = bq769x0_read_byte(dev, BQ769X0_SYS_CTRL2, &sys_ctrl2.byte);
    if (err != 0) {
        return err;
    }

    if (switches & BMS_SWITCH_CHG) {
        sys_ctrl2.CHG_ON = enabled ? 1 : 0;
    }
    if (switches & BMS_SWITCH_DIS) {
        sys_ctrl2.DSG_ON = enabled ? 1 : 0;
    }

    err = bq769x0_write_byte(dev, BQ769X0_SYS_CTRL2, sys_ctrl2.byte);
    if (err != 0) {
        return err;
    }

    return 0;
}

#endif /* CONFIG_BMS_IC_SWITCHES */

static int bq769x0_set_balancing_switches(const struct device *dev, uint32_t cells)
{
    const struct bms_ic_bq769x0_config *dev_config = dev->config;
    struct bms_ic_bq769x0_data *dev_data = dev->data;
    int err;

    for (int section = 0; section < dev_config->num_sections; section++) {
        uint8_t cells_section = (cells >> section * 5) & 0x1F;
        if (((cells_section << 1) & cells_section) || ((cells_section >> 1) & cells_section)) {
            /* balancing of adjacent cells within one section not allowed */
            return -EINVAL;
        }

        err = bq769x0_write_byte(dev, BQ769X0_CELLBAL1 + section, cells_section);
        if (err != 0) {
            return err;
        }
    }

    dev_data->balancing_status = cells;

    return 0;
}

static void bq769x0_balancing_work_handler(struct k_work *work)
{
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct bms_ic_bq769x0_data *dev_data =
        CONTAINER_OF(dwork, struct bms_ic_bq769x0_data, balancing_work);
    const struct device *dev = dev_data->dev;
    const struct bms_ic_bq769x0_config *dev_config = dev->config;
    struct bms_ic_data *ic_data = dev_data->ic_data;
    int err;

    if (k_uptime_get() - dev_data->active_timestamp >= dev_data->ic_conf.bal_idle_delay
        && ic_data->cell_voltage_max > dev_data->ic_conf.bal_cell_voltage_min
        && (ic_data->cell_voltage_max - ic_data->cell_voltage_min)
               > dev_data->ic_conf.bal_cell_voltage_diff)
    {
        ic_data->balancing_status = 0; /* current status will be set in following loop */

        int balancing_flags;
        int balancing_flags_target;

        for (int section = 0; section < dev_config->num_sections; section++) {
            /* find cells which should be balanced and sort them by voltage descending */
            int cell_list[5];
            int cell_counter = 0;
            for (int i = 0; i < 5; i++) {
                if ((ic_data->cell_voltages[section * 5 + i] - ic_data->cell_voltage_min)
                    > dev_data->ic_conf.bal_cell_voltage_diff)
                {
                    int j = cell_counter;
                    while (j > 0
                           && ic_data->cell_voltages[section * 5 + cell_list[j - 1]]
                                  < ic_data->cell_voltages[section * 5 + i])
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
                /* try to enable balancing of current cell */
                balancing_flags_target = balancing_flags | (1 << cell_list[i]);

                /* check if attempting to balance adjacent cells */
                bool adjacent_cell_collision = ((balancing_flags_target << 1) & balancing_flags)
                                               || ((balancing_flags << 1) & balancing_flags_target);

                if (adjacent_cell_collision == false) {
                    balancing_flags = balancing_flags_target;
                }
            }

            ic_data->balancing_status |= balancing_flags << section * 5;

            /* set balancing register for this section */
            err = bq769x0_write_byte(dev, BQ769X0_CELLBAL1 + section, balancing_flags);
            if (err == 0) {
                LOG_DBG("Set CELLBAL%d register to 0x%02X", section + 1, balancing_flags);
            }
            else {
                LOG_ERR("Failed to set CELBAL%d register: %d", section + 1, err);
            }
        }
    }
    else if (ic_data->balancing_status > 0) {
        /* clear all CELLBAL registers */
        for (int section = 0; section < dev_config->num_sections; section++) {
            err = bq769x0_write_byte(dev, BQ769X0_CELLBAL1 + section, 0x0);
            if (err == 0) {
                LOG_DBG("Cleared CELLBAL%d register", section + 1);
            }
            else {
                LOG_ERR("Clearing CELBAL%d register failed: %d", section + 1, err);
            }
        }

        ic_data->balancing_status = 0;
    }

    k_work_schedule(dwork, K_SECONDS(1));
}

static int bms_ic_bq769x0_balance(const struct device *dev, uint32_t cells)
{
    struct bms_ic_bq769x0_data *dev_data = dev->data;

    if (dev_data->ic_conf.auto_balancing) {
        return -EBUSY;
    }

    return bq769x0_set_balancing_switches(dev, cells);
}

static int bq769x0_activate(const struct device *dev)
{
    const struct bms_ic_bq769x0_config *dev_config = dev->config;
    struct bms_ic_bq769x0_data *dev_data = dev->data;
    int err;

    /* Datasheet: 10 ms delay (t_BOOTREADY) */
    k_sleep(K_TIMEOUT_ABS_MS(10));

    err = bq769x0_detect_crc(dev);
    if (err != 0) {
        uint8_t adcoffset;
        uint8_t adcgain1;
        uint8_t adcgain2;

        /* switch external thermistor and ADC on */
        err |= bq769x0_write_byte(dev, BQ769X0_SYS_CTRL1, 0b00011000);
        /* switch CC_EN on */
        err |= bq769x0_write_byte(dev, BQ769X0_SYS_CTRL2, 0b01000000);

        /* get ADC offset (2's complement) and gain */
        err |= bq769x0_read_byte(dev, BQ769X0_ADCOFFSET, &adcoffset);
        dev_data->adc_offset = (signed int)adcoffset;

        err |= bq769x0_read_byte(dev, BQ769X0_ADCGAIN1, &adcgain1);
        err |= bq769x0_read_byte(dev, BQ769X0_ADCGAIN2, &adcgain2);

        dev_data->adc_gain =
            365 + (((adcgain1 & 0b00001100) << 1) | ((adcgain2 & 0b11100000) >> 5));

        if (err != 0) {
            return -EIO;
        }
    }
    else {
        LOG_ERR("BMS communication error");
        return err;
    }

    gpio_pin_configure_dt(&dev_config->alert_gpio, GPIO_INPUT);
    gpio_init_callback(&dev_data->alert_cb, bq769x0_alert_isr, BIT(dev_config->alert_gpio.pin));
    gpio_add_callback_dt(&dev_config->alert_gpio, &dev_data->alert_cb);

    /* run processing once at start-up to check and clear errors */
    k_work_schedule(&dev_data->alert_work, K_NO_WAIT);

    return 0;
}

static int bms_ic_bq769x0_set_mode(const struct device *dev, enum bms_ic_mode mode)
{
    int err = 0;

    switch (mode) {
        case BMS_IC_MODE_ACTIVE:
            return bq769x0_activate(dev);
        case BMS_IC_MODE_OFF:
            /* put IC into SHIP mode (i.e. switched off) */
            err |= bq769x0_write_byte(dev, BQ769X0_SYS_CTRL1, 0x0);
            err |= bq769x0_write_byte(dev, BQ769X0_SYS_CTRL1, 0x1);
            err |= bq769x0_write_byte(dev, BQ769X0_SYS_CTRL1, 0x2);
            return err == 0 ? 0 : -EIO;
        default:
            return -ENOTSUP;
    }
}

static int bq769x0_init(const struct device *dev)
{
    const struct bms_ic_bq769x0_config *dev_config = dev->config;
    struct bms_ic_bq769x0_data *dev_data = dev->data;

    if (!i2c_is_ready_dt(&dev_config->i2c)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }
    if (!gpio_is_ready_dt(&dev_config->alert_gpio)) {
        LOG_ERR("Alert GPIO not ready");
        return -ENODEV;
    }

    dev_data->dev = dev;

    k_work_init_delayable(&dev_data->alert_work, bq769x0_alert_handler);
    k_work_init_delayable(&dev_data->balancing_work, bq769x0_balancing_work_handler);

    return 0;
}

static const struct bms_ic_driver_api bq769x0_driver_api = {
    .configure = bms_ic_bq769x0_configure,
    .assign_data = bms_ic_bq769x0_assign_data,
    .read_data = bms_ic_bq769x0_read_data,
#ifdef CONFIG_BMS_IC_SWITCHES
    .set_switches = bms_ic_bq769x0_set_switches,
#endif
    .balance = bms_ic_bq769x0_balance,
    .set_mode = bms_ic_bq769x0_set_mode,
};

#define BQ769X0_ASSERT_CURRENT_MONITORING_PROP_GREATER_ZERO(index, prop) \
    BUILD_ASSERT(COND_CODE_0(IS_ENABLED(CONFIG_BMS_IC_CURRENT_MONITORING), (1), \
                             (DT_INST_PROP_OR(index, prop, 0) > 0)), \
                 "Devicetree properties shunt-resistor-uohm and board-max-current " \
                 "must be greater than 0 for CONFIG_BMS_IC_CURRENT_MONITORING=y")

#define BQ769X0_INIT(index) \
    static struct bms_ic_bq769x0_data bq769x0_data_##index = { 0 }; \
    BQ769X0_ASSERT_CURRENT_MONITORING_PROP_GREATER_ZERO(index, shunt_resistor_uohm); \
    BQ769X0_ASSERT_CURRENT_MONITORING_PROP_GREATER_ZERO(index, board_max_current); \
    static const struct bms_ic_bq769x0_config bq769x0_config_##index = { \
        .i2c = I2C_DT_SPEC_INST_GET(index), \
        .alert_gpio = GPIO_DT_SPEC_INST_GET(index, alert_gpios), \
        .used_cell_channels = DT_INST_PROP(index, used_cell_channels), \
        .num_sections = COND_CODE_0( \
            DT_INST_PROP(index, used_cell_channels) & ~0x001F, (1), \
            (COND_CODE_0(DT_INST_PROP(index, used_cell_channels) & ~0x03FF, (2), (3)))), \
        .bus_pchg_gpio = GPIO_DT_SPEC_INST_GET(index, bus_pchg_gpios), \
        .shunt_resistor_uohm = DT_INST_PROP_OR(index, shunt_resistor_uohm, 1000), \
        .board_max_current = DT_INST_PROP_OR(index, board_max_current, 0), \
        .thermistor_beta = DT_INST_PROP(index, thermistor_beta), \
    }; \
    DEVICE_DT_INST_DEFINE(index, &bq769x0_init, NULL, &bq769x0_data_##index, \
                          &bq769x0_config_##index, POST_KERNEL, CONFIG_BMS_IC_INIT_PRIORITY, \
                          &bq769x0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ769X0_INIT)
