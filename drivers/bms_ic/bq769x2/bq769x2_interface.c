/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bq769x2_interface.h"
#include "bq769x2_priv.h"
#include "bq769x2_registers.h"

#include <string.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bq769x2_if, CONFIG_BMS_IC_LOG_LEVEL);

/*
 * Specification of max. attempts and delays for reading data from the device. Worst case command
 * IROM_SIG() takes approx 8500 us to complete according to reference manual Table 9-2.
 */
#define BQ769X2_READ_MAX_ATTEMPTS (100)
#define BQ769X2_READ_DELAY_US     (100)

int bq769x2_direct_read_u1(const struct device *dev, const uint8_t reg_addr, uint8_t *value)
{
    const struct bms_ic_bq769x2_config *config = dev->config;

    int err = config->read_bytes(dev, reg_addr, value, 1);
    if (err) {
        LOG_ERR("direct_read_u1 failed");
    }

    return err;
}

int bq769x2_direct_read_u2(const struct device *dev, const uint8_t reg_addr, uint16_t *value)
{
    const struct bms_ic_bq769x2_config *config = dev->config;
    uint8_t buf[2];

    int err = config->read_bytes(dev, reg_addr, buf, 2);
    if (err) {
        LOG_ERR("direct_read_u2 failed");
    }
    else {
        *value = buf[0] | buf[1] << 8; /* little-endian byte order */
    }

    return err;
}

int bq769x2_direct_read_i2(const struct device *dev, const uint8_t reg_addr, int16_t *value)
{
    uint16_t u16;

    int err = bq769x2_direct_read_u2(dev, reg_addr, &u16);
    if (!err) {
        *value = *(int16_t *)&u16;
    }

    return err;
}

