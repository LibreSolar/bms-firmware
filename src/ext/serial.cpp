/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_THINGSET_SERIAL

#include <string.h>
#include <stdio.h>

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/uart.h>

#include "thingset.h"
#include "data_nodes.h"

#if CONFIG_UEXT_SERIAL_THINGSET
#define UART_DEVICE_NAME DT_LABEL(DT_ALIAS(uart_uext))
#elif DT_NODE_EXISTS(DT_ALIAS(uart_dbg))
#define UART_DEVICE_NAME DT_LABEL(DT_ALIAS(uart_dbg))
#else
// cppcheck-suppress preprocessorErrorDirective
#error "No UART for ThingSet serial defined."
#endif

const struct device *uart_dev = device_get_binding(UART_DEVICE_NAME);

static char buf_resp[CONFIG_THINGSET_SERIAL_TX_BUF_SIZE];
static char buf_req[CONFIG_THINGSET_SERIAL_RX_BUF_SIZE];

static volatile size_t req_pos = 0;
static volatile bool command_flag = false;

extern ThingSet ts;

void process_1s()
{
    if (pub_serial_enable) {
        int len = ts.txt_pub(buf_resp, sizeof(buf_resp), PUB_SER);
        for (int i = 0; i < len; i++) {
            uart_poll_out(uart_dev, buf_resp[i]);
        }
        uart_poll_out(uart_dev, '\n');
    }
}

void process_asap()
{
    if (command_flag) {
        // commands must have 2 or more characters
        if (req_pos > 1) {
            printf("Received Request (%d bytes): %s\n", strlen(buf_req), buf_req);

            int len = ts.process((uint8_t *)buf_req, strlen(buf_req),
                (uint8_t *)buf_resp, sizeof(buf_resp));

            for (int i = 0; i < len; i++) {
                uart_poll_out(uart_dev, buf_resp[i]);
            }
            uart_poll_out(uart_dev, '\n');
        }

        // start listening for new commands
        command_flag = false;
        req_pos = 0;
    }
}

/**
 * Read characters from stream until line end \n detected, signal command available then
 * and wait for processing
 */
void process_input(const struct device *dev, void* user_data)
{
    uint8_t c;

    if (!uart_irq_update(uart_dev)) {
        return;
    }

    while (uart_irq_rx_ready(uart_dev) && command_flag == false) {
        uart_fifo_read(uart_dev, &c, 1);

        // \r\n and \n are markers for line end, i.e. command end
        // we accept this at any time, even if the buffer is 'full', since
        // there is always one last character left for the \0
        if (c == '\n') {
            if (req_pos > 0 && buf_req[req_pos-1] == '\r') {
                buf_req[req_pos-1] = '\0';
            }
            else {
                buf_req[req_pos] = '\0';
            }
            // start processing
            command_flag = true;
        }
        // backspace allowed if there is something in the buffer already
        else if (req_pos > 0 && c == '\b') {
            req_pos--;
        }
        // Fill the buffer up to all but 1 character (the last character is reserved for '\0')
        // Characters beyond the size of the buffer are dropped.
        else if (req_pos < (sizeof(buf_req)-1)) {
            buf_req[req_pos++] = c;
        }
    }
}

void serial_thread()
{
    uint32_t last_call = 0;

    uart_irq_callback_user_data_set(uart_dev, process_input, NULL);
    uart_irq_rx_enable(uart_dev);

    while (true) {
        uint32_t now = k_uptime_get() / 1000;

        process_asap();     // approx. every millisecond
        if (now >= last_call + 1) {
            last_call = now;
            process_1s();
        }
        k_sleep(K_MSEC(10));
    }
}

K_THREAD_DEFINE(serial, 1280, serial_thread, NULL, NULL, NULL, 6, 0, 1000);

#endif /* CONFIG_THINGSET_SERIAL */
