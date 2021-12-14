Unit Tests
==========

Writing the tests is still work in progress. New functions should be implemented in test-driven
development fashion. Tests for old functions will be added step by step.

Build and run the tests with the following command:

.. code-block:: bash

    cd tests
    west build -p -b native_posix -t run

By default, the unit tests are run for the ISL94202 IC, but you can select other ICs by specifying
an overlay located in ``tests/boards``.

.. code-block:: bash

    west build -p -b native_posix -t run -- -DDTC_OVERLAY_FILE="boards/bq769x0.overlay"
