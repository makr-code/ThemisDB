#!/bin/bash
# Fast Docker build - copies pre-built binary from WSL

set -e

cd /mnt/c/VCC/themis

echo "=== ThemisDB Fast Docker Build ==="
echo ""

# Check if binary exists
if [ ! -f build-wsl/themis_server ]; then
    echo "ERROR: build-wsl/themis_server not found"
    echo "Please build it first with:"
    echo "  wsl bash"
    echo "  cd /mnt/c/VCC/themis"
    echo "  export VCPKG_ROOT=/mnt/c/VCC/themis/vcpkg"
    echo "  cmake -S . -B build-wsl -G Ninja -DCMAKE_BUILD_TYPE=Release \\"
    echo "    -DCMAKE_TOOLCHAIN_FILE=\${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \\"
    echo "    -DTHEMIS_BUILD_WIRE_PROTOCOL=ON"
    echo "  cmake --build build-wsl --target themis_server -j8"
    exit 1
fi

echo "Step 1: Copy binary to build context..."
cp build-wsl/themis_server themis_server
chmod +x themis_server
ls -lh themis_server

echo ""
echo "Step 2: Build Docker image..."
docker build -t themis-db:wire-protocol-latest -f Dockerfile.fast .

if [ $? -eq 0 ]; then
    echo ""
    echo "=== Success! ==="
    docker images themis-db
    echo ""
    echo "To start the container:"
    echo "  docker run -d -p 8765:8765 -p 8766:8766 -v themis_data:/data themis-db:wire-protocol-latest"
    echo ""
    
    # Cleanup
    rm -f themis_server
else
    echo "ERROR: Docker build failed"
    rm -f themis_server
    exit 1
fi
