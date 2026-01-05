#!/usr/bin/env bash
set -euo pipefail

# Build script for ThemisDB MINIMAL Edition
# This builds the most lightweight version with only core database features
# No LLM, no GPU, no sharding, no advanced protocols
#
# Usage:
#   ./scripts/build-minimal.sh               # Default: Release build
#   BUILD_TYPE=Debug ./scripts/build-minimal.sh  # Debug build

VCPKG_ROOT=${VCPKG_ROOT:-$HOME/vcpkg}
BUILD_DIR=${BUILD_DIR:-build-minimal}
BUILD_TYPE=${BUILD_TYPE:-Release}
TRIPLET=${VCPKG_TARGET_TRIPLET:-x64-linux}
NUM_JOBS=${NUM_JOBS:-$(nproc)}

echo "=== ThemisDB MINIMAL Edition Build ==="
echo "VCPKG_ROOT: $VCPKG_ROOT"
echo "BUILD_DIR: $BUILD_DIR"
echo "BUILD_TYPE: $BUILD_TYPE"
echo "VCPKG_TARGET_TRIPLET: $TRIPLET"
echo "NUM_JOBS: $NUM_JOBS"
echo ""
echo "Features: Core database only (ACID, multi-model, indexes, basic queries)"
echo "Disabled: LLM, GPU, sharding, replication, advanced protocols, tracing"
echo "Binary size: ~50-80% smaller than Community Edition"
echo "Build time: ~5-10 minutes (vs ~30-40 for full build)"
echo ""

# Ensure running inside WSL/Linux
if [ -f /proc/version ]; then
  if grep -qi microsoft /proc/version 2>/dev/null; then
    echo "Running under WSL"
  elif grep -qi ubuntu /proc/version 2>/dev/null || [ -f /etc/lsb-release ]; then
    echo "Running on Ubuntu/Linux"
  else
    echo "Warning: This script is optimized for WSL/Ubuntu. Continuing anyway..."
  fi
else
  echo "Warning: Non-Linux environment detected. This script may not work correctly."
fi

# Ensure minimum tools exist
command -v git >/dev/null 2>&1 || { echo "git not found. Install it (sudo apt install git)."; exit 1; }
command -v cmake >/dev/null 2>&1 || { echo "cmake not found. Install it (sudo apt install cmake)."; exit 1; }

# Bootstrap vcpkg if necessary
if [ ! -x "$VCPKG_ROOT/vcpkg" ]; then
  echo "vcpkg not found at $VCPKG_ROOT"
  echo "Please install vcpkg first or set VCPKG_ROOT environment variable"
  echo "Example: git clone https://github.com/microsoft/vcpkg.git $HOME/vcpkg && cd $HOME/vcpkg && ./bootstrap-vcpkg.sh"
  exit 1
fi

# Clean build directory if it exists
if [ -d "$BUILD_DIR" ]; then
  echo "Cleaning existing build directory..."
  rm -rf "$BUILD_DIR"
fi

# Configure CMake for MINIMAL edition
echo ""
echo "Configuring CMake for MINIMAL edition..."
cmake -S . -B "$BUILD_DIR" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET="$TRIPLET" \
  -DTHEMIS_EDITION=MINIMAL \
  -DTHEMIS_ENABLE_LLM=OFF \
  -DTHEMIS_ENABLE_GPU=OFF \
  -DTHEMIS_ENABLE_CUDA=OFF \
  -DTHEMIS_ENABLE_VULKAN=OFF \
  -DTHEMIS_ENABLE_HIP=OFF \
  -DTHEMIS_ENABLE_CONTENT_PROCESSORS=OFF \
  -DTHEMIS_ENABLE_TRACING=OFF \
  -DTHEMIS_ENABLE_VOICE_ASSISTANT=OFF \
  -DTHEMIS_ENABLE_GRPC=OFF \
  -DTHEMIS_ENABLE_HTTP2=OFF \
  -DTHEMIS_ENABLE_HTTP3=OFF \
  -DTHEMIS_ENABLE_WEBSOCKET=OFF \
  -DTHEMIS_ENABLE_MQTT=OFF \
  -DTHEMIS_ENABLE_POSTGRES_WIRE=OFF \
  -DTHEMIS_ENABLE_MCP=OFF \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_BUILD_BENCHMARKS=OFF \
  -DBUILD_SHARED_LIBS=OFF

# Build
echo ""
echo "Building MINIMAL edition..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --parallel "$NUM_JOBS"

# Check if build succeeded
if [ -f "$BUILD_DIR/themis_server" ]; then
  echo ""
  echo "✅ Build succeeded!"
  echo "Binary location: $BUILD_DIR/themis_server"
  echo "Binary size:"
  ls -lh "$BUILD_DIR/themis_server"
  echo ""
  echo "To run the server:"
  echo "  $BUILD_DIR/themis_server --config config/config-minimal.yaml"
else
  echo ""
  echo "❌ Build failed - binary not found"
  exit 1
fi
