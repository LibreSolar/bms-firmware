/*
 * Copyright (c) 2018 Martin Jäger / Libre Solar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIT_TEST

#include "config.h"

#if CONFIG_EXT_THINGSET_SERIAL   // otherwise don't compile code to reduce firmware size

#include "ext.h"

#ifdef __MBED__
#include "mbed.h"
#elif defined(__ZEPHYR__)
#include <zephyr.h>
#include <sys/printk.h>
#include <string.h>
#include <stdio.h>
#include <drivers/uart.h>
#endif /* MBED or ZEPHYR */

#include "thingset.h"

#ifdef __ZEPHYR__
#define TX_BUF_SIZE CONFIG_EXT_THINGSET_SERIAL_TX_BUF_SIZE
#define RX_BUF_SIZE CONFIG_EXT_THINGSET_SERIAL_RX_BUF_SIZE

#define UART_DEVICE_NAME DT_ALIAS_UART_DBG_LABEL
struct device *uart_dev = device_get_binding(UART_DEVICE_NAME);
#else
#define TX_BUF_SIZE 1000
#define RX_BUF_SIZE 500
#endif

class ThingSetStream: public ExtInterface
{
public:
#ifdef __MBED__
    ThingSetStream(Stream& s, const unsigned int c): channel(c), stream(&s) {};
#elif defined(__ZEPHYR__)
    ThingSetStream(const unsigned int c): channel(c){};
#endif

    virtual void process_asap();
    virtual void process_1s();

protected:
#ifdef __MBED__
    virtual void process_input(); // this is called from the ISR typically
#elif defined (__ZEPHYR__)
    static void process_input(void*);
#endif

    const unsigned int channel;

#ifdef __MBED__
    virtual bool readable()
    {
        return stream->readable();
    }
#elif defined (__ZEPHYR__)
    static bool readable()
    {
        // Start processing interrupts in ISR.
        // This function should be called the first thing in the ISR.
        if (!uart_irq_update(uart_dev)) {
            // The function output should always be 1.
            return false;
        }
        // Check if data is available to read.
        return uart_irq_rx_ready(uart_dev);
    }

    typedef struct {
        char *buf_req_ptr;
        size_t *req_pos_ptr;
        bool *command_flag_ptr;
    } DataStreamStruct;

    // Data structure used to pass member variables into a static callback ISR.
    // Callback registration requires the function to be static.
    // But static functions dont have access to the class members.
    // Hence these members are passed as a structure of pointers.
    DataStreamStruct data_stream = {buf_req, &req_pos, &command_flag};
#endif /* MBED */

private:
#ifdef __MBED__
    Stream* stream;
#endif /* MBED */

    char buf_resp[TX_BUF_SIZE];
    char buf_req[RX_BUF_SIZE];
    size_t req_pos = 0;
    bool command_flag = false;
};

#ifdef __MBED__

template<typename T> class ThingSetSerial: public ThingSetStream
{
public:
    ThingSetSerial(T& s, const unsigned int c): ThingSetStream(s,c), ser(s) {}

    void enable() {
        Callback<void()>  cb([this]() -> void { this->process_input();});
        ser.attach(cb);
    }

private:
    bool readable()
    {
        return ser.readable();
    }

private:
    T& ser;
};

#elif defined(__ZEPHYR__)

class ThingSetSerial: public ThingSetStream
{
public:
    ThingSetSerial(const int c): ThingSetStream(c){}

    void enable() {
        /* Configure UART ISR: Configure a Callback function */
        // Callback function to be given access to the class members using the below API.
        // These members are accessed by the ISR using a pointer to the data structure.
        uart_irq_callback_user_data_set(uart_dev, process_input, (void*)&data_stream);

        /* Enable Rx interrupt in the IER */
        uart_irq_rx_enable(uart_dev);
    }
};

#endif /* MBED or ZEPHYR*/

/*
 * Construct all global ExtInterfaces here.
 * All of these are added to the list of devices
 * for later processing in the normal operation
 */

#ifdef CONFIG_EXT_THINGSET_SERIAL

extern const int pub_channel_serial;

#ifdef __MBED__
extern Serial serial;
ThingSetSerial ts_uart(serial, pub_channel_serial);
#elif defined(__ZEPHYR__)
ThingSetSerial ts_uart(pub_channel_serial);
#endif /* MBED or ZEPHYR */

#endif /* CONFIG_EXT_THINGSET_SERIAL */

extern ThingSet ts;

void ThingSetStream::process_1s()
{
    if (ts.get_pub_channel(channel)->enabled) {
#ifdef __MBED__
        ts.pub_msg_json(buf_resp, sizeof(buf_resp), channel);
        stream->puts(buf_resp);
        stream->putc('\n');
#elif defined(__ZEPHYR__)
        int len = ts.pub_msg_json(buf_resp, sizeof(buf_resp), channel);
        for (int i = 0; i < len; i++) {
            uart_poll_out(uart_dev, buf_resp[i]);
        }
        uart_poll_out(uart_dev, '\n');
#endif /* MBED or ZREPHYR */
    }
}

void ThingSetStream::process_asap()
{
    if (command_flag) {
        // commands must have 2 or more characters
        if (req_pos > 1) {
#ifdef __MBED__
            stream->printf("Received Request (%d bytes): %s\n", strlen(buf_req), buf_req);
            ts.process((uint8_t *)buf_req, strlen(buf_req), (uint8_t *)buf_resp, sizeof(buf_resp));
            stream->puts(buf_resp);
            stream->putc('\n');
            fflush(*stream);
#elif defined(__ZEPHYR__)
            printf("Received Request (%d bytes): %s\n", strlen(buf_req), buf_req);
            int len = ts.process((uint8_t *)buf_req, strlen(buf_req), (uint8_t *)buf_resp, sizeof(buf_resp));
            for (int i = 0; i < len; i++) {
                uart_poll_out(uart_dev, buf_resp[i]);
            }
            uart_poll_out(uart_dev, '\n');
#endif /* MBED or ZEPHYR */
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
#ifdef __MBED__
void ThingSetStream::process_input()
#elif defined (__ZEPHYR__)
void ThingSetStream::process_input(void* user_data)
#endif /* MBED or ZEPHYR */
{
#ifdef __ZEPHYR__
    u8_t c;
    DataStreamStruct *data_stream_ptr = (DataStreamStruct *)user_data;
    bool command_flag = *(data_stream_ptr->command_flag_ptr);
    char *buf_req = data_stream_ptr->buf_req_ptr;
    size_t req_pos = *(data_stream_ptr->req_pos_ptr);
#endif /* __ZEPHYR__ */

    while (readable() && command_flag == false) {
#ifdef __MBED__
        int c = stream->getc();
#elif defined(__ZEPHYR__)
        uart_fifo_read(uart_dev, &c, 1);
#endif /* MBED */

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
        // backspace
        // can be done always unless there is nothing in the buffer
        else if (req_pos > 0 && c == '\b') {
            req_pos--;
        }
        // we fill the buffer up to all but 1 character
        // the last character is reserved for '\0'
        // more read characters are simply dropped, unless it is \n
        // which ends the command input and triggers processing
        // else if (req_pos < (sizeof(buf_req)-1)) {
        else if (req_pos < (RX_BUF_SIZE-1)) {
            buf_req[req_pos++] = c;
        }

#ifdef __ZEPHYR__
        *(data_stream_ptr->command_flag_ptr) = command_flag;
        *(data_stream_ptr->req_pos_ptr) = req_pos;
#endif
    }
}

#endif /* CONFIG_EXT_THINGSET_SERIAL */

#endif /* UNIT_TEST */
