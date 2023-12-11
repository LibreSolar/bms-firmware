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

add_subdirectory_ifdef(CONFIG_BQ769X0   bq769x0)
add_subdirectory_ifdef(CONFIG_BQ769X2   bq769x2)
add_subdirectory_ifdef(CONFIG_ISL94202  isl94202)
