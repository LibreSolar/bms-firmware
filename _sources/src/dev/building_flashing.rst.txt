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

Device Firmware Upgrade (DFU) over CAN
""""""""""""""""""""""""""""""""""""""

For boards with large enough flash it is possible to enable support for upgrading the firmware over
CAN (using ThingSet as the transport protocol). This has been tested with the BMS C1.

First of all, the firmware has to be built with MCUboot support (using ``--sysbuild`` parameter):

.. code-block:: bash

    rm -rf build
    west build -b <board-name>@<revision> --sysbuild
    west flash

With that firmware flashed to the board, the CAN communication needs to be set up on the Linux host
computer (replace ``can0`` with your interface name):

.. code-block:: bash

    sudo ip link set can0 up type can bitrate 500000 restart-ms 500

Test the CAN communication with ``candump can0``. You should see some published messages on the bus.

For sending a new firmware image, use the Python script provided by the ThingSet SDK:

.. code-block:: bash

    ../thingset-zephyr-sdk/scripts/thingset-dfu-can.py -c can0 -t 0x01 build/app/zephyr/zephyr.signed.bin

The node address on the CAN bus is printed on the console during boot-up. If no other device is on
the bus, it ends up with ``0x01``. It can also be determined from the ``candump`` output. The source
address of the published messages (which becomes the target address for the firmware upgrade) is the
least-significant byte of the CAN ID (first byte from the right).

See also the `ThingSet specification <https://thingset.io>`_ for further details regarding the CAN
protocol.

If everything worked correctly, you should see a progress bar on the console showing the firmware
upgrade:

.. code-block:: bash

    Initializing DFU
    Flashing |################################| 322/322 KiB = 100%
    Finishing DFU
