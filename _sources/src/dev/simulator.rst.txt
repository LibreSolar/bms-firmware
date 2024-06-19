Simulator
=========

Zephyr allows to run an application on a Linux or Mac hosts using the ``native_sim`` board.

The BMS firmware currently does not mock an entire BMS IC. However, the simulated application
allows to test the ThingSet communication and other basic features.

Execute the following command to run the simulated application:

.. code-block:: bash

    west build -b native_sim -t run
