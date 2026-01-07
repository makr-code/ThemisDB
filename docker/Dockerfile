# syntax=docker/dockerfile:1.6
# Multi-stage Docker build for ThemisDB
# Uses vcpkg for complete dependency management

FROM ubuntu:24.04 AS build
ENV DEBIAN_FRONTEND=noninteractive

# Install build tools and system dependencies with security updates
RUN apt-get update && apt-get upgrade -y && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build git curl zip unzip tar pkg-config \
    ca-certificates python3 perl nasm autoconf automake libtool aria2 linux-libc-dev \
    flex bison \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

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

# Bootstrap vcpkg - use stable 2024.10.21 release (pinned for reproducibility)
ENV VCPKG_ROOT=/opt/vcpkg
# Required on non-amd64 platforms when building under emulation (ARM, s390x, ppc64le, riscv)
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_USE_ARIA2=1
# Build argument to enable online mode if cache is not available
ARG VCPKG_ENABLE_ONLINE=ON
# Asset source URL for online downloads (can be overridden for mirrors)
ARG VCPKG_ASSET_URL=https://vcpkg.io/assets
# Configure vcpkg sources: prefer local cache, fallback to online if needed
# Note: VCPKG_ASSET_SOURCES is configured dynamically based on cache detection (see vcpkg install step)
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
ENV VCPKG_DOWNLOAD_TOOL=aria2
ENV VCPKG_MAX_CONCURRENCY=4
ENV VCPKG_BUILD_TYPE=release
ENV VCPKG_INSTALLED_DIR=/src/vcpkg_installed
# Local binary cache directory for faster multi-arch builds
RUN mkdir -p /root/.cache/vcpkg/archives && chmod -R 755 /root/.cache/vcpkg

# Install dependencies via vcpkg - use cache as fallback, but allow online downloads
# Uses build arg VCPKG_ASSET_URL for asset source (default: https://vcpkg.io/assets)
RUN set -eux; \
    export VCPKG_DOWNLOAD_TOOL=aria2; export VCPKG_USE_ARIA2=1; export VCPKG_DOWNLOADER=aria2; \
    BOOST_PKGS="algorithm align array asio assert atomic beast bind chrono concept-check config container container-hash context conversion core coroutine date_time describe detail dynamic-bitset endian exception filesystem function function-types functional fusion integer intrusive io iterator lexical_cast locale logic math move mp11 mpl multiprecision numeric-conversion optional pool predef preprocessor random range ratio regex scope scope-exit smart-ptr static-assert static-string system thread throw-exception tokenizer tuple type-index type-traits typeof unordered utility variant2 winapi"; \
    for pkg in $BOOST_PKGS; do \
      repo_slug=$(echo "$pkg" | tr '-' '_'); \
      file=boostorg-${repo_slug}-boost-1.86.0.tar.gz; \
      if [ -f /opt/vcpkg/downloads/$file ]; then \
        echo "Using cached $file"; \
        continue; \
      fi; \
      for attempt in 1 2 3 4 5; do \
        echo "Downloading $file (attempt ${attempt}/5)"; \
        if aria2c --retry-wait=10 --max-tries=5 --timeout=120 --connect-timeout=60 --dir=/opt/vcpkg/downloads --out=$file \
             https://github.com/boostorg/${repo_slug}/archive/boost-1.86.0.tar.gz; then \
          break; \
        fi; \
        if [ "$attempt" -eq 5 ]; then \
          echo "Download $file failed after 5 attempts"; \
          exit 1; \
        fi; \
        sleep 20; \
      done; \
    done

