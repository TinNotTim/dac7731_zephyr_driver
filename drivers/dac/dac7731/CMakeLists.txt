# Copyright (c) 2024, Tin Chiang.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_DAC7731 dac7731.c)
