/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BQ769X2_REGISTERS_H_
#define BQ769X2_REGISTERS_H_

#include <stdint.h>

// Direct commands

#define BQ769X2_CMD_CONTROL_STATUS      0x00

#define BQ769X2_CMD_SAFETY_ALERT_A      0x02
#define BQ769X2_CMD_SAFETY_STATUS_A     0x03
#define BQ769X2_CMD_SAFETY_ALERT_B      0x04
#define BQ769X2_CMD_SAFETY_STATUS_B     0x05
#define BQ769X2_CMD_SAFETY_ALERT_C      0x06
#define BQ769X2_CMD_SAFETY_STATUS_C     0x07

#define BQ769X2_CMD_PF_ALERT_A          0x0A
#define BQ769X2_CMD_PF_STATUS_A         0x0B
#define BQ769X2_CMD_PF_ALERT_B          0x0C
#define BQ769X2_CMD_PF_STATUS_B         0x0D
#define BQ769X2_CMD_PF_ALERT_C          0x0E
#define BQ769X2_CMD_PF_STATUS_C         0x0F
#define BQ769X2_CMD_PF_ALERT_D          0x10
#define BQ769X2_CMD_PF_STATUS_D         0x11

#define BQ769X2_CMD_BATTERY_STATUS      0x12

#define BQ769X2_CMD_VOLTAGE_CELL_1      0x14
#define BQ769X2_CMD_VOLTAGE_CELL_2      0x16
#define BQ769X2_CMD_VOLTAGE_CELL_3      0x18
#define BQ769X2_CMD_VOLTAGE_CELL_4      0x1A
#define BQ769X2_CMD_VOLTAGE_CELL_5      0x1C
#define BQ769X2_CMD_VOLTAGE_CELL_6      0x1E
#define BQ769X2_CMD_VOLTAGE_CELL_7      0x20
#define BQ769X2_CMD_VOLTAGE_CELL_8      0x22
#define BQ769X2_CMD_VOLTAGE_CELL_9      0x24
#define BQ769X2_CMD_VOLTAGE_CELL_10     0x26
#define BQ769X2_CMD_VOLTAGE_CELL_11     0x28
#define BQ769X2_CMD_VOLTAGE_CELL_12     0x2A
#define BQ769X2_CMD_VOLTAGE_CELL_13     0x2C
#define BQ769X2_CMD_VOLTAGE_CELL_14     0x2E
#define BQ769X2_CMD_VOLTAGE_CELL_15     0x30
#define BQ769X2_CMD_VOLTAGE_CELL_16     0x32
#define BQ769X2_CMD_VOLTAGE_STACK       0x34
#define BQ769X2_CMD_VOLTAGE_PACK        0x36
#define BQ769X2_CMD_VOLTAGE_LD          0x38

#define BQ769X2_CMD_CURRENT_CC2         0x3A

#define BQ769X2_CMD_ALARM_STATUS        0x62
#define BQ769X2_CMD_ALARM_RAW_STATUS    0x64
#define BQ769X2_CMD_ALARM_ENABLE        0x66

#define BQ769X2_CMD_TEMP_INT            0x68
#define BQ769X2_CMD_TEMP_CFETOFF        0x6A
#define BQ769X2_CMD_TEMP_DFETOFF        0x6C
#define BQ769X2_CMD_TEMP_ALERT          0x6E
#define BQ769X2_CMD_TEMP_TS1            0x70
#define BQ769X2_CMD_TEMP_TS2            0x72
#define BQ769X2_CMD_TEMP_TS3            0x74
#define BQ769X2_CMD_TEMP_HDQ            0x76
#define BQ769X2_CMD_TEMP_DCHG           0x78
#define BQ769X2_CMD_TEMP_DDSG           0x7A

#define BQ769X2_CMD_FET_STATUS          0x7F

// Subcommands

#define BQ769X2_CMD_SUBCMD_LOWER        0x3E
#define BQ769X2_CMD_SUBCMD_UPPER        0x3F

// Data Memory

// Calibration (manual section 13.2)

// TODO

// Settings (manual section 13.3)

