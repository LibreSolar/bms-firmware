/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bms.h"
#include "board.h"
#include "helper.h"

void bms_soc_reset(Bms *bms, int percent)
{
    if (percent <= 100 && percent >= 0) {
        bms->status.soc = percent;
    }
    else if (bms->conf.ocv != NULL) {
        bms->status.soc =
            interpolate(bms->conf.ocv, soc_pct, ARRAY_SIZE(soc_pct), bms->status.cell_voltage_avg);
    }
    else {
        // no OCV curve specified, use simplified estimation instead
        float ocv_simple[2] = { bms->conf.cell_chg_voltage, bms->conf.cell_dis_voltage };
        float soc_simple[2] = { 100.0F, 0.0F };
        bms->status.soc = interpolate(ocv_simple, soc_simple, 2, bms->status.cell_voltage_avg);
    }
}

void bms_soc_update(Bms *bms)
{
    static float coulomb_counter_mAs = 0;
    static int64_t last_update = 0;
    int64_t now = k_uptime_get();

    coulomb_counter_mAs += bms->status.pack_current * (now - last_update);
    float soc_delta = coulomb_counter_mAs / (bms->conf.nominal_capacity_Ah * 3.6e4F);

    if (soc_delta > 0.1F || soc_delta < -0.1F) {
        // only update SoC after significant changes to maintain higher resolution
        float soc_tmp = bms->status.soc + soc_delta;
        bms->status.soc = CLAMP(soc_tmp, 0.0F, 100.0F);
        coulomb_counter_mAs = 0;
    }

    last_update = now;
}
