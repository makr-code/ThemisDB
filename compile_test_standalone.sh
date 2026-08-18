#!/bin/bash

# Simple standalone compilation of test_coordination_concurrency.cpp with TSan

# Get the test file
TEST_FILE="tests/access_model/test_coordination_concurrency.cpp"

# Check if test file exists
if [ ! -f "$TEST_FILE" ]; then
    echo "Test file not found: $TEST_FILE"
    exit 1
fi

# Create output directory
mkdir -p tsan_build
cd tsan_build

# TSan flags
TSAN_FLAGS="-fsanitize=thread -g -O1 -fno-omit-frame-pointer"

# Try to compile with system libraries
echo "Attempting to compile with ThreadSanitizer..."
g++ -c \
    -std=c++17 \
    $TSAN_FLAGS \
    -I /home/runner/work/ThemisDB/ThemisDB/include \
    -I /home/runner/work/ThemisDB/ThemisDB/src \
    ../$TEST_FILE \
    2>&1 | tail -50

echo ""
echo "Compilation status: $?"

