/*
 * Copyright (c) 2018 Martin Jäger / Libre Solar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIT_TEST

#include "config.h"

#ifdef CONFIG_EXT_VZLOGGER_ENABLED   // otherwise don't compile code to reduce firmware size

#include "ext/ext.h"

#include <string.h>
#include <stdio.h>


#ifdef __MBED__
    #include "mbed.h"
    #include "ext/ext.h"
    #include "bms.h"
    #include "pcb.h"
    Serial serial_uext(PIN_UEXT_TX, PIN_UEXT_RX, "serial_uext");
//    extern Serial serial_uext;
    extern BmsStatus bms_status;
    extern BmsConfig bms_conf;

#elif defined(__ZEPHYR__)
    #include <zephyr.h>
    #include <sys/printk.h>
    #include <drivers/uart.h>

    #include "thingset.h"
    #include "data_nodes.h"
    struct device *uart_uext_dev = device_get_binding(DT_ALIAS_UART_UEXT_LABEL);

#endif


class ExtVZlogger: public ExtInterface
{
public:
    ExtVZlogger(){};
    void enable() {
        serial_uext.baud(19200); // (xsider) for optocoupler
    }

    void process_1s();


};

/*
 * Construct all global ExtInterfaces here.
 * All of these are added to the list of devices
 * for later processing in the normal operation
 */
static ExtVZlogger ExtInterface;


void ExtVZlogger::process_1s()
{

#if defined(__MBED__)

    /* set a suitable header for BMS (xsider) */
    #ifdef CONFIG_BOARD_BMS_5S50_SC
        serial_uext.printf("/BMS5_P0090-07-j_%.2fV_%2.1fAh\r\n", bms_conf.cell_chg_voltage ,bms_conf.nominal_capacity_Ah);
    #endif
    #ifdef CONFIG_BOARD_BMS_15S80_SC
        serial_uext.printf("/BMS9_P0090-07-j_%.2fV_%2.1fAh\r\n", bms_conf.cell_chg_voltage ,bms_conf.nominal_capacity_Ah);
    #endif
    for (int i = 0; i < NUM_CELLS_MAX; i++) {
        serial_uext.printf("1-%i:12.7.0*255(%.2f*V)\n", i+1, bms_status.cell_voltages[i]);
    }
    serial_uext.printf("1-0:12.7.0*255(%.2f*V)\n", bms_status.pack_voltage );   //  voltage
    serial_uext.printf("1-0:11.7.0*255(%.2f*A)\n", bms_status.pack_current ); // caluclate current with division by zero protection
    serial_uext.printf("1-1:97.7.0*255(%.1f*C)\n", bms_status.bat_temp_avg);
    serial_uext.printf("1-0:98.7.0*255(%d*%%)\n", bms_status.soc);
    serial_uext.printf("1-0:99.7.0*255(%i*)\n", bms_status.balancing_status);
    serial_uext.printf("1-0:0.7.0*255(%i)\n", (bms_status.empty<<3) | (bms_status.full<<2) );
    serial_uext.printf("!\n");
#elif defined(__ZEPHYR__)

/* @TODO working printf replacement */
//    strcpy(buf_resp,"Test\n");
//    int len = sizeof(buf_resp);
//    for (int i = 0; i < len; i++) {
//        uart_poll_out(uart_uext_dev, buf_resp[i]);
//    }
//    uart_poll_out(uart_uext_dev, '+');

#endif
}

#endif /* CONFIG_EXT_THINGSET_SERIAL */

#endif /* UNIT_TEST */
