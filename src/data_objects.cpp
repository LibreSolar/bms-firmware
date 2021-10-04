/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_objects.h"

#include <zephyr.h>

#ifndef UNIT_TEST
#include <drivers/hwinfo.h>
#include <power/reboot.h>
#include <sys/crc.h>
#endif

#include "pcb.h"

#include "thingset.h"
#include "bms.h"
#include "eeprom.h"

#include <stdio.h>

extern BmsConfig bms_conf;
extern BmsStatus bms_status;

const char manufacturer[] = "Libre Solar";
const char device_type[] = DT_PROP(DT_PATH(pcb), type);
const char hardware_version[] = DT_PROP(DT_PATH(pcb), version_str);
const char firmware_version[] = FIRMWARE_VERSION_ID;

static char device_id[9];

static char auth_password[11];

// struct to define ThingSet array node
ThingSetArrayInfo cell_voltages_arr = {
    bms_status.cell_voltages, NUM_CELLS_MAX, NUM_CELLS_MAX, TS_T_FLOAT32
};

bool pub_serial_enable = IS_ENABLED(CONFIG_THINGSET_SERIAL_PUB_DEFAULT);

#if CONFIG_THINGSET_CAN
bool pub_can_enable = IS_ENABLED(CONFIG_THINGSET_CAN_PUB_DEFAULT);
uint16_t can_node_addr = CONFIG_THINGSET_CAN_DEFAULT_NODE_ID;
#endif

// used for print_register callback
static uint16_t reg_addr = 0;

/**
 * Data Objects
 *
 * IDs from 0x00 to 0x17 consume only 1 byte, so they are reserved for output data
 * objects communicated very often (to lower the data rate for LoRa and CAN)
 *
 * Normal priority data objects (consuming 2 or more bytes) start from IDs > 23 = 0x17
 */