#define BQ769X2_SET_FUSE_BLOW_VOLTAGE       0x9231
#define BQ769X2_SET_FUSE_BLOW_TIMEOUT       0x9233

#define BQ769X2_SET_CONF_POWER              0x9234
#define BQ769X2_SET_CONF_REG12              0x9236
#define BQ769X2_SET_CONF_REG0               0x9237
#define BQ769X2_SET_CONF_HWD_REG_OPT        0x9238
#define BQ769X2_SET_CONF_COMM_TYPE          0x9239
#define BQ769X2_SET_CONF_I2C_ADDR           0x923A
#define BQ769X2_SET_CONF_SPI                0x923C
#define BQ769X2_SET_CONF_COMM_IDLE          0x923D
#define BQ769X2_SET_CONF_CFETOFF            0x92FA
#define BQ769X2_SET_CONF_DFETOFF            0x92FB
#define BQ769X2_SET_CONF_ALERT              0x92FC
#define BQ769X2_SET_CONF_TS1                0x92FD
#define BQ769X2_SET_CONF_TS2                0x92FE
#define BQ769X2_SET_CONF_TS3                0x92FF
#define BQ769X2_SET_CONF_HDQ                0x9300
#define BQ769X2_SET_CONF_DCHG               0x9301
#define BQ769X2_SET_CONF_DDSG               0x9302
#define BQ769X2_SET_CONF_DA                 0x9303
#define BQ769X2_SET_CONF_VCELL_MODE         0x9304
#define BQ769X2_SET_CONF_CC3_SAMPLES        0x9307

#define BQ769X2_SET_PROT_CONF               0x925F
#define BQ769X2_SET_PROT_ENABLED_A          0x9261
#define BQ769X2_SET_PROT_ENABLED_B          0x9262
#define BQ769X2_SET_PROT_ENABLED_C          0x9263
#define BQ769X2_SET_PROT_CHG_FET_A          0x9265
#define BQ769X2_SET_PROT_CHG_FET_B          0x9266
#define BQ769X2_SET_PROT_CHG_FET_C          0x9267
#define BQ769X2_SET_PROT_DSG_FET_A          0x9269
#define BQ769X2_SET_PROT_DSG_FET_B          0x926A
#define BQ769X2_SET_PROT_DSG_FET_C          0x926B
#define BQ769X2_SET_PROT_BODY_DIODE_TH      0x9273

#define BQ769X2_SET_ALARM_DEFAULT_MASK      0x926D
#define BQ769X2_SET_ALARM_SF_ALERT_MASK_A   0x926F
#define BQ769X2_SET_ALARM_SF_ALERT_MASK_B   0x9270
#define BQ769X2_SET_ALARM_SF_ALERT_MASK_C   0x9271
#define BQ769X2_SET_ALARM_PF_ALERT_MASK_A   0x92C4
#define BQ769X2_SET_ALARM_PF_ALERT_MASK_B   0x92C5
#define BQ769X2_SET_ALARM_PF_ALERT_MASK_C   0x92C6
#define BQ769X2_SET_ALARM_PF_ALERT_MASK_D   0x92C7

#define BQ769X2_SET_PF_ENABLED_A            0x92C0
#define BQ769X2_SET_PF_ENABLED_B            0x92C1
#define BQ769X2_SET_PF_ENABLED_C            0x92C2
#define BQ769X2_SET_PF_ENABLED_D            0x92C3

#define BQ769X2_SET_FET_OPTIONS             0x9308
#define BQ769X2_SET_FET_CHG_PUMP            0x9309
#define BQ769X2_SET_FET_PCHG_START_V        0x930A // mV
#define BQ769X2_SET_FET_PCHG_STOP_V         0x930C // mV
#define BQ769X2_SET_FET_PDSG_TIMEOUT        0x930E // 10 ms
#define BQ769X2_SET_FET_PDSG_STOP_DU        0x930F // 10 mV

#define BQ769X2_SET_DSG_CURR_TH             0x9310 // userA
#define BQ769X2_SET_CHG_CURR_TH             0x9312 // userA

