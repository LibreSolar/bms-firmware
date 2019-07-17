
#include "config.h"
#include "pcb.h"
#include "bms.h"
#include "data_objects.h"

#include "mbed.h"

Serial serial_uext(PIN_UEXT_TX, PIN_UEXT_RX, "serial_uext");

extern bool blinkOn;

extern BmsStatus bms_status;
extern Serial serial;

extern int load_voltage;

void output_serial()
{
    serial.printf("|");
    for (unsigned int i = 1; i < NUM_CELLS_MAX; i++) {
        serial.printf("%ul|", bms_status.cell_voltages[i]);
    }
    serial.printf("%lu|", bms_status.pack_voltage);
    serial.printf("%li|", bms_status.pack_current);
    serial.printf("%.2f|", (float)bms_status.soc / 100.0);
    serial.printf("%.1f|", (float)bms_status.temperatures[0] / 10.0);
    serial.printf("%i|", load_voltage);
    serial.printf("%lu|", bms_status.balancing_status);

    serial.printf(" \n");
}
