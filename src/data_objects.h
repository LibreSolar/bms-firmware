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

#ifndef DATA_OBJECTS_H
#define DATA_OBJECTS_H

#include "stdint.h"

typedef struct {
    const uint16_t id;
    const uint8_t access;
    const uint8_t category;
    const uint8_t type;
    const int8_t exponent;     // only for int types
    void* data;
    const char* name;
} DataObject_t;

// ThingSet CAN standard data types
#define TS_T_POS_INT8  0
#define TS_T_POS_INT16 1
#define TS_T_POS_INT32 2
#define TS_T_POS_INT64 3
#define TS_T_NEG_INT8  4
#define TS_T_NEG_INT16 5
#define TS_T_NEG_INT32 6
#define TS_T_NEG_INT64 7
#define TS_T_BYTE_STRING 8  
#define TS_T_UTF8_STRING 12  // 0x0C
#define TS_T_FLOAT32 30  // 0x1E
#define TS_T_FLOAT64 31  // 0x1F

#define TS_T_ARRAY 16    // 0x10
#define TS_T_MAP   20    // 0x14

// ThingSet CAN special types and simple values
#define TS_T_TIMESTAMP    32  // 0x20
#define TS_T_DECIMAL_FRAC 36  // 0x24
#define TS_T_FALSE        60  // 0x3C
#define TS_T_TRUE         61  // 0x3D
#define TS_T_NULL         62  // 0x3E
#define TS_T_UNDEFINED    63  // 0x3F

// C variable types
#define T_BOOL 0
//#define T_UINT32 1
#define T_INT32 2
#define T_FLOAT32 3
#define T_STRING 4

// ThingSet data object categories
#define TS_C_ALL 0
#define TS_C_DEVICE 1      // read-only device information (e.g. manufacturer, etc.)
#define TS_C_SETTINGS 2    // user-configurable settings (open access, maybe with user password)
#define TS_C_CAL 3         // factory-calibrated settings (access restricted)
#define TS_C_DIAGNOSIS 4   // error memory, etc (at least partly access restricted)
#define TS_C_INPUT 5       // free access
#define TS_C_OUTPUT 6      // free access

// internal access rights to data objects
#define ACCESS_READ (0x1U)
#define ACCESS_WRITE (0x1U << 1)
#define ACCESS_READ_AUTH (0x1U << 2)       // read after authentication
#define ACCESS_WRITE_AUTH (0x1U << 3)      // write after authentication

// Protocol functions
#define TS_READ     0x00
#define TS_WRITE    0x01
#define TS_PUB_REQ  0x02
#define TS_SUB_REQ  0x03
#define TS_OBJ_NAME 0x04
#define TS_LIST     0x05


extern int battery_voltage;
extern int battery_current;
extern int load_voltage;
extern int cell_voltages[15];      // max. number of cells
extern float temperatures[3];
extern float SOC;

static uint16_t oid;

static const DataObject_t dataObjects[] {
    // output data
    {oid=0x4001, ACCESS_READ, TS_C_OUTPUT, T_INT32,  -3, (void*) &(battery_voltage), "vBat"},
    {++oid, ACCESS_READ, TS_C_OUTPUT, T_INT32,  -3, (void*) &(load_voltage), "vLoad"},
    {++oid, ACCESS_READ, TS_C_OUTPUT, T_INT32,  -3, (void*) &(cell_voltages[0]), "vCell1"},
    {++oid, ACCESS_READ, TS_C_OUTPUT, T_INT32,  -3, (void*) &(cell_voltages[1]), "vCell2"},
    {++oid, ACCESS_READ, TS_C_OUTPUT, T_INT32,  -3, (void*) &(cell_voltages[2]), "vCell3"},
    {++oid, ACCESS_READ, TS_C_OUTPUT, T_INT32,  -3, (void*) &(cell_voltages[3]), "vCell4"},
    {++oid, ACCESS_READ, TS_C_OUTPUT, T_INT32,  -3, (void*) &(cell_voltages[4]), "vCell5"},
    {++oid, ACCESS_READ, TS_C_OUTPUT, T_INT32,  -3, (void*) &(battery_current), "iBat"},
    {++oid, ACCESS_READ, TS_C_OUTPUT, T_FLOAT32, 0, (void*) &(temperatures[0]), "tempBat"},
    {++oid, ACCESS_READ, TS_C_OUTPUT, T_FLOAT32, 0, (void*) &(SOC), "SOC"} 
};


#endif