#define BQ769X2_SET_OPEN_WIRE_CHECK_TIME    0x9314 // s

#define BQ769X2_SET_INTERCONN_RES_1         0x9315 // mOhm
#define BQ769X2_SET_INTERCONN_RES_2         0x9317 // mOhm
#define BQ769X2_SET_INTERCONN_RES_3         0x9319 // mOhm
#define BQ769X2_SET_INTERCONN_RES_4         0x931B // mOhm
#define BQ769X2_SET_INTERCONN_RES_5         0x931D // mOhm
#define BQ769X2_SET_INTERCONN_RES_6         0x931F // mOhm
#define BQ769X2_SET_INTERCONN_RES_7         0x9321 // mOhm
#define BQ769X2_SET_INTERCONN_RES_8         0x9323 // mOhm
#define BQ769X2_SET_INTERCONN_RES_9         0x9325 // mOhm
#define BQ769X2_SET_INTERCONN_RES_10        0x9327 // mOhm
#define BQ769X2_SET_INTERCONN_RES_11        0x9329 // mOhm
#define BQ769X2_SET_INTERCONN_RES_12        0x932B // mOhm
#define BQ769X2_SET_INTERCONN_RES_13        0x932D // mOhm
#define BQ769X2_SET_INTERCONN_RES_14        0x932F // mOhm
#define BQ769X2_SET_INTERCONN_RES_15        0x9331 // mOhm
#define BQ769X2_SET_INTERCONN_RES_16        0x9333 // mOhm

#define BQ769X2_SET_MFG_STATUS_INIT         0x9343

#define BQ769X2_SET_CBAL_CONF               0x9335
#define BQ769X2_SET_CBAL_MIN_CELL_TEMP      0x9336 // °C
#define BQ769X2_SET_CBAL_MAX_CELL_TEMP      0x9337 // °C
#define BQ769X2_SET_CBAL_MAX_INT_TEMP       0x9338 // °C
#define BQ769X2_SET_CBAL_INTERVAL           0x9339 // s
#define BQ769X2_SET_CBAL_MAX_CELLS          0x933A
#define BQ769X2_SET_CBAL_CHG_MIN_CELL_V     0x933B // mV
#define BQ769X2_SET_CBAL_CHG_MIN_DELTA      0x933D // mV
#define BQ769X2_SET_CBAL_CHG_STOP_DELTA     0x933E // mV
#define BQ769X2_SET_CBAL_RLX_MIN_CELL_V     0x933F // mV
#define BQ769X2_SET_CBAL_RLX_MIN_DELTA      0x9341 // mV
#define BQ769X2_SET_CBAL_RLX_STOP_DELTA     0x9342 // mV

// Power (manual section 13.4)

#define BQ769X2_PWR_SHUTDOWN_CELL_V         0x923F // mV
#define BQ769X2_PWR_SHUTDOWN_STACK_V        0x9241 // 10 mV
#define BQ769X2_PWR_SHUTDOWN_LOW_V_DELAY    0x9243 // s
#define BQ769X2_PWR_SHUTDOWN_TEMP           0x9244 // °C
#define BQ769X2_PWR_SHUTDOWN_TEMP_DELAY     0x9245 // s
#define BQ769X2_PWR_SHUTDOWN_FET_OFF_DELAY  0x9252 // 0.25 s
#define BQ769X2_PWR_SHUTDOWN_CMD_DELAY      0x9253 // 0.25 s
#define BQ769X2_PWR_SHUTDOWN_AUTO_TIME      0x9254 // min
#define BQ769X2_PWR_SHUTDOWN_RAM_FAIL_TIME  0x9255 // s
#define BQ769X2_PWR_SLEEP_CURRENT           0x9248 // mA
#define BQ769X2_PWR_SLEEP_VOLTAGE_TIME      0x924A // s
#define BQ769X2_PWR_SLEEP_WAKE_COMP_CURRENT 0x924B // mA
#define BQ769X2_PWR_SLEEP_HYST_TIME         0x924D // s
#define BQ769X2_PWR_SLEEP_CHG_V_TH          0x924E // 10 mV
#define BQ769X2_PWR_SLEEP_PACK_TOS_DELTA    0x9250 // 10 mV

