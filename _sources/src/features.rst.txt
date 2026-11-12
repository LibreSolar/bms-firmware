Features
========

Amongst all features inherited from underlying Zephyr, the BMS has the following features:

- Monitoring and configuration using the `ThingSet`_ protocol (mapping to MQTT, CoAP and HTTP
  possible)

  - Serial interface
  - CAN bus
  - Bluetooth (not available on all boards)

- Extensions via `UEXT connector`_ possible (depending on hardware)

  - I2C
  - SPI
  - UART

- SOC estimation based on coulomb counting

- Configuration options

  - Pack layout

    - Cell chemistry (e.g. LiFePO4, NMC, NCA, LTO)
    - Nominal capacity
    - Number of cells
    - Thermistor type
    - Shunt resistor
    - Custom open circuit voltage (OCV) look-up table

  - Protection

    - Discharge short circuit limit (A)
    - Discharge short circuit delay (us)
    - Discharge over-current limit (A)
    - Discharge over-current delay (ms)
    - Charge over-current limit (A)
    - Charge over-current delay (ms)
    - Cell target charge voltage (V)
    - Cell discharge voltage limit (V)
    - Cell over-voltage limit (V)
    - Cell over-voltage error reset threshold (V)
    - Cell over-voltage delay (ms)
    - Cell under-voltage limit (V)
    - Cell under-voltage error reset threshold (V)
    - Cell under-voltage delay (ms)
    - Discharge over-temperature (DOT) limit (°C)
    - Discharge under-temperature (DUT) limit (°C)
    - Charge over-temperature (COT) limit (°C)
    - Charge under-temperature (CUT) limit (°C)
    - Temperature limit hysteresis (°C)

  - Balancing

    - Enable automatic balancing.
    - Balancing cell voltage target difference (V)
    - Minimum cell voltage to start balancing (V)
    - Current threshold to be considered idle (A)
    - Minimum idle duration before balancing (s)


.. _ThingSet: https://thingset.io
.. _UEXT connector: https://en.wikipedia.org/wiki/UEXT
