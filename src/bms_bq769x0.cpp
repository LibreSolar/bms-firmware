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

#include <math.h>     // log for thermistor calculation

#include "bms.h"
#include "bms_bq769x0_registers.h"
#include "mbed.h"

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

uint8_t _crc8_ccitt_update (uint8_t inCrc, uint8_t inData)
{
    uint8_t   i;
    uint8_t   data;

    data = inCrc ^ inData;

    for ( i = 0; i < 8; i++ )
    {
        if (( data & 0x80 ) != 0 )
        {
            data <<= 1;
            data ^= 0x07;
        }
        else
        {
            data <<= 1;
        }
    }
    return data;
}

//----------------------------------------------------------------------------

BMS::BMS(I2C& bqI2C, PinName alertPin, int bqType, int bqI2CAddress, bool crc):
    _i2c(bqI2C), _alertInterrupt(alertPin)
{
    _timer.start();
    _alertInterrupt.rise(callback(this, &BMS::setAlertInterruptFlag));

    // set some safe default values
    autoBalancingEnabled = false;
    balancingMinIdleTime_s = 1800;    // default: 30 minutes
    idleCurrentThreshold = 30; // mA

    thermistorBetaValue = 3435;  // typical value for Semitec 103AT-5 thermistor

    alertInterruptFlag = true;   // init with true to check and clear errors at start-up

    type = bqType;
    if (type == bq76920) {
        numberOfCells = 5;
    } else if (type == bq76930) {
        numberOfCells = 10;
    } else {
        numberOfCells = 15;
    }

    // initialize variables
    for (int i = 0; i < numberOfCells - 1; i++) {
        cellVoltages[i] = 0;
    }

    //crcEnabled = crc;
    //I2CAddress = bqI2CAddress;

    if (determineAddressAndCrc())
    {
        // initial settings for bq769x0
        writeRegister(SYS_CTRL1, 0b00011000);  // switch external thermistor and ADC on
        writeRegister(SYS_CTRL2, 0b01000000);  // switch CC_EN on

        // get ADC offset and gain
        adcOffset = (signed int) readRegister(ADCOFFSET);  // convert from 2's complement
        adcGain = 365 + (((readRegister(ADCGAIN1) & 0b00001100) << 1) |
            ((readRegister(ADCGAIN2) & 0b11100000) >> 5)); // uV/LSB
    }
    else {
        // TODO: do something else... e.g. set error flag
#if BQ769X0_DEBUG
        printf("BMS communication error\n");
#endif
    }
}

//----------------------------------------------------------------------------
// automatically find out address and CRC setting

bool BMS::determineAddressAndCrc(void)
{
    I2CAddress = 0x08;
    crcEnabled = true;
    writeRegister(CC_CFG, 0x19);
    if (readRegister(CC_CFG) == 0x19) {
        return true;
    }

    I2CAddress = 0x18;
    crcEnabled = true;
    writeRegister(CC_CFG, 0x19);
    if (readRegister(CC_CFG) == 0x19) {
        return true;
    }

    I2CAddress = 0x08;
    crcEnabled = false;
    writeRegister(CC_CFG, 0x19);
    if (readRegister(CC_CFG) == 0x19) {
        return true;
    }

    I2CAddress = 0x18;
    crcEnabled = false;
    writeRegister(CC_CFG, 0x19);
    if (readRegister(CC_CFG) == 0x19) {
        return true;
    }

    return false;
}

//----------------------------------------------------------------------------
// Boot IC by pulling the boot pin TS1 high for some ms

void BMS::boot(PinName bootPin)
{
    DigitalInOut boot(bootPin);

    boot = 1;
    wait_ms(5);   // wait 5 ms for device to receive boot signal (datasheet: max. 2 ms)
    boot.input();         // don't disturb temperature measurement
    wait_ms(10);  // wait for device to boot up completely (datasheet: max. 10 ms)
}


//----------------------------------------------------------------------------
// Fast function to check whether BMS has an error
// (returns 0 if everything is OK)

