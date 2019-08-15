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

#include "pcb.h"

#ifdef BMS_ISL94202

#include "bms.h"
#include "isl94202_registers.h"
#include "isl94202_hw.h"

#include "config.h"

#include <stdlib.h>     // for abs() function
#include <stdio.h>
#include <time.h>
#include <string.h>

static void set_num_cells(int num)
{
    uint8_t cell_reg = 0;
    switch(num) {
        case 3:
            cell_reg = 0b10000011;
            break;
        case 4:
            cell_reg = 0b11000011;
            break;
        case 5:
            cell_reg = 0b11000111;
            break;
        case 6:
            cell_reg = 0b11100111;
            break;
        case 7:
            cell_reg = 0b11101111;
            break;
        case 8:
            cell_reg = 0b11111111;
            break;
        default:
            break;
    }
    isl94202_write_bytes(ISL94202_MOD_CELL + 1, &cell_reg, 1);
    // ToDo: Double write for certain bytes... move this to _hw.c file
    //k_sleep(30);
    //isl94202_write_bytes(ISL94202_MOD_CELL + 1, &cell_reg, 1);
    //k_sleep(30);
}

void bms_init()
{
    isl94202_init();
    set_num_cells(4);
}

int bms_quickcheck(BmsConfig *conf, BmsStatus *status)
{
    /* ToDo */
    return 0;
}

void bms_update(BmsConfig *conf, BmsStatus *status)
{
    bms_read_voltages(status);
    bms_read_current(conf, status);
    bms_read_temperatures(conf, status);
    bms_apply_balancing(conf, status);
    bms_update_soc(conf, status);
}

void bms_update_error_flag(BmsConfig *conf, BmsStatus *status, uint32_t flag, bool value)
{
    /* ToDo */
}

void bms_shutdown()
{
    /* ToDo */
}

bool bms_chg_switch(BmsConfig *conf, BmsStatus *status, bool enable)
{
    /* ToDo */
    return 0;
}

bool bms_dis_switch(BmsConfig *conf, BmsStatus *status, bool enable)
{
    /* ToDo */
    return 0;
}

void bms_apply_balancing(BmsConfig *conf, BmsStatus *status)
{
    /* ToDo */
}

int bms_apply_dis_scp(BmsConfig *conf)
{
    return isl94202_apply_current_limit(ISL94202_OCCT_OCC,
        DSC_Thresholds, sizeof(DSC_Thresholds),
        conf->dis_sc_limit, conf->shunt_res_mOhm,
        ISL94202_DELAY_US, conf->dis_sc_delay_us);
    return 0;
}

int bms_apply_chg_ocp(BmsConfig *conf)
{
    return isl94202_apply_current_limit(ISL94202_OCCT_OCC,
        OCC_Thresholds, sizeof(OCC_Thresholds),
        conf->chg_oc_limit, conf->shunt_res_mOhm,
        ISL94202_DELAY_MS, conf->chg_oc_delay_ms);
    return 0;
}

int bms_apply_dis_ocp(BmsConfig *conf)
{
    return isl94202_apply_current_limit(ISL94202_OCDT_OCD,
        OCD_Thresholds, sizeof(OCD_Thresholds),
        conf->dis_oc_limit, conf->shunt_res_mOhm,
        ISL94202_DELAY_MS, conf->dis_oc_delay_ms);
}

int bms_apply_cell_uvp(BmsConfig *conf)
{
    /* ToDo */
    return 0;
}

int bms_apply_cell_ovp(BmsConfig *conf)
{
    /* ToDo */
    return 0;
}

int bms_apply_temp_limits(BmsConfig *bms)
{
    /* ToDo */
    return 0;
}

void bms_read_temperatures(BmsConfig *conf, BmsStatus *status)
{
    /* ToDo */
}

void bms_read_current(BmsConfig *conf, BmsStatus *status)
{
    /* ToDo */
    uint8_t buf[2];
    isl94202_read_bytes(ISL94202_ISNS, buf, 2);
    uint16_t tmp = (buf[0] + (buf[1] << 8)) & 0x0FFF;
    //status->cell_voltages[i] = tmp * 1800 / 4095 / 3;
}

void bms_read_voltages(BmsStatus *status)
{
    uint8_t buf[2];
    for (int i = 0; i < NUM_CELLS_MAX; i++) {
        isl94202_read_bytes(ISL94202_CELL1 + i*2, buf, 2);
        uint32_t tmp = (buf[0] + (buf[1] << 8)) & 0x0FFF;
        status->cell_voltages[i] = (float)tmp * 18 * 800 / 4095 / 3 / 1000;
    }
}

#if BMS_DEBUG

static const char *byte2bitstr(uint8_t b)
{
    static char str[9];
    str[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(str, ((b & z) == z) ? "1" : "0");
    }

    return str;
}

void bms_print_registers()
{
    printf("EEPROM content: ------------------\n");
    for (int i = 0; i < 0x4C; i += 2) {
        uint8_t buf[2];
        isl94202_read_bytes(i, buf, sizeof(buf));
        printf("0x%.2X: 0x%.2X%.2X = ", i, buf[1], buf[0]);
        printf("%s ", byte2bitstr(buf[1]));
        printf("%s\n", byte2bitstr(buf[0]));

    }
    printf("RAM content: ------------------\n");
    for (int i = 0x80; i <= 0xAA; i += 2) {
        uint8_t buf[2];
        isl94202_read_bytes(i, buf, sizeof(buf));
        printf("0x%.2X: 0x%.2X%.2X = ", i, buf[1], buf[0]);
        printf("%s ", byte2bitstr(buf[1]));
        printf("%s\n", byte2bitstr(buf[0]));

    }
}

#endif

#endif // BMS_ISL94202