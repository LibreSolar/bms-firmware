/* Libre Solar Battery Management System firmware
 * Copyright (c) 2016-2019 Martin Jäger (www.libre.solar)
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

#include <zephyr.h>
#include <stdio.h>

#include "bms.h"
#include "uext.h"
#include "leds.h"
#include "thingset.h"
#include "thingset_serial.h"

BmsConfig bms_conf;
BmsStatus bms_status;
extern ThingSet ts;

void main(void)
{
    printf("Booting Libre Solar BMS: %s\n", CONFIG_BOARD);

    bms_init();
    bms_init_config(&bms_conf, CELL_TYPE_LFP, 45);

    uext_init();

    while (1) {

        bms_update(&bms_conf, &bms_status);
        bms_state_machine(&bms_conf, &bms_status);

        uext_process_1s();

        //bms_print_registers();

        k_sleep(1000);
    }
}

K_THREAD_DEFINE(ts_serial_id, 4096, thingset_serial_thread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);

K_THREAD_DEFINE(leds_id, 256, leds_update_thread, NULL, NULL, NULL,	4, 0, K_NO_WAIT);
