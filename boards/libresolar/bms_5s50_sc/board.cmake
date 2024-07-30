# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=STM32F072CB" "--speed=4000" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
