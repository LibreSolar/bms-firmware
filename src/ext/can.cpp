/*
 * Copyright (c) 2018 Martin Jäger / Libre Solar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(UNIT_TEST) && !defined(__MBED__)

#include "config.h"

#include "ext.h"
#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/can.h>
#include <canbus/isotp.h>

#include "pcb.h"
#include "thingset.h"
#include "can_msg_queue.h"

#ifndef CAN_SPEED
#define CAN_SPEED 250000    // 250 kHz
#endif

#ifndef CAN_NODE_ID
#define CAN_NODE_ID 10
#endif

extern ThingSet ts;

extern const unsigned int PUB_CHANNEL_CAN;

#define RX_THREAD_STACK_SIZE 512
#define RX_THREAD_PRIORITY 2

const struct isotp_fc_opts fc_opts = {.bs = 8, .stmin = 0};

const struct isotp_msg_id rx_addr = {
    .std_id = 0x80,
    .id_type = CAN_STANDARD_IDENTIFIER,
    .use_ext_addr = 0
};
const struct isotp_msg_id tx_addr = {
    .std_id = 0x180,
    .id_type = CAN_STANDARD_IDENTIFIER,
    .use_ext_addr = 0
};

struct isotp_recv_ctx recv_ctx;

K_THREAD_STACK_DEFINE(rx_thread_stack, RX_THREAD_STACK_SIZE);
struct k_thread rx_thread_data;

struct device *can_dev;

void send_complette_cb(int error_nr, void *arg)
{
    ARG_UNUSED(arg);
    printk("TX complete cb [%d]\n", error_nr);
}

void rx_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);
    int ret, rem_len;
    unsigned int received_len;
    struct net_buf *buf;
    static u8_t rx_buffer[100];
    static u8_t tx_buffer[500];

    ret = isotp_bind(&recv_ctx, can_dev,
             &tx_addr, &rx_addr,
             &fc_opts, K_FOREVER);
    if (ret != ISOTP_N_OK) {
        printk("Failed to bind to rx ID %d [%d]\n",
               rx_addr.std_id, ret);
        return;
    }

    while (1) {
        received_len = 0;
        do {
            rem_len = isotp_recv_net(&recv_ctx, &buf, K_FOREVER);
            if (rem_len < 0) {
                printk("Receiving error [%d]\n", rem_len);
                break;
            }
            if (received_len + buf->len <= sizeof(rx_buffer)) {
                memcpy(&rx_buffer[received_len], buf->data, buf->len);
                received_len += buf->len;
            }
            else {
                printk("RX buffer too small\n");
                break;
            }
            net_buf_unref(buf);
        } while (rem_len);

        if (received_len > 0) {
            printk("Got %d bytes in total. Processing ThingSet message.\n", received_len);
            int resp_len = ts.process(rx_buffer, received_len, tx_buffer, sizeof(tx_buffer));

            if (resp_len > 0) {
                static struct isotp_send_ctx send_ctx;
                int ret = isotp_send(&send_ctx, can_dev,
                            tx_buffer, resp_len,
                            &tx_addr, &rx_addr,
                            send_complette_cb, NULL);
                if (ret != ISOTP_N_OK) {
                    printk("Error while sending data to ID %d [%d]\n",
                            tx_addr.std_id, ret);
                }
            }
        }
    }
}

class ThingSetCAN: public ExtInterface
{
public:
    ThingSetCAN(uint8_t can_node_id, const unsigned int c);

    void process_asap();
    void process_1s();

    void enable();

private:
    /**
     * Generate CAN frame for data object and put it into TX queue
     */
    bool pub_object(const data_object_t& data_obj);

    /**
     * Retrieves all data objects of configured channel and calls pub_object to enqueue them
     *
     * \returns number of can objects added to queue
     */
    int pub();

    /**
     * Try to send out all data in TX queue
     */
    void process_outbox();

    CanMsgQueue tx_queue;
    uint8_t node_id;
    const unsigned int channel;

    //struct device *can_dev;
    struct device *can_en_dev;
};

ThingSetCAN ts_can(CAN_NODE_ID, PUB_CHANNEL_CAN);

//----------------------------------------------------------------------------
// preliminary simple CAN functions to send data to the bus for logging
// Data format based on CBOR specification (except for first byte, which uses
// only 6 bit to specify type and transport protocol)
//
// Protocol details:
// https://github.com/LibreSolar/ThingSet/blob/master/can.md

ThingSetCAN::ThingSetCAN(uint8_t can_node_id, const unsigned int c):
    node_id(can_node_id),
    channel(c)
{
    can_en_dev = device_get_binding(DT_SWITCH_CAN_EN_GPIOS_CONTROLLER);
    gpio_pin_configure(can_en_dev, DT_SWITCH_CAN_EN_GPIOS_PIN,
        DT_SWITCH_CAN_EN_GPIOS_FLAGS | GPIO_OUTPUT_INACTIVE);

    can_dev = device_get_binding("CAN_1");
}

void ThingSetCAN::enable()
{
    gpio_pin_set(can_en_dev, DT_SWITCH_CAN_EN_GPIOS_PIN, 1);

    k_tid_t tid = k_thread_create(&rx_thread_data, rx_thread_stack,
                  K_THREAD_STACK_SIZEOF(rx_thread_stack),
                  rx_thread, NULL, NULL, NULL,
                  RX_THREAD_PRIORITY, 0, K_NO_WAIT);
    if (!tid) {
        printk("ERROR spawning rx thread\n");
    }
}

void ThingSetCAN::process_1s()
{
    pub();
    process_asap();
}

bool ThingSetCAN::pub_object(const data_object_t& data_obj)
{
    unsigned int can_id;
    uint8_t data[8];

    struct zcan_frame frame = {
        .id_type = CAN_EXTENDED_IDENTIFIER,
        .rtr = CAN_DATAFRAME
    };

    int encode_len = ts.encode_msg_can(data_obj, node_id, can_id, data);
    frame.ext_id = can_id;
    memcpy(frame.data, data, 8);

    if (encode_len >= 0)
    {
        frame.dlc = encode_len;
        tx_queue.enqueue(frame);
    }
    return (encode_len >= 0);
}

int ThingSetCAN::pub()
{
    int retval = 0;
    ts_pub_channel_t* can_chan = ts.get_pub_channel(channel);

    if (can_chan != NULL)
    {
        for (unsigned int element = 0; element < can_chan->num; element++)
        {
            const data_object_t *data_obj = ts.get_data_object(can_chan->object_ids[element]);
            if (data_obj != NULL && data_obj->access & TS_ACCESS_READ)
            {
                if (pub_object(*data_obj))
                {
                    retval++;
                }
            }
        }
    }
    return retval;
}

void ThingSetCAN::process_asap()
{
    process_outbox();
}

void can_pub_isr(u32_t err_flags, void *arg)
{
	// Do nothing. Publication messages are fire and forget.
}

void ThingSetCAN::process_outbox()
{
    int max_attempts = 15;
    while (!tx_queue.empty() && max_attempts > 0) {
        CanFrame msg;
        tx_queue.first(msg);
        if (can_send(can_dev, &msg, K_MSEC(10), can_pub_isr, NULL) == CAN_TX_OK) {
            tx_queue.dequeue();
        }
        else {
            //printk("Sending CAN message failed");
        }
        max_attempts--;
    }
}

#endif /* UNIT_TEST */
