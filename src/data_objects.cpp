/* LibreSolar Battery Management System firmware
 * Copyright (c) 2016-2018 Martin Jäger (www.libre.solar)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pcb.h"
#include "config.h"

#include "thingset.h"
#include "bms.h"

#include <stdio.h>

extern BmsConfig bms_conf;
extern BmsStatus bms_status;

const char manufacturer[] = "Libre Solar";
const char device_type[] = DEVICE_TYPE;
const char hardware_version[] = HARDWARE_VERSION;
const char firmware_version[] = "0.1";
uint32_t device_id = DEVICE_ID;      // from config.h

/** Data Objects
 *
 * IDs from 0x00 to 0x17 consume only 1 byte, so they are reserved for output data
 * objects communicated very often (to lower the data rate for LoRa and CAN)
 *
 * Normal priority data objects (consuming 2 or more bytes) start from IDs > 23 = 0x17
 */
const data_object_t data_objects[] = {

    // DEVICE INFORMATION /////////////////////////////////////////////////////
    // using IDs >= 0x18

    {0x18, TS_INFO, TS_READ_ALL | TS_WRITE_MAKER, TS_T_UINT32, 0, (void*) &(device_id),                         "DeviceID"},
    {0x19, TS_INFO, TS_READ_ALL,                  TS_T_STRING, 0, (void*) manufacturer,                         "Manufacturer"},
    {0x1A, TS_INFO, TS_READ_ALL,                  TS_T_STRING, 0, (void*) device_type,                          "DeviceType"},
    {0x1B, TS_INFO, TS_READ_ALL,                  TS_T_STRING, 0, (void*) hardware_version,                     "HardwareVersion"},
    {0x1C, TS_INFO, TS_READ_ALL,                  TS_T_STRING, 0, (void*) firmware_version,                     "FirmwareVersion"},

    // CONFIGURATION //////////////////////////////////////////////////////////
    // using IDs >= 0x30 except for high priority data objects

    // general battery settings
    {0x30, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_FLOAT32, 1, (void*) &(bms_conf.nominal_capacity_Ah),     "BatNom_Ah"},

    // current limits
    {0x40, TS_CONF, TS_READ_ALL | TS_WRITE_MAKER, TS_T_FLOAT32, 1, (void*) &(bms_conf.dis_sc_limit),            "PcbDisSC_A"},
    {0x41, TS_CONF, TS_READ_ALL | TS_WRITE_MAKER, TS_T_UINT32,  0, (void*) &(bms_conf.dis_sc_delay_us),         "PcbDisSC_us"},
    {0x42, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_FLOAT32, 1, (void*) &(bms_conf.dis_oc_limit),            "BatDisLim_A"},
    {0x43, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_UINT32,  0, (void*) &(bms_conf.dis_oc_delay_ms),         "BatDisLimDelay_ms"},
    {0x44, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_FLOAT32, 1, (void*) &(bms_conf.chg_oc_limit),            "BatChgLim_A"},
    {0x45, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_UINT32,  0, (void*) &(bms_conf.chg_oc_delay_ms),         "BatChgLimDelay_ms"},

    // temperature limits
    {0x48, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_FLOAT32, 1, (void*) &(bms_conf.dis_ot_limit),            "DisUpLim_degC"},
    {0x49, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_FLOAT32, 1, (void*) &(bms_conf.dis_ut_limit),            "DisLowLim_degC"},
    {0x4A, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_FLOAT32, 1, (void*) &(bms_conf.chg_ot_limit),            "ChgUpLim_degC"},
    {0x4B, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_FLOAT32, 1, (void*) &(bms_conf.chg_ut_limit),            "ChgLowLim_degC"},
    {0x4C, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_FLOAT32, 1, (void*) &(bms_conf.t_limit_hyst),            "TempLimHyst_degC"},

    // voltage limits
    {0x50, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_FLOAT32, 1, (void*) &(bms_conf.cell_ov_limit),           "CellUpLim_V"},
    {0x51, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_FLOAT32, 1, (void*) &(bms_conf.cell_ov_limit_hyst),      "CellUpLimHyst_V"},
    {0x52, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_UINT32,  0, (void*) &(bms_conf.cell_ov_delay_ms),        "CellUpLimDelay_ms"},
    {0x53, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_FLOAT32, 1, (void*) &(bms_conf.cell_uv_limit),           "CellLowLim_V"},
    {0x54, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_FLOAT32, 1, (void*) &(bms_conf.cell_uv_limit_hyst),      "CellLowLimHyst_V"},
    {0x55, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_UINT32,  0, (void*) &(bms_conf.cell_uv_delay_ms),        "CellLowLimDelay_ms"},

    // balancing
    {0x48, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_BOOL,    0, (void*) &(bms_conf.auto_balancing_enabled),  "AutoBalEn"},
    {0x49, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_FLOAT32, 1, (void*) &(bms_conf.balancing_voltage_diff_target),   "CellBalDelta_V"},
    {0x5A, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_FLOAT32, 1, (void*) &(bms_conf.balancing_cell_voltage_min),      "CellBalLowLim_V"},
    {0x5B, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_UINT16,  0, (void*) &(bms_conf.balancing_min_idle_s),            "CellBalDelay_s"},
    {0x5C, TS_CONF, TS_READ_ALL | TS_WRITE_ALL,   TS_T_FLOAT32, 1, (void*) &(bms_conf.idle_current_threshold),          "CellBalIdleTh_A"},

    // INPUT DATA /////////////////////////////////////////////////////////////
    // using IDs >= 0x60

    //{0x60, TS_INPUT, TS_READ_ALL | TS_WRITE_ALL,   TS_T_BOOL,   0, (void*) &(bms.chg_enabled_target),                   "ChgEn"},
    //{0x61, TS_INPUT, TS_READ_ALL | TS_WRITE_ALL,   TS_T_BOOL,   0, (void*) &(bms.dis_enabled_target),                   "DisEn"},

    // OUTPUT DATA ////////////////////////////////////////////////////////////
    // using IDs >= 0x70 except for high priority data objects

    {0x70, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 2, (void*) &(bms_status.pack_voltage),         "Bat_V"},
    {0x71, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 2, (void*) &(bms_status.pack_current),         "Bat_A"},
    {0x72, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 1, (void*) &(bms_status.external_temp),        "Bat_degC"},
    {0x76, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 1, (void*) &(bms_status.ic_temp),              "IC_degC"},
    //{0x75, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 1, (void*) &(mcu_temp),                        "MCU_degC"},
#if defined(PIN_ADC_TEMP_FETS) || defined(MOSFET_TEMP_SENSOR)
    {0x77, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 1, (void*) &(bms_status.mosfet_temp),          "MOSFETs_degC"},
#endif
    {0x7C, TS_OUTPUT, TS_READ_ALL, TS_T_UINT16,  0, (void*) &(bms_status.soc),                  "SOC_%"},     // output will be uint8_t
    {0x7E, TS_OUTPUT, TS_READ_ALL, TS_T_UINT32,  0, (void*) &(bms_status.error_flags),          "ErrorFlags"},
    {0x7F, TS_OUTPUT, TS_READ_ALL, TS_T_UINT16,  0, (void*) &(bms_status.state),                "BmsState"},

    //{0x80, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages),        "Cells_V"},   // reserved for future cell voltage array
    {0x81, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[0]),     "Cell1_V"},
    {0x82, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[1]),     "Cell2_V"},
    {0x83, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[2]),     "Cell3_V"},
    {0x84, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[3]),     "Cell4_V"},
    {0x85, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[4]),     "Cell5_V"},
