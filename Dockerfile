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
ARG ENABLE_GPU=OFF
ARG FORCE_CPU_ONLY=ON
ARG BUILD_TESTS=OFF
ARG BUILD_BENCHMARKS=OFF
ARG TARGETARCH=amd64

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
        libssl-dev zlib1g-dev && \
    apt-get clean

# Clone and bootstrap vcpkg with pinned baseline
RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} && \
    cd ${VCPKG_ROOT} && \
    git checkout 10b7a178346f3f0abef60cecd5130e295afd8da4 && \
    ./bootstrap-vcpkg.sh -disableMetrics

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
    case "${TARGETARCH}" in \
        amd64) TRIPLET="x64-linux" ;; \
        arm64) TRIPLET="arm64-linux" ;; \
        arm)   TRIPLET="arm-linux" ;; \
        *)     echo "ERROR: Unsupported arch ${TARGETARCH}"; exit 1 ;; \
    esac; \
    echo "${TRIPLET}" > /tmp/triplet.txt; \
    echo "✓ Architecture: ${TRIPLET} (${TARGETARCH})"

# Copy vcpkg configuration
COPY vcpkg-configuration.json ./

# Install dependencies with pre-built vcpkg artifacts
# USAGE: docker build --build-arg SKIP_VCPKG_INSTALL=ON \
#   --build-context prebuilt=PATH\TO\build-linux\vcpkg_installed ...
# Requires: BuildKit + named build context (prebuilt)
ARG ENABLE_VCPKG_CACHE=OFF
ARG SKIP_VCPKG_INSTALL=OFF

RUN --mount=type=bind,from=prebuilt,source=x64-linux,target=/vcpkg-prebuilt/x64-linux,readonly \
    set -eux; \
    TRIPLET=$(cat /tmp/triplet.txt); \
    mkdir -p /build/vcpkg_installed/${TRIPLET}; \
    \
    # Copy pre-built artifacts from WSL/Linux build (if available)
    if [ "${ENABLE_VCPKG_CACHE}" = "ON" ] && [ -d "/vcpkg-prebuilt/x64-linux/lib" ]; then \
        echo "⚡ Using pre-built artifacts from WSL/Linux build..."; \
        cp -r /vcpkg-prebuilt/x64-linux/* /build/vcpkg_installed/${TRIPLET}/; \
        \
        LIB_COUNT=$(find /build/vcpkg_installed/${TRIPLET}/lib -name '*.a' 2>/dev/null | wc -l || echo 0); \
        ARTIFACT_SIZE=$(du -sh /build/vcpkg_installed/${TRIPLET} 2>/dev/null | cut -f1 || echo "0"); \
        echo "✓ Copied ${LIB_COUNT} pre-built libraries (${ARTIFACT_SIZE})"; \
    fi; \
    \
    # Use curl for downloads (more reliable than wget)
    export VCPKG_DOWNLOAD_TOOL=curl; \
    export VCPKG_USE_ARIA2=0; \
    \
    # vcpkg install will:
    # 1. Use pre-built x64-linux libraries (if present)
    # 2. Use binary cache from vcpkg/packages (if available)  
    # 3. Use downloads cache to skip re-downloading archives
    # 4. Only build packages not in any cache
    if [ "${SKIP_VCPKG_INSTALL}" = "ON" ] && [ -d "/vcpkg-prebuilt/x64-linux/lib" ]; then \
        echo "⚠️  Skipping vcpkg install (using pre-built libs only)"; \
    else \
        echo "📦 Installing/validating packages for ${TRIPLET}..."; \
        ${VCPKG_ROOT}/vcpkg install \
            --triplet="${TRIPLET}" \
            --x-manifest-root=/build \
            --x-install-root=/build/vcpkg_installed \
            --allow-unsupported \
            --clean-after-build || { \
            echo "ERROR: vcpkg install failed"; \
            exit 1; \
        }; \
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
        # Fix char8_t compilation issue in llama-chat.cpp (GCC 11+ bug) \
        sed -i 's/#define LU8(x) u8##x/#define LU8(x) (const char*)(x)/' src/llama-chat.cpp && \
        sed -i 's/const char8_t \* haystack/const char* haystack/' src/llama-chat.cpp && \
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

# Copy llama.cpp build artifacts
COPY --from=llama /opt/llama.cpp /opt/llama.cpp

# Build ThemisDB with CMake (matching Windows presets)
RUN set -eux; \
    TRIPLET=$(cat /tmp/triplet.txt); \
    \
    echo "=========================================="; \
    echo "Building ThemisDB ${THEMIS_EDITION}"; \
    echo "=========================================="; \
    echo "Edition:        ${THEMIS_EDITION}"; \
    echo "LLM:            ${ENABLE_LLM}"; \
    echo "GPU:            ${ENABLE_GPU}"; \
    echo "CPU-only:       ${FORCE_CPU_ONLY}"; \
    echo "Tests:          ${BUILD_TESTS}"; \
    echo "Benchmarks:     ${BUILD_BENCHMARKS}"; \
    echo "Target Arch:    ${TARGETARCH}"; \
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
        -DTHEMIS_EDITION=${THEMIS_EDITION} \
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
    echo "Building themis_server..."; \
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
    LD_LIBRARY_PATH=/usr/local/lib:/opt/themis/lib

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
        curl \
        gocryptfs \
        fuse \
        libsodium23 && \
    echo "user_allow_other" >> /etc/fuse.conf && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Copy themis_server binary
COPY --from=build /src/build/themis_server /opt/themis/bin/themis_server

# Copy llama.cpp libraries (if LLM enabled)
COPY --from=llama /opt/llama.cpp/build/lib*.so* /usr/local/lib/

# Copy vcpkg runtime libraries (shared libs)
COPY --from=deps /build/vcpkg_installed/*/lib/*.so* /opt/themis/lib/ 2>/dev/null || true

# Update library cache
RUN ldconfig

# Create data directory and non-root user
RUN mkdir -p /data && \
    useradd -r -u 1000 -d /opt/themis -s /bin/false themis && \
    chown -R themis:themis /opt/themis /data

USER themis

# Expose default port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

ENTRYPOINT ["/opt/themis/bin/themis_server"]
CMD ["--config=/etc/themis/config.yml", "--data-dir=/data"]

# ============================================================================
# Stage 6: debug - Development/debugging image
# ============================================================================
FROM ubuntu:24.04 AS debug

ARG THEMIS_EDITION

ENV DEBIAN_FRONTEND=noninteractive \
    THEMIS_EDITION=${THEMIS_EDITION} \
    LD_LIBRARY_PATH=/usr/local/lib:/opt/themis/lib

WORKDIR /opt/themis

# Install debug tools
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates libssl3t64 zlib1g libstdc++6 curl \
        gdb valgrind strace ltrace lsof htop vim less \
        netcat-openbsd telnet iproute2 dnsutils && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Copy binary and libraries
COPY --from=build /src/build/themis_server /opt/themis/bin/themis_server
COPY --from=build /src/build/compile_commands.json /opt/themis/
COPY --from=llama /opt/llama.cpp/build/lib*.so* /usr/local/lib/
COPY --from=deps /build/vcpkg_installed/*/lib/*.so* /opt/themis/lib/ 2>/dev/null || true

# Copy source for debugging
COPY --from=build /src /src

RUN ldconfig && \
    mkdir -p /data && \
    chown -R root:root /opt/themis /data

EXPOSE 8080

ENTRYPOINT ["/opt/themis/bin/themis_server"]
CMD ["--config=/etc/themis/config.yml", "--data-dir=/data"]
