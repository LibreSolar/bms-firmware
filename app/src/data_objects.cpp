/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_objects.h"

#include <zephyr/kernel.h>

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/reboot.h>

#include <thingset/sdk.h>
#include <thingset/storage.h>

#include <bms/bms.h>

#include <stdio.h>

extern const struct device *bms_ic;
extern struct bms_context bms;

static char manufacturer[] = "Libre Solar";
static char device_type[] = DT_PROP(DT_PATH(pcb), type);
static char hardware_version[] = DT_PROP(DT_PATH(pcb), version_str);
static char firmware_version[] = FIRMWARE_VERSION_ID;

// struct to define ThingSet array node
static THINGSET_DEFINE_FLOAT_ARRAY(cell_voltages_arr, 3, bms.ic_data.cell_voltages,
                                   ARRAY_SIZE(bms.ic_data.cell_voltages));

static THINGSET_DEFINE_FLOAT_ARRAY(cell_temps_arr, 1, bms.ic_data.cell_temps,
                                   ARRAY_SIZE(bms.ic_data.cell_temps));

// used for xInitConf functions
static float new_capacity = 0;

/**
 * ThingSet data objects (see https://github.com/ThingSet)
 */

// DEVICE INFORMATION /////////////////////////////////////////////////////

THINGSET_ADD_GROUP(TS_ID_ROOT, APP_ID_DEVICE, "Device", THINGSET_NO_CALLBACK);

THINGSET_ADD_ITEM_STRING(APP_ID_DEVICE, APP_ID_DEVICE_MANUFACTURER, "cManufacturer", manufacturer,
                         sizeof(manufacturer), THINGSET_ANY_R, 0);

THINGSET_ADD_ITEM_STRING(APP_ID_DEVICE, APP_ID_DEVICE_TYPE, "cDeviceType", device_type,
                         sizeof(device_type), THINGSET_ANY_R, 0);

THINGSET_ADD_ITEM_STRING(APP_ID_DEVICE, APP_ID_DEVICE_HW_VER, "cHardwareVersion", hardware_version,
                         sizeof(hardware_version), THINGSET_ANY_R, 0);

THINGSET_ADD_ITEM_STRING(APP_ID_DEVICE, APP_ID_DEVICE_FW_VER, "cFirmwareVersion", firmware_version,
                         sizeof(firmware_version), THINGSET_ANY_R, 0);

THINGSET_ADD_FN_VOID(APP_ID_DEVICE, APP_ID_DEVICE_SHUTDOWN, "xShutdown", &shutdown,
                     THINGSET_ANY_RW);

THINGSET_ADD_FN_VOID(APP_ID_DEVICE, APP_ID_DEVICE_RESET, "xReset", &reset_device, THINGSET_ANY_RW);

THINGSET_ADD_FN_VOID(APP_ID_DEVICE, APP_ID_DEVICE_PRINT_REGS, "xDebugPrintRegisters",
                     &print_registers, THINGSET_ANY_RW);

// CONFIGURATION //////////////////////////////////////////////////////////

THINGSET_ADD_GROUP(TS_ID_ROOT, APP_ID_CONF, "Conf", &data_objects_update_conf);

// general battery settings

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_NOMINAL_CAPACITY, "sNominalCapacity_Ah",
                        &bms.nominal_capacity_Ah, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

// current limits

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_SHORT_CIRCUIT_CURRENT, "sShortCircuitLimit_A",
                        &bms.ic_conf.dis_sc_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

