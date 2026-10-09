# SPDX-License-Identifier: MIT
zephyr_library()
zephyr_library_include_directories(${APPLICATION_SOURCE_DIR}/include)
zephyr_library_sources_ifdef(CONFIG_GRIDROLLDG_PAW3222 drivers/input/paw3222.c drivers/input/paw3222_sdio.c)
zephyr_library_sources_ifdef(CONFIG_GRIDROLLDG_WHEEL drivers/behavior_wheel.c)
