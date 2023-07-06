/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DATA_OBJECTS_H_
#define DATA_OBJECTS_H_

/**
 * @file
 *
 * @brief Handling of ThingSet data nodes
 */

#include <thingset.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*
 * Categories / first layer node IDs
 */

/* Device information (e.g. manufacturer, etc.) */
#define APP_ID_DEVICE                0x04
#define APP_ID_DEVICE_MANUFACTURER   0x40
#define APP_ID_DEVICE_TYPE           0x41
#define APP_ID_DEVICE_HW_VER         0x42
#define APP_ID_DEVICE_FW_VER         0x43
#define APP_ID_DEVICE_SHUTDOWN       0x4A
#define APP_ID_DEVICE_RESET          0x4B
#define APP_ID_DEVICE_PRINT_REG      0x4C
#define APP_ID_DEVICE_PRINT_REG_ADDR 0x4D
#define APP_ID_DEVICE_PRINT_REGS     0x4E

/* Configurable data (settings) */
#define APP_ID_CONF                         0x05
#define APP_ID_CONF_NOMINAL_CAPACITY        0x50
#define APP_ID_CONF_SHORT_CIRCUIT_CURRENT   0x51
#define APP_ID_CONF_SHORT_CIRCUIT_DELAY     0x52
#define APP_ID_CONF_DIS_OVERCURRENT         0x53
#define APP_ID_CONF_DIS_OVERCURRENT_DELAY   0x54
#define APP_ID_CONF_CHG_OVERCURRENT         0x55
#define APP_ID_CONF_CHG_OVERCURRENT_DELAY   0x56
#define APP_ID_CONF_DIS_MAX_TEMP            0x58
#define APP_ID_CONF_DIS_MIN_TEMP            0x59
#define APP_ID_CONF_CHG_MAX_TEMP            0x5A
#define APP_ID_CONF_CHG_MIN_TEMP            0x5B
#define APP_ID_CONF_TEMP_HYST               0x5C
#define APP_ID_CONF_CELL_OVERVOLTAGE        0x60
#define APP_ID_CONF_CELL_OVERVOLTAGE_RESET  0x61
#define APP_ID_CONF_CELL_OVERVOLTAGE_DELAY  0x62
#define APP_ID_CONF_CELL_UNDERVOLTAGE       0x63
#define APP_ID_CONF_CELL_UNDERVOLTAGE_RESET 0x64
#define APP_ID_CONF_CELL_UNDERVOLTAGE_DELAY 0x65
#define APP_ID_CONF_BAL_TARGET_DIFF         0x68
#define APP_ID_CONF_BAL_MIN_VOLTAGE         0x69
#define APP_ID_CONF_BAL_IDLE_DELAY          0x6A
#define APP_ID_CONF_BAL_IDLE_CURRENT        0x6B
#define APP_ID_CONF_PRESET_NMC              0xA0
#define APP_ID_CONF_PRESET_NMC_CAPACITY     0xA1
#define APP_ID_CONF_PRESET_LFP              0xA2
#define APP_ID_CONF_PRESET_LFP_CAPACITY     0xA3

/* Measurement data */
#define APP_ID_MEAS                  0x07
#define APP_ID_MEAS_PACK_VOLTAGE     0x71
#define APP_ID_MEAS_STACK_VOLTAGE    0x72
#define APP_ID_MEAS_PACK_CURRENT     0x73
#define APP_ID_MEAS_BAT_TEMP         0x74
#define APP_ID_MEAS_IC_TEMP          0x75
#define APP_ID_MEAS_MCU_TEMP         0x76
#define APP_ID_MEAS_MOSFET_TEMP      0x77
#define APP_ID_MEAS_SOC              0x7C
#define APP_ID_MEAS_ERROR_FLAGS      0x7E
#define APP_ID_MEAS_BMS_STATE        0x7F
#define APP_ID_MEAS_CELL_VOLTAGES    0x80
#define APP_ID_MEAS_CELL_AVG_VOLTAGE 0x81
#define APP_ID_MEAS_CELL_MIN_VOLTAGE 0x82
#define APP_ID_MEAS_CELL_MAX_VOLTAGE 0x83
#define APP_ID_MEAS_BALANCING_STATUS 0x84

/* Input data (e.g. set-points) */
#define APP_ID_INPUT            0x09
#define APP_ID_INPUT_CHG_ENABLE 0x90
#define APP_ID_INPUT_DIS_ENABLE 0x91

/**
 * Callback function to be called when conf values were changed
 */
void data_objects_update_conf(enum thingset_callback_reason reason);

/**
 * Callback function to apply preset parameters for NMC type via ThingSet
 */
int32_t bat_preset_nmc();

/**
 * Callback function to apply preset parameters for LFP type via ThingSet
 */
int32_t bat_preset_lfp();

/**
 * Callback to read and print BMS register via ThingSet
 */
void print_register();

/**
 * Callback to reset device (obviously...)
 */
void reset_device();

/**
 * Callback to invoke bms_shutdown via ThingSet
 */
void shutdown();

#endif /* DATA_OBJECTS_H_ */