THINGSET_ADD_ITEM_UINT32(APP_ID_CONF, APP_ID_CONF_SHORT_CIRCUIT_DELAY, "sShortCircuitDelay_us",
                         &bms.ic_conf.dis_sc_delay_us, THINGSET_ANY_R | THINGSET_ANY_W,
                         TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_DIS_OVERCURRENT, "sDisOvercurrent_A",
                        &bms.ic_conf.dis_oc_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

THINGSET_ADD_ITEM_UINT32(APP_ID_CONF, APP_ID_CONF_DIS_OVERCURRENT_DELAY, "sDisOvercurrentDelay_ms",
                         &bms.ic_conf.dis_oc_delay_ms, THINGSET_ANY_R | THINGSET_ANY_W,
                         TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_CHG_OVERCURRENT, "sChgOvercurrent_A",
                        &bms.ic_conf.chg_oc_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

THINGSET_ADD_ITEM_UINT32(APP_ID_CONF, APP_ID_CONF_CHG_OVERCURRENT_DELAY, "sChgOvercurrentDelay_ms",
                         &bms.ic_conf.chg_oc_delay_ms, THINGSET_ANY_R | THINGSET_ANY_W,
                         TS_SUBSET_NVM);

// temperature limits

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_DIS_MAX_TEMP, "sDisMaxTemp_degC",
                        &bms.ic_conf.dis_ot_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_DIS_MIN_TEMP, "sDisMinTemp_degC",
                        &bms.ic_conf.dis_ut_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_CHG_MAX_TEMP, "sChgMaxTemp_degC",
                        &bms.ic_conf.chg_ot_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_CHG_MIN_TEMP, "sChgMinTemp_degC",
                        &bms.ic_conf.chg_ut_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_TEMP_HYST, "sTempLimitHysteresis_degC",
                        &bms.ic_conf.temp_limit_hyst, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

// voltage limits

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_CELL_OVERVOLTAGE, "sCellOvervoltage_V",
                        &bms.ic_conf.cell_ov_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_CELL_OVERVOLTAGE_RESET, "sCellOvervoltageReset_V",
                        &bms.ic_conf.cell_ov_reset, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

THINGSET_ADD_ITEM_UINT32(APP_ID_CONF, APP_ID_CONF_CELL_OVERVOLTAGE_DELAY,
                         "sCellOvervoltageDelay_ms", &bms.ic_conf.cell_ov_delay_ms,
                         THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_CELL_UNDERVOLTAGE, "sCellUndervoltage_V",
                        &bms.ic_conf.cell_uv_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_CELL_UNDERVOLTAGE_RESET,
                        "sCellUndervoltageReset_V", &bms.ic_conf.cell_uv_reset, 1,
                        THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_UINT32(APP_ID_CONF, APP_ID_CONF_CELL_UNDERVOLTAGE_DELAY,
                         "sCellUndervoltageDelay_ms", &bms.ic_conf.cell_uv_delay_ms,
                         THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

// balancing

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_BAL_TARGET_DIFF, "sBalTargetVoltageDiff_V",
                        &bms.ic_conf.bal_cell_voltage_diff, 3, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_BAL_MIN_VOLTAGE, "sBalMinVoltage_V",
                        &bms.ic_conf.bal_cell_voltage_min, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

THINGSET_ADD_ITEM_UINT16(APP_ID_CONF, APP_ID_CONF_BAL_IDLE_DELAY, "sBalIdleDelay_s",
                         &bms.ic_conf.bal_idle_delay, THINGSET_ANY_R | THINGSET_ANY_W,
                         TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_BAL_IDLE_CURRENT, "sBalIdleCurrent_A",
                        &bms.ic_conf.bal_idle_current, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

THINGSET_ADD_FN_INT32(APP_ID_CONF, APP_ID_CONF_PRESET_NMC, "xPresetNMC", &bat_preset_nmc,
                      THINGSET_ANY_RW);
THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF_PRESET_NMC, APP_ID_CONF_PRESET_NMC_CAPACITY, "fCapacity_Ah",
                        &new_capacity, 1, THINGSET_ANY_RW, 0);

THINGSET_ADD_FN_INT32(APP_ID_CONF, APP_ID_CONF_PRESET_LFP, "xPresetLFP", &bat_preset_lfp,
                      THINGSET_ANY_RW);
THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF_PRESET_LFP, APP_ID_CONF_PRESET_LFP_CAPACITY, "fCapacity_Ah",
                        &new_capacity, 1, THINGSET_ANY_RW, 0);

// MEAS DATA ////////////////////////////////////////////////////////////

THINGSET_ADD_GROUP(TS_ID_ROOT, APP_ID_MEAS, "Meas", THINGSET_NO_CALLBACK);

THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_PACK_VOLTAGE, "rPackVoltage_V",
                        &bms.ic_data.total_voltage, 2, THINGSET_ANY_R, TS_SUBSET_LIVE);

#ifdef CONFIG_BMS_IC_SWITCHES
THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_STACK_VOLTAGE, "rStackVoltage_V",
                        &bms.ic_data.external_voltage, 2, THINGSET_ANY_R, TS_SUBSET_LIVE);
#endif

#ifdef CONFIG_BMS_IC_SWITCHES
THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_PACK_CURRENT, "rPackCurrent_A",
                        &bms.ic_data.current, 2, THINGSET_ANY_R, TS_SUBSET_LIVE);
