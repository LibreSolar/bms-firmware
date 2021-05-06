/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DATA_NODES_H_
#define DATA_NODES_H_

/**
 * @file
 *
 * @brief Handling of ThingSet data nodes
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*
 * Categories / first layer node IDs
 */
#define ID_ROOT     0x00
#define ID_INFO     0x18        // read-only device information (e.g. manufacturer, device ID)
#define ID_CONF     0x30        // configurable settings
#define ID_INPUT    0x60        // input data (e.g. set-points)
#define ID_OUTPUT   0x70        // output data (e.g. measurement values)
#define ID_REC      0xA0        // recorded data (history-dependent)
#define ID_CAL      0xD0        // calibration
#define ID_EXEC     0xE0        // function call
#define ID_AUTH     0xEA
#define ID_PUB      0xF0        // publication setup
#define ID_SUB      0xF1        // subscription setup
#define ID_LOG      0x100       // access log data

/*
 * Publish/subscribe channels
 */
#define PUB_SER     (1U << 0)   // UART serial
#define PUB_CAN     (1U << 1)   // CAN bus
#define PUB_NVM     (1U << 2)   // data that should be stored in EEPROM

/*
 * Data node versioning for EEPROM
 *
 * Increment the version number each time any data node IDs stored in NVM are changed. Otherwise
 * data might get corrupted.
 */
#define DATA_NODES_VERSION 1

extern bool pub_serial_enable;
extern bool pub_can_enable;

/**
 * Callback function to be called when conf values were changed
 */
void data_nodes_update_conf();

/**
 * Initializes and reads data nodes from EEPROM
 */
void data_nodes_init();

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

#endif /* DATA_NODES_H_ */
