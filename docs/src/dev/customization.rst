Customization
=============

This firmware is developed to allow easy addition of new boards and customization of the firmware.

Hardware-specific changes
-------------------------

In Zephyr, all hardware-specific configuration is described in the
`Devicetree <https://docs.zephyrproject.org/latest/build/dts/index.html>`_.

The file ``boards/arm/board_name/board_name.dts`` contains the default devicetree specification
(DTS) for a board. It is based on the DTS of the used MCU, which is included from the main Zephyr
repository.

In order to overwrite the default devicetree specification, so-called overlays can be used. An
overlay file can be specified via the west command line. If it is stored as ``board_name.overlay``
in the ``app`` subfolder, it will be recognized automatically when building the firmware for that
board.

Application firmware configuration
----------------------------------

For configuration of the application-specific features, Zephyr uses the
`Kconfig system <https://docs.zephyrproject.org/latest/build/kconfig/index.html>`_.

The configuration can be changed using ``west build -t menuconfig`` command or manually by changing
the prj.conf file (see ``Kconfig`` file for possible options).

Similar to DTS overlays, Kconfig can also be customized per board. Create a folder ``app/boards``
and  a file ``board_name.conf`` in that folder. The configuration from this file will be merged with
the ``prj.conf`` automatically.

Change battery capacity, cell type and number of cells in series
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

By default, the BMS is configured for LiFePO4 cells (``CONFIG_CELL_TYPE_LFP``).
Possible other pre-defined options are ``CONFIG_BAT_TYPE_NMC``, ``CONFIG_BAT_TYPE_NMC_HV`` and
``CONFIG_BAT_TYPE_LTO``.

The number of cells only has to be specified via ``CONFIG_BMS_IC_ISL94202_NUM_CELLS`` for boards
with the ISL94202 chip. For all other chips it is detected automatically.

To compile the firmware with default settings e.g. for a 24V LiFePO4 battery with a nominal capacity
of 100Ah, add the following to ``prj.conf`` or the board-specific ``.conf`` file:

.. code-block:: bash

    CONFIG_BAT_CAPACITY_AH=100
    CONFIG_CELL_TYPE_LFP=y

Configure serial interface
""""""""""""""""""""""""""

The BMS exposes its data using the `ThingSet protocol <https://libre.solar/thingset/>`_.

By default, live data is published on a serial interface in a 1 second interval.

The used serial interface can be configured via Devicetree, e.g. with the following snippet in
``board_name.overlay``:

.. code-block:: bash

    / {
        chosen {
            thingset,serial = &usart2;
	    };
    };

To disable regular data publication at startup, add the following to ``prj.conf`` or the
board-specific ``.conf`` file:

.. code-block:: bash

    CONFIG_THINGSET_REPORTING_LIVE_ENABLE_PRESET=n

The default period for data publication can be changed with the following Kconfig option:

.. code-block:: bash

    CONFIG_THINGSET_REPORTING_LIVE_PERIOD_PRESET=10

Shields for UEXT connector
--------------------------

There are some shields like an OLED display which can be connected to the UEXT connector.

See the full list of shields in the ``boards/shields`` folder.

The below example compiles the firmware with the OLED shield enabled.

.. code-block:: bash

    west build -b bms_8s50_ic@0.2 app -- -DEXTRA_CONF_FILE=oled.conf -DSHIELD=uext_oled
