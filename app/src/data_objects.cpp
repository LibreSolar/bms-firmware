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

const char manufacturer[] = "Libre Solar";
const char device_type[] = DT_PROP(DT_PATH(pcb), type);
const char hardware_version[] = DT_PROP(DT_PATH(pcb), version_str);
const char firmware_version[] = FIRMWARE_VERSION_ID;

// struct to define ThingSet array node
struct ts_array cell_voltages_arr = { bms.status.cell_voltages, BOARD_NUM_CELLS_MAX,
                                      BOARD_NUM_CELLS_MAX, TS_T_FLOAT32, sizeof(float) };

// used for print_register callback
static uint16_t reg_addr = 0;

// used for xInitConf functions
static float new_capacity = 0;

/**
 * ThingSet data objects (see https://github.com/ThingSet)
 */
/* clang-format off */

// DEVICE INFORMATION /////////////////////////////////////////////////////

TS_ADD_GROUP(ID_DEVICE, "Device", TS_NO_CALLBACK, ID_ROOT);

TS_ADD_ITEM_STRING(0x40, "cManufacturer", manufacturer, 0,
    ID_DEVICE, TS_ANY_R, 0);

TS_ADD_ITEM_STRING(0x41, "cDeviceType", device_type, 0,
    ID_DEVICE, TS_ANY_R, 0);

TS_ADD_ITEM_STRING(0x42, "cHardwareVersion", hardware_version, 0,
    ID_DEVICE, TS_ANY_R, 0);

TS_ADD_ITEM_STRING(0x43, "cFirmwareVersion", firmware_version, 0,
    ID_DEVICE, TS_ANY_R, 0);

TS_ADD_FN_VOID(0x4A, "xShutdown", &shutdown, ID_DEVICE, TS_ANY_RW);

TS_ADD_FN_VOID(0x4B, "xReset", &reset_device, ID_DEVICE, TS_ANY_RW);

TS_ADD_FN_VOID(0x4C, "xPrintRegister", &print_register, ID_DEVICE, TS_ANY_RW);
TS_ADD_ITEM_UINT16(0x4D, "nRegAddr", &reg_addr, 0x4C, TS_ANY_RW, 0);

TS_ADD_FN_VOID(0x4E, "xPrintRegisters", &bms_print_registers, ID_DEVICE, TS_ANY_RW);

// CONFIGURATION //////////////////////////////////////////////////////////

TS_ADD_GROUP(ID_CONF, "Conf", &data_objects_update_conf, ID_ROOT);

// general battery settings

