/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pcb.h"

#if CONFIG_BMS_BQ769X2

#include "bms.h"
#include "registers.h"
#include "interface.h"
#include "helper.h"

#include <math.h>       // log for thermistor calculation
#include <stdlib.h>     // for abs() function
#include <stdio.h>
#include <time.h>
#include <string.h>

void bms_init_hardware()
{
    bq769x2_init();
}

void bms_update(BmsConfig *conf, BmsStatus *status)
{
    bms_read_voltages(status);
    bms_read_current(conf, status);
    bms_soc_update(conf, status);
    bms_read_temperatures(conf, status);
    bms_update_error_flags(conf, status);
    bms_apply_balancing(conf, status);
}

void bms_set_error_flag(BmsStatus *status, uint32_t flag, bool value)
{
    // TODO
}

bool bms_startup_inhibit()
{
    // Datasheet: Start-up time max. 4.3 ms
    return k_uptime_get() <= 5;
}

void bms_shutdown()
{
    // TODO
}

bool bms_chg_switch(BmsConfig *conf, BmsStatus *status, bool enable)
{
    // TODO

    return 0;
}

bool bms_dis_switch(BmsConfig *conf, BmsStatus *status, bool enable)
{
    // TODO

    return 0;
}

void bms_apply_balancing(BmsConfig *conf, BmsStatus *status)
{
    // TODO
}

float bms_apply_dis_scp(BmsConfig *conf)
{
    // TODO

    return 0;
}

float bms_apply_chg_ocp(BmsConfig *conf)
{
    // TODO

    return 0;
}

float bms_apply_dis_ocp(BmsConfig *conf)
{
    // TODO

    return 0;
}

int bms_apply_cell_uvp(BmsConfig *conf)
{
    // TODO

    return 0;
}

int bms_apply_cell_ovp(BmsConfig *conf)
{
    // TODO

    return 0;
}

int bms_apply_temp_limits(BmsConfig *bms)
{
    // TODO

    return 0;
}

void bms_read_temperatures(BmsConfig *conf, BmsStatus *status)
{
    // TODO
}

void bms_read_current(BmsConfig *conf, BmsStatus *status)
{
    // TODO
}

void bms_read_voltages(BmsStatus *status)
{
    // TODO
}

void bms_update_error_flags(BmsConfig *conf, BmsStatus *status)
{
    // TODO
}

void bms_handle_errors(BmsConfig *conf, BmsStatus *status)
{
    // TODO
}

void bms_print_register(uint8_t addr)
{
    uint8_t reg;
    bq769x2_read_bytes(addr, &reg, 1);
    printf("0x%.2X: 0x%.2X = %s\n", addr, reg, byte2bitstr(reg));
}

void bms_print_registers()
{
    // TODO
}

#endif // defined BQ769X2
