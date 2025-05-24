#!/bin/bash

# Zephyr SDK (for host tools like dtc, ninja, etc.)
export ZEPHYR_SDK_INSTALL_DIR=/home/peter/zephyr_install/zephyr-sdk-0.17.0

# Use the correct toolchain variant
export ZEPHYR_TOOLCHAIN_VARIANT=espressif
export ESPRESSIF_TOOLCHAIN_PATH=$HOME/esp/tools/xtensa-esp-elf

# Define the base directory
base_dir="/home/peter/csse4011/Forculus-Gamboge"
build_dir="$base_dir/display_node"

mkdir -p "$build_dir/boards"

echo "Building in directory: $build_dir"
echo "Running west build..."
west build -p -b m5stack_core2/esp32/procpu "$build_dir"
build_result=$?

if [ $build_result -eq 0 ]; then
    echo "✅ Build successful."
    echo "Flashing the firmware..."
    west flash --runner esp32 --esp-device /dev/ttyACM0
    screen /dev/ttyACM2 115200
else
    echo "❌ Build failed with exit code $build_result."
fi
