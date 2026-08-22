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
