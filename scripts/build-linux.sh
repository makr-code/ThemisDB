#!/usr/bin/env bash
# Build ThemisDB for Linux with automatic cache update

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
NO_CACHE=false
SKIP_TESTS=false
DEBUG=false
CONFIG="Release"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --no-cache) NO_CACHE=true; shift ;;
        --skip-tests) SKIP_TESTS=true; shift ;;
        --debug) DEBUG=true; CONFIG="Debug"; shift ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

echo "=== ThemisDB Linux Build ==="

# Step 1: Update cache
if [ "$NO_CACHE" != "true" ]; then
    echo -e "\n[1/3] Updating vcpkg cache..."
    "$SCRIPT_DIR/update-vcpkg-cache.ps1" -Triplets @("x64-linux") || true
fi

# Step 2: Configure with CMake
echo -e "\n[2/3] Configuring CMake..."
BUILD_DIR="$ROOT_DIR/build-linux"
[ -d "$BUILD_DIR" ] && rm -rf "$BUILD_DIR"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE="$CONFIG" \
    -DTHEMIS_BUILD_TESTS=$([ "$SKIP_TESTS" = "true" ] && echo "OFF" || echo "ON") \
    -DTHEMIS_BUILD_BENCHMARKS=OFF \
    -DTHEMIS_ENABLE_TRACING=$([ "$DEBUG" = "true" ] && echo "ON" || echo "OFF")

# Step 3: Build
echo -e "\n[3/3] Building with Ninja..."
ninja -C "$BUILD_DIR" -j 4

echo -e "\n=== Build Complete ==="
if [ -f "$BUILD_DIR/themis_server" ]; then
    SIZE=$(du -h "$BUILD_DIR/themis_server" | cut -f1)
    echo "Binary: $BUILD_DIR/themis_server ($SIZE)"
fi