// System Data (manual section 13.5)

#define BQ769X2_SYS_CONFIG_RAM_SIGNATURE    0x91E0

// Protections (manual section 13.6)

#define BQ769X2_PROT_CUV_THRESHOLD          0x9275 // 50.6 mV
#define BQ769X2_PROT_CUV_DELAY              0x9276 // 3.3 ms
#define BQ769X2_PROT_CUV_RECOV_HYST         0x927B // 50.6 mV
#define BQ769X2_PROT_COV_THRESHOLD          0x9278 // 50.6 mV
#define BQ769X2_PROT_COV_DELAY              0x9279 // 3.3 ms
#define BQ769X2_PROT_COV_RECOV_HYST         0x927C // 50.6 mV
#define BQ769X2_PROT_COVL_LATCH_LIMIT       0x927D
#define BQ769X2_PROT_COVL_COUNTER_DEC_DELAY 0x927E // s
#define BQ769X2_PROT_COVL_RECOV_TIME        0x927F // s
#define BQ769X2_PROT_OCC_THRESHOLD          0x9280 // 2 mV
#define BQ769X2_PROT_OCC_DELAY              0x9281 // 3.3 ms
#define BQ769X2_PROT_OCC_RECOVERY           0x9288 // mA
#define BQ769X2_PROT_OCC_PACK_TOS_DELTA     0x92B0 // 10 mV
#define BQ769X2_PROT_OCD1_THRESHOLD         0x9282 // 2 mV
#define BQ769X2_PROT_OCD1_DELAY             0x9283 // 3.3 ms
#define BQ769X2_PROT_OCD2_THRESHOLD         0x9284 // 2 mV
#define BQ769X2_PROT_OCD2_DELAY             0x9285 // 3.3 ms
#define BQ769X2_PROT_SCD_THRESHOLD          0x9286 // see SCD_threshold_setting
#define BQ769X2_PROT_SCD_DELAY              0x9287 // 15 us
#define BQ769X2_PROT_SCD_RECOV_TIME         0x9294 // s
#define BQ769X2_PROT_OCD3_THRESHOLD         0x928A // userA
#define BQ769X2_PROT_OCD3_DELAY             0x928C // s
#define BQ769X2_PROT_OCD_RECOV_THRESHOLD    0x928D // mA
#define BQ769X2_PROT_OCDL_LATCH_LIMIT       0x928F
#define BQ769X2_PROT_OCDL_COUNTER_DEC_DELAY 0x9290 // s
#define BQ769X2_PROT_OCDL_RECOV_TIME        0x9291 // s
#define BQ769X2_PROT_OCDL_RECOV_THRESHOLD   0x9292 // mA
#define BQ769X2_PROT_SCDL_LATCH_LIMIT       0x9295
#define BQ769X2_PROT_SCDL_COUNTER_DEC_DELAY 0x9296 // s
#define BQ769X2_PROT_SCDL_RECOV_TIME        0x9297 // s
#define BQ769X2_PROT_SCDL_RECOV_THRESHOLD   0x9298 // mA
#define BQ769X2_PROT_OTC_THRESHOLD          0x929A // °C
#define BQ769X2_PROT_OTC_DELAY              0x929B // s
#define BQ769X2_PROT_OTC_RECOVERY           0x929C // °C
#define BQ769X2_PROT_OTD_THRESHOLD          0x929D // °C
#define BQ769X2_PROT_OTD_DELAY              0x929E // s
#define BQ769X2_PROT_OTD_RECOVERY           0x929F // °C
#define BQ769X2_PROT_OTF_THRESHOLD          0x92A0 // °C
#define BQ769X2_PROT_OTF_DELAY              0x92A1 // s
#define BQ769X2_PROT_OTF_RECOVERY           0x92A2 // °C
#define BQ769X2_PROT_OTINT_THRESHOLD        0x92A3 // °C
#define BQ769X2_PROT_OTINT_DELAY            0x92A4 // s
#define BQ769X2_PROT_OTINT_RECOVERY         0x92A5 // °C
#define BQ769X2_PROT_UTC_THRESHOLD          0x92A6 // °C
#define BQ769X2_PROT_UTC_DELAY              0x92A7 // s
#define BQ769X2_PROT_UTC_RECOVERY           0x92A8 // °C
#define BQ769X2_PROT_UTD_THRESHOLD          0x92A9 // °C
#define BQ769X2_PROT_UTD_DELAY              0x92AA // s
#define BQ769X2_PROT_UTD_RECOVERY           0x92AB // °C
#define BQ769X2_PROT_UTINT_THRESHOLD        0x92AC // °C
#define BQ769X2_PROT_UTINT_DELAY            0x92AD // °C
#define BQ769X2_PROT_UTINT_RECOVERY         0x92AE // °C
#define BQ769X2_PROT_RECOVERY_TIME          0x92AF // s
#define BQ769X2_PROT_HWD_DELAY              0x92B2 // s
#define BQ769X2_PROT_LOAD_DET_ACTIVE_TIME   0x92B4 // s
#define BQ769X2_PROT_LOAD_DET_RETRY_DELAY   0x92B5 // s
#define BQ769X2_PROT_LOAD_DET_TIMEOUT       0x92B6 // hrs
#define BQ769X2_PROT_PTO_CHG_THRESHOLD      0x92BA // mA
#define BQ769X2_PROT_PTO_DELAY              0x92BC // s
#define BQ769X2_PROT_PTO_RESET              0x92BE // Ah

