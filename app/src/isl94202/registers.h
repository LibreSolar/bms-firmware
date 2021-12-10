/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

// register map (organized as 16 bit words)

// Overvoltage Threshold
// Charge Detect Pulse Width
#define ISL94202_OVL_CPW    (0x00U)
#define ISL94202_OVL_Pos    (0x0U)
#define ISL94202_OVL_Msk    (0xFFFU << ISL94202_OVL_Pos)
#define ISL94202_CPW_Pos    (0xC)
#define ISL94202_CPW_Msk    (0xFU << ISL94202_CPW_Pos)

// Overvoltage Recovery
#define ISL94202_OVR        (0x02U)
#define ISL94202_OVR_Pos    (0x0U)
#define ISL94202_OVR_Msk    (0xFFFU << ISL94202_OVR_Pos)

// Undervoltage Threshold
// Load Detect Pulse Width
#define ISL94202_UVL_LPW    (0x04U)
#define ISL94202_UVL_Pos    (0x0U)
#define ISL94202_UVL_Msk    (0xFFFU << ISL94202_UVL_Pos)
#define ISL94202_LPW_Pos    (0xC)
#define ISL94202_LPW_Msk    (0xFU << ISL94202_LPW_Pos)

// Undervoltage Recovery
#define ISL94202_UVR        (0x06U)
#define ISL94202_UVR_Pos    (0x0U)
#define ISL94202_UVR_Msk    (0xFFFU << ISL94202_UVR_Pos)

// Overvoltage Lockout Threshold
#define ISL94202_OVLO       (0x08U)
#define ISL94202_OVLO_Pos   (0x0U)
#define ISL94202_OVLO_Msk   (0xFFFU << ISL94202_OVLO_Pos)

// Undervoltage Lockout Threshold
#define ISL94202_UVLO       (0x0AU)
#define ISL94202_UVLO_Pos   (0x0U)
#define ISL94202_UVLO_Msk   (0xFFFU << ISL94202_UVLO_Pos)

// End-of-Charge (EOC) Threshold
#define ISL94202_EOC        (0x0CU)
#define ISL94202_EOC_Pos    (0x0U)
#define ISL94202_EOC_Msk    (0xFFFU << ISL94202_EOC_Pos)

// Low Voltage Charge Level
#define ISL94202_LVCH       (0x0EU)
#define ISL94202_LVCH_Pos   (0x0U)
#define ISL94202_LVCH_Msk   (0xFFFU << ISL94202_LVCH_Pos)

// Overvoltage Delay Time Out
#define ISL94202_OVDT       (0x10U)
#define ISL94202_OVDT_Pos   (0x0U)
#define ISL94202_OVDT_Msk   (0xFFFU << ISL94202_OVDT_Pos)

// Undervoltage Delay Time Out
#define ISL94202_UVDT       (0x12U)
#define ISL94202_UVDT_Pos   (0x0U)
#define ISL94202_UVDT_Msk   (0xFFFU << ISL94202_UVDT_Pos)

// Open-Wire Timing (OWT)
#define ISL94202_OWT        (0x14U)
#define ISL94202_OWT_Pos    (0x0U)
#define ISL94202_OWT_Msk    (0x3FFU << ISL94202_OWT_Pos)

// Discharge Overcurrent Time Out/Threshold
#define ISL94202_OCDT_OCD   (0x16U)
#define ISL94202_OCDT_Pos   (0x0U)
#define ISL94202_OCDT_Msk   (0xFFFU << ISL94202_OCDT_Pos)
#define ISL94202_OCD_Pos    (0xCU)
#define ISL94202_OCD_Msk    (0x7U << ISL94202_OCD_Pos)

// Charge Overcurrent Time Out/Threshold
#define ISL94202_OCCT_OCC   (0x18U)
#define ISL94202_OCCT_Pos   (0x0U)
#define ISL94202_OCCT_Msk   (0xFFFU << ISL94202_OCCT_Pos)
#define ISL94202_OCC_Pos    (0xCU)
#define ISL94202_OCC_Msk    (0x7U << ISL94202_OCC_Pos)

// Discharge Short-Circuit Time Out/Threshold
#define ISL94202_SCDT_SCD   (0x1AU)
#define ISL94202_SCDT_Pos   (0x0U)
#define ISL94202_SCDT_Msk   (0xFFFU << ISL94202_SCDT_Pos)
#define ISL94202_SCD_Pos    (0xCU)
#define ISL94202_SCD_Msk    (0x7U << ISL94202_SCD_Pos)

