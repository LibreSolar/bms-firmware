# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

target_sources(app PRIVATE
        bms_bq769x0.c
        bms_common.c
        bms_isl94202.c
        bq769x0_interface.cpp
        button.c
        data_nodes.cpp
        eeprom.cpp
        helper.cpp
        isl94202_interface.c
        leds.cpp
        main.cpp
)

add_subdirectory(ext)
