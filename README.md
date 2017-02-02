# BMS_24V_software
Firmware for 24/48V BMS board using bq76930 (max. 10s cells)

---

This is a development fork of the firmware for the LibreSolar BMS. See the project at http://libre.solar
which adds some extensions:

- support for SSD1306 OLED display on I2C-bus

- continuos output of logging samples via USART TX/RX on the Cortex SWD connector of the BMS. 
It can be used with a ST-LINK/V2-1 debugger/programmer of a Nucleo board (F072RB) for updating and debugging 
the firmware and for transmitting the logging-data.


