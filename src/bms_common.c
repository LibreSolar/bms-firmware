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

#include "bms.h"

#include <stdio.h>

void bms_init_config(BmsConfig *conf)
{
    // set some safe default values
    conf->auto_balancing_enabled = false;
    conf->balancing_min_idle_s = 1800;    // default: 30 minutes
    conf->idle_current_threshold = 30; // mA

    conf->thermistor_beta = 3435;  // typical value for Semitec 103AT-5 thermistor
}

void bms_update_soc(BmsConfig *conf, BmsStatus *status)
{
    status->soc = (double) status->coulomb_counter / conf->nominal_capacity * 100;
}

void bms_reset_soc(BmsConfig *conf, BmsStatus *status, int percent)
{
    if (percent <= 100 && percent >= 0)
    {
        status->coulomb_counter = conf->nominal_capacity * (percent / 100.0);
    }
    else  // reset based on OCV
    {
        printf("NumCells: %d, voltage: %lu V\n", status->connected_cells, status->pack_voltage);
        int voltage = status->pack_voltage / status->connected_cells;

        status->coulomb_counter = 0;  // initialize with totally depleted battery (0% SOC)

        for (unsigned int i = 0; i < conf->num_ocv_points; i++)
        {
            if (conf->ocv[i] <= voltage) {
                if (i == 0) {
                    status->coulomb_counter = conf->nominal_capacity;  // 100% full
                }
                else {
                    // interpolate between OCV[i] and OCV[i-1]
                    status->coulomb_counter = (double) conf->nominal_capacity / (conf->num_ocv_points - 1.0) *
                    (conf->num_ocv_points - 1.0 - i + ((float)voltage - conf->ocv[i])/(conf->ocv[i-1] - conf->ocv[i]));
                }
                return;
            }
        }
    }
}
