zephyr_library()
zephyr_library_include_directories(./)
zephyr_library_sources(beacon.c platform.c vl53l5cx_api.c)