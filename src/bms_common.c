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
#include <stdio.h>

#include "bms.h"

//----------------------------------------------------------------------------

void bms_update(BMS *bms)
{
    bms_update_current(bms);  // will only read new current value if alert was triggered
    bms_update_voltages(bms);
    bms_update_temperatures(bms);
    bms_update_balancing_switches(bms);
}

//----------------------------------------------------------------------------

void bms_set_shunt_res(BMS *bms, float res_mOhm)
{
    bms->shunt_res_mOhm = res_mOhm;
}

//----------------------------------------------------------------------------

void bms_set_thermistor_beta(BMS *bms, int beta_K)
{
    bms->thermistor_beta = beta_K;
}

//----------------------------------------------------------------------------

void bms_set_battery_capacity(BMS *bms, long capacity_mAh)
{
    bms->nominal_capacity = capacity_mAh * 3600;
}

//----------------------------------------------------------------------------

void bms_set_ocv(BMS *bms, int *voltage_vs_soc, size_t num_points)
{
    bms->OCV = voltage_vs_soc;
    bms->num_ocv_points = num_points;
}

//----------------------------------------------------------------------------

float bms_get_soc(BMS *bms)
{
    return (double) bms->coulomb_counter / bms->nominal_capacity * 100;
}

//----------------------------------------------------------------------------

void bms_reset_soc(BMS *bms, int percent)
{
    if (percent <= 100 && percent >= 0)
    {
        bms->coulomb_counter = bms->nominal_capacity * (percent / 100.0);
    }
    else  // reset based on OCV
    {
        printf("NumCells: %d, voltage: %d V\n", bms_get_connected_cells(bms), bms_pack_voltage(bms));
        int voltage = bms_pack_voltage(bms) / bms_get_connected_cells(bms);

        bms->coulomb_counter = 0;  // initialize with totally depleted battery (0% SOC)

        for (unsigned int i = 0; i < bms->num_ocv_points; i++)
        {
            if (bms->OCV[i] <= voltage) {
                if (i == 0) {
                    bms->coulomb_counter = bms->nominal_capacity;  // 100% full
                }
                else {
                    // interpolate between OCV[i] and OCV[i-1]
                    bms->coulomb_counter = (double) bms->nominal_capacity / (bms->num_ocv_points - 1.0) *
                    (bms->num_ocv_points - 1.0 - i + ((float)voltage - bms->OCV[i])/(bms->OCV[i-1] - bms->OCV[i]));
                }
                return;
            }
        }
    }
}


//----------------------------------------------------------------------------

void bms_set_idle_current_threshold(BMS *bms, int current_mA)
{
    bms->idle_current_threshold = current_mA;
}

//----------------------------------------------------------------------------

int bms_pack_current(BMS *bms)
{
    return bms->battery_current;
}

//----------------------------------------------------------------------------

int bms_pack_voltage(BMS *bms)
{
    return bms->battery_voltage;
}

//----------------------------------------------------------------------------

int bms_cell_voltage_max(BMS *bms)
{
    return bms->cell_voltages[bms->id_cell_voltage_max];
}

//----------------------------------------------------------------------------

int bms_cell_voltage_min(BMS *bms)
{
    return bms->cell_voltages[bms->id_cell_voltage_min];
}

//----------------------------------------------------------------------------

int bms_cell_voltage(BMS *bms, int idCell)
{
    return bms->cell_voltages[idCell-1];
}

//----------------------------------------------------------------------------

int bms_get_connected_cells(BMS *bms)
{
    return bms->connected_cells;
}

//----------------------------------------------------------------------------

float bms_get_temp_degC(BMS *bms, int channel)
{
    if (channel >= 1 && channel <= 3) {
        return (float)bms->temperatures[channel-1] / 10.0;
    }
    else {
        return -273.15;   // Error: Return absolute minimum temperature
    }
}

//----------------------------------------------------------------------------

float bms_get_temp_degF(BMS *bms, int channel)
{
    return bms_get_temp_degC(bms, channel) * 1.8 + 32;
}