#endif

THINGSET_ADD_ITEM_ARRAY(APP_ID_MEAS, APP_ID_MEAS_CELL_TEMPS, "rCellTemps_degC", &cell_temps_arr,
                        THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_IC_TEMP, "rICTemp_degC", &bms.ic_data.ic_temp, 1,
                        THINGSET_ANY_R, TS_SUBSET_LIVE);

// THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_MCU_TEMP, "rMCUTemp_degC", &mcu_temp, 1,
//      THINGSET_ANY_R, TS_SUBSET_LIVE);

#ifdef CONFIG_BMS_IC_SWITCHES
THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_MOSFET_TEMP, "rMOSFETTemp_degC",
                        &bms.ic_data.mosfet_temp, 1, THINGSET_ANY_R, TS_SUBSET_LIVE);
#endif

THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_SOC, "rSOC_pct", &bms.soc, 1, THINGSET_ANY_R,
                        TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_UINT32(APP_ID_MEAS, APP_ID_MEAS_ERROR_FLAGS, "rErrorFlags",
                         &bms.ic_data.error_flags, THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_UINT8(APP_ID_MEAS, APP_ID_MEAS_BMS_STATE, "rBmsState", (uint8_t *)&bms.state,
                        THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_ARRAY(APP_ID_MEAS, APP_ID_MEAS_CELL_VOLTAGES, "rCellVoltages_V",
                        &cell_voltages_arr, THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_CELL_AVG_VOLTAGE, "rCellAvgVoltage_V",
                        &bms.ic_data.cell_voltage_avg, 3, THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_CELL_MIN_VOLTAGE, "rCellMinVoltage_V",
                        &bms.ic_data.cell_voltage_min, 3, THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_CELL_MAX_VOLTAGE, "rCellMaxVoltage_V",
                        &bms.ic_data.cell_voltage_max, 3, THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_UINT32(APP_ID_MEAS, APP_ID_MEAS_BALANCING_STATUS, "rBalancingStatus",
                         &bms.ic_data.balancing_status, THINGSET_ANY_R, TS_SUBSET_LIVE);

// INPUT DATA /////////////////////////////////////////////////////////////

THINGSET_ADD_GROUP(TS_ID_ROOT, APP_ID_INPUT, "Input", THINGSET_NO_CALLBACK);

THINGSET_ADD_ITEM_BOOL(APP_ID_INPUT, APP_ID_INPUT_CHG_ENABLE, "wChgEnable", &bms.chg_enable,
                       THINGSET_ANY_R | THINGSET_ANY_W, 0);

THINGSET_ADD_ITEM_BOOL(APP_ID_INPUT, APP_ID_INPUT_DIS_ENABLE, "wDisEnable", &bms.dis_enable,
                       THINGSET_ANY_R | THINGSET_ANY_W, 0);

void data_objects_update_conf(enum thingset_callback_reason reason)
{
    if (reason == THINGSET_CALLBACK_POST_WRITE) {
        // ToDo: Validate new settings before applying them

        bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_ALL);

#ifdef CONFIG_THINGSET_STORAGE
        thingset_storage_save_queued();
#endif
    }
}

int32_t bat_preset(enum bms_cell_type type)
{
    int err;

    bms_init_config(&bms, type, new_capacity);

    err = bms_ic_configure(bms_ic, &bms.ic_conf, BMS_IC_CONF_ALL);

#ifdef CONFIG_THINGSET_STORAGE
    if (err == 0) {
        thingset_storage_save_queued();
    }
#endif

    return err;
}

int32_t bat_preset_nmc()
{
    return bat_preset(CELL_TYPE_NMC);
}

int32_t bat_preset_lfp()
{
    return bat_preset(CELL_TYPE_LFP);
}

void print_registers()
{
    bms_ic_debug_print_mem(bms_ic);
}

void reset_device()
{
    sys_reboot(SYS_REBOOT_COLD);
}

void shutdown()
{
    bms_ic_set_mode(bms_ic, BMS_IC_MODE_OFF);
}
