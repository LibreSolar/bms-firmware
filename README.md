# Libre Solar BMS Firmware

![build badge](https://github.com/LibreSolar/bms-firmware/actions/workflows/zephyr.yml/badge.svg)

This repository contains the firmware for Libre Solar Battery Management Systems based on [Zephyr RTOS](https://www.zephyrproject.org/) .

## Development and release model

The `main` branch is used for ongoing development of the firmware.

Releases are created from `main` after significant updates have been introduced to the firmware. Each release has to pass tests with multiple boards.

A release is tagged with a version number consisting of the release year and a release count for that year (starting at zero). For back-porting of bug-fixes, a branch named after the release followed by `-branch` is created, e.g. `v21.0-branch`.

## Documentation

The firmware documentation including build instructions and API reference can be found under [libre.solar/bms-firmware](https://libre.solar/bms-firmware/).

In order to build the documentation locally you need to install Doxygen, Sphinx and Breathe and run `make html` in the `docs` folder.

## License

This firmware is released under the [Apache-2.0 License](LICENSE).
