# Copyright (c) The Libre Solar Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bq769x2.c bq769x2_interface.c)

zephyr_library_sources_ifdef(CONFIG_EMUL bq769x2_emul.c)
zephyr_include_directories_ifdef(CONFIG_EMUL .)
