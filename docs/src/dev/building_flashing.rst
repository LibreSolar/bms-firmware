Building and Flashing
=====================

As the ``main`` branch may contain unstable code, make sure to select the desired release branch
(see GitHub for a list of releases and branches):

.. code-block:: bash

    git switch <your-release>-branch
    west update

Boards with ESP32 MCU
"""""""""""""""""""""

+-----------------------------+----------------------+----------------+
| Board with ESP32 MCU        | board-name           | revision(s)    |
+=============================+======================+================+
| Libre Solar BMS C1          | bms_c1               | 0.4, 0.3       |
+-----------------------------+----------------------+----------------+

The ESP32 requires additional steps to get required binary blobs:

.. code-block:: bash

    west blobs fetch hal_espressif

Afterwards you can build and flash the firmware using the board name and revision from the table
above:

.. code-block:: bash

    west build -b <board-name>@<revision>
    west flash

We recommend ``picocom`` as a serial console under Linux. Add ``--lower-dtr`` in order
to reset the board before connecting, so that the entire log output during boot can be seen.

.. code-block:: bash

    picocom /dev/ttyACM0 --lower-dtr

Press Ctrl+a followed by q to exit.

Boards with STM32 MCU
"""""""""""""""""""""

+-----------------------------+----------------------+----------------+
| Board with STM32 MCU        | board-name           | revision(s)    |
+=============================+======================+================+
| Libre Solar BMS 5S50 SC     | bms_5s50_sc          | 0.1            |
+-----------------------------+----------------------+----------------+
| Libre Solar BMS 15S80 SC    | bms_15s80_sc         | 0.1            |
+-----------------------------+----------------------+----------------+
| Libre Solar BMS 16S100 SC   | bms_16s100_sc        | 0.2            |
+-----------------------------+----------------------+----------------+
| Libre Solar BMS 8S50 IC     | bms_8s50_ic          | 0.2, 0.1       |
+-----------------------------+----------------------+----------------+
| Libre Solar BMS-8S50-IC *   | bms_8s50_ic_f072     | 0.1            |
+-----------------------------+----------------------+----------------+

(*) Only for BMS-8S50-IC with STM32F072 MCU.

Supported hardware is further described in :doc:`../supported_hardware`.

Build and flash the firmware using the board name and revision from the table above:

.. code-block:: bash

    west build -b <board-name>@<revision>
    west flash

The revision can be omitted if only a single board revision is available or if
the default (most recent) version should be used. See also
`here <https://docs.zephyrproject.org/latest/application/index.html#application-board-version>`_
for more details regarding board revision handling in Zephyr.

For flashing with a specific debug probe (runner), e.g. J-Link, use the following command:

.. code-block:: bash

    west flash -r jlink

We recommend ``picocom`` as a serial console under Linux:

.. code-block:: bash

    picocom -b 115200 /dev/ttyACM0

Press Ctrl+a followed by q to exit.
