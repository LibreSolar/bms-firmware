/* LibreSolar Battery Management System firmware
 * Copyright (c) 2016-2018 Martin Jäger (www.libre.solar)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(__MBED__) && !defined(UNIT_TEST)

#include "mbed.h"
#include "config.h"
#include "output_can.h"

#include "thingset.h"

extern Serial serial;
extern CAN can;

CANMsgQueue can_tx_queue;
CANMsgQueue can_rx_queue;

bool CANMsgQueue::full() {
    return _length == CAN_QUEUE_SIZE;
}

bool CANMsgQueue::empty() {
    return _length == 0;
}

void CANMsgQueue::enqueue(CANMessage msg) {
    if (!full()) {
        _queue[(_first + _length) % CAN_QUEUE_SIZE] = msg;
        _length++;
    }
}

int CANMsgQueue::dequeue() {
    if (!empty()) {
        _first = (_first + 1) % CAN_QUEUE_SIZE;
        _length--;
        return 1;
    } else {
        return 0;
    }
}

int CANMsgQueue::dequeue(CANMessage& msg) {
    if (!empty()) {
        msg = _queue[_first];
        _first = (_first + 1) % CAN_QUEUE_SIZE;
        _length--;
        return 1;
    } else {
        return 0;
    }
}

int CANMsgQueue::first(CANMessage& msg) {
    if (!empty()) {
        msg = _queue[_first];
        return 1;
    } else {
        return 0;
    }
}

//----------------------------------------------------------------------------
// preliminary simple CAN functions to send data to the bus for logging
// Data format based on CBOR specification (except for first byte, which uses
// only 6 bit to specify type and transport protocol)
//
// Protocol details:
// https://thingset.github.io

void can_pub_msg(data_object_t data_obj)
{
    // TODO: use ThingSet library instead of preliminary code
    /*
    union float2bytes { float f; char b[4]; } f2b;     // for conversion of float to single bytes

    int msg_priority = 6;

    CANMessage msg;
    msg.format = CANExtended;
    msg.type = CANData;

    msg.id = msg_priority << 26
        | (1U << 24) | (1U << 25)   // identify as publication message
        | data_obj.id << 8
        | can_node_id;

    // first byte: TinyTP-header or data type for single frame message
    // currently: no multi-frame and no timestamp
    msg.data[0] = 0x00;

    int32_t value;
    uint32_t value_abs;
    uint8_t exponent_abs;

    // CBOR byte order: most significant byte first
    switch (data_obj.type) {
        case T_FLOAT32:
            msg.len = 5;
            msg.data[0] |= TS_T_FLOAT32;
            f2b.f = *((float*)data_obj.data);
            msg.data[1] = f2b.b[3];
            msg.data[2] = f2b.b[2];
            msg.data[3] = f2b.b[1];
            msg.data[4] = f2b.b[0];
            break;
        case T_INT32:
            if (data_obj.exponent == 0) {
                    msg.len = 5;
                    value = *((int*)data_obj.data);
                    value_abs = abs(value);
                    if (value >= 0) {
                        msg.data[0] |= TS_T_POS_INT32;
                        value_abs = abs(value);
                    }
                    else {
                        msg.data[0] |= TS_T_NEG_INT32;
                        value_abs = abs(value - 1);         // 0 is always pos integer
                    }
                    msg.data[1] = value_abs >> 24;
                    msg.data[2] = value_abs >> 16;
                    msg.data[3] = value_abs >> 8;
                    msg.data[4] = value_abs;
            }
            else {
                msg.len = 8;
                msg.data[0] |= TS_T_DECIMAL_FRAC;
                msg.data[1] = 0x82;     // array of length 2

                // exponent in single byte value, valid: -24 ... 23
                exponent_abs = abs(data_obj.exponent);
                if (data_obj.exponent >= 0 && data_obj.exponent <= 23) {
                    msg.data[2] = exponent_abs;
                }
                else if (data_obj.exponent < 0 && data_obj.exponent > -24) {
                    msg.data[2] = (exponent_abs - 1) | 0x20;      // negative uint8
                }

                // value as positive or negative uint32
                value = *((int*)data_obj.data);
                value_abs = abs(value);
                if (value >= 0) {
                    msg.data[3] = 0x1a;     // positive int32
                    value_abs = abs(value);
                }
                else {
                    msg.data[3] = 0x3a;     // negative int32
                    value_abs = abs(value - 1);         // 0 is always pos integer
                }
                msg.data[4] = value_abs >> 24;
                msg.data[5] = value_abs >> 16;
                msg.data[6] = value_abs >> 8;
                msg.data[7] = value_abs;
            }
            break;
        case T_BOOL:
            msg.len = 1;
            if (*((bool*)data_obj.data) == true) {
                msg.data[0] = TS_T_TRUE;     // simple type: true
            }
            else {
                msg.data[0] = TS_T_FALSE;     // simple type: false
            }
            break;
        case T_STRING:
            break;
    }
    can_tx_queue.enqueue(msg);
    */
}

void can_send_data()
{
    /*
    for (unsigned int i = 0; i < sizeof(dataObjects)/sizeof(DataObject_t); ++i) {
        can_pub_msg(dataObjects[i]);
    }
    */
}

void can_process_outbox()
{
    int max_attempts = 15;
    while (!can_tx_queue.empty() && max_attempts > 0) {
        CANMessage msg;
        can_tx_queue.first(msg);
        if(can.write(msg)) {
            can_tx_queue.dequeue();
        }
        else {
            //serial.printf("Sending CAN message failed, err: %u, MCR: %u, MSR: %u, TSR: %u, id: %u\n", can.tderror(), (uint32_t)CAN1->MCR, (uint32_t)CAN1->MSR, (uint32_t)CAN1->TSR, msg.id);
        }
        max_attempts--;
    }
}

void can_process_inbox()
{
    // TODO
}

void can_receive()
{
    CANMessage msg;
    while (can.read(msg)) {
        if (!can_rx_queue.full()) {
            can_rx_queue.enqueue(msg);
            serial.printf("Message received. id: %d, data: %d\n", msg.id, msg.data[0]);
        } else {
            serial.printf("CAN rx queue full\n");
        }
    }
}

#endif /* UNIT_TEST */
