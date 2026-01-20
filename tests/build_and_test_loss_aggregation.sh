#!/bin/bash
# Build and Test Script for PR #757: Loss Aggregation
# This script builds the distributed training coordinator tests and runs them

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build-loss-test"

# Color codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo "=========================================="
echo "Building Loss Aggregation Tests"
echo "=========================================="
echo ""

# Check prerequisites
echo "Checking prerequisites..."
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}ERROR: cmake not found${NC}"
    exit 1
fi

if ! command -v ninja &> /dev/null && ! command -v make &> /dev/null; then
    echo -e "${RED}ERROR: Neither ninja nor make found${NC}"
    exit 1
fi

echo -e "${GREEN}✓${NC} Build tools available"

# Set vcpkg root if not set
if [ -z "$VCPKG_ROOT" ]; then
    if [ -d "$HOME/vcpkg" ]; then
        export VCPKG_ROOT="$HOME/vcpkg"
        echo "Using VCPKG_ROOT=$VCPKG_ROOT"
    elif command -v vcpkg &> /dev/null; then
        VCPKG_PATH=$(which vcpkg)
        export VCPKG_ROOT=$(dirname $VCPKG_PATH)
        echo "Using VCPKG_ROOT=$VCPKG_ROOT"
    else
        echo -e "${YELLOW}WARNING: VCPKG_ROOT not set and vcpkg not found${NC}"
        echo "Some dependencies may need to be installed manually"
    fi
fi

# Create build directory
echo ""
echo "Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake
echo ""
echo "Configuring CMake..."
cmake "$PROJECT_ROOT" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DTHEMIS_BUILD_TESTS=ON \
    -DTHEMIS_ENABLE_DISTRIBUTED_TRAINING=ON \
    -DTHEMIS_ENABLE_LLM=ON \
    -DTHEMIS_BUILD_BENCHMARKS=OFF \
    -DTHEMIS_STRICT_BUILD=OFF \
    ${VCPKG_ROOT:+-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake}

if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: CMake configuration failed${NC}"
    echo "Try installing dependencies manually or check vcpkg setup"
    exit 1
fi

echo -e "${GREEN}✓${NC} CMake configuration successful"

# Build
echo ""
echo "Building project..."
cmake --build . --target test_distributed_training_coordinator -j$(nproc)

if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: Build failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓${NC} Build successful"

# Run tests
echo ""
echo "=========================================="
echo "Running Loss Aggregation Tests"
echo "=========================================="
echo ""

if [ -f "./test_distributed_training_coordinator" ]; then
    ./test_distributed_training_coordinator --gtest_filter="*Loss*"
    TEST_RESULT=$?
elif [ -f "./tests/test_distributed_training_coordinator" ]; then
    ./tests/test_distributed_training_coordinator --gtest_filter="*Loss*"
    TEST_RESULT=$?
else
    echo -e "${RED}ERROR: test_distributed_training_coordinator binary not found${NC}"
    exit 1
fi

echo ""
echo "=========================================="
if [ $TEST_RESULT -eq 0 ]; then
    echo -e "${GREEN}✓ All loss aggregation tests PASSED${NC}"
else
    echo -e "${RED}✗ Some tests FAILED${NC}"
fi
echo "=========================================="

exit $TEST_RESULT
