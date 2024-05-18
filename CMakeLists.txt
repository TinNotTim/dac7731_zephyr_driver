# Copyright (c) 2024, Tin Chiang.
# SPDX-License-Identifier: Apache-2.0

# Follow the example: https://github.com/nobodyguy/HX711_zephyr_driver/blob/main
# Out-of-tree drivers for existing driver classes

set(DAC7731_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE PATH "DAC7731 module root directory")

# Subdirectories
add_subdirectory(drivers)

# Include headers
zephyr_include_directories(drivers)