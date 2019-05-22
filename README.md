# Libre Solar BMS firmware

Firmware for [5s](https://github.com/LibreSolar/BMS-5s) and [48V](https://github.com/LibreSolar/BMS48V) BMS boards using bq769x0 IC from Texas Instruments.

## Selection of board type

The board type is selected in `pcb.h` based on the `build_flags` setting in *platformio.ini*.

- 5s version with bq76920: `-D BMS_PCB_3_5S`
- 10s (48V PCB) with bq76930: `-D BMS_PCB_6_10S`
- 15s (48V PCB) with bq76940: `-D BMS_PCB_9_15S`

## Firmware development and flashing

See [here](http://libre.solar/docs/flashing/) for an explanation how to flash the software to your board.
