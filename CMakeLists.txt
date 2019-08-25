# SPDX-License-Identifier: Apache-2.0

set(BOARD bms8s50ic_l452cx)
set(BOARD_ROOT ${CMAKE_SOURCE_DIR})

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(libre_solar_bms)

add_definitions(-DBMS_8S50_IC -DZEPHYR)
include_directories(src)
include_directories(.pio/libdeps/bms-8s50-ic/ThingSet/src)

target_sources(app PRIVATE src/main_zephyr.cpp)
target_sources(app PRIVATE src/uext/oled_zephyr.cpp)
target_sources(app PRIVATE src/bms_common.c)
target_sources(app PRIVATE src/bms_isl94202.c)
target_sources(app PRIVATE src/isl94202_interface.c)
target_sources(app PRIVATE src/thingset_serial.cpp)
target_sources(app PRIVATE src/leds.cpp)
target_sources(app PRIVATE src/data_objects.cpp)
target_sources(app PRIVATE src/helper.c)
target_sources(app PRIVATE .pio/libdeps/bms-8s50-ic/ThingSet/src/thingset.cpp)
target_sources(app PRIVATE .pio/libdeps/bms-8s50-ic/ThingSet/src/thingset_cbor.cpp)
target_sources(app PRIVATE .pio/libdeps/bms-8s50-ic/ThingSet/src/cbor.c)
target_sources(app PRIVATE .pio/libdeps/bms-8s50-ic/ThingSet/src/thingset_json.cpp)
target_sources(app PRIVATE .pio/libdeps/bms-8s50-ic/ThingSet/src/jsmn.c)