int BMS::checkStatus()
{
    //  printf("errorStatus: ");
    //  printf(errorStatus);
    if (_alertInterrupt == 0 && errorStatus == 0) {
        return 0;
    } else {

        regSYS_STAT_t sys_stat;
        sys_stat.regByte = readRegister(SYS_STAT);

        // first check, if only a new CC reading is available
        if (sys_stat.bits.CC_READY == 1) {
            //printf("Interrupt: CC ready");
            updateCurrent();  // automatically clears CC ready flag
        }

        // Serious error occured
        if (sys_stat.regByte & 0b00111111)
        {
            if (alertInterruptFlag == true) {
                secSinceErrorCounter = 0;
            }
            errorStatus = sys_stat.regByte;

            unsigned int secSinceInterrupt = (_timer.read_ms() - interruptTimestamp) / 1000;

            // check for overrun of _timer.read_ms() or very slow running program
            if (abs((long)(secSinceInterrupt - secSinceErrorCounter)) > 2) {
                secSinceErrorCounter = secSinceInterrupt;
            }

            // called only once per second
            if (secSinceInterrupt >= secSinceErrorCounter)
            {
                if (sys_stat.regByte & 0b00100000) { // XR error
                    // datasheet recommendation: try to clear after waiting a few seconds
                    if (secSinceErrorCounter % 3 == 0) {
                        #if BQ769X0_DEBUG
                        printf("Attempting to clear XR error");
                        #endif
                        writeRegister(SYS_STAT, 0b00100000);
                        enableCharging();
                        enableDischarging();
                    }
                }
                if (sys_stat.regByte & 0b00010000) { // Alert error
                    if (secSinceErrorCounter % 10 == 0) {
                        #if BQ769X0_DEBUG
                        printf("Attempting to clear Alert error");
                        #endif
                        writeRegister(SYS_STAT, 0b00010000);
                        enableCharging();
                        enableDischarging();
                    }
                }
                if (sys_stat.regByte & 0b00001000) { // UV error
                    updateVoltages();
                    if (cellVoltages[idCellMinVoltage] > minCellVoltage) {
                        #if BQ769X0_DEBUG
                        printf("Attempting to clear UV error");
                        #endif
                        writeRegister(SYS_STAT, 0b00001000);
                        enableDischarging();
                    }
                }
                if (sys_stat.regByte & 0b00000100) { // OV error
                    updateVoltages();
                    if (cellVoltages[idCellMaxVoltage] < maxCellVoltage) {
                        #if BQ769X0_DEBUG
                        printf("Attempting to clear OV error");
                        #endif
                        writeRegister(SYS_STAT, 0b00000100);
                        enableCharging();
                    }
                }
                if (sys_stat.regByte & 0b00000010) { // SCD
                    if (secSinceErrorCounter % 60 == 0) {
                        #if BQ769X0_DEBUG
                        printf("Attempting to clear SCD error");
                        #endif
                        writeRegister(SYS_STAT, 0b00000010);
                        enableDischarging();
                    }
                }
                if (sys_stat.regByte & 0b00000001) { // OCD
                    if (secSinceErrorCounter % 60 == 0) {
                        #if BQ769X0_DEBUG
                        printf("Attempting to clear OCD error");
                        #endif
                        writeRegister(SYS_STAT, 0b00000001);
                        enableDischarging();
                    }
                }
                secSinceErrorCounter++;
            }
        }
        else {
            errorStatus = 0;
        }

        return errorStatus;
    }
}

//----------------------------------------------------------------------------
// checks if temperatures are within the limits, otherwise disables CHG/DSG FET

