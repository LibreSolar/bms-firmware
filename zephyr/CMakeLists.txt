# SPDX-License-Identifier: Apache-2.0

set(BOARD_ROOT ${CMAKE_SOURCE_DIR}/..)

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(libre_solar_bms)

include_directories(../src)

target_sources(app PRIVATE ../src/main_zephyr.cpp)
target_sources(app PRIVATE ../src/bms_common.c)
target_sources(app PRIVATE ../src/bms_bq769x0.c)
target_sources(app PRIVATE ../src/bms_isl94202.c)
target_sources(app PRIVATE ../src/isl94202_interface.c)
target_sources(app PRIVATE ../src/bq769x0_interface.cpp)
target_sources(app PRIVATE ../src/leds.cpp)
target_sources(app PRIVATE ../src/data_objects.cpp)
target_sources(app PRIVATE ../src/helper.cpp)

target_sources(app PRIVATE ../src/ext/ext.cpp)
target_sources(app PRIVATE ../src/ext/serial.cpp)
target_sources(app PRIVATE ../src/ext/oled.cpp)
target_sources(app PRIVATE ../src/ext/can_msg_queue.cpp)
target_sources(app PRIVATE ../src/ext/can.cpp)

add_subdirectory(../lib/thingset build/thingset)
add_subdirectory(../lib/oled-display build/oled-display)
