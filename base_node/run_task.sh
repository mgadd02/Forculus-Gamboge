#!/bin/bash


# Define the base directory
base_dir="/home/peter/csse4011/Forculus-Gamboge"

# Define the build directory based on input
build_dir="$base_dir/base_node"
# build_dir="$base_dir/base"
# Ensure directories and necessary files exist
mkdir -p "$build_dir/boards"

# Define the overlay file location dynamically and check if it exists
overlay_file="$build_dir/boards/nrf52dk_nrf52832.overlay"
if [ ! -f "$overlay_file" ]; then
    echo "Overlay file not found at $overlay_file"
    echo "Please ensure that the overlay file exists before running the build."
    exit 1
fi

# Print directory structure for debugging
echo "Building in directory: $build_dir"
echo "Using overlay file: $overlay_file"

# Run west build with the overlay file specified
echo "Running west build..."
west_build_output=$(west build -p -b nrf52dk/nrf52832 --sysbuild "$build_dir" -Dmcuboot_EXTRA_DTC_OVERLAY_FILE="$overlay_file" 2>&1)

# Check if west build was successful
if echo "$west_build_output" | grep -q "Linking C executable"; then
    echo "Build successful. Output:"
    echo "$west_build_output"
    echo "Flashing the firmware..."
    west flash --runner jlink
    # screen /dev/ttyACM1 115200 
else
    echo "Build failed. Output:"
    echo "$west_build_output"
fi
 
