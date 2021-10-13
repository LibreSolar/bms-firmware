/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "board.h"
#include "helper.h"

#include "interface.h"
#include "registers.h"

#ifndef UNIT_TEST

#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <string.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(bms, CONFIG_LOG_DEFAULT_LEVEL);

#define BQ769X2_INST DT_INST(0, ti_bq769x2_i2c)

#define I2C_DEV DT_LABEL(DT_PARENT(BQ769X2_INST))
#define I2C_ADDRESS DT_REG_ADDR(BQ769X2_INST)

#define BQ_ALERT_PORT DT_GPIO_LABEL(BQ769X2_INST, alert_gpios)
#define BQ_ALERT_PIN  DT_GPIO_PIN(BQ769X2_INST, alert_gpios)

static const struct device *i2c_dev;
static const uint8_t i2c_address = I2C_ADDRESS;

/*
 * Currently only supporting I2C without CRC (default setting for BQ76952 part number)
 *
 * See bq769x0 interface for CRC calculation.
 */

int bq769x2_write_bytes(const uint8_t reg_addr, const uint8_t *data, const size_t num_bytes)
{
    uint8_t buf[5];

    if (num_bytes > 4) {
        return -EINVAL;
    }

    buf[0] = reg_addr;		// first byte contains register address
    memcpy(buf + 1, data, num_bytes);

    return i2c_write(i2c_dev, buf, num_bytes + 1, I2C_ADDRESS);
}

int bq769x2_read_bytes(const uint8_t reg_addr, uint8_t *data, const size_t num_bytes)
{
    return i2c_write_read(i2c_dev, i2c_address, &reg_addr, 1, data, num_bytes);
}

void bq769x2_init()
{
    i2c_dev = device_get_binding(I2C_DEV);
    if (!i2c_dev) {
        LOG_ERR("I2C device not found");
    }
}

#endif // UNIT_TEST

int bq769x2_direct_read_u2(const uint8_t reg_addr, uint16_t *value)
{
    uint8_t buf[2];

    int ret = bq769x2_read_bytes(reg_addr, buf, 2);

    *value = buf[0] | buf[1] << 8;      // little-endian byte order

    return ret;
}

int bq769x2_direct_read_i2(const uint8_t reg_addr, int16_t *value)
{
    uint16_t u16;

    int ret = bq769x2_direct_read_u2(reg_addr, &u16);

    *value = *(int16_t *)&u16;

    return ret;
}
