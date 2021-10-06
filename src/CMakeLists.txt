# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

target_sources(app PRIVATE
        bms_common.c
        bms_soc.c
        button.c
        data_objects.cpp
        eeprom.cpp
        helper.cpp
        leds.cpp
        main.cpp
)

add_subdirectory(bq769x0)
add_subdirectory(bq769x2)
add_subdirectory(isl94202)

add_subdirectory(ext)
