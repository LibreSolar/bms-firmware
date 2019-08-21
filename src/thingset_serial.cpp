/* Libre Solar Battery Management System firmware
 * Copyright (c) 2016-2019 Martin Jäger (www.libre.solar)
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

#ifndef UNIT_TEST

#include <string.h>
#include "config.h"
#include "pcb.h"
#include "bms.h"
#include "thingset.h"

extern BmsStatus bms_status;
extern float load_voltage;

extern ThingSet ts;

extern ts_pub_channel_t pub_channels[];
extern const int pub_channel_serial;

#ifdef __MBED__

#include "mbed.h"

uint8_t buf_req_uart[500];
size_t req_uart_pos = 0;
static uint8_t buf_resp[1000];           // only one response buffer needed for USB and UART

Serial* ser_uart;
extern Serial serial;

static bool uart_serial_command_flag = false;

void uart_serial_isr()
{
    while (ser_uart->readable() && uart_serial_command_flag == false) {
        if (req_uart_pos < sizeof(buf_req_uart)) {
            buf_req_uart[req_uart_pos] = ser_uart->getc();

            if (buf_req_uart[req_uart_pos] == '\n') {
                if (req_uart_pos > 0 && buf_req_uart[req_uart_pos-1] == '\r')
                    buf_req_uart[req_uart_pos-1] = '\0';
                else
                    buf_req_uart[req_uart_pos] = '\0';

                // start processing
                uart_serial_command_flag = true;
            }
            else if (req_uart_pos > 0 && buf_req_uart[req_uart_pos] == '\b') { // backspace
                req_uart_pos--;
            }
            else {
                req_uart_pos++;
            }
        }
    }
}

void thingset_serial_init()
{
    ser_uart = &serial;
    ser_uart->attach(uart_serial_isr);
}

void thingset_serial_process_asap()
{
    if (uart_serial_command_flag) {
        if (req_uart_pos > 1) {
            ser_uart->printf("Received Request (%d bytes): %s\n", strlen((char *)buf_req_uart), buf_req_uart);
            ts.process(buf_req_uart, strlen((char *)buf_req_uart), buf_resp, sizeof(buf_resp));
            ser_uart->printf("%s\n", buf_resp);
        }

        // start listening for new commands
        uart_serial_command_flag = false;
        req_uart_pos = 0;
    }
}

void thingset_serial_process_1s()
{
    if (pub_channels[pub_channel_serial].enabled) {
        ts.pub_msg_json((char *)buf_resp, sizeof(buf_resp), 0);
        ser_uart->printf("%s\n", buf_resp);
    }

    fflush(stdout);
}

#elif defined(ZEPHYR)

#include <zephyr.h>
#include <sys/printk.h>
#include <console/console.h>

void thingset_serial_thread()
{
    uint8_t buf_req[500];
    uint8_t buf_resp[500];
    unsigned int req_pos = 0;

    console_init();

	while (1) {

        uint8_t c = console_getchar();

        if (req_pos < sizeof(buf_req)) {
            buf_req[req_pos] = c;

            if (buf_req[req_pos] == '\n') {
                if (req_pos > 0 && buf_req[req_pos-1] == '\r')
                    buf_req[req_pos-1] = '\0';
                else
                    buf_req[req_pos] = '\0';

                // start processing
                if (req_pos > 1) {
                    printf("Received Request (%d bytes): %s\n", strlen((char *)buf_req), buf_req);
                    int len = ts.process(buf_req, strlen((char *)buf_req), buf_resp, sizeof(buf_resp));
                    for (int i = 0; i < len; i++) {
                        console_putchar(buf_resp[i]);
                    }
                    printf("\n");
                }

                req_pos = 0;
            }
            else if (req_pos > 0 && buf_req[req_pos] == '\b') { // backspace
                req_pos--;
            }
            else {
                req_pos++;
            }
        }
        else {
            // buffer to small: discard data and wait for end of line
            if (c == '\n' || c == '\r') {
                req_pos = 0;
            }
        }
	}
}

#endif /* ZEPHYR */

#endif /* UNIT_TEST */