void BMS::checkCellTemp()
{
    int numberOfThermistors = numberOfCells/5;
    bool cellTempChargeError = 0;
    bool cellTempDischargeError = 0;

    for (int thermistor = 0; thermistor < numberOfThermistors; thermistor++) {
        cellTempChargeError |=
            temperatures[thermistor] > maxCellTempCharge - cellTempChargeErrorFlag ? cellTempHysteresis : 0 ||
            temperatures[thermistor] < minCellTempCharge + cellTempChargeErrorFlag ? cellTempHysteresis : 0;

        cellTempDischargeError |=
            temperatures[thermistor] > maxCellTempDischarge - cellTempDischargeErrorFlag ? cellTempHysteresis : 0 ||
            temperatures[thermistor] < minCellTempDischarge + cellTempDischargeErrorFlag ? cellTempHysteresis : 0;
    }

    if (cellTempChargeErrorFlag != cellTempChargeError) {
        cellTempChargeErrorFlag = cellTempChargeError;
        if (cellTempChargeError) {
            disableCharging();
            #if BQ769X0_DEBUG
            printf("Temperature error (CHG)");
            #endif
        }
        else {
            enableCharging();
            #if BQ769X0_DEBUG
            printf("Clearing temperature error (CHG)");
            #endif
        }
    }

    if (cellTempDischargeErrorFlag != cellTempDischargeError) {
        cellTempDischargeErrorFlag = cellTempDischargeError;
        if (cellTempDischargeError) {
            disableDischarging();
            #if BQ769X0_DEBUG
            printf("Temperature error (DSG)");
            #endif
        }
        else {
            enableDischarging();
            #if BQ769X0_DEBUG
            printf("Clearing temperature error (DSG)");
            #endif
        }
    }
}

//----------------------------------------------------------------------------
// puts BMS IC into SHIP mode (i.e. switched off)

void BMS::shutdown()
{
    writeRegister(SYS_CTRL1, 0x0);
    writeRegister(SYS_CTRL1, 0x1);
    writeRegister(SYS_CTRL1, 0x2);
}

//----------------------------------------------------------------------------

bool BMS::enableCharging()
{
    #if BQ769X0_DEBUG
    printf("checkStatus() = %d\n", checkStatus());
    printf("Umax = %d\n", cellVoltages[idCellMaxVoltage]);
    printf("temperatures[0] = %d\n", temperatures[0]);
    #endif

    int numberOfThermistors = numberOfCells/5;
    bool cellTempChargeError = 0;

    for (int thermistor = 0; thermistor < numberOfThermistors; thermistor++) {
        cellTempChargeError |=
            temperatures[thermistor] > maxCellTempCharge ||
            temperatures[thermistor] < minCellTempCharge;
    }

    if (checkStatus() == 0 &&
        cellVoltages[idCellMaxVoltage] < maxCellVoltage &&
        cellTempChargeError == 0)
    {
        int sys_ctrl2;
        sys_ctrl2 = readRegister(SYS_CTRL2);
        writeRegister(SYS_CTRL2, sys_ctrl2 | 0b00000001);  // switch CHG on
        #if BQ769X0_DEBUG
        printf("Enabling CHG FET\n");
        #endif
        return true;
    }
    else {
        return false;
    }
}

//----------------------------------------------------------------------------

void BMS::disableCharging()
{
    int sys_ctrl2;
    sys_ctrl2 = readRegister(SYS_CTRL2);
    writeRegister(SYS_CTRL2, sys_ctrl2 & ~0b00000001);  // switch CHG off
    #if BQ769X0_DEBUG
    printf("Disabling CHG FET\n");
    #endif
}

//----------------------------------------------------------------------------

bool BMS::enableDischarging()
{
    #if BQ769X0_DEBUG
    printf("checkStatus() = %d\n", checkStatus());
    printf("Umin = %d\n", cellVoltages[idCellMinVoltage]);
    printf("temperatures[0] = %d\n", temperatures[0]);
    #endif

    int numberOfThermistors = numberOfCells/5;
    bool cellTempDischargeError = 0;

    for (int thermistor = 0; thermistor < numberOfThermistors; thermistor++) {
        cellTempDischargeError |=
            temperatures[thermistor] > maxCellTempDischarge ||
            temperatures[thermistor] < minCellTempDischarge;
    }

    if (checkStatus() == 0 &&
        cellVoltages[idCellMinVoltage] > minCellVoltage &&
        cellTempDischargeError == 0)
    {
        int sys_ctrl2;
        sys_ctrl2 = readRegister(SYS_CTRL2);
        writeRegister(SYS_CTRL2, sys_ctrl2 | 0b00000010);  // switch DSG on
        return true;
    }
    else {
        return false;
    }
}

