#!/bin/bash


# Define the base directory
base_dir="/home/peter/csse4011/Forculus-Gamboge"

# Define the build directory based on input
build_dir="$base_dir/base_node"
# build_dir="$base_dir/base"
# Ensure directories and necessary files exist
mkdir -p "$build_dir/boards"


# Print directory structure for debugging
echo "Building in directory: $build_dir"

# Run west build with the overlay file specified
echo "Running west build..."
west_build_output=$(west build -p -b nrf52dk/nrf52832 --sysbuild "$build_dir")

# Check if west build was successful
if echo "$west_build_output" | grep -q "Linking C executable"; then
    echo "Build successful. Output:"
    echo "$west_build_output"
    echo "Flashing the firmware..."
    west flash --runner jlink
    screen /dev/ttyACM0 115200 
else
    echo "Build failed. Output:"
    echo "$west_build_output"
fi
 
