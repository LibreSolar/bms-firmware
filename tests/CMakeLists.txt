# Copyright (c) The Libre Solar Project Contributors
# SPDX-License-Identifier: Apache-2.0

set(BOARD_ROOT ${CMAKE_SOURCE_DIR}/..)
set(DTS_ROOT ${CMAKE_SOURCE_DIR}/..)

set(BOARD "native_posix")

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(libre_solar_bms_tests)

add_definitions(-DUNIT_TEST=1)
add_definitions(-DUNITY_OUTPUT_COLOR)
add_definitions(-DUNITY_EXCLUDE_SETJMP)

add_subdirectory(../app/src build/app)

zephyr_include_directories(./src)
zephyr_include_directories(../app/src)

zephyr_include_directories(../../modules/thingset/src)
zephyr_include_directories(../../modules/unity/src)

target_sources(app PRIVATE
        ../../modules/unity/src/unity.c
        src/main.cpp
        src/bq769x0_interface_stub.c
        src/bq769x0_tests.cpp
        src/bq769x2_interface_stub.c
        src/bq769x2_tests_functions.cpp
        src/bq769x2_tests_interface.cpp
        src/common_tests.cpp
        src/helper_tests.cpp
        src/isl94202_interface_stub.c
        src/isl94202_tests.cpp
)

# determine git tag and commit hash for automatic firmware versioning
find_package(Git)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --long --dirty --tags
        OUTPUT_VARIABLE FIRMWARE_VERSION_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    set(FIRMWARE_VERSION_ID "unknown")
endif()
add_definitions(-DFIRMWARE_VERSION_ID="${FIRMWARE_VERSION_ID}")
