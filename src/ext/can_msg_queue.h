/*
 * Copyright (c) 2017 Martin Jäger / Libre Solar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CAN_MSG_QUEUE_H
#define CAN_MSG_QUEUE_H

#ifdef __MBED__
#include "mbed.h"
#define CanFrame CANMessage
#elif defined(__ZEPHYR__)
#include <zephyr.h>
#include <drivers/can.h>
#define CanFrame struct zcan_frame
#endif

#define CAN_QUEUE_SIZE 30

class CanMsgQueue
{
public:
    bool full();
    bool empty();
    void enqueue(CanFrame frame);
    int dequeue(CanFrame &frame);
    int dequeue();
    int first(CanFrame& frame);

private:
    CanFrame _queue[CAN_QUEUE_SIZE];
    int _first;
    int _length;
};

#endif /* CAN_MSG_QUEUE_H */
