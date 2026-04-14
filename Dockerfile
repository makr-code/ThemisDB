# syntax=docker/dockerfile:1.6
# ============================================================================
# ThemisDB Production Dockerfile - Best Practices
# ============================================================================
# Build:    docker buildx build --build-arg THEMIS_EDITION=COMMUNITY -t themisdb:latest .
# Features: Multi-stage, BuildKit caching, vcpkg binary cache, aria2 downloads
# Editions: MINIMAL, COMMUNITY, ENTERPRISE, HYPERSCALER

# ============================================================================
# Build Arguments (configurable at build time)
# ============================================================================
ARG THEMIS_EDITION=COMMUNITY
ARG ENABLE_LLM=ON
ARG ENABLE_GPU=ON
ARG FORCE_CPU_ONLY=OFF
ARG BUILD_TESTS=OFF
ARG BUILD_BENCHMARKS=OFF
ARG TARGETARCH

# ============================================================================
# Stage 0: prebuilt - Dummy stage for optional pre-built artifacts
# ============================================================================
FROM scratch AS prebuilt

# ============================================================================
# Stage 0b: vcpkg-cache - Dummy stage for optional vcpkg binary cache
# ============================================================================
FROM scratch AS vcpkg-cache

# ============================================================================
# Stage 1: base - Build environment with vcpkg
# ============================================================================
FROM ubuntu:24.04 AS base

ENV DEBIAN_FRONTEND=noninteractive \
    VCPKG_ROOT=/opt/vcpkg \
    VCPKG_FORCE_SYSTEM_BINARIES=1 \
    VCPKG_DISABLE_METRICS=1 \
    VCPKG_BINARY_SOURCES="clear;x-azblob,https://vcpkgcache.blob.core.windows.net/public,read" \
    VCPKG_BUILD_TYPE=release

# Install build essentials + aria2 for fast downloads
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    rm -f /etc/apt/apt.conf.d/docker-clean && \
    echo 'Binary::apt::APT::Keep-Downloaded-Packages "true";' > /etc/apt/apt.conf.d/keep-cache && \
    apt-get update && apt-get install -y --no-install-recommends \
        # Build tools
        build-essential cmake ninja-build git curl ca-certificates pkg-config \
        # vcpkg dependencies
        zip unzip tar wget flex bison python3 perl nasm \
        autoconf automake libtool \
        # Download acceleration
        aria2 \
        # System libraries
        libssl-dev zlib1g-dev libkrb5-dev && \
    apt-get clean

# Clone and bootstrap vcpkg, capture current HEAD as baseline
RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} && \
    cd ${VCPKG_ROOT} && \
    VCPKG_BASELINE=$(git rev-parse HEAD) && \
    echo "${VCPKG_BASELINE}" > /tmp/vcpkg_baseline.txt && \
    ./bootstrap-vcpkg.sh -disableMetrics && \
    echo "✓ vcpkg baseline: ${VCPKG_BASELINE}"

# ============================================================================
# Stage 2: deps - Install dependencies with vcpkg
# ============================================================================
FROM base AS deps

ARG THEMIS_EDITION
ARG TARGETARCH

WORKDIR /build

# Copy edition-specific vcpkg manifests
COPY docker/vcpkg-*.json ./

# Select and configure edition manifest
RUN set -eux; \
    EDITION=$(echo "${THEMIS_EDITION}" | tr '[:upper:]' '[:lower:]'); \
    if [ -f "vcpkg-${EDITION}.json" ]; then \
        cp "vcpkg-${EDITION}.json" vcpkg.json; \
        echo "✓ Edition: ${THEMIS_EDITION}"; \
    else \
        echo "ERROR: vcpkg-${EDITION}.json not found"; exit 1; \
    fi; \
    \
    # Detect architecture triplet
    ARCH="${TARGETARCH:-}"; \
    if [ -z "${ARCH}" ]; then \
        ARCH="$(dpkg --print-architecture)"; \
    fi; \
    case "${ARCH}" in \
        amd64) TRIPLET="x64-linux" ;; \
        arm64) TRIPLET="arm64-linux" ;; \
        arm)   TRIPLET="arm-linux" ;; \
        *)     echo "ERROR: Unsupported arch ${ARCH}"; exit 1 ;; \
    esac; \
    echo "${TRIPLET}" > /tmp/triplet.txt; \
    echo "${ARCH}" > /tmp/targetarch.txt; \
    echo "✓ Architecture: ${TRIPLET} (${ARCH})"

# Copy vcpkg configuration
COPY vcpkg-configuration.json ./

# Copy custom vcpkg ports overlay (required by vcpkg-configuration.json)
COPY ports ./ports

