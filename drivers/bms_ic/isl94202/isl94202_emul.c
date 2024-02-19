/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_isl94202

#include "isl94202_registers.h"

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(isl94202_emul, CONFIG_BMS_IC_LOG_LEVEL);

struct isl94202_emul_data
{
    /* Memory of ILS94202 (registers 0x00 to 0xAB) */
    uint8_t mem[0xAB + 1];
    uint32_t cur_reg;
};

struct isl94202_emul_cfg
{
    uint16_t addr;
};

uint8_t isl94202_emul_get_byte(const struct emul *em, uint8_t addr)
{
    struct isl94202_emul_data *em_data = em->data;

    return em_data->mem[addr];
}

void isl94202_emul_set_byte(const struct emul *em, uint8_t addr, uint8_t byte)
{
    struct isl94202_emul_data *em_data = em->data;

    em_data->mem[addr] = byte;
}

uint16_t isl94202_emul_get_word(const struct emul *em, uint8_t addr)
{
    struct isl94202_emul_data *em_data = em->data;

    return em_data->mem[addr] + (em_data->mem[addr + 1] << 8);
}

void isl94202_emul_set_word(const struct emul *em, uint8_t addr, uint16_t word)
{
    struct isl94202_emul_data *em_data = em->data;

    em_data->mem[addr] = word & 0x00FF;
    em_data->mem[addr + 1] = word >> 8;
}

/* fill RAM and flash with suitable values */
void isl94202_emul_set_mem_defaults(const struct emul *em)
{
    struct isl94202_emul_data *em_data = em->data;

    // Cell voltages
    // voltage = hexvalue * 1.8 * 8 / (4095 * 3)
    // hexvalue = voltage / (1.8 * 8) * 4095 * 3
    *((uint16_t *)&em_data->mem[0x90]) = 3.0 / (1.8 * 8) * 4095 * 3; // Cell 1
    *((uint16_t *)&em_data->mem[0x92]) = 3.1 / (1.8 * 8) * 4095 * 3; // Cell 2
    *((uint16_t *)&em_data->mem[0x94]) = 3.2 / (1.8 * 8) * 4095 * 3; // Cell 3
    *((uint16_t *)&em_data->mem[0x96]) = 3.3 / (1.8 * 8) * 4095 * 3; // Cell 4
    *((uint16_t *)&em_data->mem[0x98]) = 3.4 / (1.8 * 8) * 4095 * 3; // Cell 5
    *((uint16_t *)&em_data->mem[0x9A]) = 3.5 / (1.8 * 8) * 4095 * 3; // Cell 6
    *((uint16_t *)&em_data->mem[0x9C]) = 3.6 / (1.8 * 8) * 4095 * 3; // Cell 7
    *((uint16_t *)&em_data->mem[0x9E]) = 3.7 / (1.8 * 8) * 4095 * 3; // Cell 8

    // Pack voltage
    *((uint16_t *)&em_data->mem[0xA6]) = 3.3 * 8 / (1.8 * 32) * 4095;
}

static int isl94202_emul_write_bytes(const struct emul *em, const uint8_t reg_addr,
                                     const uint8_t *data, const size_t num_bytes)
{
    struct isl94202_emul_data *em_data = em->data;

    if ((reg_addr > 0x58 && reg_addr < 0x7F) || reg_addr + num_bytes > 0xAB || num_bytes > 4) {
        return -EINVAL;
    }

    memcpy(em_data->mem + reg_addr, data, num_bytes);

    return 0;
}

static int isl94202_emul_read_bytes(const struct emul *em, const uint8_t reg_addr, uint8_t *data,
                                    const size_t num_bytes)
{
    struct isl94202_emul_data *em_data = em->data;

    if ((reg_addr > 0x58 && reg_addr < 0x7F) || reg_addr + num_bytes > 0xAB || num_bytes > 4) {
        return -EINVAL;
    }

    memcpy(data, em_data->mem + reg_addr, num_bytes);

    return 0;
}

static int isl94202_emul_transfer(const struct emul *em, struct i2c_msg *msgs, int num_msgs,
                                  int addr)
{
    if (num_msgs < 1) {
        LOG_ERR("Invalid number of messages: %d", num_msgs);
        return -EIO;
    }
    if (msgs[0].len < 1) {
        LOG_ERR("Unexpected msg0 length %d", msgs[0].len);
        return -EIO;
    }

    /* read operations are write-read, so the first message must always be a write */
    if (msgs[0].flags & I2C_MSG_READ) {
        LOG_ERR("Unexpected read operation");
        return -EIO;
    }

    uint8_t reg_addr = msgs[0].buf[0];

    if (msgs[0].flags & I2C_MSG_STOP) {
        /* simple write operation */
        isl94202_emul_write_bytes(em, reg_addr, msgs[0].buf + 1, msgs[0].len - 1);
    }
    else if (num_msgs > 1) {
        /* write-read operation with reg_addr in the first msg */
        isl94202_emul_read_bytes(em, reg_addr, msgs[1].buf, msgs[1].len);
    }
    else {
        LOG_ERR("Unexpected I2C msg. flags: 0x%x, num_msgs: %d", msgs[0].flags, num_msgs);
        return -EIO;
    }

    return 0;
}

static struct i2c_emul_api bus_api = {
    .transfer = isl94202_emul_transfer,
};

static int isl94202_emul_init(const struct emul *target, const struct device *parent)
{
    return 0;
}

#define ISL94202_EMUL(n) \
    static struct isl94202_emul_data isl94202_emul_data_##n; \
    static const struct isl94202_emul_cfg isl94202_emul_cfg_##n = { \
        .addr = DT_INST_REG_ADDR(n), \
    }; \
    EMUL_DT_INST_DEFINE(n, isl94202_emul_init, &isl94202_emul_data_##n, &isl94202_emul_cfg_##n, \
                        &bus_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(ISL94202_EMUL)
