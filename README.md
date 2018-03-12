# Libre Solar BMS software

Firmware for [5s](https://github.com/LibreSolar/BMS-5s) and [48V](https://github.com/LibreSolar/BMS48V) BMS boards using bq769x0 IC from Texas Instruments.

## Selection of board type

The board type is defined in the file *config.h*:

For 5s version:
```
#define BMS_PCB_5S
```

For 5s version:
```
#define BMS_PCB_48V
```

## Firmware development and flashing

See [here](http://libre.solar/docs/flashing/) for an explanation how to flash the software to your board.