// Cell Balance Minimum Voltage (CBMIN)
#define ISL94202_CBMIN      (0x1CU)
#define ISL94202_CBMIN_Pos  (0x0U)
#define ISL94202_CBMIN_Msk  (0xFFFU << ISL94202_CBMIN_Pos)

// Cell Balance Maximum Voltage (CBMAX)
#define ISL94202_CBMAX      (0x1EU)
#define ISL94202_CBMAX_Pos  (0x0U)
#define ISL94202_CBMAX_Msk  (0xFFFU << ISL94202_CBMAX_Pos)

// Cell Balance Minimum Differential Voltage (CBMINDV)
#define ISL94202_CBMINDV        (0x20U)
#define ISL94202_CBMINDV_Pos    (0x0U)
#define ISL94202_CBMINDV_Msk    (0xFFFU << ISL94202_CBMINDV_Pos)

// Cell Balance Maximum Differential Voltage (CBMAXDV)
#define ISL94202_CBMAXDV        (0x22U)
#define ISL94202_CBMAXDV_Pos    (0x0U)
#define ISL94202_CBMAXDV_Msk    (0xFFFU << ISL94202_CBMAXDV_Pos)

// Cell Balance On Time (CBON)
#define ISL94202_CBONT      (0x24U)
#define ISL94202_CBONT_Pos  (0x0U)
#define ISL94202_CBONT_Msk  (0xFFFU << ISL94202_CBON_Pos)

// Cell Balance Off Time (CBOFF)
#define ISL94202_CBOFFT     (0x26U)
#define ISL94202_CBOFFT_Pos (0x0U)
#define ISL94202_CBOFFT_Msk (0xFFFU << ISL94202_CBOF_Pos)

// Cell Balance Minimum Temperature Limit (CBUTS)
#define ISL94202_CBUTS      (0x28U)
#define ISL94202_CBUTS_Pos  (0x0U)
#define ISL94202_CBUTS_Msk  (0xFFFU << ISL94202_CBUTS_Pos)

// Cell Balance Minimum Temperature Recovery Level (CBUTR)
#define ISL94202_CBUTR      (0x2AU)
#define ISL94202_CBUTR_Pos  (0x0U)
#define ISL94202_CBUTR_Msk  (0xFFFU << ISL94202_CBUTR_Pos)

// Cell Balance Maximum Temperature Limit (CBOTS)
#define ISL94202_CBOTS      (0x2CU)
#define ISL94202_CBOTS_Pos  (0x0U)
#define ISL94202_CBOTS_Msk  (0xFFFU << ISL94202_CBOTS_Pos)

// Cell Balance Maximum Temperature Recovery Level (CBOTR)
#define ISL94202_CBOTR      (0x2EU)
#define ISL94202_CBOTR_Pos  (0x0U)
#define ISL94202_CBOTR_Msk  (0xFFFU << ISL94202_CBOTR_Pos)

// Charge Over-Temperature Voltage
#define ISL94202_COTS       (0x30U)
#define ISL94202_COTS_Pos   (0x0U)
#define ISL94202_COTS_Msk   (0xFFFU << ISL94202_COTS_Pos)

// Charge Over-Temperature Recovery Voltage
#define ISL94202_COTR       (0x32U)
#define ISL94202_COTR_Pos   (0x0U)
#define ISL94202_COTR_Msk   (0xFFFU << ISL94202_COTR_Pos)

// Charge Under-Temperature Voltage
#define ISL94202_CUTS       (0x34U)
#define ISL94202_CUTS_Pos   (0x0U)
#define ISL94202_CUTS_Msk   (0xFFFU << ISL94202_CUTS_Pos)

// Charge Under-Temperature Recovery Voltage
#define ISL94202_CUTR       (0x36U)
#define ISL94202_CUTR_Pos   (0x0U)
#define ISL94202_CUTR_Msk   (0xFFFU << ISL94202_CUTR_Pos)

// Discharge Over-Temperature Voltage
#define ISL94202_DOTS       (0x38U)
#define ISL94202_DOTS_Pos   (0x0U)
#define ISL94202_DOTS_Msk   (0xFFFU << ISL94202_DOTS_Pos)

