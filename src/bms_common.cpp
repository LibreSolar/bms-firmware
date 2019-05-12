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
#include "mbed.h"

//----------------------------------------------------------------------------
// should be called at least once every 250 ms to get correct coulomb counting

void BMS::update()
{
    updateCurrent();  // will only read new current value if alert was triggered
    updateVoltages();
    updateTemperatures();
    updateBalancingSwitches();
}

//----------------------------------------------------------------------------

void BMS::setShuntResistorValue(float res_mOhm)
{
    shuntResistorValue_mOhm = res_mOhm;
}

//----------------------------------------------------------------------------

void BMS::setThermistorBetaValue(int beta_K)
{
    thermistorBetaValue = beta_K;
}

//----------------------------------------------------------------------------

void BMS::setBatteryCapacity(long capacity_mAh)
{
    nominalCapacity = capacity_mAh * 3600;
}

//----------------------------------------------------------------------------

void BMS::setOCV(int voltageVsSOC[NUM_OCV_POINTS])
{
    OCV = voltageVsSOC;
}

//----------------------------------------------------------------------------

float BMS::getSOC(void)
{
    return (double) coulombCounter / nominalCapacity * 100;
}

//----------------------------------------------------------------------------
// SOC calculation based on average cell open circuit voltage

void BMS::resetSOC(int percent)
{
    if (percent <= 100 && percent >= 0)
    {
        coulombCounter = nominalCapacity * (percent / 100.0);
    }
    else  // reset based on OCV
    {
        printf("NumCells: %d, voltage: %d V\n", getNumberOfConnectedCells(), getBatteryVoltage());
        int voltage = getBatteryVoltage() / getNumberOfConnectedCells();

        coulombCounter = 0;  // initialize with totally depleted battery (0% SOC)

        for (int i = 0; i < NUM_OCV_POINTS; i++)
        {
            if (OCV[i] <= voltage) {
                if (i == 0) {
                    coulombCounter = nominalCapacity;  // 100% full
                }
                else {
                    // interpolate between OCV[i] and OCV[i-1]
                    coulombCounter = (double) nominalCapacity / (NUM_OCV_POINTS - 1.0) *
                    (NUM_OCV_POINTS - 1.0 - i + ((float)voltage - OCV[i])/(OCV[i-1] - OCV[i]));
                }
                return;
            }
        }
    }
}


//----------------------------------------------------------------------------

void BMS::setIdleCurrentThreshold(int current_mA)
{
    idleCurrentThreshold = current_mA;
}

//----------------------------------------------------------------------------

int BMS::getBatteryCurrent()
{
    return batCurrent;
}

//----------------------------------------------------------------------------

int BMS::getBatteryVoltage()
{
    return batVoltage;
}

//----------------------------------------------------------------------------

int BMS::getMaxCellVoltage()
{
    return cellVoltages[idCellMaxVoltage];
}

//----------------------------------------------------------------------------

int BMS::getMinCellVoltage()
{
    return cellVoltages[idCellMinVoltage];
}

//----------------------------------------------------------------------------

int BMS::getCellVoltage(int idCell)
{
    return cellVoltages[idCell-1];
}

//----------------------------------------------------------------------------

int BMS::getNumberOfCells(void)
{
    return numberOfCells;
}

//----------------------------------------------------------------------------

int BMS::getNumberOfConnectedCells(void)
{
    return connectedCells;
}

//----------------------------------------------------------------------------

float BMS::getTemperatureDegC(int channel)
{
    if (channel >= 1 && channel <= 3) {
        return (float)temperatures[channel-1] / 10.0;
    }
    else {
        return -273.15;   // Error: Return absolute minimum temperature
    }
}

//----------------------------------------------------------------------------

float BMS::getTemperatureDegF(int channel)
{
    return getTemperatureDegC(channel) * 1.8 + 32;
}

