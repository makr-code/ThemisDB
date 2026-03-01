#!/bin/bash
# ThemisDB v1.4.0 Linux Release Build Script
# Usage: ./build-release-linux.sh

set -e

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build-linux-release"
INSTALL_DIR="${BUILD_DIR}/install"

echo "=========================================="
echo "ThemisDB v1.4.0 Linux Release Build"
echo "=========================================="
echo "Repository: ${REPO_ROOT}"
echo "Build directory: ${BUILD_DIR}"
echo ""

# Ensure dependencies are installed
echo "[1/4] Checking system dependencies..."
REQUIRED_TOOLS=("cmake" "g++" "git" "python3")
for tool in "${REQUIRED_TOOLS[@]}"; do
    if ! command -v "$tool" &> /dev/null; then
        echo "ERROR: $tool is not installed"
        exit 1
    fi
done

# Check vcpkg
if [ ! -d "${REPO_ROOT}/vcpkg" ]; then
    echo "WARNING: vcpkg not found, cloning..."
    git clone https://github.com/Microsoft/vcpkg.git "${REPO_ROOT}/vcpkg"
    "${REPO_ROOT}/vcpkg/bootstrap-vcpkg.sh"
fi

# Configure CMake
echo "[2/4] Configuring CMake..."
cmake -S "${REPO_ROOT}" \
    -B "${BUILD_DIR}" \
    -G "Ninja Multi-Config" \
    -DCMAKE_TOOLCHAIN_FILE="${REPO_ROOT}/vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_ROOT="${REPO_ROOT}/vcpkg" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTHEMIS_ENABLE_LLM=ON \
    -DTHEMIS_ENABLE_GPU=ON \
    -DTHEMIS_ENABLE_CUDA=OFF \
    -DTHEMIS_ENABLE_TRACING=ON \
    -DTHEMIS_EDITION=HYPERSCALER

# Build
echo "[3/4] Building themis_server and tests..."
cmake --build "${BUILD_DIR}" --config Release --parallel 8

# Install
echo "[4/4] Installing..."
cmake --install "${BUILD_DIR}" --config Release

# Verify
if [ -f "${INSTALL_DIR}/bin/themis_server" ]; then
    echo ""
    echo "✅ SUCCESS: themis_server built successfully"
    echo "   Binary: ${INSTALL_DIR}/bin/themis_server"
    "${INSTALL_DIR}/bin/themis_server" --help | head -10
else
    echo "❌ FAILED: themis_server not found"
    exit 1
fi

echo ""
echo "=========================================="
echo "Build completed successfully"
echo "=========================================="
