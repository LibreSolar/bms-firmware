/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_THINGSET_SERIAL

#include <stdio.h>
#include <string.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "data_objects.h"
#include "thingset.h"

#if CONFIG_UEXT_SERIAL_THINGSET && DT_NODE_EXISTS(DT_ALIAS(uart_uext))
#define UART_DEVICE_NODE DT_ALIAS(uart_uext)
#elif DT_NODE_EXISTS(DT_ALIAS(uart_dbg))
#define UART_DEVICE_NODE DT_ALIAS(uart_dbg)
#else
// cppcheck-suppress preprocessorErrorDirective
#error "No UART for ThingSet serial defined."
#endif

const struct device *uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static char tx_buf[CONFIG_THINGSET_SERIAL_TX_BUF_SIZE];
static char rx_buf[CONFIG_THINGSET_SERIAL_RX_BUF_SIZE];

static volatile size_t rx_buf_pos = 0;

static struct k_sem command_flag; // used as an event to signal a received command
static struct k_sem rx_buf_mutex; // binary semaphore used as mutex in ISR context

extern ThingSet ts;

const char serial_subset_path[] = "mSerial";
static ThingSetDataObject *serial_subset;

void serial_pub_msg()
{
    if (pub_serial_enable) {
        int len = ts.txt_statement(tx_buf, sizeof(tx_buf), serial_subset);
        for (int i = 0; i < len; i++) {
            uart_poll_out(uart_dev, tx_buf[i]);
        }
        uart_poll_out(uart_dev, '\n');
    }
}

void serial_process_command()
{
    // commands must have 2 or more characters
    if (rx_buf_pos > 1) {
        printf("Received Request (%d bytes): %s\n", strlen(rx_buf), rx_buf);

        int len = ts.process((uint8_t *)rx_buf, strlen(rx_buf), (uint8_t *)tx_buf, sizeof(tx_buf));

        for (int i = 0; i < len; i++) {
            uart_poll_out(uart_dev, tx_buf[i]);
        }
        uart_poll_out(uart_dev, '\n');
    }

    // release buffer and start waiting for new commands
    rx_buf_pos = 0;
    k_sem_give(&rx_buf_mutex);
}

/*
 * Read characters from stream until line end \n is detected, afterwards signal available command.
 */
void serial_cb(const struct device *dev, void *user_data)
{
    uint8_t c = 0;

    if (!uart_irq_update(uart_dev)) {
        return;
    }

    while (uart_irq_rx_ready(uart_dev) && k_sem_take(&rx_buf_mutex, K_NO_WAIT) == 0) {

        uart_fifo_read(uart_dev, &c, 1);

        // \r\n and \n are markers for line end, i.e. command end
        // we accept this at any time, even if the buffer is 'full', since
        // there is always one last character left for the \0
        if (c == '\n') {
            if (rx_buf_pos > 0 && rx_buf[rx_buf_pos - 1] == '\r') {
                rx_buf[rx_buf_pos - 1] = '\0';
            }
            else {
                rx_buf[rx_buf_pos] = '\0';
            }
            // start processing command and keep the rx_buf_mutex locked
            k_sem_give(&command_flag);
            return;
        }
        // backspace allowed if there is something in the buffer already
        else if (rx_buf_pos > 0 && c == '\b') {
            rx_buf_pos--;
        }
        // Fill the buffer up to all but 1 character (the last character is reserved for '\0')
        // Characters beyond the size of the buffer are dropped.
        else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
            rx_buf[rx_buf_pos++] = c;
        }

        k_sem_give(&rx_buf_mutex);
    }
}

void serial_thread()
{
    k_sem_init(&command_flag, 0, 1);
    k_sem_init(&rx_buf_mutex, 1, 1);

    __ASSERT_NO_MSG(device_is_ready(uart_dev));

    uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
    uart_irq_rx_enable(uart_dev);

    serial_subset = ts.get_endpoint(serial_subset_path, strlen(serial_subset_path));

    int64_t t_start = k_uptime_get();

    while (true) {
        if (k_sem_take(&command_flag, K_TIMEOUT_ABS_MS(t_start)) == 0) {
            serial_process_command();
        }
        else {
            // semaphore timed out (should happen exactly every 1 second)
            t_start += 1000;
            serial_pub_msg();
        }
    }
}

K_THREAD_DEFINE(serial_thread_id, 1280, serial_thread, NULL, NULL, NULL, 6, 0, 1000);

#endif /* CONFIG_THINGSET_SERIAL */
