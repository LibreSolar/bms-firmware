# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

target_sources(app PRIVATE
        bms_common.c
        bms_soc.c
        button.c
        data_objects.cpp
        helper.cpp
        leds.cpp
        main.cpp
)

zephyr_sources_ifdef(CONFIG_SHIELD_UEXT_OLED oled.c)