RUN . /etc/profile.d/vcpkg.sh && \
    # Check if cache has actual content (excluding placeholder files)
    CACHE_FILES=$(find ${VCPKG_ROOT}/downloads -type f ! -name '.gitkeep' ! -name 'README.md' | wc -l) && \
    echo "==> Found $CACHE_FILES cached download files" && \
    echo "==> Using HYBRID mode: cache first, online fallback for missing packages" && \
    # Use local cache first, then fall back to online downloads if files are missing
    export VCPKG_ASSET_SOURCES="clear;files,/opt/vcpkg/downloads,readwrite;x-azurl,https://vcpkg.io/assets,readwrite" && \
    echo "Installing dependencies for ${VCPKG_TRIPLET}..." && \
    set -eux; \
    export VCPKG_BINARY_SOURCES="clear;files,/src/vcpkg_installed,readwrite;files,/opt/vcpkg/downloads,readwrite"; \
    export VCPKG_DOWNLOAD_TOOL=aria2; export VCPKG_USE_ARIA2=1; export VCPKG_DOWNLOADER=aria2; \
    for attempt in 1 2 3; do \
        echo "vcpkg install attempt ${attempt}/3"; \
        if ${VCPKG_ROOT}/vcpkg install --triplet=${VCPKG_TRIPLET} 2>&1 | tee /tmp/vcpkg_install.log; then \
            break; \
        fi; \
        if [ "$attempt" -eq 3 ]; then \
            echo "vcpkg install failed after 3 attempts; tail of log:"; \
            tail -n 200 /tmp/vcpkg_install.log; \
            exit 1; \
        fi; \
        echo "vcpkg install failed; retrying in 15s..."; \
        sleep 15; \
    done

# Copy source code
COPY CMakeLists.txt ./
COPY VERSION ./
COPY include ./include
COPY cmake ./cmake
COPY src ./src

# Enable embedded LLM via llama.cpp (always ON)
ARG ENABLE_LLM=ON
ARG LLAMA_GIT_REF=master

# Clone llama.cpp for embedded LLM support
RUN echo "Cloning llama.cpp (${LLAMA_GIT_REF}) - LLM support enabled"; \
    git clone --depth=1 https://github.com/ggerganov/llama.cpp.git /src/llama.cpp && \
    (cd /src/llama.cpp && git fetch --depth=1 origin ${LLAMA_GIT_REF} || true && git checkout ${LLAMA_GIT_REF} || true)

# All vcpkg manifest dependencies are installed above (with retries)

# Build argument for QNAP compatibility (older CPUs without AVX)
ARG QNAP_BUILD=OFF

# Build argument for ThemisDB Edition: COMMUNITY (default), ENTERPRISE, or HYPERSCALER
ARG THEMIS_EDITION=COMMUNITY

# Build ThemisDB with optimized triplet detection
RUN . /etc/profile.d/vcpkg.sh && \
    echo "=== Building ThemisDB ${THEMIS_EDITION} Edition for ${VCPKG_TRIPLET:-x64-linux} ===" && \
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
        -DTHEMIS_STATIC_BUILD=OFF \
        -DTHEMIS_EDITION=${THEMIS_EDITION} \
        -DTHEMIS_ENABLE_LLM=${ENABLE_LLM} 2>&1 | tee /tmp/cmake_config.log && \
    ninja -C build 2>&1 | tee /tmp/cmake_build.log && \
    ls -lh /src/build/themis_server

# Runtime stage - minimal Ubuntu image
FROM ubuntu:24.04 AS runtime
ENV DEBIAN_FRONTEND=noninteractive

# Image metadata
ARG THEMIS_VERSION
ARG THEMIS_EDITION=COMMUNITY
LABEL org.opencontainers.image.title="ThemisDB ${THEMIS_EDITION} Edition" \
    org.opencontainers.image.description="ThemisDB ${THEMIS_EDITION} Edition server image" \
    org.opencontainers.image.version="$THEMIS_VERSION" \
    org.opencontainers.image.edition="$THEMIS_EDITION"

RUN apt-get update && apt-get upgrade -y && apt-get install -y --no-install-recommends \
    ca-certificates curl libstdc++6 jq \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Create non-root user 'themis' (following PostgreSQL best practice: postgres user with uid 999)
RUN groupadd -r themis --gid=999 && \
    useradd -r -g themis --uid=999 --home-dir=/var/lib/themisdb --shell=/bin/bash themis

