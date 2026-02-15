#!/bin/bash
set -e

cd /mnt/c/VCC/themis

export VCPKG_ROOT=/mnt/c/VCC/themis/vcpkg
export CC=gcc-11
export CXX=g++-11

echo "=== ThemisDB WSL Build (Community Edition) ==="
echo "VCPKG_ROOT: $VCPKG_ROOT"
echo "CC: $CC"
echo "CXX: $CXX"
echo ""

rm -rf build-wsl-ninja-release

echo "Configuring with CMake..."
cmake -S . -B build-wsl-ninja-release \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_ROOT=$VCPKG_ROOT \
  2>&1 | tail -100

echo ""
echo "Building..."
cmake --build build-wsl-ninja-release --parallel 8 2>&1 | tail -100

echo ""
echo "=== Build Complete ==="
ls -lh build-wsl-ninja-release/themis_server
