/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#define EEPROM_UPDATE_INTERVAL  (6*60*60)       // update every 6 hours

/**
 * @file
 *
 * @brief Handling of internal or external EEPROM to store device configuration
 */

/**
 * Store current charge controller data to EEPROM
 */
void eeprom_store_data();

/**
 * Restore charge controller data from EEPROM and write to variables in RAM
 */
void eeprom_restore_data();

/**
 * Stores data to EEPROM every 6 hours (can be called regularly)
 */
void eeprom_update();

#endif /* EEPROM_H_ */
