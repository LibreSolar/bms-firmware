/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_THINGSET_CAN

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/can.h>

#ifdef CONFIG_ISOTP
#include <canbus/isotp.h>
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(ext_can, CONFIG_CAN_LOG_LEVEL);

#include "thingset.h"
#include "data_nodes.h"

#if DT_NODE_EXISTS(DT_CHILD(DT_PATH(switches), can_en))
#define CAN_EN_GPIO DT_CHILD(DT_PATH(switches), can_en)
#endif

extern ThingSet ts;
extern uint16_t can_node_addr;

const struct device *can_dev;

#ifdef CONFIG_ISOTP

#define RX_THREAD_STACK_SIZE 512
#define RX_THREAD_PRIORITY 2

const struct isotp_fc_opts fc_opts = {.bs = 8, .stmin = 0};

struct isotp_msg_id rx_addr = {
    .id_type = CAN_EXTENDED_IDENTIFIER,
    .use_ext_addr = 0,      // Normal ISO-TP addressing (using only CAN ID)
    .use_fixed_addr = 1,    // enable SAE J1939 compatible addressing
};

struct isotp_msg_id tx_addr = {
    .id_type = CAN_EXTENDED_IDENTIFIER,
    .use_ext_addr = 0,      // Normal ISO-TP addressing (using only CAN ID)
    .use_fixed_addr = 1,    // enable SAE J1939 compatible addressing
};

struct isotp_recv_ctx recv_ctx;

void send_complete_cb(int error_nr, void *arg)
{
    ARG_UNUSED(arg);
    LOG_DBG("TX complete callback, err: %d", error_nr);
}

void can_rx_thread()
{
    int ret, rem_len;
    unsigned int received_len;
    struct net_buf *buf;
    static uint8_t rx_buffer[100];
    static uint8_t tx_buffer[500];

    // CAN node ID retrieved from EEPROM --> reset necessary after change via ThingSet serial
    rx_addr.ext_id = TS_CAN_BASE_REQRESP | TS_CAN_PRIO_REQRESP | TS_CAN_TARGET_SET(can_node_addr);
    tx_addr.ext_id = TS_CAN_BASE_REQRESP | TS_CAN_PRIO_REQRESP | TS_CAN_SOURCE_SET(can_node_addr);

    while (1) {
        ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr, &fc_opts, K_FOREVER);
        if (ret != ISOTP_N_OK) {
            LOG_DBG("Failed to bind to rx ID %d [%d]", rx_addr.ext_id, ret);
            return;
        }

        received_len = 0;
        do {
            rem_len = isotp_recv_net(&recv_ctx, &buf, K_FOREVER);
            if (rem_len < 0) {
                LOG_DBG("Receiving error [%d]", rem_len);
                break;
            }
            if (received_len + buf->len <= sizeof(rx_buffer)) {
                memcpy(&rx_buffer[received_len], buf->data, buf->len);
                received_len += buf->len;
            }
            else {
                LOG_DBG("RX buffer too small");
                break;
            }
            net_buf_unref(buf);
        } while (rem_len);

        // we need to unbind the receive ctx so that control frames are received in the send ctx
        isotp_unbind(&recv_ctx);

        if (received_len > 0) {
            LOG_DBG("Got %d bytes via ISO-TP. Processing ThingSet message.", received_len);
            LOG_DBG("RX buf: %x %x %x %x", rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3]);
            int resp_len = ts.process(rx_buffer, received_len, tx_buffer, sizeof(tx_buffer));
            LOG_DBG("TX buf: %x %x %x %x", tx_buffer[0], tx_buffer[1], tx_buffer[2], tx_buffer[3]);

            if (resp_len > 0) {
                static struct isotp_send_ctx send_ctx;
                int ret = isotp_send(&send_ctx, can_dev, tx_buffer, resp_len,
                            &recv_ctx.tx_addr, &recv_ctx.rx_addr, send_complete_cb, NULL);
                if (ret != ISOTP_N_OK) {
                    LOG_DBG("Error while sending data to ID %d [%d]", tx_addr.ext_id, ret);
                }
            }
        }
    }
}

K_THREAD_DEFINE(can_rx, RX_THREAD_STACK_SIZE, can_rx_thread, NULL, NULL, NULL,
    RX_THREAD_PRIORITY, 0, 1500);

#endif /* CONFIG_ISOTP */

void can_pub_isr(uint32_t err_flags, void *arg)
{
	// Do nothing. Publication messages are fire and forget.
}

void can_pub_thread()
{
    unsigned int can_id;
    uint8_t can_data[8];

    const struct device *can_en_dev = device_get_binding(DT_GPIO_LABEL(CAN_EN_GPIO, gpios));
    gpio_pin_configure(can_en_dev, DT_GPIO_PIN(CAN_EN_GPIO, gpios),
        DT_GPIO_FLAGS(CAN_EN_GPIO, gpios) | GPIO_OUTPUT_ACTIVE);

    can_dev = device_get_binding("CAN_1");

    int64_t t_start = k_uptime_get();

    while (true) {
        if (pub_can_enable) {
            int data_len = 0;
            int start_pos = 0;
            while ((data_len = ts.bin_pub_can(start_pos, PUB_CAN, can_node_addr, can_id, can_data))
                     != -1)
            {
                struct zcan_frame frame = {0};
                frame.id_type = CAN_EXTENDED_IDENTIFIER;
                frame.rtr     = CAN_DATAFRAME;
                frame.id      = can_id;
                memcpy(frame.data, can_data, 8);

                if (data_len >= 0) {
                    frame.dlc = data_len;

                    if (can_send(can_dev, &frame, K_MSEC(10), can_pub_isr, NULL) != CAN_TX_OK) {
                        LOG_DBG("Error sending CAN frame");
                    }
                }
            }
        }

        t_start += 1000;
        k_sleep(K_TIMEOUT_ABS_MS(t_start));
    }
}

K_THREAD_DEFINE(can_pub, 1024, can_pub_thread, NULL, NULL, NULL, 6, 0, 1000);

#endif /* CONFIG_THINGSET_CAN */
