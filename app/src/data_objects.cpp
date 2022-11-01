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

#include "board.h"

#include "bms.h"
#include "eeprom.h"
#include "thingset.h"

#include <stdio.h>

extern Bms bms;

const char manufacturer[] = "Libre Solar";
const char device_type[] = DT_PROP(DT_PATH(pcb), type);
const char hardware_version[] = DT_PROP(DT_PATH(pcb), version_str);
const char firmware_version[] = FIRMWARE_VERSION_ID;

static char device_id[9];

static char auth_password[11];

// struct to define ThingSet array node
ThingSetArrayInfo cell_voltages_arr = { bms.status.cell_voltages, BOARD_NUM_CELLS_MAX,
                                        BOARD_NUM_CELLS_MAX, TS_T_FLOAT32, sizeof(float) };

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
/* clang-format off */
static ThingSetDataObject data_objects[] = {

    // DEVICE INFORMATION /////////////////////////////////////////////////////
    // using IDs >= 0x18

    TS_ITEM_STRING(0x1D, "cNodeID", device_id, sizeof(device_id),
        ID_ROOT, TS_ANY_R | TS_MKR_W, SUBSET_NVM),

    TS_GROUP(ID_INFO, "Device", TS_NO_CALLBACK, ID_ROOT),

    TS_ITEM_STRING(0x1A, "cManufacturer", manufacturer, 0,
        ID_INFO, TS_ANY_R, 0),

    TS_ITEM_STRING(0x1B, "cDeviceType", device_type, 0,
        ID_INFO, TS_ANY_R, 0),

    TS_ITEM_STRING(0x1C, "cHardwareVersion", hardware_version, 0,
        ID_INFO, TS_ANY_R, 0),

    TS_ITEM_STRING(0x1F, "cFirmwareVersion", firmware_version, 0,
        ID_INFO, TS_ANY_R, 0),

    // CONFIGURATION //////////////////////////////////////////////////////////
    // using IDs >= 0x30 except for high priority data objects

    TS_GROUP(ID_CONF, "Conf", &data_objects_update_conf, ID_ROOT),

    // general battery settings

    TS_ITEM_FLOAT(0x31, "sBatNom_Ah", &bms.conf.nominal_capacity_Ah, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    // current limits

    TS_ITEM_FLOAT(0x40, "sPcbDisSC_A", &bms.conf.dis_sc_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_UINT32(0x41, "sPcbDisSC_us", &bms.conf.dis_sc_delay_us,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x42, "sBatDisLim_A", &bms.conf.dis_oc_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_UINT32(0x43, "sBatDisLimDelay_ms", &bms.conf.dis_oc_delay_ms,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x44, "sBatChgLim_A", &bms.conf.chg_oc_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_UINT32(0x45, "sBatChgLimDelay_ms", &bms.conf.chg_oc_delay_ms,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    // temperature limits

    TS_ITEM_FLOAT(0x48, "sDisUpLim_degC", &bms.conf.dis_ot_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x49, "sDisLowLim_degC", &bms.conf.dis_ut_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x4A, "sChgUpLim_degC", &bms.conf.chg_ot_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x4B, "sChgLowLim_degC", &bms.conf.chg_ut_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x4C, "sTempLimHyst_degC", &bms.conf.t_limit_hyst, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    // voltage limits

    TS_ITEM_FLOAT(0x50, "sCellUpLim_V", &bms.conf.cell_ov_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x51, "sCellUpLimReset_V", &bms.conf.cell_ov_reset, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_UINT32(0x52, "sCellUpLimDelay_ms", &bms.conf.cell_ov_delay_ms,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x53, "sCellLowLim_V", &bms.conf.cell_uv_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x54, "sCellLowLimReset_V", &bms.conf.cell_uv_reset, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_UINT32(0x55, "sCellLowLimDelay_ms", &bms.conf.cell_uv_delay_ms,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    // balancing

    // 0x58 reserved (previously used for sAutoBalEn)

    TS_ITEM_FLOAT(0x59, "sBalCellDiff_V", &bms.conf.bal_cell_voltage_diff, 3,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x5A, "sBalCellLowLim_V", &bms.conf.bal_cell_voltage_min, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_UINT16(0x5B, "sBalIdleDelay_s", &bms.conf.bal_idle_delay,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_FLOAT(0x5C, "sBalIdleTh_A", &bms.conf.bal_idle_current, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    // INPUT DATA /////////////////////////////////////////////////////////////
    // using IDs >= 0x60

    TS_GROUP(ID_INPUT, "Input", TS_NO_CALLBACK, ID_ROOT),

    TS_ITEM_BOOL(0x61, "wChgEn", &bms.status.chg_enable,
        ID_INPUT, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    TS_ITEM_BOOL(0x62, "wDisEn", &bms.status.dis_enable,
        ID_INPUT, TS_ANY_R | TS_ANY_W, SUBSET_NVM),

    // OUTPUT DATA ////////////////////////////////////////////////////////////
    // using IDs >= 0x70 except for high priority data objects

    TS_GROUP(ID_MEAS, "Meas", TS_NO_CALLBACK, ID_ROOT),

    TS_ITEM_FLOAT(0x71, "rBat_V", &bms.status.pack_voltage, 2,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_FLOAT(0x72, "rBat_A", &bms.status.pack_current, 2,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_FLOAT(0x73, "rBat_degC", &bms.status.bat_temp_avg, 1,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_FLOAT(0x74, "rIC_degC", &bms.status.ic_temp, 1,
        ID_MEAS, TS_ANY_R, 0),

    //TS_ITEM_FLOAT(0x75, "rMCU_degC", &mcu_temp, 1,
    //    ID_MEAS, TS_ANY_R, 0),

#if defined(CONFIG_ISL94202) || defined(CONFIG_BQ769X2)
    TS_ITEM_FLOAT(0x76, "rMOSFET_degC", &bms.status.mosfet_temp, 1,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),
#endif

    TS_ITEM_FLOAT(0x7C, "rSOC_pct", &bms.status.soc, 1,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_UINT32(0x7E, "rErrorFlags", &bms.status.error_flags,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_UINT16(0x7F, "rBmsState", &bms.status.state,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_ARRAY(0x80, "rCells_V", &cell_voltages_arr, 3,
        ID_MEAS, TS_ANY_R, SUBSET_SER),

    TS_ITEM_FLOAT(0x9A, "rCellAvg_V", &bms.status.cell_voltage_avg, 3,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_FLOAT(0x9B, "rCellMin_V", &bms.status.cell_voltage_min, 3,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_FLOAT(0x9C, "rCellMax_V", &bms.status.cell_voltage_max, 3,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    TS_ITEM_UINT32(0x9D, "rBalancingStatus", &bms.status.balancing_status,
        ID_MEAS, TS_ANY_R, SUBSET_SER | SUBSET_CAN),

    // RECORDED DATA ///////////////////////////////////////////////////////
    // using IDs >= 0xA0

    //TS_GROUP(ID_REC, "Rec", TS_NO_CALLBACK, ID_ROOT),

    // CALIBRATION DATA ///////////////////////////////////////////////////////
    // using IDs >= 0xD0

    //TS_GROUP(ID_CAL, "cal", TS_NO_CALLBACK, ID_ROOT),

    // FUNCTION CALLS (EXEC) //////////////////////////////////////////////////
    // using IDs >= 0xE0

    TS_GROUP(ID_RPC, "RPC", TS_NO_CALLBACK, ID_ROOT),

    TS_FUNCTION(0xE1, "xShutdown", &shutdown, ID_RPC, TS_ANY_RW),
    TS_FUNCTION(0xE2, "xReset", &reset_device, ID_RPC, TS_ANY_RW),
    //TS_FUNCTION(0xE3, "xBootloaderSTM", &start_stm32_bootloader, ID_RPC, TS_ANY_RW),
    TS_FUNCTION(0xE4, "xSaveSettings", &eeprom_store_data, ID_RPC, TS_ANY_RW),
    TS_FUNCTION(0xEA, "xPrintRegister", &print_register, ID_RPC, TS_ANY_RW),
    TS_ITEM_UINT16(0xEB, "nRegAddr", &reg_addr, 0xEA, TS_ANY_RW, 0),
    TS_FUNCTION(0xEC, "xPrintRegisters", &bms_print_registers, ID_RPC, TS_ANY_RW),

    TS_FUNCTION(0xEE, "xAuth", &thingset_auth, 0, TS_ANY_RW),
    TS_ITEM_STRING(0xEF, "uPassword", auth_password, sizeof(auth_password), 0xEE, TS_ANY_RW, 0),

    // PUBLICATION DATA ///////////////////////////////////////////////////////
    // using IDs >= 0xF0

    TS_SUBSET(0xF3, "mSerial", SUBSET_SER, ID_ROOT, TS_ANY_RW),
#if CONFIG_THINGSET_CAN
    TS_SUBSET(0xF7, "mCan", SUBSET_CAN, ID_ROOT, TS_ANY_RW),
#endif

    TS_GROUP(ID_PUB, "_pub", TS_NO_CALLBACK, ID_ROOT),

    TS_GROUP(0xF1, "mSerial", TS_NO_CALLBACK, ID_PUB),
    TS_ITEM_BOOL(0xF2, "wEnable", &pub_serial_enable, 0xF1, TS_ANY_RW, 0),

#if CONFIG_THINGSET_CAN
    TS_GROUP(0xF5, "mCan", TS_NO_CALLBACK, ID_PUB),
    TS_ITEM_BOOL(0xF6, "wEnable", &pub_can_enable, 0xF5, TS_ANY_RW, 0),
#endif
};
/* clang-format on */

ThingSet ts(data_objects, ARRAY_SIZE(data_objects));

void data_objects_update_conf()
{
    // ToDo: Validate new settings before applying them

    bms_configure(&bms);

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

void shutdown()
{
    bms_shutdown(&bms);
}

void thingset_auth()
{
    static const char pass_exp[] = CONFIG_THINGSET_EXPERT_PASSWORD;
    static const char pass_mkr[] = CONFIG_THINGSET_MAKER_PASSWORD;

    if (strlen(pass_exp) == strlen(auth_password)
        && strncmp(auth_password, pass_exp, strlen(pass_exp)) == 0)
    {
        printf("Authenticated as expert user\n");
        ts.set_authentication(TS_EXP_MASK | TS_USR_MASK);
    }
    else if (strlen(pass_mkr) == strlen(auth_password)
             && strncmp(auth_password, pass_mkr, strlen(pass_mkr)) == 0)
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
        out[len - i - 1] = alphabet[(in >> (i * 5)) % 32];
    }
    out[len] = '\0';
}