# Note: vcpkg downloads are handled via BuildKit cache mounts (see RUN --mount below)
# No need to copy vcpkg/downloads - will cache at build time if available
# Previous literal COPY would fail if directory doesn't exist

# Install dependencies with TRIPLE CACHE STRATEGY:
# 1. BuildKit cache mounts (persistent Docker cache between builds)
# 2. Host downloads copied to container (4.42 GB local cache)
# 3. vcpkg binary cache (compiled packages as .zip archives)
ARG VCPKG_BINARY_SOURCES="clear;default"

RUN --mount=type=bind,from=prebuilt,target=/vcpkg-prebuilt,readonly \
    --mount=type=cache,target=/opt/vcpkg/downloads,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/packages,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/buildtrees,sharing=locked \
    set -eux; \
    TRIPLET=$(cat /tmp/triplet.txt); \
    mkdir -p /build/vcpkg_installed/${TRIPLET}; \
    \
    # Copy host downloads to BuildKit cache (one-time per file)
    if [ -d /vcpkg-host-downloads ] && [ "$(ls -A /vcpkg-host-downloads 2>/dev/null)" ]; then \
        echo "📦 Copying host downloads cache to BuildKit cache..."; \
        cp -n /vcpkg-host-downloads/*.tar.gz /opt/vcpkg/downloads/ 2>/dev/null || true; \
        cp -n /vcpkg-host-downloads/*.zip /opt/vcpkg/downloads/ 2>/dev/null || true; \
        cp -n /vcpkg-host-downloads/*.7z /opt/vcpkg/downloads/ 2>/dev/null || true; \
        CACHED=$(ls /opt/vcpkg/downloads/*.tar.gz /opt/vcpkg/downloads/*.zip 2>/dev/null | wc -l); \
        echo "✓ $CACHED cached downloads available"; \
    else \
        echo "⚠ No host downloads cache, will download from internet"; \
    fi; \
    \
    # Configure vcpkg with binary caching
    export VCPKG_BINARY_SOURCES="clear;files,/opt/vcpkg/archives,readwrite"; \
    export VCPKG_BUILD_TYPE=release; \
    export VCPKG_MAX_CONCURRENCY=8; \
    \
    # Install/compile dependencies (will use cached downloads)
    echo "📦 Installing packages for ${TRIPLET}..."; \
    ${VCPKG_ROOT}/vcpkg install \
        --triplet="${TRIPLET}" \
        --x-manifest-root=/build \
        --x-install-root=/build/vcpkg_installed \
        --allow-unsupported \
        --clean-after-build || { \
        echo "ERROR: vcpkg install failed"; \
        exit 1; \
    }; \
    \
    # Create symlinks for CMake to find vcpkg includes/libs
    ln -sf /build/vcpkg_installed/${TRIPLET}/include /build/include; \
    ln -sf /build/vcpkg_installed/${TRIPLET}/lib /build/lib; \
    ln -sf /build/vcpkg_installed/${TRIPLET}/debug/lib /build/debug; \
    \
    # Create symlink for CMake compatibility (/build/include -> /build/vcpkg_installed/*/include)
    if [ -d "/build/vcpkg_installed/${TRIPLET}/include" ]; then \
        ln -sf "/build/vcpkg_installed/${TRIPLET}/include" "/build/include"; \
        echo "✓ Created symlink: /build/include -> /build/vcpkg_installed/${TRIPLET}/include"; \
    fi; \
    \
    echo "✓ Dependencies complete: ${TRIPLET}"; \
    ls -lh /build/vcpkg_installed/${TRIPLET}/lib/ 2>/dev/null | head -20 || true

# ============================================================================
# Stage 3: llama - Build llama.cpp (optional for LLM support)
# ============================================================================
FROM base AS llama

ARG ENABLE_LLM

WORKDIR /opt

# Copy llama.cpp source (filtered by .dockerignore)
COPY llama.cpp ./llama.cpp

# Build llama.cpp if LLM enabled
RUN if [ "$ENABLE_LLM" = "ON" ]; then \
        echo "Building llama.cpp for LLM support..."; \
        cd /opt/llama.cpp && \
        \
        # Fix char8_t compilation issue in llama-chat.cpp (GCC 11+ bug, file may not exist in newer versions) \
        if [ -f src/llama-chat.cpp ]; then \
            sed -i 's/#define LU8(x) u8##x/#define LU8(x) (const char*)(x)/' src/llama-chat.cpp && \
            sed -i 's/const char8_t \* haystack/const char* haystack/' src/llama-chat.cpp; \
        fi && \
        \
        rm -rf build && \
        mkdir -p build && \
        cd build && \
        cmake .. -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DLLAMA_BUILD_TESTS=OFF \
            -DLLAMA_BUILD_EXAMPLES=OFF \
            -DLLAMA_BUILD_SERVER=OFF \
            -DLLAMA_BUILD_SHARED_LIB=ON \
            -DLLAMA_NATIVE=OFF \
            -DLLAMA_AVX2=ON && \
        ninja -j4 && \
        echo "✓ llama.cpp built successfully"; \
    else \
        echo "LLM disabled - skipping llama.cpp build"; \
        mkdir -p /opt/llama.cpp/build/lib; \
    fi

