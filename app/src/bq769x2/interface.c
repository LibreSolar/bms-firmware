/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "board.h"
#include "helper.h"

#include "interface.h"
#include "registers.h"

#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <string.h>

LOG_MODULE_REGISTER(bq769x2_if, CONFIG_LOG_DEFAULT_LEVEL);

#define BQ769X2_INST DT_INST(0, ti_bq769x2_i2c)

#define I2C_DEV DT_LABEL(DT_PARENT(BQ769X2_INST))
#define I2C_ADDRESS DT_REG_ADDR(BQ769X2_INST)

#define BQ_ALERT_PORT DT_GPIO_LABEL(BQ769X2_INST, alert_gpios)
#define BQ_ALERT_PIN  DT_GPIO_PIN(BQ769X2_INST, alert_gpios)

#ifndef UNIT_TEST

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

    int err = bq769x2_read_bytes(reg_addr, buf, 2);
    if (err) {
        LOG_ERR("direct_read_u2 failed");
    }
    else {
        *value = buf[0] | buf[1] << 8;      // little-endian byte order
    }

    return err;
}

int bq769x2_direct_read_i2(const uint8_t reg_addr, int16_t *value)
{
    uint16_t u16;

    int err = bq769x2_direct_read_u2(reg_addr, &u16);
    if (!err) {
        *value = *(int16_t *)&u16;
    }

    return err;
}

static int bq769x2_subcmd_read(const uint16_t subcmd, uint32_t *value, const size_t num_bytes)
{
    static uint8_t buf_data[0x20];
    int err;

    // write the subcommand we want to read data from
    uint8_t buf_subcmd[2] = { subcmd & 0x00FF, subcmd >> 8 };
    err = bq769x2_write_bytes(BQ769X2_CMD_SUBCMD_LOWER, buf_subcmd, 2);
    if (err) {
        goto err;
    }

    k_usleep(500);  // most subcommands need approx 500 us to complete

    // wait until data is ready
    int num_tries = 0;
    do {
        err = bq769x2_read_bytes(BQ769X2_CMD_SUBCMD_LOWER, buf_data, 2);
        if (err) {
            goto err;
        }
        else if (num_tries > 10) {
            LOG_ERR("Reached maximum number of tries to read subcommand");
            return -EIO;
        }
        num_tries++;
    } while (buf_subcmd[0] != buf_data[0] || buf_subcmd[1] != buf_data[1]);

    // read data length
    uint8_t data_length;
    err = bq769x2_read_bytes(BQ769X2_SUBCMD_DATA_LENGTH, &data_length, 1);
    if (err) {
        goto err;
    }
    data_length = data_length - 4;  // substract subcmd + checksum + length bytes
    if (err || data_length > 0x20 || num_bytes > 4) {
        LOG_ERR("Subcmd data length 0x%X or num_bytes invalid", data_length);
        return -EIO;
    }

    // read all available data (needed for checksum calculation, may be more than num_bytes)
    *value = 0;
    uint8_t checksum = buf_subcmd[0] + buf_subcmd[1];
    err = bq769x2_read_bytes(BQ769X2_SUBCMD_DATA_START, buf_data, data_length);
    if (err) {
        goto err;
    }
    for (int i = 0; i < data_length; i++) {
        if (i < num_bytes) {
            *value += buf_data[i] << (i * 8);
        }
        checksum += buf_data[i];
    }
    checksum = ~checksum;

    // validate data with checksum
    err = bq769x2_read_bytes(BQ769X2_SUBCMD_DATA_CHECKSUM, buf_data, 1);
    if (err) {
        goto err;
    }
    else if (buf_data[0] != checksum) {
        LOG_ERR("Subcmd checksum incorrect: calculated 0x%X, read 0x%X", checksum, buf_data[0]);
        return -EIO;
    }

    return 0;

err:
    LOG_ERR("I2C error for subcmd_read: %d", err);
    return err;
}