//----------------------------------------------------------------------------

void BMS::disableDischarging()
{
    int sys_ctrl2;
    sys_ctrl2 = readRegister(SYS_CTRL2);
    writeRegister(SYS_CTRL2, sys_ctrl2 & ~0b00000010);  // switch DSG off
    #if BQ769X0_DEBUG
    printf("Disabling DISCHG FET\n");
    #endif
}

//----------------------------------------------------------------------------

void BMS::enableAutoBalancing(void)
{
    autoBalancingEnabled = true;
}


//----------------------------------------------------------------------------

void BMS::setBalancingThresholds(int idleTime_min, int absVoltage_mV, int voltageDifference_mV)
{
    balancingMinIdleTime_s = idleTime_min * 60;
    balancingMinCellVoltage_mV = absVoltage_mV;
    balancingMaxVoltageDifference_mV = voltageDifference_mV;
}

//----------------------------------------------------------------------------
// sets balancing registers if balancing is allowed
// (sufficient idle time + voltage)

void BMS::updateBalancingSwitches(void)
{
    long idleSeconds = (_timer.read_ms() - idleTimestamp) / 1000;
    int numberOfSections = numberOfCells/5;

    // check for _timer.read_ms() overflow
    if (idleSeconds < 0) {
        idleTimestamp = 0;
        idleSeconds = _timer.read_ms() / 1000;
    }

    // check if balancing allowed
    if (checkStatus() == 0 &&
        idleSeconds >= balancingMinIdleTime_s &&
        cellVoltages[idCellMaxVoltage] > balancingMinCellVoltage_mV &&
        (cellVoltages[idCellMaxVoltage] - cellVoltages[idCellMinVoltage]) > balancingMaxVoltageDifference_mV)
    {
        //printf("Balancing enabled!");
        balancingStatus = 0;  // current status will be set in following loop

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
                if ((cellVoltages[section*5 + i] - cellVoltages[idCellMinVoltage]) > balancingMaxVoltageDifference_mV) {
                    int j = cellCounter;
                    while (j > 0 && cellVoltages[section*5 + cellList[j - 1]] < cellVoltages[section*5 + i])
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

            #if BQ769X0_DEBUG
            //printf("Setting CELLBAL%d register to: %s\n", section+1, byte2char(balancingFlags));
            #endif

            balancingStatus |= balancingFlags << section*5;

            // set balancing register for this section
            writeRegister(CELLBAL1+section, balancingFlags);

        } // section loop
    }
    else if (balancingStatus > 0)
    {
        // clear all CELLBAL registers
        for (int section = 0; section < numberOfSections; section++)
        {
            #if BQ769X0_DEBUG
            printf("Clearing Register CELLBAL%d\n", section+1);
            #endif

            writeRegister(CELLBAL1+section, 0x0);
        }

        balancingStatus = 0;
    }
}

//----------------------------------------------------------------------------

int BMS::getBalancingStatus()
{
    return balancingStatus;
}

//----------------------------------------------------------------------------

void BMS::setTemperatureLimits(int minDischarge_degC, int maxDischarge_degC,
  int minCharge_degC, int maxCharge_degC, int hysteresis_degC)
{
    // Temperature limits (°C/10)
    minCellTempDischarge = minDischarge_degC * 10;
    maxCellTempDischarge = maxDischarge_degC * 10;
    minCellTempCharge = minCharge_degC * 10;
    maxCellTempCharge = maxCharge_degC * 10;
    cellTempHysteresis = hysteresis_degC * 10;
}

//----------------------------------------------------------------------------