// Permanent Fail (manual section 13.7)

// TODO

// Security (manual section 13.8)

// TODO

// map for short-circuit protection thresholds (overcurrent etc. can be set to arbitrary values)
static const uint16_t SCD_threshold_setting [] =
    { 10, 20, 40, 60, 80, 100, 125, 150, 175, 200, 250, 300, 350, 400, 450, 500 }; // mV

// Status register content

typedef union {
    struct {
        uint8_t RSVD        :2;
        uint8_t CUV         :1;     // Cell undervoltage
        uint8_t COV         :1;     // Cell overvoltage
        uint8_t OCC         :1;     // Overcurrent in charge
        uint8_t OCD1        :1;     // Overcurrent in discharge 1
        uint8_t OCD2        :1;     // Overcurrent in discharge 2
        uint8_t SCD         :1;     // Short circuit in discharge
    };
    uint8_t byte;
} SAFETY_STATUS_A_Type;

typedef union {
    struct {
        uint8_t UTC         :1;     // Undertemperature in charge
        uint8_t UTD         :1;     // Undertemperature in discharge
        uint8_t UTINT       :1;     // Internal die undertemperature
        uint8_t RSVD        :1;
        uint8_t OTC         :1;     // Overtemperature in charge
        uint8_t OTD         :1;     // Overtemperature in discharge
        uint8_t OTINT       :1;     // Internal die overtemperature
        uint8_t OTF         :1;     // FET overtemperature
    };
    uint8_t byte;
} SAFETY_STATUS_B_Type;

typedef union {
    struct {
        uint8_t RSVD1       :1;
        uint8_t HWDF        :1;     // Host watchdog safety fault
        uint8_t PTO         :1;     // Precharge timeout
        uint8_t RSVD2       :1;
        uint8_t COVL        :1;     // Latched cell overvoltage
        uint8_t OCDL        :1;     // Latched overcurrent in discharge
        uint8_t SCDL        :1;     // Latched short circuit in discharge
        uint8_t OCD3        :1;     // Overcurrent in discharge 3
    };
    uint8_t byte;
} SAFETY_STATUS_C_Type;

typedef union {
    struct {
        uint8_t CHG_FET     :1;
        uint8_t PCHG_FET    :1;
        uint8_t DSG_FET     :1;
        uint8_t PDSG_FET    :1;
        uint8_t DCHG_PIN    :1;
        uint8_t DDSG_PIN    :1;
        uint8_t ALRT_PIN    :1;
        uint8_t RSVD        :1;
    };
    uint8_t byte;
} FET_STATUS_Type;

#endif /* BQ769X2_REGISTERS_H_ */