static int bq769x2_subcmd_write(const uint16_t subcmd, const uint32_t value, const size_t num_bytes)
{
    uint8_t buf_data[4];
    int err;

    if (num_bytes > 4) {
        LOG_ERR("Subcmd num_bytes 0x%X invalid", num_bytes);
        return -ENOTSUP;
    }

    // write the subcommand we want to write data to
    uint8_t buf_subcmd[2] = { subcmd & 0x00FF, subcmd >> 8 };
    err = bq769x2_write_bytes(BQ769X2_CMD_SUBCMD_LOWER, buf_subcmd, 2);
    if (err) {
        goto err;
    }

    // write actual data and calculate checksum
    uint8_t checksum = buf_subcmd[0] + buf_subcmd[1];
    for (int i = 0; i < num_bytes; i++) {
        buf_data[i] = (value >> (i * 8)) & 0x000000FF;
        checksum += buf_data[i];
    }
    checksum = ~checksum;
    err = bq769x2_write_bytes(BQ769X2_SUBCMD_DATA_START, buf_data, num_bytes);
    if (err) {
        goto err;
    }

    // write checksum and data length as one word
    buf_data[0] = checksum;
    buf_data[1] = num_bytes + 4;
    err = bq769x2_write_bytes(BQ769X2_SUBCMD_DATA_CHECKSUM, buf_data, 2);
    if (err) {
        goto err;
    }

    return 0;

err:
    LOG_ERR("I2C error for subcmd_read: %d", err);
    return err;
}

int bq769x2_subcmd_read_u1(const uint16_t subcmd, uint8_t *value)
{
    uint32_t u32;
    int err = bq769x2_subcmd_read(subcmd, &u32, 1);
    if (!err) {
        *value = (uint8_t)u32;
    }
    return err;
}

int bq769x2_subcmd_read_u2(const uint16_t subcmd, uint16_t *value)
{
    uint32_t u32;
    int err = bq769x2_subcmd_read(subcmd, &u32, 2);
    if (!err) {
        *value = (uint16_t)u32;
    }
    return err;
}

int bq769x2_subcmd_read_u4(const uint16_t subcmd, uint32_t *value)
{
    uint32_t u32;
    int err = bq769x2_subcmd_read(subcmd, &u32, 4);
    if (!err) {
        *value = u32;
    }
    return err;
}

int bq769x2_subcmd_read_i1(const uint16_t subcmd, int8_t *value)
{
    uint32_t u32;
    int err = bq769x2_subcmd_read(subcmd, &u32, 1);
    if (!err) {
        *value = *(int8_t *)&u32;
    }
    return err;
}

int bq769x2_subcmd_read_i2(const uint16_t subcmd, int16_t *value)
{
    uint32_t u32;
    int err = bq769x2_subcmd_read(subcmd, &u32, 2);
    if (!err) {
        uint16_t u16 = (uint16_t)u32; // doing narrowing conversion first avoids compiler warnings
        *value = *(int16_t *)&u16;
    }
    return err;
}

int bq769x2_subcmd_read_i4(const uint16_t subcmd, int32_t *value)
{
    int32_t i32;
    int err = bq769x2_subcmd_read(subcmd, (uint32_t *)&i32, 4);
    if (!err) {
        *value = i32;
    }
    return err;
}

int bq769x2_subcmd_read_f4(const uint16_t subcmd, float *value)
{
    float f32;
    int err = bq769x2_subcmd_read(subcmd, (uint32_t *)&f32, 4);
    if (!err) {
        *value = f32;
    }
    return err;
}

int bq769x2_subcmd_write_u1(const uint16_t subcmd, uint8_t value)
{
    return bq769x2_subcmd_write(subcmd, value, 1);
}

int bq769x2_subcmd_write_u2(const uint16_t subcmd, uint16_t value)
{
    return bq769x2_subcmd_write(subcmd, value, 2);
}

int bq769x2_subcmd_write_u4(const uint16_t subcmd, uint32_t value)
{
    return bq769x2_subcmd_write(subcmd, value, 4);
}

int bq769x2_subcmd_write_i1(const uint16_t subcmd, int8_t value)
{
    return bq769x2_subcmd_write(subcmd, value, 1);
}

int bq769x2_subcmd_write_i2(const uint16_t subcmd, int16_t value)
{
    return bq769x2_subcmd_write(subcmd, value, 2);
}

int bq769x2_subcmd_write_i4(const uint16_t subcmd, int32_t value)
{
    return bq769x2_subcmd_write(subcmd, value, 4);
}

int bq769x2_subcmd_write_f4(const uint16_t subcmd, float value)
{
    uint32_t *u32 = (uint32_t *)&value;
    return bq769x2_subcmd_write(subcmd, *u32, 4);
}
