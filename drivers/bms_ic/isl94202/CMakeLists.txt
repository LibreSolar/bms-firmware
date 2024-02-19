# Copyright (c) The Libre Solar Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(isl94202.c isl94202_interface.c)

zephyr_library_sources_ifdef(CONFIG_EMUL isl94202_emul.c)
zephyr_include_directories_ifdef(CONFIG_EMUL .)