# Copy built binary
COPY --from=build /src/build/themis_server /usr/local/bin/themis_server

# Copy entrypoint script
COPY docker/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

# Copy configuration files
COPY config/config.json /etc/themis/config.json
COPY config/*.yaml /etc/themis/
COPY config/policies.json /etc/themis/
COPY config/processors /etc/themis/processors
COPY config/schemas /etc/themis/schemas

# Ensure config files are readable by themis user
RUN chown -R themis:themis /etc/themis && \
    chmod -R 755 /etc/themis

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

# Setup data directories with proper ownership (PostgreSQL-style: /var/lib/postgresql/data -> /var/lib/themisdb)
RUN mkdir -p /var/lib/themisdb /var/lib/themisdb/data /var/lib/themisdb/vector_indexes /var/log/themis && \
    chown -R themis:themis /var/lib/themisdb /var/log/themis && \
    chmod 755 /var/lib/themisdb /var/log/themis && \
    chmod +x /usr/local/bin/themis_server && \
    ldconfig

# Preconfigured environment variables (can be overridden at runtime)
# Core configuration
ENV THEMIS_CONFIG_PATH=/etc/themis/config.json
ENV THEMIS_DATA_DIR=/var/lib/themisdb
ENV THEMIS_PORT=18765
ENV THEMIS_HOST=0.0.0.0
ENV THEMIS_EDITION=${THEMIS_EDITION}

# Storage configuration
ENV THEMIS_ROCKSDB_PATH=/var/lib/themisdb/data
ENV THEMIS_VECTOR_INDEX_PATH=/var/lib/themisdb/vector_indexes
ENV THEMIS_MEMTABLE_SIZE_MB=256
ENV THEMIS_BLOCK_CACHE_SIZE_MB=1024

# Server configuration
ENV THEMIS_WORKER_THREADS=8

# Logging configuration
ENV THEMIS_LOG_LEVEL=info
ENV THEMIS_LOG_DIR=/var/log/themis

# Feature flags
ENV THEMIS_ENABLE_TRACING=false
ENV THEMIS_ENABLE_SEMANTIC_CACHE=true
ENV THEMIS_ENABLE_LLM_STORE=true
ENV THEMIS_ENABLE_CDC=true
ENV THEMIS_ENABLE_TIMESERIES=true

# OpenTelemetry/Metrics
ENV THEMIS_OTLP_ENDPOINT=http://localhost:4318
ENV THEMIS_SERVICE_NAME=themis-server

# Ensure runtime libraries are discoverable without relying on pre-set LD_LIBRARY_PATH
ENV LD_LIBRARY_PATH=/usr/local/lib/themisdb:/usr/local/lib

VOLUME ["/var/lib/themisdb"]

# Port mappings for all interfaces (optional ones require explicit build flags)
# Core ports (always available):
# - 8080: HTTP/1.1 REST API, GraphQL, HTTP/2 (if enabled with -DTHEMIS_ENABLE_HTTP2=ON)
# - 18765: Binary Wire Protocol, gRPC
# - 4318: OpenTelemetry/Prometheus metrics (OTLP)
EXPOSE 8080
EXPOSE 18765
EXPOSE 4318

# Optional protocol ports (require explicit build flags):
# - 1883: MQTT plain (requires -DTHEMIS_ENABLE_MQTT=ON)
# - 8883: MQTT over TLS (requires -DTHEMIS_ENABLE_MQTT=ON)
# - 8083: MQTT over WebSocket (requires -DTHEMIS_ENABLE_MQTT=ON)
# - 5432: PostgreSQL Wire Protocol (requires -DTHEMIS_ENABLE_POSTGRES_WIRE=ON)
# - 3000: MCP server for LLM integration (requires -DTHEMIS_ENABLE_MCP=ON)

# Health check for container orchestration
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
  CMD curl -f http://localhost:8080/health || exit 1

# Note: Container starts as root, entrypoint.sh will switch to themis user after setup
# This allows proper directory creation and ownership setup on first run

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD []
