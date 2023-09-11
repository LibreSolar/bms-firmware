/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_objects.h"

#include <zephyr/kernel.h>

#ifndef UNIT_TEST
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/reboot.h>
#endif

#include <thingset/sdk.h>
#include <thingset/storage.h>

#include "board.h"

#include "bms.h"

#include <stdio.h>

extern Bms bms;

char manufacturer[] = "Libre Solar";
char device_type[] = DT_PROP(DT_PATH(pcb), type);
char hardware_version[] = DT_PROP(DT_PATH(pcb), version_str);
char firmware_version[] = FIRMWARE_VERSION_ID;

// struct to define ThingSet array node
static THINGSET_DEFINE_FLOAT_ARRAY(cell_voltages_arr, 3, bms.status.cell_voltages,
                                   ARRAY_SIZE(bms.status.cell_voltages));

// used for print_register callback
static uint16_t reg_addr = 0;

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

THINGSET_ADD_FN_VOID(APP_ID_DEVICE, APP_ID_DEVICE_PRINT_REG, "xPrintRegister", &print_register,
                     THINGSET_ANY_RW);
THINGSET_ADD_ITEM_UINT16(APP_ID_DEVICE_PRINT_REG, APP_ID_DEVICE_PRINT_REG_ADDR, "nRegAddr",
                         &reg_addr, THINGSET_ANY_RW, 0);

THINGSET_ADD_FN_VOID(APP_ID_DEVICE, APP_ID_DEVICE_PRINT_REGS, "xPrintRegisters",
                     &bms_print_registers, THINGSET_ANY_RW);

// CONFIGURATION //////////////////////////////////////////////////////////

THINGSET_ADD_GROUP(TS_ID_ROOT, APP_ID_CONF, "Conf", &data_objects_update_conf);

// general battery settings

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_NOMINAL_CAPACITY, "sNominalCapacity_Ah",
                        &bms.conf.nominal_capacity_Ah, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

