cmake_minimum_required(VERSION 3.13.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C CXX)
target_sources(app PRIVATE
        src/main.cpp
)
target_include_directories(
        app
        PRIVATE
        include
)