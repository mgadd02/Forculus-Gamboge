#!/bin/bash


# Define the base directory
base_dir="~/csse4011/repo/mycode/apps/project/Forculus-Gamboge"

# Define the build directory based on input
build_dir="$base_dir/admin_node"
# Ensure directories and necessary files exist
mkdir -p "$build_dir/boards"



# Print directory structure for debugging
echo "Building in directory: $build_dir"

# Run west build with the overlay file specified
echo "Running west build for prac$prac_num..."
west_build_output=$(west build -p -b m5stack_core2/esp32/procpu --sysbuild "$build_dir" 2>&1)

# Check if west build was successful
if echo "$west_build_output" | grep -q "Linking C executable"; then
    echo "Build successful. Output:"
    echo "$west_build_output"
    echo "Flashing the firmware..."
    west flash --runner jlink
else
    echo "Build failed. Output:"
    echo "$west_build_output"
fi
 