// current limits

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_SHORT_CIRCUIT_CURRENT, "sShortCircuitLimit_A",
                        &bms.conf.dis_sc_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_UINT32(APP_ID_CONF, APP_ID_CONF_SHORT_CIRCUIT_DELAY, "sShortCircuitDelay_us",
                         &bms.conf.dis_sc_delay_us, THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_DIS_OVERCURRENT, "sDisOvercurrent_A",
                        &bms.conf.dis_oc_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_UINT32(APP_ID_CONF, APP_ID_CONF_DIS_OVERCURRENT_DELAY, "sDisOvercurrentDelay_ms",
                         &bms.conf.dis_oc_delay_ms, THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_CHG_OVERCURRENT, "sChgOvercurrent_A",
                        &bms.conf.chg_oc_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_UINT32(APP_ID_CONF, APP_ID_CONF_CHG_OVERCURRENT_DELAY, "sChgOvercurrentDelay_ms",
                         &bms.conf.chg_oc_delay_ms, THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

// temperature limits

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_DIS_MAX_TEMP, "sDisMaxTemp_degC",
                        &bms.conf.dis_ot_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_DIS_MIN_TEMP, "sDisMinTemp_degC",
                        &bms.conf.dis_ut_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_CHG_MAX_TEMP, "sChgMaxTemp_degC",
                        &bms.conf.chg_ot_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_CHG_MIN_TEMP, "sChgMinTemp_degC",
                        &bms.conf.chg_ut_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_TEMP_HYST, "sTempLimitHysteresis_degC",
                        &bms.conf.t_limit_hyst, 1, THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

// voltage limits

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_CELL_OVERVOLTAGE, "sCellOvervoltage_V",
                        &bms.conf.cell_ov_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_CELL_OVERVOLTAGE_RESET, "sCellOvervoltageReset_V",
                        &bms.conf.cell_ov_reset, 1, THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_UINT32(APP_ID_CONF, APP_ID_CONF_CELL_OVERVOLTAGE_DELAY,
                         "sCellOvervoltageDelay_ms", &bms.conf.cell_ov_delay_ms,
                         THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_CELL_UNDERVOLTAGE, "sCellUndervoltage_V",
                        &bms.conf.cell_uv_limit, 1, THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_CELL_UNDERVOLTAGE_RESET,
                        "sCellUndervoltageReset_V", &bms.conf.cell_uv_reset, 1,
                        THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_UINT32(APP_ID_CONF, APP_ID_CONF_CELL_UNDERVOLTAGE_DELAY,
                         "sCellUndervoltageDelay_ms", &bms.conf.cell_uv_delay_ms,
                         THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

// balancing

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_BAL_TARGET_DIFF, "sBalTargetVoltageDiff_V",
                        &bms.conf.bal_cell_voltage_diff, 3, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_BAL_MIN_VOLTAGE, "sBalMinVoltage_V",
                        &bms.conf.bal_cell_voltage_min, 1, THINGSET_ANY_R | THINGSET_ANY_W,
                        TS_SUBSET_NVM);

THINGSET_ADD_ITEM_UINT16(APP_ID_CONF, APP_ID_CONF_BAL_IDLE_DELAY, "sBalIdleDelay_s",
                         &bms.conf.bal_idle_delay, THINGSET_ANY_R | THINGSET_ANY_W, TS_SUBSET_NVM);

THINGSET_ADD_ITEM_FLOAT(APP_ID_CONF, APP_ID_CONF_BAL_IDLE_CURRENT, "sBalIdleCurrent_A",
                        &bms.conf.bal_idle_current, 1, THINGSET_ANY_R | THINGSET_ANY_W,
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
                        &bms.status.pack_voltage, 2, THINGSET_ANY_R, TS_SUBSET_LIVE);

#if defined(CONFIG_BQ769X2)
THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_STACK_VOLTAGE, "rStackVoltage_V",
                        &bms.status.stack_voltage, 2, THINGSET_ANY_R, TS_SUBSET_LIVE);
#endif

THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_PACK_CURRENT, "rPackCurrent_A",
                        &bms.status.pack_current, 2, THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_BAT_TEMP, "rBatTemp_degC",
                        &bms.status.bat_temp_avg, 1, THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_IC_TEMP, "rICTemp_degC", &bms.status.ic_temp, 1,
                        THINGSET_ANY_R, TS_SUBSET_LIVE);

// THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_MCU_TEMP, "rMCUTemp_degC", &mcu_temp, 1,
//      THINGSET_ANY_R, TS_SUBSET_LIVE);

#if defined(CONFIG_ISL94202) || defined(CONFIG_BQ769X2)
THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_MOSFET_TEMP, "rMOSFETTemp_degC",
                        &bms.status.mosfet_temp, 1, THINGSET_ANY_R, TS_SUBSET_LIVE);
#endif

THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_SOC, "rSOC_pct", &bms.status.soc, 1,
                        THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_UINT32(APP_ID_MEAS, APP_ID_MEAS_ERROR_FLAGS, "rErrorFlags",
                         &bms.status.error_flags, THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_UINT16(APP_ID_MEAS, APP_ID_MEAS_BMS_STATE, "rBmsState", &bms.status.state,
                         THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_ARRAY(APP_ID_MEAS, APP_ID_MEAS_CELL_VOLTAGES, "rCellVoltages_V",
                        &cell_voltages_arr, THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_CELL_AVG_VOLTAGE, "rCellAvgVoltage_V",
                        &bms.status.cell_voltage_avg, 3, THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_CELL_MIN_VOLTAGE, "rCellMinVoltage_V",
                        &bms.status.cell_voltage_min, 3, THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_FLOAT(APP_ID_MEAS, APP_ID_MEAS_CELL_MAX_VOLTAGE, "rCellMaxVoltage_V",
                        &bms.status.cell_voltage_max, 3, THINGSET_ANY_R, TS_SUBSET_LIVE);

THINGSET_ADD_ITEM_UINT32(APP_ID_MEAS, APP_ID_MEAS_BALANCING_STATUS, "rBalancingStatus",
                         &bms.status.balancing_status, THINGSET_ANY_R, TS_SUBSET_LIVE);

// INPUT DATA /////////////////////////////////////////////////////////////

THINGSET_ADD_GROUP(TS_ID_ROOT, APP_ID_INPUT, "Input", THINGSET_NO_CALLBACK);

THINGSET_ADD_ITEM_BOOL(APP_ID_INPUT, APP_ID_INPUT_CHG_ENABLE, "wChgEnable", &bms.status.chg_enable,
                       THINGSET_ANY_R | THINGSET_ANY_W, 0);

THINGSET_ADD_ITEM_BOOL(APP_ID_INPUT, APP_ID_INPUT_DIS_ENABLE, "wDisEnable", &bms.status.dis_enable,
                       THINGSET_ANY_R | THINGSET_ANY_W, 0);

void data_objects_update_conf(enum thingset_callback_reason reason)
{
    if (reason == THINGSET_CALLBACK_POST_WRITE) {
        // ToDo: Validate new settings before applying them

        bms_configure(&bms);

#ifdef CONFIG_THINGSET_STORAGE
        thingset_storage_save_queued();
#endif
    }
}

int32_t bat_preset(enum CellType type)
{
    int err;

    bms_init_config(&bms, type, new_capacity);
    err = bms_configure(&bms);
    bms_update(&bms);

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

void print_register()
{
    bms_print_register(reg_addr);
}

void reset_device()
{
#ifndef UNIT_TEST
    sys_reboot(SYS_REBOOT_COLD);
#endif
}

void shutdown()
{
    bms_shutdown(&bms);
}
