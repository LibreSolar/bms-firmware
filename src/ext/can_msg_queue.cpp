/*
 * Copyright (c) 2017 Martin Jäger / Libre Solar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIT_TEST

#include "config.h"

#if CONFIG_EXT_THINGSET_CAN

#include "ext/can_msg_queue.h"

bool CanMsgQueue::full()
{
    return _length == CAN_QUEUE_SIZE;
}

bool CanMsgQueue::empty()
{
    return _length == 0;
}

void CanMsgQueue::enqueue(CanFrame frame)
{
    if (!full()) {
        _queue[(_first + _length) % CAN_QUEUE_SIZE] = frame;
        _length++;
    }
}

int CanMsgQueue::dequeue()
{
    if (!empty()) {
        _first = (_first + 1) % CAN_QUEUE_SIZE;
        _length--;
        return 1;
    }
    else {
        return 0;
    }
}

int CanMsgQueue::dequeue(CanFrame &frame)
{
    if (!empty()) {
        frame = _queue[_first];
        _first = (_first + 1) % CAN_QUEUE_SIZE;
        _length--;
        return 1;
    }
    else {
        return 0;
    }
}

int CanMsgQueue::first(CanFrame &frame)
{
    if (!empty()) {
        frame = _queue[_first];
        return 1;
    }
    else {
        return 0;
    }
}

#endif /* CONFIG_EXT_THINGSET_CAN */

#endif /* UNIT_TEST */
