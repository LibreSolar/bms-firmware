
#include "config.h"
#include "pcb.h"
#include "bms.h"
#include "data_objects.h"

#include "mbed.h"

Serial serial_uext(PIN_UEXT_TX, PIN_UEXT_RX, "serial_uext");

extern bool blinkOn;

extern bms_t bms;
extern Serial serial;

extern int battery_voltage;
extern int battery_current;
extern int load_voltage;
extern int cell_voltages[15];      // max. number of cells
extern float temperatures[3];
extern float SOC;

void output_serial()
{
    serial.printf("|");
    for (int i = 1; i <= 5; i++) {
        serial.printf("%i|", bms_cell_voltage(&bms, i));
    }
    serial.printf("%i|", bms_pack_voltage(&bms));
    serial.printf("%i|", bms_pack_current(&bms));
    serial.printf("%.2f|", bms_get_soc(&bms));
    serial.printf("%.1f|", bms_get_temp_degC(&bms, 1));
    //serial.printf("%.1f|", bms_getTemperatureDegC(2));
    //serial.printf("%.1f|", bms_getTemperatureDegC(3));
    serial.printf("%i|", load_voltage);
    serial.printf("%i|", bms_get_balancing_status(&bms));

    serial.printf(" \n");
}
