
Workspace Setup
===============

This guide assumes you have already installed the Zephyr SDK and the ``west`` tool according to the
`Zephyr documentation <https://docs.zephyrproject.org/latest/getting_started/>`_.

Below commands initialize a new workspace and pull all required source files:

.. code-block:: bash

    # create a new west workspace and pull the BMS firmware
    west init -m https://github.com/LibreSolar/bms-firmware west-workspace
    cd west-workspace/bms-firmware

    # pull Zephyr upstream repository and modules (may take a while)
    west update

Afterwards, most important folders in your workspace will look like this:

.. code-block:: bash

    west-workspace/         # contains .west/config
    │
    ├── bms-firmware/       # application firmware repository
    │   ├── app/            # application source files
    │   ├── boards/         # board specifications
    │   ├── tests/          # unit test source files
    │   └── west.yml        # main manifest file
    │
    ├── modules/            # modules imported by Zephyr and BMS firmware
    |
    ├── tools/              # tools used by Zephyr
    │
    └── zephyr/             # upstream Zephyr repository

If you already have a west workspace set up (e.g. for other Libre Solar firmware), you can also
re-use it to avoid having many copies of upstream Zephyr and modules:

.. code-block:: bash

    # go to your workspace directory
    cd your-zephyr-workspace

    # pull the BMS firmware
    git clone https://github.com/LibreSolar/bms-firmware
    cd bms-firmware

    # re-configure and update the workspace
    # (to be done each time you switch between applications in same workspace)
    west config manifest.path bms-firmware
    west update
