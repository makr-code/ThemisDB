#!/bin/bash
# Build ThemisDB Docker Image from within WSL
# This avoids all file path issues by building everything inside Docker

set -e

cd /mnt/c/VCC/themis

echo "=== ThemisDB Docker Build (Full In-Docker) ==="
echo ""
echo "Hinweis: Dies wird ~10-15 Minuten dauern (vcpkg Dependencies werden gebaut)"
echo ""

# Simple Dockerfile that lives in /tmp and builds everything
cat > /tmp/Dockerfile.themis-builder <<'EOF'
FROM ubuntu:22.04

# Install all build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git curl wget zip unzip tar pkg-config ca-certificates ninja-build \
    libboost-dev libboost-system-dev libboost-thread-dev libssl-dev zlib1g-dev libbrotli-dev \
    libboost-system1.74.0 libboost-thread1.74.0 libssl3 libstdc++6 libc6 libzstd1 libbrotli1 libcurl4 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copy minimal source
COPY CMakeLists.txt ./
COPY include/ ./include/
COPY src/ ./src/
COPY vcpkg.json ./

# Setup and build
ENV VCPKG_ROOT=/build/external/vcpkg
RUN git clone --depth=1 https://github.com/Microsoft/vcpkg.git external/vcpkg && \
    cd external/vcpkg && ./bootstrap-vcpkg.sh -disableMetrics && cd /build

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
    -DTHEMIS_BUILD_WIRE_PROTOCOL=ON && \
    cmake --build build --target themis_server -j$(nproc) && \
    ls -lh build/themis_server

RUN mkdir -p /output && cp build/themis_server /output/

# Runtime stage
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates libssl3 libstdc++6 libc6 \
    libboost-system1.74.0 libboost-thread1.74.0 libzstd1 libbrotli1 libcurl4 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=0 /output/themis_server /app/themis_server
RUN chmod +x /app/themis_server && mkdir -p /data /etc/themis

RUN useradd -m -u 1000 themis 2>/dev/null || true && \
    chown -R themis:themis /app /data /etc/themis

USER themis
EXPOSE 8765 8766
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD test -f /app/themis_server || exit 1
CMD ["/app/themis_server"]
EOF

echo "Building Docker image..."
docker build -t themis-db:wire-protocol-latest \
    -f /tmp/Dockerfile.themis-builder \
    /mnt/c/VCC/themis

if [ $? -eq 0 ]; then
    echo ""
    echo "=== Erfolg! ===  "
    docker images themis-db
    echo ""
    echo "Zum Starten:"
    echo "  docker run -d -p 8765:8765 -p 8766:8766 -v themis_data:/data themis-db:wire-protocol-latest"
else
    echo ""
    echo "ERROR: Docker Build fehlgeschlagen"
    exit 1
fi
