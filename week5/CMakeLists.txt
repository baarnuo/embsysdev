# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test C CXX)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)

target_sources(app PRIVATE 
    src/main.c
    ../parser/TimeParser.cpp
)
