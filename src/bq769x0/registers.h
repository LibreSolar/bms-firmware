/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BQ769X0_REGISTERS_H_
#define BQ769X0_REGISTERS_H_

#include <stdint.h>

// register map
#define BQ769X0_SYS_STAT        0x00
#define BQ769X0_CELLBAL1        0x01
#define BQ769X0_CELLBAL2        0x02
#define BQ769X0_CELLBAL3        0x03
#define BQ769X0_SYS_CTRL1       0x04
#define BQ769X0_SYS_CTRL2       0x05
#define BQ769X0_PROTECT1        0x06
#define BQ769X0_PROTECT2        0x07
#define BQ769X0_PROTECT3        0x08
#define BQ769X0_OV_TRIP         0x09
#define BQ769X0_UV_TRIP         0x0A
#define BQ769X0_CC_CFG          0x0B

#define BQ769X0_VC1_HI_BYTE     0x0C
#define BQ769X0_VC1_LO_BYTE     0x0D
#define BQ769X0_VC2_HI_BYTE     0x0E
#define BQ769X0_VC2_LO_BYTE     0x0F
#define BQ769X0_VC3_HI_BYTE     0x10
#define BQ769X0_VC3_LO_BYTE     0x11
#define BQ769X0_VC4_HI_BYTE     0x12
#define BQ769X0_VC4_LO_BYTE     0x13
#define BQ769X0_VC5_HI_BYTE     0x14
#define BQ769X0_VC5_LO_BYTE     0x15
#define BQ769X0_VC6_HI_BYTE     0x16
#define BQ769X0_VC6_LO_BYTE     0x17
#define BQ769X0_VC7_HI_BYTE     0x18
#define BQ769X0_VC7_LO_BYTE     0x19
#define BQ769X0_VC8_HI_BYTE     0x1A
#define BQ769X0_VC8_LO_BYTE     0x1B
#define BQ769X0_VC9_HI_BYTE     0x1C
#define BQ769X0_VC9_LO_BYTE     0x1D
#define BQ769X0_VC10_HI_BYTE    0x1E
#define BQ769X0_VC10_LO_BYTE    0x1F
#define BQ769X0_VC11_HI_BYTE    0x20
#define BQ769X0_VC11_LO_BYTE    0x21
#define BQ769X0_VC12_HI_BYTE    0x22
#define BQ769X0_VC12_LO_BYTE    0x23
#define BQ769X0_VC13_HI_BYTE    0x24
#define BQ769X0_VC13_LO_BYTE    0x25
#define BQ769X0_VC14_HI_BYTE    0x26
#define BQ769X0_VC14_LO_BYTE    0x27
#define BQ769X0_VC15_HI_BYTE    0x28
#define BQ769X0_VC15_LO_BYTE    0x29

#define BQ769X0_BAT_HI_BYTE     0x2A
#define BQ769X0_BAT_LO_BYTE     0x2B

#define BQ769X0_TS1_HI_BYTE     0x2C
#define BQ769X0_TS1_LO_BYTE     0x2D
#define BQ769X0_TS2_HI_BYTE     0x2E
#define BQ769X0_TS2_LO_BYTE     0x2F
#define BQ769X0_TS3_HI_BYTE     0x30
#define BQ769X0_TS3_LO_BYTE     0x31

#define BQ769X0_CC_HI_BYTE      0x32
#define BQ769X0_CC_LO_BYTE      0x33

#define BQ769X0_ADCGAIN1        0x50
#define BQ769X0_ADCOFFSET       0x51
#define BQ769X0_ADCGAIN2        0x59

// for bit clear operations of the SYS_STAT register
#define BQ769X0_SYS_STAT_CC_READY           (0x80)
#define BQ769X0_SYS_STAT_DEVICE_XREADY      (0x20)
#define BQ769X0_SYS_STAT_OVRD_ALERT         (0x10)
#define BQ769X0_SYS_STAT_UV                 (0x08)
#define BQ769X0_SYS_STAT_OV                 (0x04)
#define BQ769X0_SYS_STAT_SCD                (0x02)
#define BQ769X0_SYS_STAT_OCD                (0x01)
#define BQ769X0_SYS_STAT_FLAGS              (0x3F)

#define LOW_BYTE(data)			(uint8_t)(0xff & data)
#define HIGH_BYTE(data)			(uint8_t)(0xff & (data >> 8))

// maps for settings in protection registers
static const uint16_t SCD_delay_setting [4] =
    { 70, 100, 200, 400 }; // us
static const uint16_t SCD_threshold_setting [8] =
    { 44, 67, 89, 111, 133, 155, 178, 200 }; // mV

static const uint16_t OCD_delay_setting [8] =
    { 8, 20, 40, 80, 160, 320, 640, 1280 }; // ms
static const uint16_t OCD_threshold_setting [16] =
    { 17, 22, 28, 33, 39, 44, 50, 56, 61, 67, 72, 78, 83, 89, 94, 100 };  // mV

static const uint16_t UV_delay_setting [4] = { 1, 4, 8, 16 };  // s
static const uint16_t OV_delay_setting [4] = { 1, 2, 4, 8 };   // s

typedef union {
    struct {
        uint8_t OCD            :1;
        uint8_t SCD            :1;
        uint8_t OV             :1;
        uint8_t UV             :1;
        uint8_t OVRD_ALERT     :1;
        uint8_t DEVICE_XREADY  :1;
        uint8_t RSVD           :1;
        uint8_t CC_READY       :1;
    };
    uint8_t byte;
} SYS_STAT_Type;

typedef union {
    struct {
        uint8_t SHUT_B        :1;
        uint8_t SHUT_A        :1;
        uint8_t RSVD1         :1;
        uint8_t TEMP_SEL      :1;
        uint8_t ADC_EN        :1;
        uint8_t RSVD2         :2;
        uint8_t LOAD_PRESENT  :1;
    };
    uint8_t byte;
} SYS_CTRL1_Type;

typedef union {
    struct {
        uint8_t CHG_ON      :1;
        uint8_t DSG_ON      :1;
        uint8_t WAKE_T      :2;
        uint8_t WAKE_EN     :1;
        uint8_t CC_ONESHOT  :1;
        uint8_t CC_EN       :1;
        uint8_t DELAY_DIS   :1;
    };
    uint8_t byte;
} SYS_CTRL2_Type;

typedef union {
    struct {
        uint8_t SCD_THRESH      :3;
        uint8_t SCD_DELAY       :2;
        uint8_t RSVD            :2;
        uint8_t RSNS            :1;
    };
    uint8_t byte;
} PROTECT1_Type;

typedef union {
    struct {
        uint8_t OCD_THRESH      :4;
        uint8_t OCD_DELAY       :3;
        uint8_t RSVD            :1;
    };
    uint8_t byte;
} PROTECT2_Type;

typedef union {
    struct {
        uint8_t RSVD            :4;
        uint8_t OV_DELAY        :2;
        uint8_t UV_DELAY        :2;
    };
    uint8_t byte;
} PROTECT3_Type;

typedef union {
    struct {
        uint8_t RSVD        :3;
        uint8_t CB5         :1;
        uint8_t CB4         :1;
        uint8_t CB3         :1;
        uint8_t CB2         :1;
        uint8_t CB1         :1;
    };
    uint8_t byte;
} CELLBAL_Type;

typedef union {
    struct {
        uint8_t BQ769X0_VC_HI;
        uint8_t BQ769X0_VC_LO;
    };
    uint16_t word;
} VCELL_Type;

#endif /* BQ769X0_REGISTERS_H_ */
