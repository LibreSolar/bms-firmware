/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

// register map
#define SYS_STAT        0x00
#define CELLBAL1        0x01
#define CELLBAL2        0x02
#define CELLBAL3        0x03
#define SYS_CTRL1       0x04
#define SYS_CTRL2       0x05
#define PROTECT1        0x06
#define PROTECT2        0x07
#define PROTECT3        0x08
#define OV_TRIP         0x09
#define UV_TRIP         0x0A
#define CC_CFG          0x0B

#define VC1_HI_BYTE     0x0C
#define VC1_LO_BYTE     0x0D
#define VC2_HI_BYTE     0x0E
#define VC2_LO_BYTE     0x0F
#define VC3_HI_BYTE     0x10
#define VC3_LO_BYTE     0x11
#define VC4_HI_BYTE     0x12
#define VC4_LO_BYTE     0x13
#define VC5_HI_BYTE     0x14
#define VC5_LO_BYTE     0x15
#define VC6_HI_BYTE     0x16
#define VC6_LO_BYTE     0x17
#define VC7_HI_BYTE     0x18
#define VC7_LO_BYTE     0x19
#define VC8_HI_BYTE     0x1A
#define VC8_LO_BYTE     0x1B
#define VC9_HI_BYTE     0x1C
#define VC9_LO_BYTE     0x1D
#define VC10_HI_BYTE    0x1E
#define VC10_LO_BYTE    0x1F
#define VC11_HI_BYTE    0x20
#define VC11_LO_BYTE    0x21
#define VC12_HI_BYTE    0x22
#define VC12_LO_BYTE    0x23
#define VC13_HI_BYTE    0x24
#define VC13_LO_BYTE    0x25
#define VC14_HI_BYTE    0x26
#define VC14_LO_BYTE    0x27
#define VC15_HI_BYTE    0x28
#define VC15_LO_BYTE    0x29

#define BAT_HI_BYTE     0x2A
#define BAT_LO_BYTE     0x2B

#define TS1_HI_BYTE     0x2C
#define TS1_LO_BYTE     0x2D
#define TS2_HI_BYTE     0x2E
#define TS2_LO_BYTE     0x2F
#define TS3_HI_BYTE     0x30
#define TS3_LO_BYTE     0x31

#define CC_HI_BYTE      0x32
#define CC_LO_BYTE      0x33

#define ADCGAIN1        0x50
#define ADCOFFSET       0x51
#define ADCGAIN2        0x59

// function from TI reference design
#define LOW_BYTE(Data)			(uint8_t)(0xff & Data)
#define HIGH_BYTE(Data)			(uint8_t)(0xff & (Data >> 8))

// for bit clear operations of the SYS_STAT register
#define STAT_CC_READY           (0x80)
#define STAT_DEVICE_XREADY      (0x20)
#define STAT_OVRD_ALERT         (0x10)
#define STAT_UV                 (0x08)
#define STAT_OV                 (0x04)
#define STAT_SCD                (0x02)
#define STAT_OCD                (0x01)
#define STAT_FLAGS              (0x3F)

// maps for settings in protection registers
const uint16_t SCD_delay_setting [4] =
    { 70, 100, 200, 400 }; // us
const uint16_t SCD_threshold_setting [8] =
    { 44, 67, 89, 111, 133, 155, 178, 200 }; // mV

const uint16_t OCD_delay_setting [8] =
    { 8, 20, 40, 80, 160, 320, 640, 1280 }; // ms
const uint16_t OCD_threshold_setting [16] =
    { 17, 22, 28, 33, 39, 44, 50, 56, 61, 67, 72, 78, 83, 89, 94, 100 };  // mV

const uint16_t UV_delay_setting [4] = { 1, 4, 8, 16 };  // s
const uint16_t OV_delay_setting [4] = { 1, 2, 4, 8 };   // s

typedef union regSYS_STAT {
    struct {
        uint8_t OCD            :1;
        uint8_t SCD            :1;
        uint8_t OV             :1;
        uint8_t UV             :1;
        uint8_t OVRD_ALERT     :1;
        uint8_t DEVICE_XREADY  :1;
        uint8_t RSVD           :1;
        uint8_t CC_READY       :1;
    } bits;
    uint8_t regByte;
} regSYS_STAT_t;

typedef union regSYS_CTRL1 {
    struct {
        uint8_t SHUT_B        :1;
        uint8_t SHUT_A        :1;
        uint8_t RSVD1         :1;
        uint8_t TEMP_SEL      :1;
        uint8_t ADC_EN        :1;
        uint8_t RSVD2         :2;
        uint8_t LOAD_PRESENT  :1;
    } bits;
    uint8_t regByte;
} regSYS_CTRL1_t;

typedef union regSYS_CTRL2 {
    struct {
        uint8_t CHG_ON      :1;
        uint8_t DSG_ON      :1;
        uint8_t WAKE_T      :2;
        uint8_t WAKE_EN     :1;
        uint8_t CC_ONESHOT  :1;
        uint8_t CC_EN       :1;
        uint8_t DELAY_DIS   :1;
    } bits;
    uint8_t regByte;
} regSYS_CTRL2_t;

typedef union regPROTECT1 {
    struct {
        uint8_t SCD_THRESH      :3;
        uint8_t SCD_DELAY       :2;
        uint8_t RSVD            :2;
        uint8_t RSNS            :1;
    } bits;
    uint8_t regByte;
} regPROTECT1_t;

typedef union regPROTECT2 {
    struct {
        uint8_t OCD_THRESH      :4;
        uint8_t OCD_DELAY       :3;
        uint8_t RSVD            :1;
    } bits;
    uint8_t regByte;
} regPROTECT2_t;

typedef union regPROTECT3 {
    struct {
        uint8_t RSVD            :4;
        uint8_t OV_DELAY        :2;
        uint8_t UV_DELAY        :2;
    } bits;
    uint8_t regByte;
} regPROTECT3_t;

typedef union regCELLBAL {
    struct {
        uint8_t RSVD        :3;
        uint8_t CB5         :1;
        uint8_t CB4         :1;
        uint8_t CB3         :1;
        uint8_t CB2         :1;
        uint8_t CB1         :1;
    } bits;
    uint8_t regByte;
} regCELLBAL_t;

typedef union regVCELL {
    struct {
        uint8_t VC_HI;
        uint8_t VC_LO;
    } bytes;
    uint16_t regWord;
} regVCELL_t;
