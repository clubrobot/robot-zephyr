cmake_minimum_required(VERSION 3.13.0)

if (NOT DEFINED BOARD)
    set(BOARD nucleo_h563zi)
endif ()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C CXX)

target_sources(app PRIVATE src/main.c)