long BMS::setShortCircuitProtection(long current_mA, int delay_us)
{
    regPROTECT1_t protect1;

    // only RSNS = 1 considered
    protect1.bits.RSNS = 1;

    protect1.bits.SCD_THRESH = 0;
    for (int i = sizeof(SCD_threshold_setting)-1; i > 0; i--) {
        if (current_mA * shuntResistorValue_mOhm / 1000 >= SCD_threshold_setting[i]) {
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

    writeRegister(PROTECT1, protect1.regByte);

    // returns the actual current threshold value
    return (long)SCD_threshold_setting[protect1.bits.SCD_THRESH] * 1000 /
        shuntResistorValue_mOhm;
}

//----------------------------------------------------------------------------

long BMS::setOvercurrentChargeProtection(long current_mA, int delay_ms)
{
    // ToDo: Software protection for charge overcurrent
    return 0;
}

//----------------------------------------------------------------------------

long BMS::setOvercurrentDischargeProtection(long current_mA, int delay_ms)
{
    regPROTECT2_t protect2;

    // Remark: RSNS must be set to 1 in PROTECT1 register

    protect2.bits.OCD_THRESH = 0;
    for (int i = sizeof(OCD_threshold_setting)-1; i > 0; i--) {
        if (current_mA * shuntResistorValue_mOhm / 1000 >= OCD_threshold_setting[i]) {
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

    writeRegister(PROTECT2, protect2.regByte);

    // returns the actual current threshold value
    return (long)OCD_threshold_setting[protect2.bits.OCD_THRESH] * 1000 /
        shuntResistorValue_mOhm;
}


//----------------------------------------------------------------------------

int BMS::setCellUndervoltageProtection(int voltage_mV, int delay_s)
{
    regPROTECT3_t protect3;
    int uv_trip = 0;

    minCellVoltage = voltage_mV;

    protect3.regByte = readRegister(PROTECT3);

    uv_trip = ((((long)voltage_mV - adcOffset) * 1000 / adcGain) >> 4) & 0x00FF;
    uv_trip += 1;   // always round up for lower cell voltage
    writeRegister(UV_TRIP, uv_trip);

    protect3.bits.UV_DELAY = 0;
    for (int i = sizeof(UV_delay_setting)-1; i > 0; i--) {
        if (delay_s >= UV_delay_setting[i]) {
            protect3.bits.UV_DELAY = i;
            break;
        }
    }

    writeRegister(PROTECT3, protect3.regByte);

    // returns the actual current threshold value
    return ((long)1 << 12 | uv_trip << 4) * adcGain / 1000 + adcOffset;
}

//----------------------------------------------------------------------------

int BMS::setCellOvervoltageProtection(int voltage_mV, int delay_s)
{
    regPROTECT3_t protect3;
    int ov_trip = 0;

    maxCellVoltage = voltage_mV;

    protect3.regByte = readRegister(PROTECT3);

    ov_trip = ((((long)voltage_mV - adcOffset) * 1000 / adcGain) >> 4) & 0x00FF;
    writeRegister(OV_TRIP, ov_trip);

    protect3.bits.OV_DELAY = 0;
    for (int i = sizeof(OV_delay_setting)-1; i > 0; i--) {
        if (delay_s >= OV_delay_setting[i]) {
            protect3.bits.OV_DELAY = i;
            break;
        }
    }

    writeRegister(PROTECT3, protect3.regByte);

    // returns the actual current threshold value
    return ((long)1 << 13 | ov_trip << 4) * adcGain / 1000 + adcOffset;
}


//----------------------------------------------------------------------------

void BMS::updateTemperatures()
{
    float tmp = 0;
    int adcVal = 0;
    int vtsx = 0;
    unsigned long rts = 0;

    // calculate R_thermistor according to bq769x0 datasheet
    adcVal = (readRegister(TS1_HI_BYTE) & 0b00111111) << 8 | readRegister(TS1_LO_BYTE);
    vtsx = adcVal * 0.382; // mV
    rts = 10000.0 * vtsx / (3300.0 - vtsx); // Ohm

    // Temperature calculation using Beta equation
    // - According to bq769x0 datasheet, only 10k thermistors should be used
    // - 25°C reference temperature for Beta equation assumed
    tmp = 1.0/(1.0/(273.15+25) + 1.0/thermistorBetaValue*log(rts/10000.0)); // K
    temperatures[0] = (tmp - 273.15) * 10.0;

    if (type == bq76930 || type == bq76940) {
        adcVal = (readRegister(TS2_HI_BYTE) & 0b00111111) << 8 | readRegister(TS2_LO_BYTE);
        vtsx = adcVal * 0.382; // mV
        rts = 10000.0 * vtsx / (3300.0 - vtsx); // Ohm
        tmp = 1.0/(1.0/(273.15+25) + 1.0/thermistorBetaValue*log(rts/10000.0)); // K
        temperatures[1] = (tmp - 273.15) * 10.0;
    }

    if (type == bq76940) {
        adcVal = (readRegister(TS3_HI_BYTE) & 0b00111111) << 8 | readRegister(TS3_LO_BYTE);
        vtsx = adcVal * 0.382; // mV
        rts = 10000.0 * vtsx / (3300.0 - vtsx); // Ohm
        tmp = 1.0/(1.0/(273.15+25) + 1.0/thermistorBetaValue*log(rts/10000.0)); // K
        temperatures[2] = (tmp - 273.15) * 10.0;
    }
}


//----------------------------------------------------------------------------

void BMS::updateCurrent()
{
    int adcVal = 0;
    regSYS_STAT_t sys_stat;
    sys_stat.regByte = readRegister(SYS_STAT);

    // check if new current reading available
    if (sys_stat.bits.CC_READY == 1)
    {
        //printf("reading CC register...\n");
        adcVal = (readRegister(CC_HI_BYTE) << 8) | readRegister(CC_LO_BYTE);
        batCurrent = (int16_t) adcVal * 8.44 / shuntResistorValue_mOhm;  // mA

        coulombCounter += batCurrent / 4;  // is read every 250 ms

        // reduce resolution for actual current value
        if (batCurrent > -10 && batCurrent < 10) {
            batCurrent = 0;
        }

        // reset idleTimestamp
        if (abs(batCurrent) > idleCurrentThreshold) {
            idleTimestamp = _timer.read_ms();
        }

        // no error occured which caused alert
        if (!(sys_stat.regByte & 0b00111111)) {
            alertInterruptFlag = false;
        }

        writeRegister(SYS_STAT, 0b10000000);  // Clear CC ready flag
    }
}

//----------------------------------------------------------------------------
// reads all cell voltages to array cellVoltages[NUM_CELLS] and updates batVoltage

void BMS::updateVoltages()
{
    long adcVal = 0;
    char buf[4];
    int connectedCellsTemp = 0;

    uint8_t crc;

    // read cell voltages
    buf[0] = (char) VC1_HI_BYTE;
    _i2c.write(I2CAddress << 1, buf, 1);;

    idCellMaxVoltage = 0;
    idCellMinVoltage = 0;
    for (int i = 0; i < numberOfCells; i++)
    {
        if (crcEnabled == true) {
            _i2c.read(I2CAddress << 1, buf, 4);
            adcVal = (buf[0] & 0b00111111) << 8 | buf[2];

            // CRC of first bytes includes slave address (including R/W bit) and data
            crc = _crc8_ccitt_update(0, (I2CAddress << 1) | 1);
            crc = _crc8_ccitt_update(crc, buf[0]);
            if (crc != buf[1]) return; // don't save corrupted value

            // CRC of subsequent bytes contain only data
            crc = _crc8_ccitt_update(0, buf[2]);
            if (crc != buf[3]) return; // don't save corrupted value
        }
        else {
            _i2c.read(I2CAddress << 1, buf, 2);
            adcVal = (buf[0] & 0b00111111) << 8 | buf[1];
        }

        cellVoltages[i] = adcVal * adcGain / 1000 + adcOffset;

        if (cellVoltages[i] > 500) {
            connectedCellsTemp++;
        }

        if (cellVoltages[i] > cellVoltages[idCellMaxVoltage]) {
            idCellMaxVoltage = i;
        }
        if (cellVoltages[i] < cellVoltages[idCellMinVoltage] && cellVoltages[i] > 500) {
            idCellMinVoltage = i;
        }
    }
    connectedCells = connectedCellsTemp;

    // read battery pack voltage
    adcVal = (readRegister(BAT_HI_BYTE) << 8) | readRegister(BAT_LO_BYTE);
    batVoltage = 4.0 * adcGain * adcVal / 1000.0 + connectedCells * adcOffset;
}

//----------------------------------------------------------------------------

void BMS::writeRegister(int address, int data)
{
    uint8_t crc = 0;
    char buf[3];

    buf[0] = (char) address;
    buf[1] = data;

    if (crcEnabled == true) {
        // CRC is calculated over the slave address (including R/W bit), register address, and data.
        crc = _crc8_ccitt_update(crc, (I2CAddress << 1) | 0);
        crc = _crc8_ccitt_update(crc, buf[0]);
        crc = _crc8_ccitt_update(crc, buf[1]);
        buf[2] = crc;
        _i2c.write(I2CAddress << 1, buf, 3);
    }
    else {
        _i2c.write(I2CAddress << 1, buf, 2);
    }
}

//----------------------------------------------------------------------------

int BMS::readRegister(int address)
{
    uint8_t crc = 0;
    char buf[2];

    #if BQ769X0_DEBUG
    //printf("Read register: 0x%x \n", address);
    #endif

    buf[0] = (char)address;
    _i2c.write(I2CAddress << 1, buf, 1);;

    if (crcEnabled == true) {
        do {
            _i2c.read(I2CAddress << 1, buf, 2);
            // CRC is calculated over the slave address (including R/W bit) and data.
            crc = _crc8_ccitt_update(crc, (I2CAddress << 1) | 1);
            crc = _crc8_ccitt_update(crc, buf[0]);
        } while (crc != buf[1]);
        return buf[0];
    }
    else {
        _i2c.read(I2CAddress << 1, buf, 1);
        return buf[0];
    }
}

//----------------------------------------------------------------------------
// The bq769x0 drives the ALERT pin high if the SYS_STAT register contains
// a new value (either new CC reading or an error)

void BMS::setAlertInterruptFlag()
{
    interruptTimestamp = _timer.read_ms();
    alertInterruptFlag = true;
}

#if BQ769X0_DEBUG

//----------------------------------------------------------------------------
// for debug purposes

void BMS::printRegisters()
{
    printf("0x00 SYS_STAT:  %s\n", byte2char(readRegister(SYS_STAT)));
    printf("0x01 CELLBAL1:  %s\n", byte2char(readRegister(CELLBAL1)));
    printf("0x04 SYS_CTRL1: %s\n", byte2char(readRegister(SYS_CTRL1)));
    printf("0x05 SYS_CTRL2: %s\n", byte2char(readRegister(SYS_CTRL2)));
    printf("0x06 PROTECT1:  %s\n", byte2char(readRegister(PROTECT1)));
    printf("0x07 PROTECT2:  %s\n", byte2char(readRegister(PROTECT2)));
    printf("0x08 PROTECT3:  %s\n", byte2char(readRegister(PROTECT3)));
    printf("0x09 OV_TRIP:   %s\n", byte2char(readRegister(OV_TRIP)));
    printf("0x0A UV_TRIP:   %s\n", byte2char(readRegister(UV_TRIP)));
    printf("0x0B CC_CFG:    %s\n", byte2char(readRegister(CC_CFG)));
    printf("0x32 CC_HI:     %s\n", byte2char(readRegister(CC_HI_BYTE)));
    printf("0x33 CC_LO:     %s\n", byte2char(readRegister(CC_LO_BYTE)));
    /*
    printf("0x50 ADCGAIN1:  %s\n", byte2char(readRegister(ADCGAIN1)));
    printf("0x51 ADCOFFSET: %s\n", byte2char(readRegister(ADCOFFSET)));
    printf("0x59 ADCGAIN2:  %s\n", byte2char(readRegister(ADCGAIN2)));
    */
}

#endif