#if NUM_CELLS_MAX > 5
    {0x86, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[5]),     "Cell6_V"},
    {0x87, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[6]),     "Cell7_V"},
    {0x88, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[7]),     "Cell8_V"},
#if NUM_CELLS_MAX > 8
    {0x89, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[8]),     "Cell9_V"},
    {0x8A, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[9]),     "Cell10_V"},
#if NUM_CELLS_MAX > 10
    {0x8B, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[10]),    "Cell11_V"},
    {0x8C, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[11]),    "Cell12_V"},
    {0x8D, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[12]),    "Cell13_V"},
    {0x8E, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[13]),    "Cell14_V"},
    {0x8F, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[14]),    "Cell15_V"},
    //{0x90, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[15]),    "Cell16_V"},
    //{0x91, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[16]),    "Cell17_V"},
    //{0x92, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltages[17]),    "Cell18_V"},
#endif
#endif
#endif
    {0x9A, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltage_avg),     "CellAvg_V"},
    {0x9B, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltage_min),     "CellMin_V"},
    {0x9C, TS_OUTPUT, TS_READ_ALL, TS_T_FLOAT32, 3, (void*) &(bms_status.cell_voltage_max),     "CellMax_V"},

    // RECORDED DATA ///////////////////////////////////////////////////////
    // using IDs >= 0xA0

    // CALIBRATION DATA ///////////////////////////////////////////////////////
    // using IDs >= 0xD0

    // FUNCTION CALLS (EXEC) //////////////////////////////////////////////////
#ifndef UNIT_TEST
    //{0xE0, TS_EXEC, TS_EXEC_ALL, TS_Textern _BOOL, 0, (void*) &NVIC_SystemReset,     "Reset"},
#endif
    //{0xE1, TS_EXEC, TS_EXEC_ALL, TS_T_BOOL, 0, (void*) &start_dfu_bootloader, "BootloaderSTM"},
    //{0xE2, TS_EXEC, TS_EXEC_ALL, TS_T_BOOL, 0, (void*) &eeprom_store_data,    "SaveSettings"},
};

// stores object-ids of values to be published via Serial
const uint16_t pub_serial_ids[] = {
    0x01, // internal time stamp
    0x70, 0x71, 0x72, 0x73, 0x7A,  // voltage + current
    0x74, 0x76, 0x77,           // temperatures
    0x04, 0x78, 0x79,           // LoadState, ChgState, DCDCState
    0xA0, 0xA1, 0xA2, 0xA3,     // daily energy throughput
    0x0F, 0x10,     // SolarMaxDay_W, LoadMaxDay_W
    0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9,     // V, I, T max
    0x06, 0xA4  // SOC, coulomb counter
};

ts_pub_channel_t pub_channels[] = {
    { "Serial_1s",      pub_serial_ids,     sizeof(pub_serial_ids)/sizeof(uint16_t) },
};

// TODO: find better solution than manual configuration of channel number
volatile const int pub_channel_serial = 0;

ThingSet ts(
    data_objects, sizeof(data_objects)/sizeof(data_object_t),
    pub_channels, sizeof(pub_channels)/sizeof(ts_pub_channel_t)
);
