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

// CONFIGURATION //////////////////////////////////////////////////////////

TS_ADD_GROUP(ID_CONF, "Conf", &data_objects_update_conf, ID_ROOT);

// general battery settings

TS_ADD_ITEM_FLOAT(0x50, "sBatNom_Ah", &bms.conf.nominal_capacity_Ah, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

// current limits

TS_ADD_ITEM_FLOAT(0x51, "sPcbDisSC_A", &bms.conf.dis_sc_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_UINT32(0x52, "sPcbDisSC_us", &bms.conf.dis_sc_delay_us,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x53, "sBatDisLim_A", &bms.conf.dis_oc_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_UINT32(0x54, "sBatDisLimDelay_ms", &bms.conf.dis_oc_delay_ms,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x55, "sBatChgLim_A", &bms.conf.chg_oc_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_UINT32(0x56, "sBatChgLimDelay_ms", &bms.conf.chg_oc_delay_ms,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

// temperature limits

TS_ADD_ITEM_FLOAT(0x58, "sDisUpLim_degC", &bms.conf.dis_ot_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x59, "sDisLowLim_degC", &bms.conf.dis_ut_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x5A, "sChgUpLim_degC", &bms.conf.chg_ot_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x5B, "sChgLowLim_degC", &bms.conf.chg_ut_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x5C, "sTempLimHyst_degC", &bms.conf.t_limit_hyst, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

// voltage limits

TS_ADD_ITEM_FLOAT(0x60, "sCellUpLim_V", &bms.conf.cell_ov_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x61, "sCellUpLimReset_V", &bms.conf.cell_ov_reset, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_UINT32(0x62, "sCellUpLimDelay_ms", &bms.conf.cell_ov_delay_ms,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x63, "sCellLowLim_V", &bms.conf.cell_uv_limit, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x64, "sCellLowLimReset_V", &bms.conf.cell_uv_reset, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_UINT32(0x65, "sCellLowLimDelay_ms", &bms.conf.cell_uv_delay_ms,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

// balancing

TS_ADD_ITEM_FLOAT(0x68, "sBalCellDiff_V", &bms.conf.bal_cell_voltage_diff, 3,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x69, "sBalCellLowLim_V", &bms.conf.bal_cell_voltage_min, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_UINT16(0x6A, "sBalIdleDelay_s", &bms.conf.bal_idle_delay,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

TS_ADD_ITEM_FLOAT(0x6B, "sBalIdleTh_A", &bms.conf.bal_idle_current, 1,
    ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM);

// MEAS DATA ////////////////////////////////////////////////////////////

TS_ADD_GROUP(ID_MEAS, "Meas", TS_NO_CALLBACK, ID_ROOT);

TS_ADD_ITEM_FLOAT(0x71, "rPack_V", &bms.status.pack_voltage, 2,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

#if defined(CONFIG_BQ769X2)
TS_ADD_ITEM_FLOAT(0x72, "rStack_V", &bms.status.stack_voltage, 2,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);
#endif

TS_ADD_ITEM_FLOAT(0x73, "rPack_A", &bms.status.pack_current, 2,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_FLOAT(0x74, "rBat_degC", &bms.status.bat_temp_avg, 1,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_FLOAT(0x75, "rIC_degC", &bms.status.ic_temp, 1,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

//TS_ADD_ITEM_FLOAT(0x76, "rMCU_degC", &mcu_temp, 1,
//    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

#if defined(CONFIG_ISL94202) || defined(CONFIG_BQ769X2)
TS_ADD_ITEM_FLOAT(0x77, "rMOSFET_degC", &bms.status.mosfet_temp, 1,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);
#endif

TS_ADD_ITEM_FLOAT(0x7C, "rSOC_pct", &bms.status.soc, 1,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_UINT32(0x7E, "rErrorFlags", &bms.status.error_flags,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_UINT16(0x7F, "rBmsState", &bms.status.state,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_ARRAY(0x80, "rCells_V", &cell_voltages_arr, 3,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_FLOAT(0x81, "rCellAvg_V", &bms.status.cell_voltage_avg, 3,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_FLOAT(0x82, "rCellMin_V", &bms.status.cell_voltage_min, 3,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_FLOAT(0x83, "rCellMax_V", &bms.status.cell_voltage_max, 3,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

TS_ADD_ITEM_UINT32(0x84, "rBalancingStatus", &bms.status.balancing_status,
    ID_MEAS, TS_ANY_R, SUBSET_LIVE);

// INPUT DATA /////////////////////////////////////////////////////////////

TS_ADD_GROUP(ID_INPUT, "Input", TS_NO_CALLBACK, ID_ROOT);

TS_ADD_ITEM_BOOL(0x90, "wChgEn", &bms.status.chg_enable,
    ID_INPUT, TS_ANY_R | TS_ANY_W, 0);

TS_ADD_ITEM_BOOL(0x91, "wDisEn", &bms.status.dis_enable,
    ID_INPUT, TS_ANY_R | TS_ANY_W, 0);

// FUNCTION CALLS (EXEC) //////////////////////////////////////////////////
// using IDs >= 0xE0

TS_ADD_GROUP(ID_RPC, "RPC", TS_NO_CALLBACK, ID_ROOT);

TS_ADD_FUNCTION(0xE1, "xShutdown", &shutdown, ID_RPC, TS_ANY_RW);
TS_ADD_FUNCTION(0xE2, "xReset", &reset_device, ID_RPC, TS_ANY_RW);
//TS_ADD_FUNCTION(0xE3, "xBootloaderSTM", &start_stm32_bootloader, ID_RPC, TS_ANY_RW);
#ifdef CONFIG_THINGSET_STORAGE
TS_ADD_FUNCTION(0xE4, "xSaveSettings", &thingset_storage_save, ID_RPC, TS_ANY_RW);
#endif
TS_ADD_FUNCTION(0xEA, "xPrintRegister", &print_register, ID_RPC, TS_ANY_RW);
TS_ADD_ITEM_UINT16(0xEB, "nRegAddr", &reg_addr, 0xEA, TS_ANY_RW, 0);
TS_ADD_FUNCTION(0xEC, "xPrintRegisters", &bms_print_registers, ID_RPC, TS_ANY_RW);

/* clang-format on */

void data_objects_update_conf()
{
    // ToDo: Validate new settings before applying them

    bms_configure(&bms);

#ifdef CONFIG_THINGSET_STORAGE
    thingset_storage_save();
#endif
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