// Discharge Over-Temperature Recovery Voltage
#define ISL94202_DOTR       (0x3AU)
#define ISL94202_DOTR_Pos   (0x0U)
#define ISL94202_DOTR_Msk   (0xFFFU << ISL94202_DOTR_Pos)

// Discharge Under-Temperature Voltage
#define ISL94202_DUTS       (0x3CU)
#define ISL94202_DUTS_Pos   (0x0U)
#define ISL94202_DUTS_Msk   (0xFFFU << ISL94202_DUTS_Pos)

// Discharge Under-Temperature Recovery Voltage
#define ISL94202_DUTR       (0x3EU)
#define ISL94202_DUTR_Pos   (0x0U)
#define ISL94202_DUTR_Msk   (0xFFFU << ISL94202_DUTR_Pos)

// Internal Over-Temperature Voltage
#define ISL94202_IOTS       (0x40U)
#define ISL94202_IOTS_Pos   (0x0U)
#define ISL94202_IOTS_Msk   (0xFFFU << ISL94202_IOTS_Pos)

// Internal Over-Temperature Recovery Voltage
#define ISL94202_IOTR       (0x42U)
#define ISL94202_IOTR_Pos   (0x0U)
#define ISL94202_IOTR_Msk   (0xFFFU << ISL94202_IOTR_Pos)

// Sleep Level Voltage
#define ISL94202_SLL        (0x44U)
#define ISL94202_SLL_Pos    (0x0U)
#define ISL94202_SLL_Msk    (0xFFFU << ISL94202_SLL_Pos)

// Sleep Delay
// Watchdog Timer (WDT)
#define ISL94202_SLT_WDT    (0x46U)
#define ISL94202_SLT_Pos    (0x0U)
#define ISL94202_SLT_Msk    (0x7FFU << ISL94202_SLT_Pos)
#define ISL94202_WDT_Pos    (0xBU)
#define ISL94202_WDT_Msk    (0x1FU << ISL94202_WDT_Pos)

// Mode Timer
// Cell Configuration
#define ISL94202_MOD_CELL   (0x48U)
#define ISL94202_MOD_Pos    (0x0U)
#define ISL94202_MOD_Msk    (0xFFU << ISL94202_MOD_Pos)
#define ISL94202_CELL_Pos   (0x8U)
#define ISL94202_CELL_Msk   (0xFFU << ISL94202_CELL_Pos)

// Setup 0
#define ISL94202_SETUP0             (0x4AU)
// Open-Wire PSD
#define ISL94202_SETUP0_OWPSD_Pos   (0x0U)
#define ISL94202_SETUP0_OWPSD_Msk   (0x1U << ISL94202_SETUP0_OWPSD_Pos)
// Disable Open-Wire-Scan
#define ISL94202_SETUP0_DOWD_Pos    (0x1U)
#define ISL94202_SETUP0_DOWD_Msk    (0x1U << ISL94202_SETUP0_DOWD_Pos)
// Precharge FET Enable
#define ISL94202_SETUP0_PCFETE_Pos  (0x2U)
#define ISL94202_SETUP0_PCFETE_Msk  (0x1U << ISL94202_SETUP0_PCFETE_Pos)
// External Temp Gain
#define ISL94202_SETUP0_TGAIN_Pos   (0x4U)
#define ISL94202_SETUP0_TGAIN_Msk   (0x1U << ISL94202_SETUP0_TGAIN_Pos)
// xTemp2 Mode Control
#define ISL94202_SETUP0_XT2M_Pos    (0x5U)
#define ISL94202_SETUP0_XT2M_Msk    (0x1U << ISL94202_SETUP0_XT2M_Pos)
// Cell fail PSD
#define ISL94202_SETUP0_CFPSD_Pos   (0x7U)
#define ISL94202_SETUP0_CFPSD_Msk   (0x1U << ISL94202_SETUP0_CFPSD_Pos)

