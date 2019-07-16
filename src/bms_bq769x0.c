/* Battery management system based on bq769x0 for ARM mbed
 * Copyright (c) 2015-2018 Martin Jäger (www.libre.solar)
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

#include "config.h"
#include "pcb.h"

#if defined(BMS_BQ76920) || defined(BMS_BQ76930) || defined(BMS_BQ76940)

#include "bms.h"
#include "bms_bq769x0_registers.h"
#include "bq769x0_hw.h"

#include <math.h>       // log for thermistor calculation
#include <stdlib.h>     // for abs() function
#include <stdio.h>
#include <time.h>
#include <string.h>

// static (private) variables
//----------------------------------------------------------------------------

static int adc_gain;    // factory-calibrated, read out from chip (uV/LSB)
static int adc_offset;  // factory-calibrated, read out from chip (mV)

//----------------------------------------------------------------------------

const char *byte2char(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

//----------------------------------------------------------------------------

void bms_init(BMS *bms)
{
    bq769x0_init();

    // set some safe default values
    bms->auto_balancing_enabled = false;
    bms->balancing_min_idle_s = 1800;    // default: 30 minutes
    bms->idle_current_threshold = 30; // mA

    bms->thermistor_beta = 3435;  // typical value for Semitec 103AT-5 thermistor

    // initialize variables
    for (int i = 0; i < NUM_CELLS_MAX; i++) {
        bms->cell_voltages[i] = 0;
    }

}

//----------------------------------------------------------------------------
int bms_check_status(BMS *bms)
{
    //  printf("error_status: ");
    //  printf(error_status);
    if (!bq769x0_alert_flag() && bms->error_status == 0) {
        return 0;
    } else {

        regSYS_STAT_t sys_stat;
        sys_stat.regByte = bq769x0_read_byte(SYS_STAT);

        // first check, if only a new CC reading is available
        if (sys_stat.bits.CC_READY == 1) {
            //printf("Interrupt: CC ready");
            bms_update_current(bms);  // automatically clears CC ready flag
        }

        // Serious error occured
        if (sys_stat.regByte & 0b00111111)
        {
            if (bq769x0_alert_flag() == true) {
                bms->sec_since_error_counter = 0;
            }
            bms->error_status = sys_stat.regByte;

            unsigned int secSinceInterrupt = time(NULL) - bq769x0_alert_timestamp();

            // TODO!!
            // check for overrun of _timer.read_ms() or very slow running program
            if (abs((long)(secSinceInterrupt - bms->sec_since_error_counter)) > 2) {
                bms->sec_since_error_counter = secSinceInterrupt;
            }

            // called only once per second
            if (secSinceInterrupt >= bms->sec_since_error_counter)
            {
                if (sys_stat.regByte & 0b00100000) { // XR error
                    // datasheet recommendation: try to clear after waiting a few seconds
                    if (bms->sec_since_error_counter % 3 == 0) {
                        #if BMS_DEBUG
                        printf("Attempting to clear XR error");
                        #endif
                        bq769x0_write_byte(SYS_STAT, 0b00100000);
                        bms_chg_switch(bms, true);
                        bms_dis_switch(bms, true);
                    }
                }
                if (sys_stat.regByte & 0b00010000) { // Alert error
                    if (bms->sec_since_error_counter % 10 == 0) {
                        #if BMS_DEBUG
                        printf("Attempting to clear Alert error");
                        #endif
                        bq769x0_write_byte(SYS_STAT, 0b00010000);
                        bms_chg_switch(bms, true);
                        bms_dis_switch(bms, true);
                    }
                }
                if (sys_stat.regByte & 0b00001000) { // UV error
                    bms_update_voltages(bms);
                    if (bms->cell_voltages[bms->id_cell_voltage_min] > bms->cell_voltage_limit_min) {
                        #if BMS_DEBUG
                        printf("Attempting to clear UV error");
                        #endif
                        bq769x0_write_byte(SYS_STAT, 0b00001000);
                        bms_dis_switch(bms, true);
                    }
                }
                if (sys_stat.regByte & 0b00000100) { // OV error
                    bms_update_voltages(bms);
                    if (bms->cell_voltages[bms->id_cell_voltage_max] < bms->cell_voltage_limit_max) {
                        #if BMS_DEBUG
                        printf("Attempting to clear OV error");
                        #endif
                        bq769x0_write_byte(SYS_STAT, 0b00000100);
                        bms_chg_switch(bms, true);
                    }
                }
                if (sys_stat.regByte & 0b00000010) { // SCD
                    if (bms->sec_since_error_counter % 60 == 0) {
                        #if BMS_DEBUG
                        printf("Attempting to clear SCD error");
                        #endif
                        bq769x0_write_byte(SYS_STAT, 0b00000010);
                        bms_dis_switch(bms, true);
                    }
                }
                if (sys_stat.regByte & 0b00000001) { // OCD
                    if (bms->sec_since_error_counter % 60 == 0) {
                        #if BMS_DEBUG
                        printf("Attempting to clear OCD error");
                        #endif
                        bq769x0_write_byte(SYS_STAT, 0b00000001);
                        bms_dis_switch(bms, true);
                    }
                }
                bms->sec_since_error_counter++;
            }
        }
        else {
            bms->error_status = 0;
        }

        return bms->error_status;
    }
}

//----------------------------------------------------------------------------
void bms_check_cell_temp(BMS *bms)
{
    int numberOfThermistors = NUM_CELLS_MAX/5;
    bool cellTempChargeError = 0;
    bool cellTempDischargeError = 0;

    for (int thermistor = 0; thermistor < numberOfThermistors; thermistor++) {
        cellTempChargeError |=
            bms->temperatures[thermistor] > bms->chg_temp_limit_max - bms->chg_temp_error_flag ? bms->temp_limit_hysteresis : 0 ||
            bms->temperatures[thermistor] < bms->chg_temp_limit_min + bms->chg_temp_error_flag ? bms->temp_limit_hysteresis : 0;

        cellTempDischargeError |=
            bms->temperatures[thermistor] > bms->dis_temp_limit_max - bms->dis_temp_error_flag ? bms->temp_limit_hysteresis : 0 ||
            bms->temperatures[thermistor] < bms->dis_temp_limit_min + bms->dis_temp_error_flag ? bms->temp_limit_hysteresis : 0;
    }

    if (bms->chg_temp_error_flag != cellTempChargeError) {
        bms->chg_temp_error_flag = cellTempChargeError;
        if (cellTempChargeError) {
            bms_chg_switch(bms, false);
            #if BMS_DEBUG
            printf("Temperature error (CHG)");
            #endif
        }
        else {
            bms_chg_switch(bms, true);
            #if BMS_DEBUG
            printf("Clearing temperature error (CHG)");
            #endif
        }
    }

    if (bms->dis_temp_error_flag != cellTempDischargeError) {
        bms->dis_temp_error_flag = cellTempDischargeError;
        if (cellTempDischargeError) {
            bms_dis_switch(bms, false);
            #if BMS_DEBUG
            printf("Temperature error (DSG)");
            #endif
        }
        else {
            bms_dis_switch(bms, true);
            #if BMS_DEBUG
            printf("Clearing temperature error (DSG)");
            #endif
        }
    }
}

//----------------------------------------------------------------------------
// puts BMS IC into SHIP mode (i.e. switched off)

void bms_shutdown(BMS *bms)
{
    bq769x0_write_byte(SYS_CTRL1, 0x0);
    bq769x0_write_byte(SYS_CTRL1, 0x1);
    bq769x0_write_byte(SYS_CTRL1, 0x2);
}

//----------------------------------------------------------------------------

bool bms_chg_switch(BMS *bms, bool enable)
{
    if (enable) {
        #if BMS_DEBUG
        printf("check_status() = %d\n", bms_check_status(bms));
        printf("Umax = %d\n", bms->cell_voltages[bms->id_cell_voltage_max]);
        printf("temperatures[0] = %d\n", bms->temperatures[0]);
        #endif

        int numberOfThermistors = NUM_CELLS_MAX/5;
        bool cellTempChargeError = 0;

        for (int thermistor = 0; thermistor < numberOfThermistors; thermistor++) {
            cellTempChargeError |=
                bms->temperatures[thermistor] > bms->chg_temp_limit_max ||
                bms->temperatures[thermistor] < bms->chg_temp_limit_min;
        }

        if (bms_check_status(bms) == 0 &&
            bms->cell_voltages[bms->id_cell_voltage_max] < bms->cell_voltage_limit_max &&
            cellTempChargeError == 0)
        {
            int sys_ctrl2;
            sys_ctrl2 = bq769x0_read_byte(SYS_CTRL2);
            bq769x0_write_byte(SYS_CTRL2, sys_ctrl2 | 0b00000001);  // switch CHG on
            #if BMS_DEBUG
            printf("Enabling CHG FET\n");
            #endif
            return true;
        }
        else {
            return false;
        }
    }
    else {
        int sys_ctrl2;
        sys_ctrl2 = bq769x0_read_byte(SYS_CTRL2);
        bq769x0_write_byte(SYS_CTRL2, sys_ctrl2 & ~0b00000001);  // switch CHG off
        #if BMS_DEBUG
        printf("Disabling CHG FET\n");
        #endif
        return true;
    }
}

//----------------------------------------------------------------------------

bool bms_dis_switch(BMS *bms, bool enable)
{
    #if BMS_DEBUG
    printf("check_status() = %d\n", bms_check_status(bms));
    printf("Umin = %d\n", bms->cell_voltages[bms->id_cell_voltage_min]);
    printf("temperatures[0] = %d\n", bms->temperatures[0]);
    #endif

    if (enable) {
        int numberOfThermistors = NUM_CELLS_MAX/5;
        bool cellTempDischargeError = 0;

        for (int thermistor = 0; thermistor < numberOfThermistors; thermistor++) {
            cellTempDischargeError |=
                bms->temperatures[thermistor] > bms->dis_temp_limit_max ||
                bms->temperatures[thermistor] < bms->dis_temp_limit_min;
        }

        if (bms_check_status(bms) == 0 &&
            bms->cell_voltages[bms->id_cell_voltage_min] > bms->cell_voltage_limit_min &&
            cellTempDischargeError == 0)
        {
            int sys_ctrl2;
            sys_ctrl2 = bq769x0_read_byte(SYS_CTRL2);
            bq769x0_write_byte(SYS_CTRL2, sys_ctrl2 | 0b00000010);  // switch DSG on
            return true;
        }
        else {
            return false;
        }
    }
    else {
        int sys_ctrl2;
        sys_ctrl2 = bq769x0_read_byte(SYS_CTRL2);
        bq769x0_write_byte(SYS_CTRL2, sys_ctrl2 & ~0b00000010);  // switch DSG off
        #if BMS_DEBUG
        printf("Disabling DISCHG FET\n");
        #endif
        return true;
    }
}

//----------------------------------------------------------------------------

void bms_auto_balancing(BMS *bms, bool enabled)
{
    bms->auto_balancing_enabled = enabled;
}


//----------------------------------------------------------------------------

void bms_balancing_thresholds(BMS *bms, int idleTime_min, int absVoltage_mV, int voltageDifference_mV)
{
    bms->balancing_min_idle_s = idleTime_min * 60;
    bms->balancing_cell_voltage_min = absVoltage_mV;
    bms->balancing_voltage_diff_target = voltageDifference_mV;
}

//----------------------------------------------------------------------------

void bms_update_balancing_switches(BMS *bms)
{
    long idleSeconds = time(NULL) - bms->idle_timestamp;
    int numberOfSections = NUM_CELLS_MAX/5;

    // check for _timer.read_ms() overflow
    if (idleSeconds < 0) {
        bms->idle_timestamp = 0;
        idleSeconds = time(NULL);
    }

    // check if balancing allowed
    if (bms_check_status(bms) == 0 &&
        idleSeconds >= bms->balancing_min_idle_s &&
        bms->cell_voltages[bms->id_cell_voltage_max] > bms->balancing_cell_voltage_min &&
        (bms->cell_voltages[bms->id_cell_voltage_max] - bms->cell_voltages[bms->id_cell_voltage_min]) > bms->balancing_voltage_diff_target)
    {
        //printf("Balancing enabled!");
        bms->balancing_status = 0;  // current status will be set in following loop

        //regCELLBAL_t cellbal;
        int balancingFlags;
        int balancingFlagsTarget;

        for (int section = 0; section < numberOfSections; section++)
        {
            // find cells which should be balanced and sort them by voltage descending
            int cellList[5];
            int cellCounter = 0;
            for (int i = 0; i < 5; i++)
            {
                if ((bms->cell_voltages[section*5 + i] - bms->cell_voltages[bms->id_cell_voltage_min]) > bms->balancing_voltage_diff_target) {
                    int j = cellCounter;
                    while (j > 0 && bms->cell_voltages[section*5 + cellList[j - 1]] < bms->cell_voltages[section*5 + i])
                    {
                        cellList[j] = cellList[j - 1];
                        j--;
                    }
                    cellList[j] = i;
                    cellCounter++;
                }
            }

            balancingFlags = 0;
            for (int i = 0; i < cellCounter; i++)
            {
                // try to enable balancing of current cell
                balancingFlagsTarget = balancingFlags | (1 << cellList[i]);

                // check if attempting to balance adjacent cells
                bool adjacentCellCollision =
                    ((balancingFlagsTarget << 1) & balancingFlags) ||
                    ((balancingFlags << 1) & balancingFlagsTarget);

                if (adjacentCellCollision == false) {
                    balancingFlags = balancingFlagsTarget;
                }
            }

            #if BMS_DEBUG
            //printf("Setting CELLBAL%d register to: %s\n", section+1, byte2char(balancingFlags));
            #endif

            bms->balancing_status |= balancingFlags << section*5;

            // set balancing register for this section
            bq769x0_write_byte(CELLBAL1+section, balancingFlags);

        } // section loop
    }
    else if (bms->balancing_status > 0)
    {
        // clear all CELLBAL registers
        for (int section = 0; section < numberOfSections; section++)
        {
            #if BMS_DEBUG
            printf("Clearing Register CELLBAL%d\n", section+1);
            #endif

            bq769x0_write_byte(CELLBAL1+section, 0x0);
        }

        bms->balancing_status = 0;
    }
}

//----------------------------------------------------------------------------

int bms_get_balancing_status(BMS *bms)
{
    return bms->balancing_status;
}

//----------------------------------------------------------------------------

void bms_temperature_limits(BMS *bms, int min_dis_degC, int max_dis_degC, int min_chg_degC, int max_chg_degC, int hysteresis_degC)
{
    // Temperature limits (°C/10)
    bms->dis_temp_limit_min = min_dis_degC * 10;
    bms->dis_temp_limit_max = max_dis_degC * 10;
    bms->chg_temp_limit_min = min_chg_degC * 10;
    bms->chg_temp_limit_max = max_chg_degC * 10;
    bms->temp_limit_hysteresis = hysteresis_degC * 10;
}

//----------------------------------------------------------------------------

long bms_dis_sc_limit(BMS *bms, long current_mA, int delay_us)
{
    regPROTECT1_t protect1;

    // only RSNS = 1 considered
    protect1.bits.RSNS = 1;

    protect1.bits.SCD_THRESH = 0;
    for (int i = sizeof(SCD_threshold_setting)-1; i > 0; i--) {
        if (current_mA * bms->shunt_res_mOhm / 1000 >= SCD_threshold_setting[i]) {
            protect1.bits.SCD_THRESH = i;
            break;
        }
    }

    protect1.bits.SCD_DELAY = 0;
    for (int i = sizeof(SCD_delay_setting)-1; i > 0; i--) {
        if (delay_us >= SCD_delay_setting[i]) {
            protect1.bits.SCD_DELAY = i;
            break;
        }
    }

    bq769x0_write_byte(PROTECT1, protect1.regByte);

    // returns the actual current threshold value
    return (long)SCD_threshold_setting[protect1.bits.SCD_THRESH] * 1000 /
        bms->shunt_res_mOhm;
}

//----------------------------------------------------------------------------

long bms_chg_oc_limit(BMS *bms, long current_mA, int delay_ms)
{
    // ToDo: Software protection for charge overcurrent
    return 0;
}

//----------------------------------------------------------------------------

long bms_dis_oc_limit(BMS *bms, long current_mA, int delay_ms)
{
    regPROTECT2_t protect2;

    // Remark: RSNS must be set to 1 in PROTECT1 register

    protect2.bits.OCD_THRESH = 0;
    for (int i = sizeof(OCD_threshold_setting)-1; i > 0; i--) {
        if (current_mA * bms->shunt_res_mOhm / 1000 >= OCD_threshold_setting[i]) {
            protect2.bits.OCD_THRESH = i;
            break;
        }
    }

    protect2.bits.OCD_DELAY = 0;
    for (int i = sizeof(OCD_delay_setting)-1; i > 0; i--) {
        if (delay_ms >= OCD_delay_setting[i]) {
            protect2.bits.OCD_DELAY = i;
            break;
        }
    }

    bq769x0_write_byte(PROTECT2, protect2.regByte);

    // returns the actual current threshold value
    return (long)OCD_threshold_setting[protect2.bits.OCD_THRESH] * 1000 /
        bms->shunt_res_mOhm;
}


//----------------------------------------------------------------------------

int bms_cell_uv_limit(BMS *bms, int voltage_mV, int delay_s)
{
    regPROTECT3_t protect3;
    int uv_trip = 0;

    bms->cell_voltage_limit_min = voltage_mV;

    protect3.regByte = bq769x0_read_byte(PROTECT3);

    uv_trip = ((((long)voltage_mV - adc_offset) * 1000 / adc_gain) >> 4) & 0x00FF;
    uv_trip += 1;   // always round up for lower cell voltage
    bq769x0_write_byte(UV_TRIP, uv_trip);

    protect3.bits.UV_DELAY = 0;
    for (int i = sizeof(UV_delay_setting)-1; i > 0; i--) {
        if (delay_s >= UV_delay_setting[i]) {
            protect3.bits.UV_DELAY = i;
            break;
        }
    }

    bq769x0_write_byte(PROTECT3, protect3.regByte);

    // returns the actual current threshold value
    return ((long)1 << 12 | uv_trip << 4) * adc_gain / 1000 + adc_offset;
}

//----------------------------------------------------------------------------

int bms_cell_ov_limit(BMS *bms, int voltage_mV, int delay_s)
{
    regPROTECT3_t protect3;
    int ov_trip = 0;

    bms->cell_voltage_limit_max = voltage_mV;

    protect3.regByte = bq769x0_read_byte(PROTECT3);

    ov_trip = ((((long)voltage_mV - adc_offset) * 1000 / adc_gain) >> 4) & 0x00FF;
    bq769x0_write_byte(OV_TRIP, ov_trip);

    protect3.bits.OV_DELAY = 0;
    for (int i = sizeof(OV_delay_setting)-1; i > 0; i--) {
        if (delay_s >= OV_delay_setting[i]) {
            protect3.bits.OV_DELAY = i;
            break;
        }
    }

    bq769x0_write_byte(PROTECT3, protect3.regByte);

    // returns the actual current threshold value
    return ((long)1 << 13 | ov_trip << 4) * adc_gain / 1000 + adc_offset;
}


//----------------------------------------------------------------------------

void bms_update_temperatures(BMS *bms)
{
    float tmp = 0;
    int adcVal = 0;
    int vtsx = 0;
    unsigned long rts = 0;

    // calculate R_thermistor according to bq769x0 datasheet
    adcVal = (bq769x0_read_byte(TS1_HI_BYTE) & 0b00111111) << 8 | bq769x0_read_byte(TS1_LO_BYTE);
    vtsx = adcVal * 0.382; // mV
    rts = 10000.0 * vtsx / (3300.0 - vtsx); // Ohm

    // Temperature calculation using Beta equation
    // - According to bq769x0 datasheet, only 10k thermistors should be used
    // - 25°C reference temperature for Beta equation assumed
    tmp = 1.0/(1.0/(273.15+25) + 1.0/bms->thermistor_beta*log(rts/10000.0)); // K
    bms->temperatures[0] = (tmp - 273.15) * 10.0;

    if (NUM_THERMISTORS_MAX >= 2) {     // bq76930 or bq76940
        adcVal = (bq769x0_read_byte(TS2_HI_BYTE) & 0b00111111) << 8 | bq769x0_read_byte(TS2_LO_BYTE);
        vtsx = adcVal * 0.382; // mV
        rts = 10000.0 * vtsx / (3300.0 - vtsx); // Ohm
        tmp = 1.0/(1.0/(273.15+25) + 1.0/bms->thermistor_beta*log(rts/10000.0)); // K
        bms->temperatures[1] = (tmp - 273.15) * 10.0;
    }

    if (NUM_THERMISTORS_MAX == 3) {     // bq76940
        adcVal = (bq769x0_read_byte(TS3_HI_BYTE) & 0b00111111) << 8 | bq769x0_read_byte(TS3_LO_BYTE);
        vtsx = adcVal * 0.382; // mV
        rts = 10000.0 * vtsx / (3300.0 - vtsx); // Ohm
        tmp = 1.0/(1.0/(273.15+25) + 1.0/bms->thermistor_beta*log(rts/10000.0)); // K
        bms->temperatures[2] = (tmp - 273.15) * 10.0;
    }
}


//----------------------------------------------------------------------------

void bms_update_current(BMS *bms)
{
    int adcVal = 0;
    regSYS_STAT_t sys_stat;
    sys_stat.regByte = bq769x0_read_byte(SYS_STAT);

    // check if new current reading available
    if (sys_stat.bits.CC_READY == 1)
    {
        //printf("reading CC register...\n");
        adcVal = (bq769x0_read_byte(CC_HI_BYTE) << 8) | bq769x0_read_byte(CC_LO_BYTE);
        bms->battery_current = (int16_t) adcVal * 8.44 / bms->shunt_res_mOhm;  // mA

        bms->coulomb_counter += bms->battery_current / 4;  // is read every 250 ms

        // reduce resolution for actual current value
        if (bms->battery_current > -10 && bms->battery_current < 10) {
            bms->battery_current = 0;
        }

        // reset idle_timestamp
        if (abs(bms->battery_current) > bms->idle_current_threshold) {
            bms->idle_timestamp = time(NULL);
        }

        // no error occured which caused alert
        if (!(sys_stat.regByte & 0b00111111)) {
            bq769x0_alert_flag_reset();
        }

        bq769x0_write_byte(SYS_STAT, 0b10000000);  // Clear CC ready flag
    }
}

//----------------------------------------------------------------------------

void bms_update_voltages(BMS *bms)
{
    int adc_val = 0;
    int connectedCellsTemp = 0;

    bms->id_cell_voltage_max = 0;
    bms->id_cell_voltage_min = 0;
    for (int i = 0; i < NUM_CELLS_MAX; i++)
    {
        adc_val = bq769x0_read_word(VC1_HI_BYTE + i*2) & 0x3FFF;

        bms->cell_voltages[i] = adc_val * adc_gain / 1000 + adc_offset;

        if (bms->cell_voltages[i] > 500) {
            connectedCellsTemp++;
        }

        if (bms->cell_voltages[i] > bms->cell_voltages[bms->id_cell_voltage_max]) {
            bms->id_cell_voltage_max = i;
        }
        if (bms->cell_voltages[i] < bms->cell_voltages[bms->id_cell_voltage_min] && bms->cell_voltages[i] > 500) {
            bms->id_cell_voltage_min = i;
        }
    }
    bms->connected_cells = connectedCellsTemp;

    // read battery pack voltage
    adc_val = bq769x0_read_word(BAT_HI_BYTE);
    bms->battery_voltage = 4.0 * adc_gain * adc_val / 1000.0 + bms->connected_cells * adc_offset;
}


#if BMS_DEBUG

//----------------------------------------------------------------------------
// for debug purposes

void bms_print_registers()
{
    printf("0x00 SYS_STAT:  %s\n", byte2char(bq769x0_read_byte(SYS_STAT)));
    printf("0x01 CELLBAL1:  %s\n", byte2char(bq769x0_read_byte(CELLBAL1)));
    printf("0x04 SYS_CTRL1: %s\n", byte2char(bq769x0_read_byte(SYS_CTRL1)));
    printf("0x05 SYS_CTRL2: %s\n", byte2char(bq769x0_read_byte(SYS_CTRL2)));
    printf("0x06 PROTECT1:  %s\n", byte2char(bq769x0_read_byte(PROTECT1)));
    printf("0x07 PROTECT2:  %s\n", byte2char(bq769x0_read_byte(PROTECT2)));
    printf("0x08 PROTECT3:  %s\n", byte2char(bq769x0_read_byte(PROTECT3)));
    printf("0x09 OV_TRIP:   %s\n", byte2char(bq769x0_read_byte(OV_TRIP)));
    printf("0x0A UV_TRIP:   %s\n", byte2char(bq769x0_read_byte(UV_TRIP)));
    printf("0x0B CC_CFG:    %s\n", byte2char(bq769x0_read_byte(CC_CFG)));
    printf("0x32 CC_HI:     %s\n", byte2char(bq769x0_read_byte(CC_HI_BYTE)));
    printf("0x33 CC_LO:     %s\n", byte2char(bq769x0_read_byte(CC_LO_BYTE)));
    /*
    printf("0x50 ADCGAIN1:  %s\n", byte2char(bq769x0_read_byte(ADCGAIN1)));
    printf("0x51 ADCOFFSET: %s\n", byte2char(bq769x0_read_byte(ADCOFFSET)));
    printf("0x59 ADCGAIN2:  %s\n", byte2char(bq769x0_read_byte(ADCGAIN2)));
    */
}

#endif

#endif // defined BQ769x0