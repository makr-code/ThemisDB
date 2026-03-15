#!/bin/bash
set -e
cd /mnt/c/VCC/themis
VCPKG_ROOT=/mnt/c/VCC/themis/vcpkg
export CC=gcc-11
export CXX=g++-11
export PATH=/usr/bin:/usr/local/bin:$PATH

echo "=== CMake Konfiguration ==="
echo "VCPKG_ROOT: $VCPKG_ROOT"
echo "CC: $(which $CC)"
echo "Ninja: $(which ninja)"

cmake -S . -B build-wsl-ninja-release \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_MAKE_PROGRAM=/usr/bin/ninja \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_ROOT="$VCPKG_ROOT" 2>&1
echo "CMake Exit: $?"
