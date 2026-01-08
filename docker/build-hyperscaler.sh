#!/bin/bash
# Build ThemisDB HYPERSCALER Docker image from pre-built Windows binaries
# Usage: ./build-docker-hyperscaler.sh [VERSION] [REGISTRY]

set -e

VERSION="${1:-1.4.0}"
REGISTRY="${2:-themisdb}"
TAG="${REGISTRY}:${VERSION}-hyperscaler"

echo "================================="
echo "Building ThemisDB HYPERSCALER v${VERSION} Docker Image"
echo "Target Tag: ${TAG}"
echo "================================="

# Check if Windows binary exists
WINDOWS_BINARY="c:/VCC/themis/build-msvc/Release/themis_server.exe"
if [ ! -f "$WINDOWS_BINARY" ]; then
    echo "ERROR: Windows binary not found at $WINDOWS_BINARY"
    echo "Please build on Windows first with: cmake --build build-msvc --config Release"
    exit 1
fi

# Copy binary to temporary Linux executable (WSL can execute Windows binaries but Docker needs ELF)
echo "Preparing binary for Docker..."
# For now, we'll skip the binary copy since Docker on Windows/WSL can build Linux from source

# Build the runtime image
cd "$(dirname "$0")/.."

echo ""
echo "Building Docker image..."

# Use the hyperscaler-runtime Dockerfile which just needs pre-built binary
# Since we can't easily cross-compile binaries, we'll build using WSL Linux build environment

if [ -f "build-wsl/themis_server" ]; then
    echo "Found WSL-built binary, using that..."
    # Copy WSL binary
    mkdir -p build-docker-context
    cp build-wsl/themis_server build-docker-context/
    
    docker build \
        --build-arg THEMIS_VERSION="${VERSION}" \
        -f docker/Dockerfile.hyperscaler-runtime \
        -t "${TAG}" \
        -t "${REGISTRY}:hyperscaler" \
        .
    
    rm -rf build-docker-context
else
    echo "No WSL binary found. Using source build (slower)..."
    
    docker build \
        --build-arg THEMIS_VERSION="${VERSION}" \
        --build-arg ENABLE_LLM=ON \
        -f docker/Dockerfile.hyperscaler \
        -t "${TAG}" \
        -t "${REGISTRY}:hyperscaler" \
        .
fi

echo ""
echo "✓ Build complete!"
echo "  Image: ${TAG}"
echo ""
echo "Usage:"
echo "  docker run -p 8080:8080 -p 18765:18765 ${TAG}"
echo ""
echo "Compose:"
echo "  docker-compose -f docker/compose/docker-compose-raid-hyperscaler.yml up"
