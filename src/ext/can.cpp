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

#include "pcb.h"
#include "thingset.h"
#include "can_msg_queue.h"

#ifndef CAN_SPEED
#define CAN_SPEED 250000    // 250 kHz
#endif

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

    struct device *can_dev;
    struct device *can_en_dev;
};

extern const unsigned int PUB_CHANNEL_CAN;

#ifndef CAN_NODE_ID
#define CAN_NODE_ID 10
#endif

ThingSetCAN ts_can(CAN_NODE_ID, PUB_CHANNEL_CAN);

//----------------------------------------------------------------------------
// preliminary simple CAN functions to send data to the bus for logging
// Data format based on CBOR specification (except for first byte, which uses
// only 6 bit to specify type and transport protocol)
//
// Protocol details:
// https://github.com/LibreSolar/ThingSet/blob/master/can.md

extern ThingSet ts;

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

void ThingSetCAN::process_outbox()
{
    int max_attempts = 15;
    while (!tx_queue.empty() && max_attempts > 0) {
        CanFrame msg;
        tx_queue.first(msg);
        if (can_send(can_dev, &msg, K_MSEC(100), NULL, NULL) == CAN_TX_OK) {
            tx_queue.dequeue();
        }
        else {
            //printk("Sending CAN message failed, MCR: %x, MSR: %x, TSR: %x\n", (uint32_t)CAN->MCR, (uint32_t)CAN->MSR, (uint32_t)CAN->TSR);
            //printf("Sending CAN message failed, MCR: %x, MSR: %x, TSR: %x\n", (uint32_t)CAN1->MCR, (uint32_t)CAN1->MSR, (uint32_t)CAN1->TSR);
        }
        max_attempts--;
    }
}

#endif /* UNIT_TEST */
