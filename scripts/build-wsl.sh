#!/bin/bash
set -e

REPO_ROOT="$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"
cd "$REPO_ROOT"

export VCPKG_ROOT="${VCPKG_ROOT:-$REPO_ROOT/vcpkg}"
export CC="${CC:-gcc}"
export CXX="${CXX:-g++}"

echo "=== ThemisDB WSL/Linux Build (Community Edition) ==="
echo "REPO_ROOT:  $REPO_ROOT"
echo "VCPKG_ROOT: $VCPKG_ROOT"
echo "CC:         $CC"
echo "CXX:        $CXX"
echo ""

cmake --preset linux-release
cmake --build --preset linux-release

echo ""
echo "=== Build Complete ==="
ls -lh build-gcc-linux-release/themis_server
