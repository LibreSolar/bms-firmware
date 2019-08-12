# SPDX-License-Identifier: Apache-2.0

set(BOARD bms8s50ic_l452cx)
set(BOARD_ROOT ${CMAKE_SOURCE_DIR})

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(libre_solar_bms)

add_definitions(-DBMS_8S50_IC)
include_directories(src)

target_sources(app PRIVATE src/main_zephyr.cpp)
target_sources(app PRIVATE src/uext/oled_zephyr.cpp)
target_sources(app PRIVATE src/bms_common.c)
target_sources(app PRIVATE src/bms_isl94202.c)
target_sources(app PRIVATE src/isl94202_hw.c)