# ============================================================================
# Stage 4: build - Compile ThemisDB
# ============================================================================
FROM deps AS build

ARG THEMIS_EDITION
ARG ENABLE_LLM
ARG ENABLE_GPU
ARG FORCE_CPU_ONLY
ARG BUILD_TESTS
ARG BUILD_BENCHMARKS
ARG TARGETARCH

WORKDIR /src

# Copy source code (structure matches Windows build)
COPY CMakeLists.txt VERSION ./
COPY cmake ./cmake
COPY include ./include
COPY src ./src
COPY proto ./proto
COPY internal ./internal

# Copy vcpkg manifests from deps stage
COPY --from=deps /build/vcpkg.json ./vcpkg.json
COPY --from=deps /build/vcpkg-configuration.json ./vcpkg-configuration.json
COPY --from=deps /build/vcpkg_installed /build/vcpkg_installed
COPY --from=deps /tmp/triplet.txt /tmp/triplet.txt
COPY --from=deps /tmp/targetarch.txt /tmp/targetarch.txt

# Copy llama.cpp build artifacts
COPY --from=llama /opt/llama.cpp /opt/llama.cpp

# Build ThemisDB with CMake (matching Windows presets)
RUN set -eux; \
    TRIPLET=$(cat /tmp/triplet.txt); \
    ARCH=$(cat /tmp/targetarch.txt); \
    EDITION_UPPER=$(echo "${THEMIS_EDITION}" | tr '[:lower:]' '[:upper:]'); \
    \
    echo "=========================================="; \
    echo "Building ThemisDB ${EDITION_UPPER}"; \
    echo "=========================================="; \
    echo "Edition:        ${EDITION_UPPER}"; \
    echo "LLM:            ${ENABLE_LLM}"; \
    echo "GPU:            ${ENABLE_GPU}"; \
    echo "CPU-only:       ${FORCE_CPU_ONLY}"; \
    echo "Tests:          ${BUILD_TESTS}"; \
    echo "Benchmarks:     ${BUILD_BENCHMARKS}"; \
    echo "Target Arch:    ${ARCH}"; \
    echo "vcpkg Triplet:  ${TRIPLET}"; \
    echo "=========================================="; \
    \
    # Configure CMake (matching Windows CMakePresets.json)
    cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_TARGET_TRIPLET=${TRIPLET} \
        -DVCPKG_INSTALLED_DIR=/build/vcpkg_installed \
        -DVCPKG_MANIFEST_MODE=OFF \
        -DTHEMIS_EDITION=${EDITION_UPPER} \
        -DTHEMIS_ENABLE_LLM=${ENABLE_LLM} \
        -DTHEMIS_ENABLE_GPU=${ENABLE_GPU} \
        -DTHEMIS_ENABLE_VULKAN=$([ "${FORCE_CPU_ONLY}" = "ON" ] && echo "OFF" || echo "${ENABLE_GPU}") \
        -DTHEMIS_ENABLE_CUDA=OFF \
        -DTHEMIS_BUILD_TESTS=${BUILD_TESTS} \
        -DTHEMIS_BUILD_BENCHMARKS=${BUILD_BENCHMARKS} \
        -DTHEMIS_ENABLE_TRACING=OFF \
        -DTHEMIS_STRICT_BUILD=OFF \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && \
    \
    # Build themis_server
    echo "Building themis_server..." && \
    ninja -C build -j$(nproc) themis_server && \
    \
    # Verify binary
    if [ ! -f build/themis_server ]; then \
        echo "ERROR: themis_server binary not found"; \
        exit 1; \
    fi; \
    \
    echo "✓ ThemisDB built successfully"; \
    ls -lh build/themis_server

# ============================================================================
# Stage 5: runtime - Production image (minimal)
# ============================================================================
FROM ubuntu:24.04 AS runtime

ARG THEMIS_EDITION

# Metadata labels (OCI standard)
LABEL org.opencontainers.image.title="ThemisDB" \
      org.opencontainers.image.description="ThemisDB ${THEMIS_EDITION} Edition - Multi-model database with LLM integration" \
      org.opencontainers.image.vendor="ThemisDB Project" \
      org.opencontainers.image.version="1.4.0" \
      org.opencontainers.image.source="https://github.com/yourusername/themis" \
      org.opencontainers.image.licenses="MIT"

