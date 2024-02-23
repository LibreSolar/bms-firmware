/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_DRIVERS_BMS_IC_H_
#define INCLUDE_DRIVERS_BMS_IC_H_

/**
 * @brief
 * API for different Battery Management System (BMS) front-end ICs
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <bms/bms_common.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <stdbool.h>
#include <stdint.h>

/* Caution: Maximum number of flags is 31 (BIT(30)) because the flags must fit to an int32_t */
#define BMS_IC_CONF_VOLTAGE_LIMITS BIT(0)
#define BMS_IC_CONF_TEMP_LIMITS    BIT(1)
#define BMS_IC_CONF_CURRENT_LIMITS BIT(2)
#define BMS_IC_CONF_BALANCING      BIT(3)
#define BMS_IC_CONF_ALERTS         BIT(4)
#define BMS_IC_CONF_VOLTAGE_REGS   BIT(5)
#define BMS_IC_CONF_ALL            GENMASK(5, 0)

#define BMS_IC_DATA_CELL_VOLTAGES BIT(0)
#define BMS_IC_DATA_PACK_VOLTAGES BIT(1)
#define BMS_IC_DATA_TEMPERATURES  BIT(2)
#define BMS_IC_DATA_CURRENT       BIT(3)
#define BMS_IC_DATA_BALANCING     BIT(4)
#define BMS_IC_DATA_ERROR_FLAGS   BIT(5)
#define BMS_IC_DATA_ALL           GENMASK(5, 0)

/**
 * BMS IC operation modes
 */
enum bms_ic_mode
{
    /** Normal operation */
    BMS_IC_MODE_ACTIVE,
    /** Low-power mode with FETs still enabled */
    BMS_IC_MODE_IDLE,
    /** Low-power mode with FETs off, but regulators still enabled, e.g. to power host MCU */
    BMS_IC_MODE_STANDBY,
    /** Lowest power mode with all FETs and regulators disabled */
    BMS_IC_MODE_OFF,
};

/**
 * BMS configuration values, stored in RAM.
 */
struct bms_ic_conf
{
    /* Cell voltage limits */
    /** Cell target charge voltage (V) */
    float cell_chg_voltage_limit;
    /** Cell discharge voltage limit (V) */
    float cell_dis_voltage_limit;
    /** Cell over-voltage limit (V) */
    float cell_ov_limit;
    /** Cell over-voltage error reset threshold (V) */
    float cell_ov_reset;
    /** Cell over-voltage delay (ms) */
    uint32_t cell_ov_delay_ms;
    /** Cell under-voltage limit (V) */
    float cell_uv_limit;
    /** Cell under-voltage error reset threshold (V)*/
    float cell_uv_reset;
    /** Cell under-voltage delay (ms) */
    uint32_t cell_uv_delay_ms;

#ifdef CONFIG_BMS_IC_CURRENT_MONITORING
    /* Current limits */
    /** Charge over-current limit (A) */
    float chg_oc_limit;
    /** Charge over-current delay (ms) */
    uint32_t chg_oc_delay_ms;
    /** Discharge over-current limit (A) */
    float dis_oc_limit;
    /** Discharge over-current delay (ms) */
    uint32_t dis_oc_delay_ms;
    /** Discharge short circuit limit (A) */
    float dis_sc_limit;
    /** Discharge short circuit delay (us) */
    uint32_t dis_sc_delay_us;
#endif

    /* Cell temperature limits */
    /** Discharge over-temperature (DOT) limit (°C) */
    float dis_ot_limit;
    /** Discharge under-temperature (DUT) limit (°C) */
    float dis_ut_limit;
    /** Charge over-temperature (COT) limit (°C) */
    float chg_ot_limit;
    /** Charge under-temperature (CUT) limit (°C) */
    float chg_ut_limit;
    /** Temperature limit hysteresis (°C) */
    float temp_limit_hyst;

    /* Balancing settings */
    /** Balancing cell voltage target difference (V) */
    float bal_cell_voltage_diff;
    /** Minimum cell voltage to start balancing (V) */
    float bal_cell_voltage_min;
    /** Current threshold to be considered idle (A) */
    float bal_idle_current;
    /** Minimum idle duration before balancing (s) */
    uint16_t bal_idle_delay;
    /** Enable/disable automatic balancing (controlled by the IC or driver) */
    bool auto_balancing;

