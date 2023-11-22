Supported Hardware
==================

Boards
------

The software is easily configurable to support different BMS boards with STM32 and ESP32 MCUs:

+--------------------------------+----------------------+------------+----------------+
| Board                          | MCU                  | IC         | Revisions      |
+================================+======================+============+================+
| Libre Solar `BMS-5S50-SC`_     | STM32F072            | bq76920    | 0.1            |
+--------------------------------+----------------------+------------+----------------+
| Libre Solar `BMS-15S80-SC`_    | STM32F072            | bq76930/40 | 0.1            |
+--------------------------------+----------------------+------------+----------------+
| Libre Solar `BMS-16S100-SC`_   | STM32G0B1            | BQ76952    | 0.2            |
+--------------------------------+----------------------+------------+----------------+
| Libre Solar `BMS-C1`_          | ESP32-C3             | BQ76952    | 0.4, 0.3       |
+--------------------------------+----------------------+------------+----------------+
| Libre Solar `BMS-8S50-IC`_ *   | STM32L452            | ISL94202   | 0.2, 0.1       |
+--------------------------------+----------------------+------------+----------------+

(*) Revision 0.1 of this board is also available with STM32F072 MCU.

.. _BMS-5S50-SC: https://github.com/LibreSolar/bms-5s50-sc
.. _BMS-15S80-SC: https://github.com/LibreSolar/bms-15s80-sc
.. _BMS-16S100-SC: https://github.com/LibreSolar/bms-16s100-sc
.. _BMS-C1: https://github.com/LibreSolar/bms-c1
.. _BMS-8S50-IC: https://github.com/LibreSolar/bms-8s50-ic

BMS ICs
-------

The firmware allows to use different BMS monitoring ICs, which usually provide single-cell voltage monitoring, balancing, current monitoring and protection features.

The chips currently supported by the firmware are listed below.

+--------------------+------------------+---------+-----------------+
| Manufacturer       | Chip / Datasheet | # Cells | Status          |
+====================+==================+=========+=================+
| Texas Instruments  | `bq76920`_       |   3s-5s | full support    |
+--------------------+------------------+---------+-----------------+
| Texas Instruments  | `bq76930`_       |  6s-10s | full support    |
+--------------------+------------------+---------+-----------------+
| Texas Instruments  | `bq76940`_       |  9s-15s | full support    |
+--------------------+------------------+---------+-----------------+
| Texas Instruments  | `BQ76952`_       |  3s-16s | full support    |
+--------------------+------------------+---------+-----------------+
| Renesas / Intersil | `ISL94202`_      |   3s-8s | full support    |
+--------------------+------------------+---------+-----------------+

.. _bq76920: https://www.ti.com/lit/ds/symlink/bq76920.pdf
.. _bq76930: https://www.ti.com/lit/ds/symlink/bq76930.pdf
.. _bq76940: https://www.ti.com/lit/ds/symlink/bq76940.pdf
.. _BQ76952: https://www.ti.com/lit/ds/symlink/bq76952.pdf
.. _ISL94202: https://www.renesas.com/us/en/document/dst/isl94202-datasheet