// Setup 1
#define ISL94202_SETUP1             (0x4BU)
// Enable CBAL during EOC
#define ISL94202_SETUP1_CB_EOC_Pos  (0x0U)
#define ISL94202_SETUP1_CB_EOC_Msk  (0x1U << ISL94202_SETUP1_CB_EOC_Pos)
// Enable UVLO Power-Down
#define ISL94202_SETUP1_UVLOPD_Pos  (0x3U)
#define ISL94202_SETUP1_UVLOPD_Msk  (0x1U << ISL94202_SETUP1_UVLOPD_Pos)
// CFET on during OV (Discharging)
#define ISL94202_SETUP1_DFODOV_Pos  (0x4U)
#define ISL94202_SETUP1_DFODOV_Msk  (0x1U << ISL94202_SETUP1_DFODOV_Pos)
// DFET on during UV (Charging)
#define ISL94202_SETUP1_DFODUV_Pos  (0x5U)
#define ISL94202_SETUP1_DFODUV_Msk  (0x1U << ISL94202_SETUP1_DFODUV_Pos)
// CB during Charge
#define ISL94202_SETUP1_CBDC_Pos    (0x6U)
#define ISL94202_SETUP1_CBDC_Msk    (0x1U << ISL94202_SETUP1_CBDC_Pos)
// CB during Discharge
#define ISL94202_SETUP1_CBDD_Pos    (0x7U)
#define ISL94202_SETUP1_CBDD_Msk    (0x1U << ISL94202_SETUP1_CBDD_Pos)

// RAM registers

// Status 0
#define ISL94202_STAT0              (0x80U)
#define ISL94202_STAT0_OVF_Pos      (0x0U)
#define ISL94202_STAT0_OVF_Msk      (0x1U << ISL94202_STAT0_OVF_Pos)
#define ISL94202_STAT0_OVLOF_Pos    (0x1U)
#define ISL94202_STAT0_OVLOF_Msk    (0x1U << ISL94202_STAT0_OVLOF_Pos)
#define ISL94202_STAT0_UVF_Pos      (0x2U)
#define ISL94202_STAT0_UVF_Msk      (0x1U << ISL94202_STAT0_UVF_Pos)
#define ISL94202_STAT0_UVLOF_Pos    (0x3U)
#define ISL94202_STAT0_UVLOF_Msk    (0x1U << ISL94202_STAT0_UVLOF_Pos)
#define ISL94202_STAT0_DOTF_Pos     (0x4U)
#define ISL94202_STAT0_DOTF_Msk     (0x1U << ISL94202_STAT0_DOTF_Pos)
#define ISL94202_STAT0_DUTF_Pos     (0x5U)
#define ISL94202_STAT0_DUTF_Msk     (0x1U << ISL94202_STAT0_DUTF_Pos)
#define ISL94202_STAT0_COTF_Pos     (0x6U)
#define ISL94202_STAT0_COTF_Msk     (0x1U << ISL94202_STAT0_COTF_Pos)
#define ISL94202_STAT0_CUTF_Pos     (0x7U)
#define ISL94202_STAT0_CUTF_Msk     (0x1U << ISL94202_STAT0_CUTF_Pos)

// Status 1
#define ISL94202_STAT1              (0x81U)
#define ISL94202_STAT1_IOTF_Pos     (0x0U)
#define ISL94202_STAT1_IOTF_Msk     (0x1U << ISL94202_STAT1_IOTF_Pos)
#define ISL94202_STAT1_COCF_Pos     (0x1U)
#define ISL94202_STAT1_COCF_Msk     (0x1U << ISL94202_STAT1_COCF_Pos)
#define ISL94202_STAT1_DOCF_Pos     (0x2U)
#define ISL94202_STAT1_DOCF_Msk     (0x1U << ISL94202_STAT1_DOCF_Pos)
#define ISL94202_STAT1_DSCF_Pos     (0x3U)
#define ISL94202_STAT1_DSCF_Msk     (0x1U << ISL94202_STAT1_DSCF_Pos)
#define ISL94202_STAT1_CELLF_Pos    (0x4U)
#define ISL94202_STAT1_CELLF_Msk    (0x1U << ISL94202_STAT1_CELLF_Pos)
#define ISL94202_STAT1_OPENF_Pos    (0x5U)
#define ISL94202_STAT1_OPENF_Msk    (0x1U << ISL94202_STAT1_OPENF_Pos)
#define ISL94202_STAT1_VEOC_Pos     (0x7U)
#define ISL94202_STAT1_VEOC_Msk     (0x1U << ISL94202_STAT1_VEOC_Pos)

