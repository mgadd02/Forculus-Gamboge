cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(base_node)

# Local source files
file(GLOB local_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.c
)
# Global source files
file(GLOB global_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/rxBluetooth.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/txBluetooth.c
)

# Combine all sources
target_sources(app PRIVATE ${local_sources} ${global_sources})

# Include both local and global headers
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