TS_ADD_ITEM_FLOAT(0x50, "sNominalCapacity_Ah", &bms.conf.nominal_capacity_Ah, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

// current limits

TS_ADD_ITEM_FLOAT(0x51, "sShortCircuitLimit_A", &bms.conf.dis_sc_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_UINT32(0x52, "sShortCircuitDelay_us", &bms.conf.dis_sc_delay_us,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x53, "sDisOvercurrent_A", &bms.conf.dis_oc_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_UINT32(0x54, "sDisOvercurrentDelay_ms", &bms.conf.dis_oc_delay_ms,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x55, "sChgOvercurrent_A", &bms.conf.chg_oc_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_UINT32(0x56, "sChgOvercurrentDelay_ms", &bms.conf.chg_oc_delay_ms,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

// temperature limits

TS_ADD_ITEM_FLOAT(0x58, "sDisMaxTemp_degC", &bms.conf.dis_ot_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x59, "sDisMinTemp_degC", &bms.conf.dis_ut_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x5A, "sChgMaxTemp_degC", &bms.conf.chg_ot_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x5B, "sChgMinTemp_degC", &bms.conf.chg_ut_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x5C, "sTempLimitHysteresis_degC", &bms.conf.t_limit_hyst, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

// voltage limits

TS_ADD_ITEM_FLOAT(0x60, "sCellOvervoltage_V", &bms.conf.cell_ov_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x61, "sCellOvervoltageReset_V", &bms.conf.cell_ov_reset, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_UINT32(0x62, "sCellOvervoltageDelay_ms", &bms.conf.cell_ov_delay_ms,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x63, "sCellUndervoltage_V", &bms.conf.cell_uv_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x64, "sCellUndervoltageReset_V", &bms.conf.cell_uv_reset, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_UINT32(0x65, "sCellUndervoltageDelay_ms", &bms.conf.cell_uv_delay_ms,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

// balancing

TS_ADD_ITEM_FLOAT(0x68, "sBalTargetVoltageDiff_V", &bms.conf.bal_cell_voltage_diff, 3,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x69, "sBalMinVoltage_V", &bms.conf.bal_cell_voltage_min, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_UINT16(0x6A, "sBalIdleDelay_s", &bms.conf.bal_idle_delay,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x6B, "sBalIdleCurrent_A", &bms.conf.bal_idle_current, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_FN_INT32(0xA0, "xPresetNMC", &bat_preset_nmc, ID_CONF, TS_ANY_RW);
TS_ADD_ITEM_FLOAT(0xA1, "fCapacity_Ah", &new_capacity, 1, 0xA0, TS_ANY_RW, 0);

TS_ADD_FN_INT32(0xA2, "xPresetLFP", &bat_preset_lfp, ID_CONF, TS_ANY_RW);
TS_ADD_ITEM_FLOAT(0xA3, "fCapacity_Ah", &new_capacity, 1, 0xA2, TS_ANY_RW, 0);

// MEAS DATA ////////////////////////////////////////////////////////////

TS_ADD_GROUP(ID_MEAS, "Meas", TS_NO_CALLBACK, ID_ROOT);

TS_ADD_ITEM_FLOAT(0x71, "rPackVoltage_V", &bms.status.pack_voltage, 2,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

#if defined(CONFIG_BQ769X2)
TS_ADD_ITEM_FLOAT(0x72, "rStackVoltage_V", &bms.status.stack_voltage, 2,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);
#endif

TS_ADD_ITEM_FLOAT(0x73, "rPackCurrent_A", &bms.status.pack_current, 2,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_FLOAT(0x74, "rBatTemp_degC", &bms.status.bat_temp_avg, 1,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_FLOAT(0x75, "rICTemp_degC", &bms.status.ic_temp, 1,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

//TS_ADD_ITEM_FLOAT(0x76, "rMCUTemp_degC", &mcu_temp, 1,
//    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

#if defined(CONFIG_ISL94202) || defined(CONFIG_BQ769X2)
TS_ADD_ITEM_FLOAT(0x77, "rMOSFETTemp_degC", &bms.status.mosfet_temp, 1,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);
#endif

TS_ADD_ITEM_FLOAT(0x7C, "rSOC_pct", &bms.status.soc, 1,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_UINT32(0x7E, "rErrorFlags", &bms.status.error_flags,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_UINT16(0x7F, "rBmsState", &bms.status.state,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_ARRAY(0x80, "rCellVoltages_V", &cell_voltages_arr, 3,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_FLOAT(0x81, "rCellAvgVoltage_V", &bms.status.cell_voltage_avg, 3,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_FLOAT(0x82, "rCellMinVoltage_V", &bms.status.cell_voltage_min, 3,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_FLOAT(0x83, "rCellMaxVoltage_V", &bms.status.cell_voltage_max, 3,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_UINT32(0x84, "rBalancingStatus", &bms.status.balancing_status,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

// INPUT DATA /////////////////////////////////////////////////////////////

TS_ADD_GROUP(ID_INPUT, "Input", TS_NO_CALLBACK, ID_ROOT);

TS_ADD_ITEM_BOOL(0x90, "wChgEnable", &bms.status.chg_enable,
    ID_INPUT, TS_ANY_R | TS_ANY_W, 0);

TS_ADD_ITEM_BOOL(0x91, "wDisEnable", &bms.status.dis_enable,
    ID_INPUT, TS_ANY_R | TS_ANY_W, 0);

/* clang-format on */

void data_objects_update_conf()
{
    // ToDo: Validate new settings before applying them

    bms_configure(&bms);

#ifdef CONFIG_THINGSET_STORAGE
    thingset_storage_save();
#endif
}

int32_t bat_preset(enum CellType type)
{
    int err;

    bms_init_config(&bms, type, new_capacity);
    err = bms_configure(&bms);
    bms_update(&bms);

#ifdef CONFIG_THINGSET_STORAGE
    if (err == 0) {
        thingset_storage_save();
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
