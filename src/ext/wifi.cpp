/*
 * Copyright (c) 2018 Martin Jäger / Libre Solar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIT_TEST

#include "config.h"

#ifdef WIFI_ENABLED

#include <inttypes.h>

#include "load.h"
#include "thingset.h"
#include "pcb.h"
#include "ESP32.h"

UARTSerial uext_serial(PIN_UEXT_TX, PIN_UEXT_RX, 115200);

ESP32 wifi(uext_serial);
//DigitalOut wifi_enable(PIN_UEXT_SCL);
DigitalOut wifi_enable(PIN_UEXT_SSEL);

enum WifiState {
    STATE_WIFI_RESET,       // state before reset
    STATE_WIFI_INIT,        // initial state
    STATE_WIFI_CONN,        // successfully connected to WiFi AP
    STATE_LAN_CONN,         // local IP address obtained
    STATE_INTERNET_CONN,    // internet connection active
    STATE_WIFI_IDLE,        // switch off WiFi to save energy
    STATE_WIFI_ERROR
};

extern LoadOutput load;

extern ThingSet ts;
extern const int pub_channel_emoncms;

int wifi_check_ap_connected()
{
    printf("WiFi: Getting wifi module status... ");
    int status = wifi.get_conn_status();
    printf("%d\n", status);

    if (status == ESP32_STATUS_AP_CONNECTED
        || status == ESP32_STATUS_TCP_ACTIVE
        || status == ESP32_STATUS_TCP_DIS)
    {
        return 1;
    }
    else {
        return 0;
    }
}

int wifi_connect_ap()
{
    if (wifi_check_ap_connected()) {
        return 1;
    }
    else {
        printf("WiFi: Joining network with SSID and PASS... ");
        int res = wifi.join_AP(WIFI_SSID, WIFI_PASS);
        printf("%d\n", res);
        return res;
    }
}

int wifi_check_lan_connected()
{
    char buffer[30];
    printf("WiFi: Getting IP address... ");
    int res = wifi.get_IP(buffer, sizeof(buffer));
    printf("%s\n", buffer);
    return res;
}

int wifi_check_internet_connected()
{
    printf("WiFi: Ping %s... ", EMONCMS_HOST);
    int res = wifi.ping(EMONCMS_HOST);
    printf((res > 0) ? "OK\n" : "ERROR\n");
    return res;
}

void wifi_setup_internet_conn()
{
    int res = 0;

    printf("WiFi: Setting normal transmission mode... ");
    res = wifi.set_ip_mode(ESP32_IP_MODE_NORMAL);
    printf((res > 0) ? "OK\n" : "ERROR\n");

    printf("WiFi: Setting single connection mode... ");
    res = wifi.set_single();
    printf((res > 0) ? "OK\n" : "ERROR\n");
}

int wifi_send_emoncms_data()
{
    char url[800];
    int res = 0;

    printf("WiFi: Starting TCP connection to %s:%s ... ", EMONCMS_HOST, "80");
    res = wifi.start_TCP_conn(EMONCMS_HOST, "80", false);
    printf((res > 0) ? "OK\n" : "ERROR\n");

    if (res <= 0) {
        wifi.close_TCP_conn();
        return 0;
    }

    // create link
    sprintf(url, "/emoncms/input/post?node=%s&apikey=%s&json=", EMONCMS_NODE, EMONCMS_APIKEY);

    // Write the ThingSet publication message to URL end - 2 bytes and afterwards
    // overwrite two publication message start bytes "# " with correct URL
    // content again to avoid strcpy
    int ts_pos = strlen(url) - 2;
    int ts_len = ts.pub_msg_json(url + ts_pos, sizeof(url) - ts_pos - 1, pub_channel_emoncms);
    url[ts_pos] = 'n';
    url[ts_pos+1] = '=';
    url[ts_pos+ts_len] = '\0';  // ThingSet does not null-terminate
    //printf("%s\n", url);

    wait(0.1);
    printf("WiFi: Sending data... ");
    res = wifi.send_URL(url, (char *)EMONCMS_HOST);
    printf((res > 0) ? "OK\n" : "ERROR\n");

    //wifi.close_TCP_conn();

    return 0;
}

int wifi_reset(void)
{
    int res = 0;

    wifi_enable = 0;
    wait(0.5);
    wifi_enable = 1;

    printf("WiFi: Resetting wifi module... ");
    res = wifi.reset();
    printf((res > 0) ? "OK\n" : "ERROR\n");

    //printf("WiFi: ESP8266 module firmware... \n");
    //wifi.print_firmware();

    printf("WiFi: Setting wifi station mode... ");
    res = wifi.set_wifi_mode(ESP32_WIFI_MODE_STATION);
    printf((res > 0) ? "OK\n" : "ERROR\n");

    //printf("WiFi: Listing APs... \n");
    //char buf[1000];
    //wifi.list_APs(buf, sizeof(buf));
    //printf(buf);

    return res;
}

// implement specific extension inherited from ExtInterface
class ExtWifi: public ExtInterface
{
    public:
        ExtWifi() {};
        void enable();
        void process_1s();
};

static ExtWifi ext_wifi; // local instance, will self register itself

void ExtWifi::enable()
{
#ifdef PIN_UEXT_DIS
    DigitalOut uext_dis(PIN_UEXT_DIS);
    uext_dis = 0;
#endif
}

void ExtWifi::process_1s()
{
    static WifiState state = STATE_WIFI_RESET;
    static int error_counter = 0;

    if (!load.usb_pgood) {
        wifi_enable = 0;
        state = STATE_WIFI_IDLE;
    }

    if (uptime() % 10 == 0) {

        printf("WiFi state: %d, error counter: %d\n", state, error_counter);

        switch (state) {
            case STATE_WIFI_RESET:
                if (wifi_reset() > 0) {
                    state = STATE_WIFI_INIT;
                }
                else {
                    state = STATE_WIFI_ERROR;
                    printf("WiFi: ESP module not recognized. Going to error state.\n");
                }
                break;
            case STATE_WIFI_INIT:
                if (wifi_connect_ap()) {
                    error_counter = 0;
                    state = STATE_WIFI_CONN;
                    break;
                }
                else if (error_counter > 10) {    // after 10 attempts go back to previous state
                    error_counter = 0;
                    state = STATE_WIFI_RESET;
                }
                error_counter++;
                break;
            case STATE_WIFI_CONN:
                if (wifi_check_lan_connected()) {   // check if already got IP address
                    wifi_setup_internet_conn();
                    error_counter = 0;
                    state = STATE_LAN_CONN;
                    break;
                }
                else if (error_counter > 10) {    // after 10 attempts go back to previous state
                    error_counter = 0;
                    state = STATE_WIFI_INIT;
                }
                error_counter++;
                break;
            case STATE_LAN_CONN:
                if (wifi_check_internet_connected()) {
                    error_counter = 0;
                    state = STATE_INTERNET_CONN;
                    break;
                }
                else if (error_counter > 10) {    // after 10 attempts go back to previous state
                    error_counter = 0;
                    state = STATE_WIFI_CONN;
                }
                error_counter++;
                break;
            case STATE_INTERNET_CONN:
                if (wifi_check_internet_connected()) {  // still connected?
                    error_counter = 0;
#ifdef EMONCMS_ENABLED
                    wifi_send_emoncms_data();
#endif
                    break;
                }
                else if (error_counter > 10) {    // after 10 attempts go back to previous state
                    error_counter = 0;
                    state = STATE_WIFI_CONN;
                }
                error_counter++;
                break;
            case STATE_WIFI_ERROR:
                if (error_counter > 240) {    // after approx 1h try again
                    error_counter = 0;
                    state = STATE_WIFI_RESET;
                }
                error_counter++;
                break;
            case STATE_WIFI_IDLE:
                if (load.usb_pgood) {
                    state = STATE_WIFI_RESET;
                }
                break;
        }

    }
}

#endif /* WIFI_ENABLED */

#endif /* UNIT_TEST */