static ThingSetDataObject data_objects[] = {

    // DEVICE INFORMATION /////////////////////////////////////////////////////
    // using IDs >= 0x18

    TS_GROUP(ID_INFO, "info", TS_NO_CALLBACK, ID_ROOT),

    TS_ITEM_STRING(0x19, "DeviceID", device_id, sizeof(device_id),
        ID_INFO, TS_ANY_R | TS_MKR_W, SUBSET_NVM),

    TS_ITEM_STRING(0x1A, "Manufacturer", manufacturer, 0,
        ID_INFO, TS_ANY_R, 0),

    TS_ITEM_STRING(0x1B, "DeviceType", device_type, 0,
        ID_INFO, TS_ANY_R, 0),

    TS_ITEM_STRING(0x1C, "HardwareVersion", hardware_version, 0,
        ID_INFO, TS_ANY_R, 0),

    TS_ITEM_STRING(0x1D, "FirmwareVersion", firmware_version, 0,
        ID_INFO, TS_ANY_R, 0),

    // CONFIGURATION //////////////////////////////////////////////////////////
    // using IDs >= 0x30 except for high priority data objects

    TS_GROUP(ID_CONF, "conf", &data_objects_update_conf, ID_ROOT),

    // general battery settings

    TS_ITEM_FLOAT(0x31, "BatNom_Ah", &bms_conf.nominal_capacity_Ah, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    // current limits

    TS_ITEM_FLOAT(0x40, "PcbDisSC_A", &bms_conf.dis_sc_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_UINT32(0x41, "PcbDisSC_us", &bms_conf.dis_sc_delay_us,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x42, "BatDisLim_A", &bms_conf.dis_oc_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_UINT32(0x43, "BatDisLimDelay_ms", &bms_conf.dis_oc_delay_ms,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x44, "BatChgLim_A", &bms_conf.chg_oc_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_UINT32(0x45, "BatChgLimDelay_ms", &bms_conf.chg_oc_delay_ms,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    // temperature limits

    TS_ITEM_FLOAT(0x48, "DisUpLim_degC", &bms_conf.dis_ot_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x49, "DisLowLim_degC", &bms_conf.dis_ut_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x4A, "ChgUpLim_degC", &bms_conf.chg_ot_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x4B, "ChgLowLim_degC", &bms_conf.chg_ut_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x4C, "TempLimHyst_degC", &bms_conf.t_limit_hyst, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    // voltage limits

    TS_ITEM_FLOAT(0x50, "CellUpLim_V", &bms_conf.cell_ov_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x51, "CellUpLimReset_V", &bms_conf.cell_ov_reset, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_UINT32(0x52, "CellUpLimDelay_ms", &bms_conf.cell_ov_delay_ms,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x53, "CellLowLim_V", &bms_conf.cell_uv_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x54, "CellLowLimReset_V", &bms_conf.cell_uv_reset, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_UINT32(0x55, "CellLowLimDelay_ms", &bms_conf.cell_uv_delay_ms,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    // balancing

    TS_ITEM_BOOL(0x58, "AutoBalEn", &bms_conf.auto_balancing_enabled,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x59, "BalCellDiff_V", &bms_conf.bal_cell_voltage_diff, 3,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x5A, "BalCellLowLim_V", &bms_conf.bal_cell_voltage_min, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_UINT16(0x5B, "BalIdleDelay_s", &bms_conf.bal_idle_delay,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x5C, "BalIdleTh_A", &bms_conf.bal_idle_current, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    // INPUT DATA /////////////////////////////////////////////////////////////
    // using IDs >= 0x60

    TS_GROUP(ID_INPUT, "input", TS_NO_CALLBACK, ID_ROOT),

    TS_ITEM_BOOL(0x61, "ChgEn", &bms_status.chg_enable,
        ID_INPUT, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_BOOL(0x62, "DisEn", &bms_status.dis_enable,
        ID_INPUT, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    // OUTPUT DATA ////////////////////////////////////////////////////////////
    // using IDs >= 0x70 except for high priority data objects

    TS_GROUP(ID_MEAS, "meas", TS_NO_CALLBACK, ID_ROOT),

    TS_ITEM_FLOAT(0x71, "Bat_V", &bms_status.pack_voltage, 2,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_FLOAT(0x72, "Bat_A", &bms_status.pack_current, 2,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_FLOAT(0x73, "Bat_degC", &bms_status.bat_temp_avg, 1,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_FLOAT(0x74, "IC_degC", &bms_status.ic_temp, 1,
        ID_MEAS, TS_ANY_R, 0),

    //TS_ITEM_FLOAT(0x75, "MCU_degC", &mcu_temp, 1,
    //    ID_MEAS, TS_ANY_R, 0),

#ifdef CONFIG_BMS_ISL94202   // currently only implemented in ISL94202-based boards (using TS2)
    TS_ITEM_FLOAT(0x76, "MOSFETs_degC", &bms_status.mosfet_temp, 1,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),
#endif

    TS_ITEM_FLOAT(0x7C, "SOC_pct", &bms_status.soc, 1,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_UINT32(0x7E, "ErrorFlags", &bms_status.error_flags,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_UINT16(0x7F, "BmsState", &bms_status.state,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_ARRAY(0x80, "Cells_V", &cell_voltages_arr, 3,
        ID_MEAS, TS_ANY_R, SUBSET_SER),

    TS_ITEM_FLOAT(0x9A, "CellAvg_V", &bms_status.cell_voltage_avg, 3,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_FLOAT(0x9B, "CellMin_V", &bms_status.cell_voltage_min, 3,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_FLOAT(0x9C, "CellMax_V", &bms_status.cell_voltage_max, 3,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_UINT32(0x9D, "BalancingStatus", &bms_status.balancing_status,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    // RECORDED DATA ///////////////////////////////////////////////////////
    // using IDs >= 0xA0

    TS_GROUP(ID_REC, "rec", TS_NO_CALLBACK, ID_ROOT),

    // CALIBRATION DATA ///////////////////////////////////////////////////////
    // using IDs >= 0xD0

    TS_GROUP(ID_CAL, "cal", TS_NO_CALLBACK, ID_ROOT),

    // FUNCTION CALLS (EXEC) //////////////////////////////////////////////////
    // using IDs >= 0xE0

    TS_GROUP(ID_RPC, "rpc", TS_NO_CALLBACK, ID_ROOT),

    TS_FUNCTION(0xE1, "shutdown", &bms_shutdown, ID_RPC, TS_ANY_RW),
    TS_FUNCTION(0xE2, "reset", &reset_device, ID_RPC, TS_ANY_RW),
    //TS_FUNCTION(0xE3, "bootloader-stm", &start_stm32_bootloader, ID_RPC, TS_ANY_RW),
    TS_FUNCTION(0xE4, "save-settings", &eeprom_store_data, ID_RPC, TS_ANY_RW),
    TS_FUNCTION(0xEA, "print-register", &print_register, ID_RPC, TS_ANY_RW),
    TS_ITEM_UINT16(0xEB, "RegAddr", &reg_addr, 0xEA, TS_ANY_RW, 0),
    TS_FUNCTION(0xEC, "print-registers", &bms_print_registers, ID_RPC, TS_ANY_RW),

    TS_FUNCTION(0xEE, "auth", &thingset_auth, 0, TS_ANY_RW),
    TS_ITEM_STRING(0xEF, "Password", auth_password, sizeof(auth_password), 0xEE, TS_ANY_RW, 0),

    // PUBLICATION DATA ///////////////////////////////////////////////////////
    // using IDs >= 0xF0

    TS_SUBSET(0xF3, "serial", SUBSET_SER, 0xF1, TS_ANY_RW),
#if CONFIG_THINGSET_CAN
    TS_SUBSET(0xF7, "can", SUBSET_CAN, 0xF5, TS_ANY_RW),
#endif

    TS_GROUP(ID_PUB, ".pub", TS_NO_CALLBACK, ID_ROOT),

    TS_GROUP(0xF1, "serial", TS_NO_CALLBACK, ID_PUB),
    TS_ITEM_BOOL(0xF2, "Enable", &pub_serial_enable, 0xF1, TS_ANY_RW, 0),

#if CONFIG_THINGSET_CAN
    TS_GROUP(0xF5, "can", TS_NO_CALLBACK, ID_PUB),
    TS_ITEM_BOOL(0xF6, "Enable", &pub_can_enable, 0xF5, TS_ANY_RW, 0),
#endif
};

ThingSet ts(data_objects, ARRAY_SIZE(data_objects));

void data_objects_update_conf()
{
    // ToDo: Validate new settings before applying them

    bms_apply_cell_ovp(&bms_conf);
    bms_apply_cell_uvp(&bms_conf);

    bms_apply_dis_scp(&bms_conf);
    bms_apply_dis_ocp(&bms_conf);
    bms_apply_chg_ocp(&bms_conf);

    bms_apply_temp_limits(&bms_conf);

    eeprom_store_data();
}

void data_objects_init()
{
#ifndef UNIT_TEST
    uint8_t buf[12];
    hwinfo_get_device_id(buf, sizeof(buf));

    uint64_t id64 = crc32_ieee(buf, sizeof(buf));
    id64 += ((uint64_t)CONFIG_LIBRE_SOLAR_TYPE_ID) << 32;

    uint64_to_base32(id64, device_id, sizeof(device_id), alphabet_crockford);
#endif

    eeprom_restore_data();
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

void thingset_auth()
{
    static const char pass_exp[] = CONFIG_THINGSET_EXPERT_PASSWORD;
    static const char pass_mkr[] = CONFIG_THINGSET_MAKER_PASSWORD;

    if (strlen(pass_exp) == strlen(auth_password) &&
        strncmp(auth_password, pass_exp, strlen(pass_exp)) == 0)
    {
        printf("Authenticated as expert user\n");
        ts.set_authentication(TS_EXP_MASK | TS_USR_MASK);
    }
    else if (strlen(pass_mkr) == strlen(auth_password) &&
        strncmp(auth_password, pass_mkr, strlen(pass_mkr)) == 0)
    {
        printf("Authenticated as maker\n");
        ts.set_authentication(TS_MKR_MASK | TS_USR_MASK);
    }
    else {
        printf("Reset authentication\n");
        ts.set_authentication(TS_USR_MASK);
    }
}

void uint64_to_base32(uint64_t in, char *out, size_t size, const char *alphabet)
{
    // 13 is the maximum number of characters needed to encode 64-bit variable to base32
    int len = (size > 13) ? 13 : size;

    // find out actual length of output string
    for (int i = 0; i < len; i++) {
        if ((in >> (i * 5)) == 0) {
            len = i;
            break;
        }
    }

    for (int i = 0; i < len; i++) {
        out[len-i-1] = alphabet[(in >> (i * 5)) % 32];
    }
    out[len] = '\0';
}
