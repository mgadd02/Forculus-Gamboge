cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr)
project(admin_node)

# Collect all files in apps/prac2/src
FILE(GLOB app_sources src/*.c)

# Collect all files in mylib 
FILE(GLOB lib_sources lib/*.c)

# Tell CMake to build with the app and lib sources
target_sources(app PRIVATE ${app_sources} ${lib_sources})

# Tell CMake where our header files are
target_include_directories(app PRIVATE ./include)
