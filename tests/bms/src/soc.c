/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <bms/bms.h>

static struct bms_context soc_test_context(float current)
{
    return (struct bms_context) {
        .soc = 50.0F,
        .nominal_capacity_Ah = 0.001F,
        .ic_data = {
            .current = current,
        },
    };
}

ZTEST(soc, test_first_sample_initializes_time_baseline)
{
    struct bms_context bms = soc_test_context(1.0F);

    bms_soc_reset(&bms, 50);
    k_sleep(K_MSEC(10));
    bms_soc_update(&bms);

    zassert_equal(bms.soc, 50.0F);
    zassert_true(bms.soc_last_update_ms >= 0);
}

ZTEST(soc, test_reset_discards_previous_integration_interval)
{
    struct bms_context bms = soc_test_context(1.0F);

    bms_soc_reset(&bms, 50);
    bms_soc_update(&bms);
    k_sleep(K_MSEC(10));
    bms_soc_update(&bms);
    zassert_true(bms.soc > 50.1F);

    bms_soc_reset(&bms, 50);
    k_sleep(K_MSEC(10));
    bms_soc_update(&bms);

    zassert_equal(bms.soc, 50.0F);
    zassert_true(bms.soc_last_update_ms >= 0);
}

ZTEST(soc, test_contexts_integrate_independently)
{
    struct bms_context charging = soc_test_context(1.0F);
    struct bms_context discharging = soc_test_context(-1.0F);

    bms_soc_reset(&charging, 50);
    bms_soc_reset(&discharging, 50);
    bms_soc_update(&charging);
    bms_soc_update(&discharging);

    k_sleep(K_MSEC(10));
    bms_soc_update(&charging);
    bms_soc_update(&discharging);

    zassert_true(charging.soc > 50.1F);
    zassert_true(discharging.soc < 49.9F);
}

ZTEST_SUITE(soc, NULL, NULL, NULL, NULL, NULL);

ZTEST(soc, test_custom_curve_with_zero_soc_prefix)
{
    float voltage[NUM_OCV_POINTS];
    float charge[NUM_OCV_POINTS];
    struct bms_context bms = { 0 };
    for (int i = 0; i < NUM_OCV_POINTS; i++) {
        voltage[i] = 2.5F + 0.05F * i;
        charge[i] = i <= 5 ? 0.0F : (i - 5) * 100.0F / 15.0F;
    }
    bms.ocv_points = voltage;
    bms.soc_points = charge;
    bms.ic_conf.cell_dis_voltage_limit = 2.5F;
    bms.ic_conf.cell_chg_voltage_limit = 3.5F;
    bms.ic_data.cell_voltage_avg = 3.0F;
    bms_soc_reset(&bms, -1);
    zassert_within(bms.soc, 100.0F / 3.0F, 0.001F);
}

ZTEST(soc, test_empty_curve_uses_voltage_limits)
{
    float empty[NUM_OCV_POINTS] = { 0 };
    struct bms_context bms = { 0 };
    bms.ocv_points = empty;
    bms.soc_points = empty;
    bms.ic_conf.cell_dis_voltage_limit = 2.5F;
    bms.ic_conf.cell_chg_voltage_limit = 3.5F;
    bms.ic_data.cell_voltage_avg = 3.0F;
    bms_soc_reset(&bms, -1);
    zassert_within(bms.soc, 50.0F, 0.001F);
    bms.ocv_points = NULL;
    bms_soc_reset(&bms, -1);
    zassert_within(bms.soc, 50.0F, 0.001F);
}

ZTEST(soc, test_nonzero_curve_tail_is_not_empty)
{
    float voltage[NUM_OCV_POINTS];
    float charge[NUM_OCV_POINTS] = { 0 };
    struct bms_context bms = { 0 };
    for (int i = 0; i < NUM_OCV_POINTS; i++) {
        voltage[i] = 2.5F + 0.05F * i;
    }
    charge[NUM_OCV_POINTS - 1] = 100.0F;
    bms.ocv_points = voltage;
    bms.soc_points = charge;
    bms.ic_conf.cell_dis_voltage_limit = 2.5F;
    bms.ic_conf.cell_chg_voltage_limit = 3.5F;
    bms.ic_data.cell_voltage_avg = 3.0F;
    bms_soc_reset(&bms, -1);
    zassert_within(bms.soc, 0.0F, 0.001F);
}

ZTEST(soc, test_explicit_soc_overrides_curve)
{
    struct bms_context bms = { 0 };
    bms_soc_reset(&bms, 42);
    zassert_equal(bms.soc, 42.0F);
    zassert_equal(bms.soc_last_update_ms, -1);
    zassert_equal(bms.soc_coulomb_counter_mAs, 0.0F);
}
