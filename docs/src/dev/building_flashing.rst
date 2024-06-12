Building and Flashing
=====================

As the ``main`` branch may contain unstable code, make sure to select the desired release branch
(see GitHub for a list of releases and branches):

.. code-block:: bash

    git switch <your-release>-branch
    west update

The ``app`` subfolder contains all application source files and the CMake entry point to build the
firmware, so we go into that directory first.

.. code-block:: bash

    cd app

Boards with ESP32 MCU
"""""""""""""""""""""

The ESP32 requires additional steps to get required binary blobs:

.. code-block:: bash

    west blobs fetch hal_espressif

Afterwards you can build and flash the firmware the same way as for STM32:

.. code-block:: bash

    west build -b <board-name>@<revision>
    west flash

+--------------------------------+----------------------+----------------+
| Board with ESP32 MCU           | board-name           | revision       |
+================================+======================+================+
| Libre Solar `BMS-C1`_          | bms_c1               | 0.4, 0.3       |
+--------------------------------+----------------------+----------------+

For monitoring the serial monitor built into the Espressif toolchain can be used with:

.. code-block:: bash

    west espressif monitor

Press Ctrl+T followed by X to exit.

Boards with STM32 MCU
"""""""""""""""""""""

Initial board selection (see ``boards`` subfolder for correct names):

.. code-block:: bash

    west build -b <board-name>@<revision>

``<board-name>`` should be one of the listed board in the table below.
The appended ``@<revision>`` specifies the board version.
Revision can be omitted if only a single board revision is available or if
the default (most recent) version should be used. See also
`here <https://docs.zephyrproject.org/latest/application/index.html#application-board-version>`_
for more details regarding board revision handling in Zephyr.

+--------------------------------+----------------------+----------------+
| Board with STM32 MCU           | board-name           | revision       |
+================================+======================+================+
| Libre Solar `BMS-5S50-SC`_     | bms_5s50_sc          | 0.1            |
+--------------------------------+----------------------+----------------+
| Libre Solar `BMS-15S80-SC`_    | bms_15s80_sc         | 0.1            |
+--------------------------------+----------------------+----------------+
| Libre Solar `BMS-16S100-SC`_   | bms_16s100_sc        | 0.2            |
+--------------------------------+----------------------+----------------+
| Libre Solar `BMS-8S50-IC`_     | bms_8s50_ic          | 0.2, 0.1       |
+--------------------------------+----------------------+----------------+
| Libre Solar `BMS-8S50-IC`_ *   | bms_8s50_ic_f072     | 0.1            |
+--------------------------------+----------------------+----------------+

(*) Only for BMS-8S50-IC with STM32F072 MCU.
Supported hardware is further described in :doc:`../supported_hardware`. 

Flash with specific debug probe (runner), e.g. J-Link:

.. code-block:: bash

    west flash -r jlink

