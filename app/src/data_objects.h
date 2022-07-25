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
#define ID_INFO   0x01   // read-only device information (e.g. manufacturer, device ID)
#define ID_MEAS   0x02   // output data (e.g. measurement values)
#define ID_STATE  0x03   // recorded data (history-dependent)
#define ID_REC    0x04   // recorded data (history-dependent)
#define ID_INPUT  0x05   // input data (e.g. set-points)
#define ID_CONF   0x06   // configurable settings
#define ID_CAL    0x07   // calibration
#define ID_REPORT 0x0A   // reports
#define ID_RPC    0x0E   // remote procedure calls
#define ID_DFU    0x0F   // device firmware upgrade
#define ID_PUB    0x100  // publication setup
#define ID_CTRL   0x8000 // control functions

/*
 * Subset definitions for statements and publish/subscribe
 */
#define SUBSET_SER  (1U << 0) // UART serial
#define SUBSET_CAN  (1U << 1) // CAN bus
#define SUBSET_NVM  (1U << 2) // data that should be stored in EEPROM
#define SUBSET_CTRL (1U << 3) // control data sent and received via CAN

/*
 * Data node versioning for EEPROM
 *
 * Increment the version number each time any data node IDs stored in NVM are changed. Otherwise
 * data might get corrupted.
 */
#define DATA_OBJECTS_VERSION 1

extern bool pub_serial_enable;
extern bool pub_can_enable;

/**
 * Callback function to be called when conf values were changed
 */
void data_objects_update_conf();

/**
 * Initializes and reads data nodes from EEPROM
 */
void data_objects_init();

/**
 * Callback to read and print BMS register via ThingSet
 */
void print_register();

/**
 * Callback to reset device (obviously...)
 */
void reset_device();

/**
 * Callback to provide authentication mechanism via ThingSet
 */
void thingset_auth();

/**
 * Alphabet used for base32 encoding
 *
 * https://en.wikipedia.org/wiki/Base32#Crockford's_Base32
 */
const char alphabet_crockford[] = "0123456789abcdefghjkmnpqrstvwxyz";

/**
 * Convert numeric device ID to base32 string
 */
void uint64_to_base32(uint64_t in, char *out, size_t size, const char *alphabet);

#endif /* DATA_OBJECTS_H_ */
