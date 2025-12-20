# Multi-stage Docker build for ThemisDB
# Uses vcpkg for complete dependency management

FROM ubuntu:22.04 AS build
ENV DEBIAN_FRONTEND=noninteractive

# Install build tools and system dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build git curl zip unzip tar pkg-config \
    ca-certificates python3 perl nasm autoconf automake libtool aria2 linux-libc-dev \
    flex bison \
    && rm -rf /var/lib/apt/lists/*

# Upgrade to a recent CMake (>=3.25) required by Boost 1.86
RUN apt-get update && apt-get install -y --no-install-recommends \
    wget gpg software-properties-common \
    && rm -rf /var/lib/apt/lists/* \
    && wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg \
    && echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main" > /etc/apt/sources.list.d/kitware.list \
    && apt-get update && apt-get install -y --no-install-recommends cmake \
    && cmake --version \
    && rm -rf /var/lib/apt/lists/*

# Ensure CMake is <4.0 for ports requiring legacy policy (<3.5), but >=3.25 for Boost
RUN apt-get update && apt-get install -y --no-install-recommends python3-pip \
    && pip3 install --no-cache-dir "cmake>=3.25,<4.0" \
    && cmake --version \
    && rm -rf /var/lib/apt/lists/*

# Bootstrap vcpkg - use stable 2024.10.21 release
ENV VCPKG_ROOT=/opt/vcpkg
# Required on non-amd64 platforms when building under emulation (ARM, s390x, ppc64le, riscv)
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_USE_ARIA2=1
# Build argument to enable online mode if cache is not available
ARG VCPKG_ENABLE_ONLINE=ON
# Configure vcpkg sources: prefer local cache, fallback to online if needed
ENV VCPKG_BINARY_SOURCES="clear;files,/src/vcpkg_installed,readwrite;files,/opt/vcpkg/downloads,readwrite"
ENV VCPKG_KEEP_ENV_VARS=HTTPS_PROXY,HTTP_PROXY,ALL_PROXY,NO_PROXY,VCPKG_ENABLE_ONLINE

RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} \
    && cd ${VCPKG_ROOT} \
    && git checkout 2024.10.21 \
    && ${VCPKG_ROOT}/bootstrap-vcpkg.sh -disableMetrics \
    && ./vcpkg update \
    && VCPKG_BASELINE=$(git rev-parse HEAD) \
    && echo "Using vcpkg baseline: $VCPKG_BASELINE"

# Pre-seed vcpkg downloads cache with source archives from local cache (OFFLINE build)
# This contains all previously downloaded source packages (~2GB)
# Best practice: Create empty directory if cache doesn't exist, vcpkg will download on demand
RUN mkdir -p ${VCPKG_ROOT}/downloads
# Copy cache if available (will copy .gitkeep if directory is empty, that's fine)
COPY --chown=root:root vcpkg/downloads/ ${VCPKG_ROOT}/downloads/

# Set up environment
ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++

# Auto-detect architecture and set appropriate vcpkg triplet
# Supports x64-linux (amd64), arm64-linux (aarch64), arm-linux (armv7)
ARG TARGETARCH
ARG VCPKG_TRIPLET
RUN TRIPLET="${VCPKG_TRIPLET}"; \
    if [ -z "$TRIPLET" ]; then \
      case "${TARGETARCH}" in \
        amd64) TRIPLET=x64-linux ;; \
        arm64) TRIPLET=arm64-linux ;; \
        arm) TRIPLET=arm-linux ;; \
        *) TRIPLET=x64-linux ;; \
      esac; \
    fi && \
    echo "export VCPKG_TRIPLET=${TRIPLET}" > /etc/profile.d/vcpkg.sh && \
    echo "Using vcpkg triplet: ${TRIPLET}"

# Set default triplet environment variable
ENV VCPKG_DEFAULT_TRIPLET=${VCPKG_TRIPLET:-x64-linux}

WORKDIR /src

# Include local vcpkg overlay ports (used for xsimd patch prefetch avoidance)
COPY ports-overlays ./ports-overlays
ENV VCPKG_OVERLAY_PORTS=/src/ports-overlays

# Copy vcpkg manifest files first (for better layer caching)
# Use simplified vcpkg.docker.json for faster builds
COPY vcpkg.docker.json ./vcpkg.json
COPY vcpkg-configuration.json ./

## Pin vcpkg registry baseline to checked out commit to avoid version db mismatches
RUN cd ${VCPKG_ROOT} \
    && VCPKG_BASELINE=$(git rev-parse HEAD) \
    && echo "{\"default-registry\":{\"kind\":\"builtin\",\"baseline\":\"$VCPKG_BASELINE\"}}" > /src/vcpkg-configuration.json

# Install dependencies via vcpkg manifest mode with local caching
# Pre-built libs reduce build time significantly
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_DISABLE_METRICS=1
ENV VCPKG_USE_ARIA2=1
ENV VCPKG_DOWNLOADER=aria2
ENV VCPKG_MAX_CONCURRENCY=4
ENV VCPKG_INSTALLED_DIR=/src/vcpkg_installed
# Local binary cache directory for faster multi-arch builds
RUN mkdir -p /root/.cache/vcpkg/archives && chmod -R 755 /root/.cache/vcpkg

# Install dependencies via vcpkg - will use cache if available, download if needed
RUN . /etc/profile.d/vcpkg.sh && \
    # Check if cache has actual content (excluding placeholder files)
    CACHE_FILES=$(find ${VCPKG_ROOT}/downloads -type f ! -name '.gitkeep' ! -name 'README.md' | wc -l) && \
    if [ "$CACHE_FILES" -gt 0 ]; then \
        echo "==> Using OFFLINE mode with cached downloads ($CACHE_FILES files)"; \
        export VCPKG_ASSET_SOURCES="files,/opt/vcpkg/downloads,readwrite"; \
    else \
        echo "==> Using ONLINE mode (no cache found, will download packages)"; \
        export VCPKG_ASSET_SOURCES="x-azurl,https://vcpkg.io/assets,readwrite;x-block-origin"; \
    fi && \
    echo "Installing dependencies for ${VCPKG_TRIPLET}..." && \
    set -eux; \
    export VCPKG_BINARY_SOURCES="clear;files,/src/vcpkg_installed,readwrite;files,/opt/vcpkg/downloads,readwrite"; \
    ${VCPKG_ROOT}/vcpkg install --triplet=${VCPKG_TRIPLET} 2>&1 | tee /tmp/vcpkg_install.log || ( \
        echo "vcpkg install failed; tail of log:"; \
        tail -n 100 /tmp/vcpkg_install.log; \
        exit 1 \
    )

# Copy source code
COPY CMakeLists.txt ./
COPY VERSION ./
COPY include ./include
COPY src ./src

# All vcpkg manifest dependencies are installed above (with retries)

# Build argument for QNAP compatibility (older CPUs without AVX)
ARG QNAP_BUILD=OFF

# Build ThemisDB with optimized triplet detection
RUN . /etc/profile.d/vcpkg.sh && \
    echo "=== Building for ${VCPKG_TRIPLET:-x64-linux} ===" && \
    cmake -S . -B build -G Ninja \
        -DCMAKE_MAKE_PROGRAM=/usr/bin/ninja \
        -DCMAKE_C_COMPILER=/usr/bin/gcc \
        -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
        -DCMAKE_BUILD_TYPE=Release \
        -DTHEMIS_CORE_SHARED=OFF \
        -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_TARGET_TRIPLET=${VCPKG_TRIPLET} \
        -DVCPKG_MANIFEST_DIR=/src \
        -DVCPKG_INSTALLED_DIR=/src/vcpkg_installed \
        -DVCPKG_FEATURE_FLAGS=manifests,versions \
        -DTHEMIS_BUILD_TESTS=OFF \
        -DTHEMIS_BUILD_BENCHMARKS=OFF \
        -DTHEMIS_ENABLE_TRACING=OFF \
        -DTHEMIS_QNAP_BUILD=${QNAP_BUILD} \
        -DTHEMIS_STATIC_BUILD=OFF 2>&1 | tee /tmp/cmake_config.log && \
    ninja -C build 2>&1 | tee /tmp/cmake_build.log && \
    ls -lh /src/build/themis_server

# Runtime stage - minimal Ubuntu image
FROM ubuntu:22.04 AS runtime
ENV DEBIAN_FRONTEND=noninteractive

# Image metadata
ARG THEMIS_VERSION
LABEL org.opencontainers.image.title="ThemisDB" \
    org.opencontainers.image.description="ThemisDB server image" \
    org.opencontainers.image.version="$THEMIS_VERSION"

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

# Copy built binary
COPY --from=build /src/build/themis_server /usr/local/bin/themis_server

# Copy configuration files
COPY config/config.json /etc/themis/config.json
COPY config/*.yaml /etc/themis/
COPY config/policies.json /etc/themis/
COPY config/processors /etc/themis/processors
COPY config/schemas /etc/themis/schemas

# Copy documentation
COPY README.md LICENSE CHANGELOG.md SECURITY.md /usr/local/share/themis/docs/

# Copy OpenAPI specification
COPY openapi/openapi.yaml /usr/local/share/themis/openapi/

# Copy client libraries (SDKs)
COPY clients /usr/local/share/themis/clients

# Copy examples
COPY examples /usr/local/share/themis/examples

# Copy tools
COPY tools/plugin_signer /usr/local/share/themis/tools/plugin_signer
COPY tools/sign_plugin_manifest.py /usr/local/share/themis/tools/
COPY tools/sign_pii_engine.py /usr/local/share/themis/tools/

# Copy vcpkg installed libraries that are needed at runtime
ARG TARGETARCH
ARG VCPKG_TRIPLET
COPY --from=build /src/vcpkg_installed /tmp/vcpkg_installed
RUN VCPKG_TRIPLET_COPY="${VCPKG_TRIPLET:-x64-linux}"; \
    if [ -z "$VCPKG_TRIPLET" ]; then \
      case "${TARGETARCH}" in \
        amd64) VCPKG_TRIPLET_COPY="x64-linux" ;; \
        arm64) VCPKG_TRIPLET_COPY="arm64-linux" ;; \
        arm) VCPKG_TRIPLET_COPY="arm-linux" ;; \
      esac; \
    fi && \
    echo "Copying shared libraries from ${VCPKG_TRIPLET_COPY}..." && \
    mkdir -p /usr/local/lib/themisdb && \
    if [ -d "/tmp/vcpkg_installed/${VCPKG_TRIPLET_COPY}/lib" ]; then \
      find /tmp/vcpkg_installed/${VCPKG_TRIPLET_COPY}/lib -name "*.so*" -exec cp -v {} /usr/local/lib/themisdb/ \; ; \
    fi && \
    # Create symlinks in /usr/local/lib for compatibility
    cd /usr/local/lib/themisdb && \
    for lib in *.so.*; do \
      [ -f "$lib" ] || continue; \
      base=$(echo $lib | sed 's/\.so\..*/\.so/'); \
      ln -sf "$lib" "$base" 2>/dev/null || true; \
    done && \
    cd - && \
    rm -rf /tmp/vcpkg_installed && \
    ldconfig

# Setup runtime environment
RUN mkdir -p /etc/themis /usr/local/share/themis

# Setup runtime environment
RUN mkdir -p /data /var/log/themis && \
    chmod +x /usr/local/bin/themis_server && \
    ldconfig

ENV THEMIS_CONFIG_PATH=/etc/themis/config.json
ENV THEMIS_PORT=18765
# Ensure runtime libraries are discoverable without relying on pre-set LD_LIBRARY_PATH
ENV LD_LIBRARY_PATH=/usr/local/lib/themisdb:/usr/local/lib

VOLUME ["/data"]
EXPOSE 8080 18765

ENTRYPOINT ["/usr/local/bin/themis_server"]
CMD ["--config", "/etc/themis/config.json"]