    /* Built-in voltage regulator settings */
    /**
     * Bitfield to enable/disable built-in voltage regulators (usually LDOs). The bit positions
     * correspond to the regulator numbers in the manufacturer datasheet (e.g. 0x2 would enable
     * REG1).
     */
    uint8_t vregs_enable;

    /* Alert settings */
    /** Error flags which should trigger an alert action (if supported by the IC) */
    uint32_t alert_mask;
};

/**
 * Current BMS IC status including measurements and error flags
 */
struct bms_ic_data
{
    /** Single cell voltages (V) */
    float cell_voltages[CONFIG_BMS_IC_MAX_CELLS];
    /** Maximum cell voltage (V) */
    float cell_voltage_max;
    /** Minimum cell voltage (V) */
    float cell_voltage_min;
    /** Average cell voltage (V) */
    float cell_voltage_avg;
    /** Battery internal stack voltage (V) */
    float total_voltage;
#ifdef CONFIG_BMS_IC_SWITCHES
    /** Battery external pack voltage (V) */
    float external_voltage;
#endif

#ifdef CONFIG_BMS_IC_CURRENT_MONITORING
    /** Module/pack current, charging direction has positive sign (A) */
    float current;
#endif

    /** Cell temperatures (°C) */
    float cell_temps[CONFIG_BMS_IC_MAX_THERMISTORS];
    /** Maximum cell temperature (°C) */
    float cell_temp_max;
    /** Minimum cell temperature (°C) */
    float cell_temp_min;
    /** Average cell temperature (°C) */
    float cell_temp_avg;
    /** Internal BMS IC temperature (°C) */
    float ic_temp;
#ifdef CONFIG_BMS_IC_SWITCHES
    /** MOSFET temperature (°C) */
    float mosfet_temp;
#endif

    /** Actual number of cells connected (may be less than CONFIG_BMS_IC_MAX_CELLS) */
    uint8_t connected_cells;

    /** Actual number of thermistors used (may be less than CONFIG_BMS_IC_MAX_THERMISTORS) */
    uint8_t used_thermistors;

    /** Balancing channels on/off status */
    uint32_t balancing_status;

