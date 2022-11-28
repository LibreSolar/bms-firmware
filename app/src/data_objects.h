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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
/*
 * Categories / first layer node IDs
 */
#define ID_ROOT   0x00
#define ID_DEVICE 0x04   // read-only device information (e.g. manufacturer, device ID)
#define ID_CONF   0x05   // configurable data (settings)
#define ID_MEAS   0x07   // output data (e.g. measurement values)
#define ID_INPUT  0x09   // input data (e.g. set-points)
#define ID_CAL    0x0C   // calibration
#define ID_RPC    0x0E   // remote procedure calls
#define ID_CTRL   0x8000 // control functions

/**
 * Callback function to be called when conf values were changed
 */
void data_objects_update_conf();

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