static int bq769x2_subcmd_read(const struct device *dev, const uint16_t subcmd, uint8_t *bytes,
                               const size_t num_bytes)
{
    const struct bms_ic_bq769x2_config *config = dev->config;
    static uint8_t buf_data[0x20];
    int err;

    /* write the subcommand we want to read data from */
    uint8_t buf_subcmd[2] = { subcmd & 0x00FF, subcmd >> 8 };
    err = config->write_bytes(dev, BQ769X2_CMD_SUBCMD_LOWER, buf_subcmd, 2);
    if (err) {
        goto err;
    }

    /* wait until data is ready (commands need at least 50 us to complete) */
    k_usleep(50);
    int attempts = 1;
    while (true) {
        err = config->read_bytes(dev, BQ769X2_CMD_SUBCMD_LOWER, buf_data, 2);
        if (err) {
            goto err;
        }
        else if (buf_subcmd[0] == buf_data[0] && buf_subcmd[1] == buf_data[1]) {
            /* processing inside chip finished */
            break;
        }
        else if (attempts >= BQ769X2_READ_MAX_ATTEMPTS) {
            LOG_ERR("Failed to read subcmd 0x%04X after %d attempts", subcmd,
                    BQ769X2_READ_MAX_ATTEMPTS);
            return -EIO;
        }
        else {
            /* try again */
            attempts++;
            k_usleep(BQ769X2_READ_DELAY_US);
        }
    }

    /* read data length */
    uint8_t data_length;
    err = config->read_bytes(dev, BQ769X2_SUBCMD_DATA_LENGTH, &data_length, 1);
    if (err) {
        goto err;
    }
    data_length = data_length - BQ769X2_SUBCMD_OVERHEAD_BYTES;
    if (err || data_length > 0x20) {
        LOG_ERR("Subcmd data length 0x%X invalid", data_length);
        return -EIO;
    }

    /* read all available data (needed for checksum calculation, may be more than num_bytes) */
    uint8_t checksum = buf_subcmd[0] + buf_subcmd[1];
    err = config->read_bytes(dev, BQ769X2_SUBCMD_DATA_START, buf_data, data_length);
    if (err) {
        goto err;
    }
    for (int i = 0; i < data_length; i++) {
        if (i < num_bytes) {
            bytes[i] = buf_data[i];
        }
        checksum += buf_data[i];
    }
    checksum = ~checksum;

    /* validate data with checksum */
    err = config->read_bytes(dev, BQ769X2_SUBCMD_DATA_CHECKSUM, buf_data, 1);
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

static int bq769x2_subcmd_write(const struct device *dev, const uint16_t subcmd,
                                const uint32_t value, const size_t num_bytes)
{
    const struct bms_ic_bq769x2_config *config = dev->config;
    uint8_t buf_data[4];
    int err;

    __ASSERT(num_bytes <= 4, "Subcmd num_bytes 0x%X invalid", num_bytes);

    /* write the subcommand we want to write data to */
    uint8_t buf_subcmd[2] = { subcmd & 0x00FF, subcmd >> 8 };
    err = config->write_bytes(dev, BQ769X2_CMD_SUBCMD_LOWER, buf_subcmd, 2);
    if (err) {
        goto err;
    }

    if (num_bytes > 0) {
        /* write actual data and calculate checksum */
        uint8_t checksum = buf_subcmd[0] + buf_subcmd[1];
        for (int i = 0; i < num_bytes; i++) {
            buf_data[i] = (value >> (i * 8)) & 0x000000FF;
            checksum += buf_data[i];
        }
        checksum = ~checksum;
        err = config->write_bytes(dev, BQ769X2_SUBCMD_DATA_START, buf_data, num_bytes);
        if (err) {
            goto err;
        }

        /* write checksum and data length as one word */
        buf_data[0] = checksum;
        buf_data[1] = num_bytes + BQ769X2_SUBCMD_OVERHEAD_BYTES;
        err = config->write_bytes(dev, BQ769X2_SUBCMD_DATA_CHECKSUM, buf_data, 2);
        if (err) {
            goto err;
        }
    }

    /*
     * Add a small delay to avoid failures of subsequent read operations. Suitable value was
     * found through testing (datasheet does not mention any required delays after writing).
     */
    k_usleep(200);

    return 0;

err:
    LOG_ERR("I2C error for subcmd_write: %d", err);
    return err;
}

int bq769x2_subcmd_cmd_only(const struct device *dev, const uint16_t subcmd)
{
    __ASSERT(!BQ769X2_IS_DATA_MEM_REG_ADDR(subcmd), "invalid subcmd: 0x%x", subcmd);

    return bq769x2_subcmd_write(dev, subcmd, 0, 0);
}

int bq769x2_subcmd_read_u1(const struct device *dev, const uint16_t subcmd, uint8_t *value)
{
    __ASSERT(!BQ769X2_IS_DATA_MEM_REG_ADDR(subcmd), "invalid subcmd: 0x%x", subcmd);

    uint8_t buf[1];

    int err = bq769x2_subcmd_read(dev, subcmd, buf, sizeof(buf));
    if (!err) {
        *value = buf[0];
    }

    return err;
}

int bq769x2_subcmd_read_u2(const struct device *dev, const uint16_t subcmd, uint16_t *value)
{
    __ASSERT(!BQ769X2_IS_DATA_MEM_REG_ADDR(subcmd), "invalid subcmd: 0x%x", subcmd);

    uint8_t buf[2];

    int err = bq769x2_subcmd_read(dev, subcmd, buf, sizeof(buf));
    if (!err) {
        *value = buf[0] | buf[1] << 8;
    }

    return err;
}

int bq769x2_subcmd_read_u4(const struct device *dev, const uint16_t subcmd, uint32_t *value)
{
    __ASSERT(!BQ769X2_IS_DATA_MEM_REG_ADDR(subcmd), "invalid subcmd: 0x%x", subcmd);

    uint8_t buf[4];

    int err = bq769x2_subcmd_read(dev, subcmd, buf, sizeof(buf));
    if (!err) {
        *value = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
    }

    return err;
}

int bq769x2_subcmd_write_u1(const struct device *dev, const uint16_t subcmd, uint8_t value)
{
    __ASSERT(!BQ769X2_IS_DATA_MEM_REG_ADDR(subcmd), "invalid subcmd: 0x%x", subcmd);

    return bq769x2_subcmd_write(dev, subcmd, value, 1);
}

int bq769x2_subcmd_write_u2(const struct device *dev, const uint16_t subcmd, uint16_t value)
{
    __ASSERT(!BQ769X2_IS_DATA_MEM_REG_ADDR(subcmd), "invalid subcmd: 0x%x", subcmd);

    return bq769x2_subcmd_write(dev, subcmd, value, 2);
}

int bq769x2_subcmd_write_i2(const struct device *dev, const uint16_t subcmd, int16_t value)
{
    __ASSERT(!BQ769X2_IS_DATA_MEM_REG_ADDR(subcmd), "invalid subcmd: 0x%x", subcmd);

    return bq769x2_subcmd_write(dev, subcmd, value, 2);
}

int bq769x2_config_update_mode(const struct device *dev, bool config_update)
{
    struct bms_ic_bq769x2_data *data = dev->data;
    int err;

    if (config_update) {
        err = bq769x2_subcmd_cmd_only(dev, BQ769X2_SUBCMD_SET_CFGUPDATE);
        k_usleep(2000);
    }
    else {
        err = bq769x2_subcmd_cmd_only(dev, BQ769X2_SUBCMD_EXIT_CFGUPDATE);
        k_usleep(1000);
    }

    if (!err) {
        data->config_update_mode_enabled = config_update;
    }

    return err;
}

int bq769x2_datamem_read_u1(const struct device *dev, const uint16_t reg_addr, uint8_t *value)
{
    __ASSERT(BQ769X2_IS_DATA_MEM_REG_ADDR(reg_addr), "invalid data memory register");

    uint8_t buf[1];

    int err = bq769x2_subcmd_read(dev, reg_addr, buf, sizeof(buf));
    if (!err) {
        *value = buf[0];
    }

    return err;
}

int bq769x2_datamem_read_u2(const struct device *dev, const uint16_t reg_addr, uint16_t *value)
{
    __ASSERT(BQ769X2_IS_DATA_MEM_REG_ADDR(reg_addr), "invalid data memory register");

    uint8_t buf[2];

    int err = bq769x2_subcmd_read(dev, reg_addr, buf, sizeof(buf));
    if (!err) {
        *value = buf[0] | buf[1] << 8;
    }

    return err;
}

int bq769x2_datamem_read_f4(const struct device *dev, const uint16_t reg_addr, float *value)
{
    __ASSERT(BQ769X2_IS_DATA_MEM_REG_ADDR(reg_addr), "invalid data memory register");

    uint8_t buf[4];

    int err = bq769x2_subcmd_read(dev, reg_addr, buf, sizeof(buf));
    if (!err) {
        *(uint32_t *)value = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
    }

    return err;
}

int bq769x2_datamem_write_u1(const struct device *dev, const uint16_t reg_addr, uint8_t value)
{
    __maybe_unused const struct bms_ic_bq769x2_data *data = dev->data;

    __ASSERT(data->config_update_mode_enabled, "bq769x2 config update mode not enabled");
    __ASSERT(BQ769X2_IS_DATA_MEM_REG_ADDR(reg_addr), "invalid data memory register");

    return bq769x2_subcmd_write(dev, reg_addr, value, 1);
}

int bq769x2_datamem_write_u2(const struct device *dev, const uint16_t reg_addr, uint16_t value)
{
    __maybe_unused const struct bms_ic_bq769x2_data *data = dev->data;

    __ASSERT(data->config_update_mode_enabled, "bq769x2 config update mode not enabled");
    __ASSERT(BQ769X2_IS_DATA_MEM_REG_ADDR(reg_addr), "invalid data memory register");

    return bq769x2_subcmd_write(dev, reg_addr, value, 2);
}

int bq769x2_datamem_write_i1(const struct device *dev, const uint16_t reg_addr, int8_t value)
{
    __maybe_unused const struct bms_ic_bq769x2_data *data = dev->data;

    __ASSERT(data->config_update_mode_enabled, "bq769x2 config update mode not enabled");
    __ASSERT(BQ769X2_IS_DATA_MEM_REG_ADDR(reg_addr), "invalid data memory register");

    return bq769x2_subcmd_write(dev, reg_addr, value, 1);
}

int bq769x2_datamem_write_i2(const struct device *dev, const uint16_t reg_addr, int16_t value)
{
    __maybe_unused const struct bms_ic_bq769x2_data *data = dev->data;

    __ASSERT(data->config_update_mode_enabled, "bq769x2 config update mode not enabled");
    __ASSERT(BQ769X2_IS_DATA_MEM_REG_ADDR(reg_addr), "invalid data memory register");

    return bq769x2_subcmd_write(dev, reg_addr, value, 2);
}

int bq769x2_datamem_write_f4(const struct device *dev, const uint16_t reg_addr, float value)
{
    __maybe_unused const struct bms_ic_bq769x2_data *data = dev->data;

    __ASSERT(data->config_update_mode_enabled, "bq769x2 config update mode not enabled");
    __ASSERT(BQ769X2_IS_DATA_MEM_REG_ADDR(reg_addr), "invalid data memory register");

    uint32_t *u32 = (uint32_t *)&value;

    return bq769x2_subcmd_write(dev, reg_addr, *u32, 4);
}