// Status 2
#define ISL94202_STAT2              (0x82U)
#define ISL94202_STAT2_LDPRSNT_Pos  (0x0U)
#define ISL94202_STAT2_LDPRSNT_Msk  (0x1U << ISL94202_STAT2_LDPRSNT_Pos)
#define ISL94202_STAT2_CHPRSNT_Pos  (0x1U)
#define ISL94202_STAT2_CHPRSNT_Msk  (0x1U << ISL94202_STAT2_CHPRSNT_Pos)
#define ISL94202_STAT2_CHING_Pos    (0x2U)
#define ISL94202_STAT2_CHING_Msk    (0x1U << ISL94202_STAT2_CHING_Pos)
#define ISL94202_STAT2_DCHING_Pos   (0x3U)
#define ISL94202_STAT2_DCHING_Msk   (0x1U << ISL94202_STAT2_DCHING_Pos)
#define ISL94202_STAT2_ECCUSED_Pos  (0x4U)
#define ISL94202_STAT2_ECCUSED_Msk  (0x1U << ISL94202_STAT2_ECCUSED_Pos)
#define ISL94202_STAT2_ECCFAIL_Pos  (0x5U)
#define ISL94202_STAT2_ECCFAIL_Msk  (0x1U << ISL94202_STAT2_ECCFAIL_Pos)
#define ISL94202_STAT2_INTSCAN_Pos  (0x6U)
#define ISL94202_STAT2_INTSCAN_Msk  (0x1U << ISL94202_STAT2_INTSCAN_Pos)
#define ISL94202_STAT2_LVCHG_Pos    (0x7U)
#define ISL94202_STAT2_LVCHG_Msk    (0x1U << ISL94202_STAT2_LVCHG_Pos)

// Status 3
#define ISL94202_STAT3              (0x83U)
#define ISL94202_STAT3_CBOT_Pos     (0x0U)
#define ISL94202_STAT3_CBOT_Msk     (0x1U << ISL94202_STAT3_CBOT_Pos)
#define ISL94202_STAT3_CBUT_Pos     (0x1U)
#define ISL94202_STAT3_CBUT_Msk     (0x1U << ISL94202_STAT3_CBUT_Pos)
#define ISL94202_STAT3_CBOV_Pos     (0x2U)
#define ISL94202_STAT3_CBOV_Msk     (0x1U << ISL94202_STAT3_CBOV_Pos)
#define ISL94202_STAT3_CBUV_Pos     (0x3U)
#define ISL94202_STAT3_CBUV_Msk     (0x1U << ISL94202_STAT3_CBUV_Pos)
#define ISL94202_STAT3_INIDLE_Pos   (0x4U)
#define ISL94202_STAT3_INIDLE_Msk   (0x1U << ISL94202_STAT3_INIDLE_Pos)
#define ISL94202_STAT3_INDOZE_Pos   (0x5U)
#define ISL94202_STAT3_INDOZE_Msk   (0x1U << ISL94202_STAT3_INDOZE_Pos)
#define ISL94202_STAT3_INSLEEP_Pos  (0x6U)
#define ISL94202_STAT3_INSLEEP_Msk  (0x1U << ISL94202_STAT3_INSLEEP_Pos)

// Cell Balance FET Control
#define ISL94202_CBFC               (0x84U)

// Control 0
#define ISL94202_CTRL0              (0x85U)
#define ISL94202_CTRL0_AO_Pos       (0x0U)
#define ISL94202_CTRL0_AO_Msk       (0xFU << ISL94202_CTRL0_AO_Pos)
#define ISL94202_CTRL0_CG_Pos       (0x4U)
#define ISL94202_CTRL0_CG_Msk       (0x3U << ISL94202_CTRL0_CG_Pos)
#define ISL94202_CTRL0_ADCSTRT_Pos  (0x6U)
#define ISL94202_CTRL0_ADCSTRT_Msk  (0x1U << ISL94202_CTRL0_ADCSTRT_Pos)

// Control 1
#define ISL94202_CTRL1              (0x86U)
#define ISL94202_CTRL1_DFET_Pos     (0x0U)
#define ISL94202_CTRL1_DFET_Msk     (0x1U << ISL94202_CTRL1_DFET_Pos)
#define ISL94202_CTRL1_CFET_Pos     (0x1U)
#define ISL94202_CTRL1_CFET_Msk     (0x1U << ISL94202_CTRL1_CFET_Pos)
#define ISL94202_CTRL1_PCFET_Pos    (0x2U)
#define ISL94202_CTRL1_PCFET_Msk    (0x1U << ISL94202_CTRL1_PCFET_Pos)
#define ISL94202_CTRL1_PSD_Pos      (0x3U)
#define ISL94202_CTRL1_PSD_Msk      (0x1U << ISL94202_CTRL1_PSD_Pos)

