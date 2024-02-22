/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq769x2_i2c

#include "bq769x2_registers.h"

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bq769x2_emul, CONFIG_BMS_IC_LOG_LEVEL);

/*
 * Memory layout of bq769x2 for direct commands and subcommands
 *
 * Used subcommand address space starts with 0x9180 and ends below 0x9400
 */
#define BQ_DIRECT_MEM_SIZE (0x80)
#define BQ_SUBCMD_MEM_SIZE (0x9400)

struct bq769x0_emul_data
{
    /* Memory of bq769x2 for direct commands */
    uint8_t direct_mem[BQ_DIRECT_MEM_SIZE];
    /* Memory of bq769x2 for subcommands / data */
    uint8_t subcmd_mem[BQ_SUBCMD_MEM_SIZE];
    uint32_t cur_reg;
};

struct bq769x0_emul_cfg
{
    uint16_t addr;
};

uint8_t bq769x2_emul_get_direct_mem(const struct emul *em, uint8_t addr)
{
    struct bq769x0_emul_data *em_data = em->data;

    return em_data->direct_mem[addr];
}

void bq769x2_emul_set_direct_mem(const struct emul *em, uint8_t addr, uint8_t byte)
{
    struct bq769x0_emul_data *em_data = em->data;

    em_data->direct_mem[addr] = byte;
}

uint8_t bq769x2_emul_get_data_mem(const struct emul *em, uint16_t addr)
{
    struct bq769x0_emul_data *em_data = em->data;

    return em_data->subcmd_mem[addr];
}

void bq769x2_emul_set_data_mem(const struct emul *em, uint16_t addr, uint8_t byte)
{
    struct bq769x0_emul_data *em_data = em->data;

    em_data->subcmd_mem[addr] = byte;
}

/*
 * This function emulates the actual behavior of the chip for some subcmds, if required for the unit
 * tests.
 *
 * Currently only command-only subcmds are supported (without data).
 */
static void bq769x0_emul_process_subcmd(const struct emul *em, const uint16_t subcmd_addr)
{
    struct bq769x0_emul_data *em_data = em->data;

    switch (subcmd_addr) {
        case BQ769X2_SUBCMD_SET_CFGUPDATE:
            k_usleep(2000);
            em_data->direct_mem[BQ769X2_CMD_BATTERY_STATUS] = 0x01;
            break;
        case BQ769X2_SUBCMD_EXIT_CFGUPDATE:
            k_usleep(1000);
            em_data->direct_mem[BQ769X2_CMD_BATTERY_STATUS] = 0x00;
            break;
    };
}

static int bq769x0_emul_write_bytes(const struct emul *em, const uint8_t reg_addr,
                                    const uint8_t *data, const size_t num_bytes)
{
    struct bq769x0_emul_data *em_data = em->data;

    if (reg_addr >= sizeof(em_data->direct_mem)) {
        return -EINVAL;
    }

    memcpy(&em_data->direct_mem[reg_addr], data, num_bytes);

    if (reg_addr == BQ769X2_SUBCMD_DATA_CHECKSUM || reg_addr == BQ769X2_SUBCMD_DATA_LENGTH) {
        /* writing to BQ769X2_SUBCMD_DATA_LENGTH starts execution of a subcommand */

        uint16_t subcmd_addr = (em_data->direct_mem[BQ769X2_CMD_SUBCMD_UPPER] << 8)
                               + em_data->direct_mem[BQ769X2_CMD_SUBCMD_LOWER];
        uint8_t subcmd_bytes =
            em_data->direct_mem[BQ769X2_SUBCMD_DATA_LENGTH] - BQ769X2_SUBCMD_OVERHEAD_BYTES;

        if (subcmd_addr >= sizeof(em_data->subcmd_mem)) {
            return -EINVAL;
        }

        memcpy(&em_data->subcmd_mem[subcmd_addr], &em_data->direct_mem[BQ769X2_SUBCMD_DATA_START],
               subcmd_bytes);
    }
    else if (reg_addr == BQ769X2_CMD_SUBCMD_LOWER && num_bytes == 2) {
        /* writing to SUBCMD register initiates a subcmd read */

        uint16_t subcmd_addr = (em_data->direct_mem[BQ769X2_CMD_SUBCMD_UPPER] << 8)
                               + em_data->direct_mem[BQ769X2_CMD_SUBCMD_LOWER];

        if (subcmd_addr >= sizeof(em_data->subcmd_mem)) {
            return -EINVAL;
        }

        bq769x0_emul_process_subcmd(em, subcmd_addr);

        memcpy(&em_data->direct_mem[BQ769X2_SUBCMD_DATA_START], &em_data->subcmd_mem[subcmd_addr],
               4);

        // always assume maximum data type length of 4 bytes
        uint8_t checksum = em_data->direct_mem[BQ769X2_CMD_SUBCMD_UPPER]
                           + em_data->direct_mem[BQ769X2_CMD_SUBCMD_LOWER];
        for (int i = 0; i < 4; i++) {
            checksum += em_data->direct_mem[BQ769X2_SUBCMD_DATA_START + i];
        }
        checksum = ~checksum;

        em_data->direct_mem[BQ769X2_SUBCMD_DATA_LENGTH] = 4 + BQ769X2_SUBCMD_OVERHEAD_BYTES;
        em_data->direct_mem[BQ769X2_SUBCMD_DATA_CHECKSUM] = checksum;
    }

    return 0;
}

static int bq769x0_emul_read_bytes(const struct emul *em, const uint8_t reg_addr, uint8_t *data,
                                   const size_t num_bytes)
{
    struct bq769x0_emul_data *em_data = em->data;

    if (reg_addr >= sizeof(em_data->direct_mem)) {
        return -EINVAL;
    }

    memcpy(data, em_data->direct_mem + reg_addr, num_bytes);

    return 0;
}

static int bq769x0_emul_transfer(const struct emul *em, struct i2c_msg *msgs, int num_msgs,
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
        bq769x0_emul_write_bytes(em, reg_addr, msgs[0].buf + 1, msgs[0].len - 1);
    }
    else if (num_msgs > 1) {
        /* write-read operation with reg_addr in the first msg */
        bq769x0_emul_read_bytes(em, reg_addr, msgs[1].buf, msgs[1].len);
    }
    else {
        LOG_ERR("Unexpected I2C msg. flags: 0x%x, num_msgs: %d", msgs[0].flags, num_msgs);
        return -EIO;
    }

    return 0;
}

static struct i2c_emul_api bus_api = {
    .transfer = bq769x0_emul_transfer,
};

static int bq769x2_emul_init(const struct emul *target, const struct device *parent)
{
    return 0;
}

#define BQ769X2_EMUL(n) \
    static struct bq769x0_emul_data bq769x0_emul_data_##n; \
    static const struct bq769x0_emul_cfg bq769x0_emul_cfg_##n = { \
        .addr = DT_INST_REG_ADDR(n), \
    }; \
    EMUL_DT_INST_DEFINE(n, bq769x2_emul_init, &bq769x0_emul_data_##n, &bq769x0_emul_cfg_##n, \
                        &bus_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(BQ769X2_EMUL)
