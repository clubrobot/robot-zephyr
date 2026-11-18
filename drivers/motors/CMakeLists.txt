# SPDX-License-Identifier: AGPL-3.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_DRV8844 DRV8844/DRV8844.cpp)

