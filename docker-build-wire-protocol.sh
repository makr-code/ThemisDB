#!/bin/bash
# ThemisDB Docker Build Script with Wire Protocol Support
# This script builds the Docker image using the pre-built themis_server binary with Wire Protocol

set -e

# Configuration
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCKER_CONTEXT="${REPO_ROOT}"
DOCKER_IMAGE_NAME="themis-db"
DOCKER_IMAGE_TAG="wire-protocol-latest"

# Check if themis_server binary exists
if [ ! -f "${REPO_ROOT}/build-wsl/themis_server" ]; then
    echo "ERROR: themis_server binary not found at ${REPO_ROOT}/build-wsl/themis_server"
    echo "Please build the binary first:"
    echo "  wsl bash -c 'cd /mnt/c/VCC/themis && export VCPKG_ROOT=/mnt/c/VCC/themis/vcpkg && cmake -S . -B build-wsl -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/mnt/c/VCC/themis/vcpkg/scripts/buildsystems/vcpkg.cmake -DTHEMIS_BUILD_WIRE_PROTOCOL=ON && cmake --build build-wsl --target themis_server -j8'"
    exit 1
fi

echo "=== Building ThemisDB Docker Image with Wire Protocol ==="
echo "Context: ${DOCKER_CONTEXT}"
echo "Dockerfile: ${DOCKER_CONTEXT}/Dockerfile.prebuilt"
echo "Image: ${DOCKER_IMAGE_NAME}:${DOCKER_IMAGE_TAG}"
echo ""

# Build the Docker image
docker build \
    --build-arg VCPKG_TRIPLET=x64-linux \
    -t "${DOCKER_IMAGE_NAME}:${DOCKER_IMAGE_TAG}" \
    -t "${DOCKER_IMAGE_NAME}:latest" \
    -f "${DOCKER_CONTEXT}/Dockerfile.prebuilt" \
    "${DOCKER_CONTEXT}"

echo ""
echo "=== Docker image built successfully ==="
echo ""
echo "To run the container with Wire Protocol enabled:"
echo "  docker run -d -p 8765:8765 -p 8766:8766 -v themis_data:/data ${DOCKER_IMAGE_NAME}:${DOCKER_IMAGE_TAG}"
echo ""
echo "To check if Wire Protocol server is listening:"
echo "  netstat -tnlp | grep 8766"
echo ""
