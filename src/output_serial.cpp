
#include "config.h"
#include "pcb.h"
#include "bms.h"
#include "data_objects.h"

#include "mbed.h"

Serial serial_uext(PIN_UEXT_TX, PIN_UEXT_RX, "serial_uext");

extern bool blinkOn;

extern BMS bms;
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
        serial.printf("%i|", bms.cell_voltage(i));
    }
    serial.printf("%i|", bms.pack_voltage());
    serial.printf("%i|", bms.pack_current());
    serial.printf("%.2f|", bms.get_soc());
    serial.printf("%.1f|", bms.get_temp_degC(1));
    //serial.printf("%.1f|", bms.getTemperatureDegC(2));
    //serial.printf("%.1f|", bms.getTemperatureDegC(3));
    serial.printf("%i|", load_voltage);
    serial.printf("%i|", bms.get_balancing_status());

    serial.printf(" \n");
}
