#!/bin/bash
# Quick Docker build using WSL-built binary
# Much faster than full vcpkg build in Docker

set -e

VERSION="1.4.0"
WSL_BUILD="c:/VCC/themis/build-wsl/themis_server"
DOCKER_TAG="themisdb:${VERSION}-hyperscaler"

echo "================================="
echo "Quick ThemisDB HYPERSCALER Docker Build"
echo "================================="

# Check if WSL binary exists
if [ ! -f "$WSL_BUILD" ]; then
    echo "WSL binary not found. Building with WSL first..."
    cd /mnt/c/VCC/themis
    
    # Build with WSL
    export VCPKG_ROOT=/mnt/c/VCC/themis/vcpkg
    cmake --build build-wsl --target themis_server -j8
    
    if [ ! -f "$WSL_BUILD" ]; then
        echo "ERROR: WSL build failed"
        exit 1
    fi
fi

echo "✓ WSL binary found: $WSL_BUILD"

# Create minimal Dockerfile that just packages the binary
cat > /tmp/Dockerfile.quick << 'EOF'
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl libstdc++6 libssl3 zlib1g && \
    rm -rf /var/lib/apt/lists/*

RUN groupadd -r themis --gid=999 && \
    useradd -r -g themis --uid=999 -d /var/lib/themisdb themis

COPY themis_server /usr/local/bin/themis_server
RUN chmod +x /usr/local/bin/themis_server

RUN mkdir -p /etc/themis /var/lib/themisdb && \
    chown -R themis:themis /etc/themis /var/lib/themisdb

COPY config.json /etc/themis/config.json

EXPOSE 8080 18765 4318

USER themis
WORKDIR /var/lib/themisdb

HEALTHCHECK --interval=10s --timeout=3s CMD curl -f http://localhost:8080/health || exit 1

ENTRYPOINT ["/usr/local/bin/themis_server"]
CMD ["--config", "/etc/themis/config.json"]
EOF

# Create build context
mkdir -p /tmp/docker-build-context
cp "$WSL_BUILD" /tmp/docker-build-context/themis_server
cp c:/VCC/themis/config/config.json /tmp/docker-build-context/config.json

# Build Docker image
cd /tmp/docker-build-context
docker build -f /tmp/Dockerfile.quick -t "$DOCKER_TAG" -t "themisdb:hyperscaler" .

# Cleanup
rm -rf /tmp/docker-build-context /tmp/Dockerfile.quick

echo ""
echo "✓ Docker build complete!"
echo "  Image: $DOCKER_TAG"
echo ""
echo "Test with:"
echo "  docker run --rm $DOCKER_TAG --version"
