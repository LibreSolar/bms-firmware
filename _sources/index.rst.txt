======================================
Libre Solar BMS Firmware Documentation
======================================

This documentation describes the firmware based on `Zephyr RTOS`_ for different Libre Solar Battery
Management Systems.

The firmware is under ongoing development. Most recent version can be found on
`GitHub <https://github.com/LibreSolar/bms-firmware>`_.

This documentation is licensed under the Creative Commons Attribution-ShareAlike 4.0 International
(CC BY-SA 4.0) License.

.. image:: static/images/cc-by-sa-centered.png

The full license text is available at `<https://creativecommons.org/licenses/by-sa/4.0/>`_.

.. _Zephyr RTOS: https://zephyrproject.org

.. toctree::
    :caption: Overview
    :hidden:

    src/features
    src/supported_hardware

.. toctree::
    :caption: Development
    :hidden:

    src/dev/workspace_setup
    src/dev/building_flashing
    src/dev/customization
    src/dev/simulator
    src/dev/unit_tests

.. toctree::
    :caption: API Reference
    :hidden:

    src/api/bms
    src/api/bms_ic
    src/api/data_objects
    src/api/misc
