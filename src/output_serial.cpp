
#include "config.h"
#include "pcb.h"
#include "bms.h"
#include "data_objects.h"

#include "mbed.h"

Serial serial_uext(PIN_UEXT_TX, PIN_UEXT_RX, "serial_uext");

extern bool blinkOn;

extern BmsStatus bms_status;
extern Serial serial;

extern float load_voltage;

void output_serial()
{
    serial.printf("|");
    for (unsigned int i = 1; i < NUM_CELLS_MAX; i++) {
        serial.printf("%.2f|", bms_status.cell_voltages[i]);
    }
    serial.printf("%.2f|", bms_status.pack_voltage);
    serial.printf("%.2f|", bms_status.pack_current);
    serial.printf("%d|", bms_status.soc);
    serial.printf("%.1f|", bms_status.temperatures[0]);
    serial.printf("%.1f|", load_voltage);
    serial.printf("%lu|", bms_status.balancing_status);

    serial.printf(" \n");
}
