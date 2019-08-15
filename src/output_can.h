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

#ifndef OUTPUT_CAN_H
#define OUTPUT_CAN_H

#include "mbed.h"
#include "config.h"
#include "data_objects.h"

#define PUB_MULTIFRAME_EN (0x1U << 7)
#define PUB_TIMESTAMP_EN (0x1U << 6)

#define CAN_QUEUE_SIZE 30

const uint8_t can_node_id = CAN_NODE_ID;

class CANMsgQueue {
public:
    bool full();
    bool empty();
    void enqueue(CANMessage msg);
    int dequeue(CANMessage& msg);
    int dequeue();
    int first(CANMessage& msg);

private:
    CANMessage _queue[CAN_QUEUE_SIZE];
    int _first;
    int _length;
};

void can_send_data();
void can_receive();
void can_process_outbox();
void can_process_inbox();

#endif
