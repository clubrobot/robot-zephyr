cmake_minimum_required(VERSION 3.21)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES CXX)

add_subdirectory(drivers)
include_directories(include)

zephyr_include_directories(drivers)
zephyr_include_directories(include)