// Control 2
#define ISL94202_CTRL2              (0x87U)
#define ISL94202_CTRL2_CBAL_ON_Pos  (0x0U)
#define ISL94202_CTRL2_CBAL_ON_Msk  (0x1U << ISL94202_CTRL2_CBAL_ON_Pos)
#define ISL94202_CTRL2_OW_STRT_Pos  (0x1U)
#define ISL94202_CTRL2_OW_STRT_Msk  (0x1U << ISL94202_CTRL2_OW_STRT_Pos)
#define ISL94202_CTRL2_UCSCAN_Pos   (0x2U)
#define ISL94202_CTRL2_UCSCAN_Msk   (0x1U << ISL94202_CTRL2_UCSCAN_Pos)
#define ISL94202_CTRL2_UCCMON_Pos   (0x3U)
#define ISL94202_CTRL2_UCCMON_Msk   (0x1U << ISL94202_CTRL2_UCCMON_Pos)
#define ISL94202_CTRL2_UCLMON_Pos   (0x4U)
#define ISL94202_CTRL2_UCLMON_Msk   (0x1U << ISL94202_CTRL2_UCLMON_Pos)
#define ISL94202_CTRL2_UCCBAL_Pos   (0x5U)
#define ISL94202_CTRL2_UCCBAL_Msk   (0x1U << ISL94202_CTRL2_UCCBAL_Pos)
#define ISL94202_CTRL2_UCFET_Pos    (0x6U)
#define ISL94202_CTRL2_UCFET_Msk    (0x1U << ISL94202_CTRL2_UCFET_Pos)

// Control 3
#define ISL94202_CTRL3              (0x88U)
#define ISL94202_CTRL3_IDLE_Pos     (0x0U)
#define ISL94202_CTRL3_IDLE_Msk     (0x1U << ISL94202_CTRL3_IDLE_Pos)
#define ISL94202_CTRL3_DOZE_Pos     (0x1U)
#define ISL94202_CTRL3_DOZE_Msk     (0x1U << ISL94202_CTRL3_DOZE_Pos)
#define ISL94202_CTRL3_SLEEP_Pos    (0x2U)
#define ISL94202_CTRL3_SLEEP_Msk    (0x1U << ISL94202_CTRL3_SLEEP_Pos)
#define ISL94202_CTRL3_PDWN_Pos     (0x3U)
#define ISL94202_CTRL3_PDWN_Msk     (0x1U << ISL94202_CTRL3_PDWN_Pos)

// EEPROM Enable
#define ISL94202_EEEN               (0x89U)
#define ISL94202_EEEN_EEEN_Pos      (0x0U)
#define ISL94202_EEEN_EEEN_Msk      (0x1U << ISL94202_EEEN_EEEN_Pos)

#define ISL94202_CELLMIN    (0x8AU)
#define ISL94202_CELLMAX    (0x8CU)
#define ISL94202_ISNS       (0x8EU)
#define ISL94202_CELL1      (0x90U)
#define ISL94202_CELL2      (0x92U)
#define ISL94202_CELL3      (0x94U)
#define ISL94202_CELL4      (0x96U)
#define ISL94202_CELL5      (0x98U)
#define ISL94202_CELL6      (0x9AU)
#define ISL94202_CELL7      (0x9CU)
#define ISL94202_CELL8      (0x9EU)
#define ISL94202_IT         (0xA0U)
#define ISL94202_XT1        (0xA2U)
#define ISL94202_XT2        (0xA4U)
#define ISL94202_VBATT      (0xA6U)
#define ISL94202_VRGO       (0xA8U)
#define ISL94202_ADC        (0xAAU)

static const uint16_t OCD_Thresholds[] =
    { 4, 8, 16, 24, 32, 48, 64, 96 }; // mV

static const uint16_t OCC_Thresholds[] =
    { 1, 2, 4, 6, 8, 12, 16, 24 }; // mV

static const uint16_t DSC_Thresholds[] =
    { 16, 24, 32, 48, 64, 96, 128, 256 }; // mV

static const uint16_t ISL94202_Current_Gain[] =
    { 50, 5, 500, 500 };

// Scale Value for delay times (first 2 bytes)
#define ISL94202_DELAY_US   (0U)
#define ISL94202_DELAY_MS   (1U)
#define ISL94202_DELAY_S    (2U)
#define ISL94202_DELAY_MIN  (3U)
