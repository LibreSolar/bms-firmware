# SPDX-License-Identifier: Apache-2.0

set(BOARD_ROOT ${CMAKE_SOURCE_DIR}/..)
set(DTS_ROOT ${CMAKE_SOURCE_DIR})

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(libre_solar_bms)

add_subdirectory(../src build/src)

add_subdirectory(../lib/thingset build/thingset)
add_subdirectory(../lib/oled-display build/oled-display)

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
