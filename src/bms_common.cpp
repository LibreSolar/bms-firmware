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
    update_current();  // will only read new current value if alert was triggered
    update_voltages();
    update_temperatures();
    update_balancing_switches();
}

//----------------------------------------------------------------------------

void BMS::set_shunt_res(float res_mOhm)
{
    shunt_res_mOhm = res_mOhm;
}

//----------------------------------------------------------------------------

void BMS::set_thermistor_beta(int beta_K)
{
    thermistor_beta = beta_K;
}

//----------------------------------------------------------------------------

void BMS::set_battery_capacity(long capacity_mAh)
{
    nominal_capacity = capacity_mAh * 3600;
}

//----------------------------------------------------------------------------

void BMS::set_ocv(int voltageVsSOC[NUM_OCV_POINTS])
{
    OCV = voltageVsSOC;
}

//----------------------------------------------------------------------------

float BMS::get_soc(void)
{
    return (double) coulomb_counter / nominal_capacity * 100;
}

//----------------------------------------------------------------------------
// SOC calculation based on average cell open circuit voltage

void BMS::reset_soc(int percent)
{
    if (percent <= 100 && percent >= 0)
    {
        coulomb_counter = nominal_capacity * (percent / 100.0);
    }
    else  // reset based on OCV
    {
        printf("NumCells: %d, voltage: %d V\n", get_connected_cells(), pack_voltage());
        int voltage = pack_voltage() / get_connected_cells();

        coulomb_counter = 0;  // initialize with totally depleted battery (0% SOC)

        for (int i = 0; i < NUM_OCV_POINTS; i++)
        {
            if (OCV[i] <= voltage) {
                if (i == 0) {
                    coulomb_counter = nominal_capacity;  // 100% full
                }
                else {
                    // interpolate between OCV[i] and OCV[i-1]
                    coulomb_counter = (double) nominal_capacity / (NUM_OCV_POINTS - 1.0) *
                    (NUM_OCV_POINTS - 1.0 - i + ((float)voltage - OCV[i])/(OCV[i-1] - OCV[i]));
                }
                return;
            }
        }
    }
}


//----------------------------------------------------------------------------

void BMS::set_idle_current_threshold(int current_mA)
{
    idle_current_threshold = current_mA;
}

//----------------------------------------------------------------------------

int BMS::pack_current()
{
    return battery_current;
}

//----------------------------------------------------------------------------

int BMS::pack_voltage()
{
    return battery_voltage;
}

//----------------------------------------------------------------------------

int BMS::cell_voltage_max()
{
    return cell_voltages[id_cell_voltage_max];
}

//----------------------------------------------------------------------------

int BMS::cell_voltage_min()
{
    return cell_voltages[id_cell_voltage_min];
}

//----------------------------------------------------------------------------

int BMS::cell_voltage(int idCell)
{
    return cell_voltages[idCell-1];
}

//----------------------------------------------------------------------------

int BMS::get_num_cells_max(void)
{
    return num_cells_max;
}

//----------------------------------------------------------------------------

int BMS::get_connected_cells(void)
{
    return connected_cells;
}

//----------------------------------------------------------------------------

float BMS::get_temp_degC(int channel)
{
    if (channel >= 1 && channel <= 3) {
        return (float)temperatures[channel-1] / 10.0;
    }
    else {
        return -273.15;   // Error: Return absolute minimum temperature
    }
}

//----------------------------------------------------------------------------

float BMS::get_temp_degF(int channel)
{
    return get_temp_degC(channel) * 1.8 + 32;
}

