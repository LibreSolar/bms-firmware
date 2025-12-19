Unit Tests
==========

Writing the tests is still work in progress. New functions should be implemented in test-driven
development fashion. Tests for old functions will be added step by step.

The tests use Zephyr's Ztest framework and the twister tool. Build and run the tests with the
following command:

.. code-block:: bash

    ../zephyr/scripts/twister -T ./tests --integration --inline-logs -n