    /** BMS errors stored as BMS_ERR_* flags */
    uint32_t error_flags;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Zephyr driver API function pointer declarations (see further down for documentation)
 */

typedef int (*bms_ic_api_configure)(const struct device *dev, struct bms_ic_conf *ic_conf,
                                    uint32_t flags);

typedef void (*bms_ic_api_assign_data)(const struct device *dev, struct bms_ic_data *ic_data);

typedef int (*bms_ic_api_read_data)(const struct device *dev, uint32_t flags);

typedef int (*bms_ic_api_set_switches)(const struct device *dev, uint8_t switches, bool enabled);

typedef int (*bms_ic_api_balance)(const struct device *dev, uint32_t cells);

typedef int (*bms_ic_api_set_mode)(const struct device *dev, enum bms_ic_mode mode);

typedef int (*bms_ic_api_read_mem)(const struct device *dev, const uint16_t addr, uint8_t *data,
                                   const size_t len);

typedef int (*bms_ic_api_write_mem)(const struct device *dev, const uint16_t addr,
                                    const uint8_t *data, const size_t len);

typedef int (*bms_ic_api_debug_print_mem)(const struct device *dev);

__subsystem struct bms_ic_driver_api
{
    bms_ic_api_configure configure;
    bms_ic_api_assign_data assign_data;
    bms_ic_api_read_data read_data;
    bms_ic_api_set_switches set_switches;
    bms_ic_api_balance balance;
    bms_ic_api_set_mode set_mode;
    bms_ic_api_read_mem read_mem;
    bms_ic_api_write_mem write_mem;
    bms_ic_api_debug_print_mem debug_print_mem;
};

/**
 * @endcond
 */

/**
 * @brief Write config to IC.
 *
 * Most BMS ICs can apply configuration values only in discrete steps or with limited resolution.
 * The actually applied configuration is written back to ic_conf.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param ic_conf BMS configuration to apply.
 * @param flags Flags to specify which parts of the configuration should be applied. See
 *              BMS_IC_CONF_* defines for valid flags.
 *
 * @retval appl_flags successfully applied configuration flags (may be different than requested)
 * @retval -ENOTSUP if none of the requested flags is supported
 * @retval -EIO for communication error
 */
static inline int bms_ic_configure(const struct device *dev, struct bms_ic_conf *ic_conf,
                                   uint32_t flags)
{
    const struct bms_ic_driver_api *api = (const struct bms_ic_driver_api *)dev->api;

    if (api->configure == NULL) {
        return -ENOSYS;
    }

    return api->configure(dev, ic_conf, flags);
}

/**
 * @brief Assign bms_ic_data object to use for reading data from the IC.
 *
 * This data is updated by calls to @a bms_ic_read_data.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param ic_data Pointer to a statically allocated bms_ic_data object.
 *
 * @return 0 for success or negative error code otherwise.
 */
static inline void bms_ic_assign_data(const struct device *dev, struct bms_ic_data *ic_data)
{
    const struct bms_ic_driver_api *api = (const struct bms_ic_driver_api *)dev->api;

    api->assign_data(dev, ic_data);
}

/**
 * @brief Read data from the IC.
 *
 * @a bms_ic_assign_data must be called before using this function.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param flags Flags to specify which parts of the data should be updated. See BMS_IC_DATA_*
 *              defines for valid flags.
 *
 * @retval 0 for success
 * @retval -EINVAL if not all requested data is provided by the IC
 * @retval -EIO for communication error
 * @retval -ENOMEM if the data was not assigned with @a bms_ic_assign_data
 */
static inline int bms_ic_read_data(const struct device *dev, uint32_t flags)
{
    const struct bms_ic_driver_api *api = (const struct bms_ic_driver_api *)dev->api;

    return api->read_data(dev, flags);
}

#ifdef CONFIG_BMS_IC_SWITCHES
/**
 * @brief Switch the specified MOSFET(s) on or off.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param switches MOSFET(s) to switch on/off.
 * @param enabled If the MOSFET(s) should be enabled (on) or disabled (off)
 *
 * @return 0 for success or negative error code otherwise.
 */
static inline int bms_ic_set_switches(const struct device *dev, uint8_t switches, bool enabled)
{
    const struct bms_ic_driver_api *api = (const struct bms_ic_driver_api *)dev->api;

    if (api->set_switches == NULL) {
        return -ENOSYS;
    }

    return api->set_switches(dev, switches, enabled);
}
#endif

/**
 * @brief Manually set balancing switches of the IC.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cells Bitset defining the cell(s) to be balanced. Set to 0 to disable balancing.
 *
 * @retval 0 for success
 * @retval -EBUSY if automatic balancing was enabled through bms_ic_configure
 * @return -EINVAL if an invalid set of cells is requested to be balanced
 */
static inline int bms_ic_balance(const struct device *dev, uint32_t cells)
{
    const struct bms_ic_driver_api *api = (const struct bms_ic_driver_api *)dev->api;

    return api->balance(dev, cells);
}

/**
 * @brief Request the IC to go into specified operating mode.
 *
 * Usually used to set the device into different sleep modes for reduced power consumption.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mode Desired BMS IC operating mode.
 *
 * @return 0 for success or negative error code otherwise.
 */
static inline int bms_ic_set_mode(const struct device *dev, enum bms_ic_mode mode)
{
    const struct bms_ic_driver_api *api = (const struct bms_ic_driver_api *)dev->api;

    if (api->set_mode == NULL) {
        return -ENOSYS;
    }

    return api->set_mode(dev, mode);
}

static inline int bms_ic_read_mem(const struct device *dev, const uint16_t addr, uint8_t *data,
                                  const size_t len)
{
    const struct bms_ic_driver_api *api = (const struct bms_ic_driver_api *)dev->api;

    if (api->read_mem == NULL) {
        return -ENOSYS;
    }

    return api->read_mem(dev, addr, data, len);
}

static inline int bms_ic_write_mem(const struct device *dev, const uint16_t addr,
                                   const uint8_t *data, const size_t len)
{
    const struct bms_ic_driver_api *api = (const struct bms_ic_driver_api *)dev->api;

    if (api->write_mem == NULL) {
        return -ENOSYS;
    }

    return api->write_mem(dev, addr, data, len);
}

static inline int bms_ic_debug_print_mem(const struct device *dev)
{
    const struct bms_ic_driver_api *api = (const struct bms_ic_driver_api *)dev->api;

    if (api->debug_print_mem == NULL) {
        return -ENOSYS;
    }

    return api->debug_print_mem(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_BMS_IC_H_ */