ENV DEBIAN_FRONTEND=noninteractive \
    THEMIS_EDITION=${THEMIS_EDITION} \
    LD_LIBRARY_PATH=/usr/local/lib:/opt/themis/lib \
    THEMIS_MODEL_DIR=/opt/themis/models

WORKDIR /opt/themis

# Install minimal runtime dependencies + gocryptfs for user storage encryption
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    rm -f /etc/apt/apt.conf.d/docker-clean && \
    apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        libssl3t64 \
        zlib1g \
        libstdc++6 \
        # OpenMP runtime required by RocksDB parallel compression and LLM libs
        libgomp1 \
        # NUMA support for multi-socket performance (required by some RocksDB builds)
        libnuma1 \
        # liblzma for XZ compression support
        liblz4-1 \
        libzstd1 \
        libsnappy1v5 \
        curl \
        gocryptfs \
        fuse \
        libsodium23 && \
    echo "user_allow_other" >> /etc/fuse.conf && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Copy themis_server binary
COPY --from=build /src/build/themis_server /opt/themis/bin/themis_server

# Bundle exactly one default GGUF model for startup initialization.
# Other models remain runtime/on-demand.
COPY models/tinyllama-1.1b-q4_0.gguf /opt/themis/models/default.gguf

# Copy llama.cpp libraries (if LLM enabled)
COPY --from=llama /opt/llama.cpp/build/lib*.so* /usr/local/lib/

# Copy vcpkg runtime libraries (shared libs) if present
RUN --mount=type=bind,from=deps,source=/build/vcpkg_installed,target=/deps_vcpkg,readonly \
    mkdir -p /opt/themis/lib && \
    cp -a /deps_vcpkg/*/lib/*.so* /opt/themis/lib/ 2>/dev/null || true

# Update library cache
RUN ldconfig

# Create required directories:
#   /data           – persistent database files (mount a volume here)
#   /data/llm_cache – LLM response cache RocksDB (writable at runtime)
#   /data/rocksdb   – main RocksDB storage
#   /etc/themis     – optional config file location
# Non-root user "themis" owns all runtime paths.
RUN mkdir -p /data/rocksdb /data/llm_cache /etc/themis && \
    useradd -r -u 1000 -d /opt/themis -s /bin/false themis && \
    chown -R themis:themis /opt/themis /data /etc/themis

USER themis

# Expose default port
EXPOSE 8765

# Health check — polls the /health HTTP endpoint exposed by the server
# (GET /health returns 200 when the server is ready to accept queries)
HEALTHCHECK --interval=30s --timeout=5s --start-period=30s --retries=5 \
    CMD curl -f http://localhost:8765/health || exit 1

ENTRYPOINT ["/opt/themis/bin/themis_server"]
# Default: use /data as working dir so relative paths resolve correctly.
# The --config flag is optional; server starts without a config file.
CMD ["--data-dir=/data"]

# ============================================================================
# Stage 6: debug - Development/debugging image
# ============================================================================
FROM ubuntu:24.04 AS debug

ARG THEMIS_EDITION

ENV DEBIAN_FRONTEND=noninteractive \
    THEMIS_EDITION=${THEMIS_EDITION} \
    LD_LIBRARY_PATH=/usr/local/lib:/opt/themis/lib \
    THEMIS_MODEL_DIR=/opt/themis/models

WORKDIR /opt/themis

# Install debug tools
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates libssl3t64 zlib1g libstdc++6 libgomp1 \
        libnuma1 liblz4-1 libzstd1 libsnappy1v5 curl \
        gdb valgrind strace ltrace lsof htop vim less \
        netcat-openbsd telnet iproute2 dnsutils && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Copy binary and libraries
COPY --from=build /src/build/themis_server /opt/themis/bin/themis_server
COPY --from=build /src/build/compile_commands.json /opt/themis/
COPY --from=llama /opt/llama.cpp/build/lib*.so* /usr/local/lib/
COPY models/tinyllama-1.1b-q4_0.gguf /opt/themis/models/default.gguf
RUN --mount=type=bind,from=deps,source=/build/vcpkg_installed,target=/deps_vcpkg,readonly \
    mkdir -p /opt/themis/lib && \
    cp -a /deps_vcpkg/*/lib/*.so* /opt/themis/lib/ 2>/dev/null || true

# Copy source for debugging
COPY --from=build /src /src

RUN ldconfig && \
    mkdir -p /data/rocksdb /data/llm_cache /etc/themis && \
    chown -R root:root /opt/themis /data /etc/themis

EXPOSE 8765

ENTRYPOINT ["/opt/themis/bin/themis_server"]
CMD ["--data-dir=/data"]
