# syntax=docker/dockerfile:1.6
# ThemisDB Hyperscaler Edition - Docker Build
# Includes LLM support (llama.cpp) + GPU acceleration + optimized for distributed sharding

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

# Bootstrap vcpkg - use stable 2024.10.21 release (pinned for reproducibility)
ENV VCPKG_ROOT=/opt/vcpkg
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_USE_ARIA2=1
ARG VCPKG_ENABLE_ONLINE=ON
ENV VCPKG_BINARY_SOURCES="clear;files,/src/vcpkg_installed,readwrite;files,/opt/vcpkg/downloads,readwrite"
ENV VCPKG_KEEP_ENV_VARS=HTTPS_PROXY,HTTP_PROXY,ALL_PROXY,NO_PROXY,VCPKG_ENABLE_ONLINE

RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} \
    && cd ${VCPKG_ROOT} \
    && git checkout 2024.10.21 \
    && ${VCPKG_ROOT}/bootstrap-vcpkg.sh -disableMetrics \
    && ./vcpkg update

RUN mkdir -p ${VCPKG_ROOT}/downloads
COPY --chown=root:root vcpkg/downloads/ ${VCPKG_ROOT}/downloads/

ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++
ENV VCPKG_DEFAULT_TRIPLET=x64-linux

WORKDIR /src

COPY ports-overlays ./ports-overlays
ENV VCPKG_OVERLAY_PORTS=/src/ports-overlays

# Copy vcpkg manifest files
COPY vcpkg.docker.json ./vcpkg.json
COPY vcpkg-configuration.json ./

# Pin vcpkg registry baseline
RUN cd ${VCPKG_ROOT} \
    && VCPKG_BASELINE=$(git rev-parse HEAD) \
    && echo "{\"default-registry\":{\"kind\":\"builtin\",\"baseline\":\"$VCPKG_BASELINE\"}}" > /src/vcpkg-configuration.json

ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_DISABLE_METRICS=1
ENV VCPKG_USE_ARIA2=1
ENV VCPKG_DOWNLOADER=aria2
ENV VCPKG_DOWNLOAD_TOOL=aria2
ENV VCPKG_MAX_CONCURRENCY=4
ENV VCPKG_BUILD_TYPE=release
ENV VCPKG_INSTALLED_DIR=/src/vcpkg_installed
RUN mkdir -p /root/.cache/vcpkg/archives && chmod -R 755 /root/.cache/vcpkg

# Pre-download Boost packages
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

# Install vcpkg dependencies with retry logic
RUN CACHE_FILES=$(find ${VCPKG_ROOT}/downloads -type f ! -name '.gitkeep' ! -name 'README.md' | wc -l) && \
    echo "==> Found $CACHE_FILES cached download files" && \
    export VCPKG_ASSET_SOURCES="clear;files,/opt/vcpkg/downloads,readwrite;x-azurl,https://vcpkg.io/assets,readwrite" && \
    echo "Installing dependencies for x64-linux..." && \
    set -eux; \
    export VCPKG_BINARY_SOURCES="clear;files,/src/vcpkg_installed,readwrite;files,/opt/vcpkg/downloads,readwrite"; \
    export VCPKG_DOWNLOAD_TOOL=aria2; export VCPKG_USE_ARIA2=1; export VCPKG_DOWNLOADER=aria2; \
    for attempt in 1 2 3; do \
        echo "vcpkg install attempt ${attempt}/3"; \
        if ${VCPKG_ROOT}/vcpkg install --triplet=x64-linux 2>&1 | tee /tmp/vcpkg_install.log; then \
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

# Clone llama.cpp for LLM support
ARG LLAMA_GIT_REF=master
RUN echo "Cloning llama.cpp (${LLAMA_GIT_REF}) - LLM support enabled"; \
    git clone --depth=1 https://github.com/ggerganov/llama.cpp.git /src/llama.cpp && \
    (cd /src/llama.cpp && git fetch --depth=1 origin ${LLAMA_GIT_REF} || true && git checkout ${LLAMA_GIT_REF} || true)

# Build ThemisDB Hyperscaler Edition with LLM + GPU support
RUN echo "=== Building ThemisDB HYPERSCALER Edition for x64-linux ===" && \
    export CCACHE_DIR=/tmp/ccache && \
    export CXXFLAGS="-Wno-deprecated-declarations" && \
    export CFLAGS="-Wno-deprecated-declarations" && \
    cmake -S . -B build -G Ninja \
        -DCMAKE_MAKE_PROGRAM=/usr/bin/ninja \
        -DCMAKE_C_COMPILER=/usr/bin/gcc \
        -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
        -DCMAKE_BUILD_TYPE=Release \
        -DTHEMIS_CORE_SHARED=OFF \
        -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_TARGET_TRIPLET=x64-linux \
        -DVCPKG_MANIFEST_DIR=/src \
        -DVCPKG_INSTALLED_DIR=/src/vcpkg_installed \
        -DVCPKG_FEATURE_FLAGS=manifests,versions \
        -DTHEMIS_BUILD_TESTS=OFF \
        -DTHEMIS_BUILD_BENCHMARKS=OFF \
        -DTHEMIS_ENABLE_TRACING=ON \
        -DTHEMIS_STATIC_BUILD=OFF \
        -DTHEMIS_EDITION=HYPERSCALER \
        -DTHEMIS_MODELS_MODE=SKIP \
        -DTHEMIS_ENABLE_LLM=ON \
        -DTHEMIS_ENABLE_GPU=ON \
        -DTHEMIS_ENABLE_WEBSOCKETS=OFF 2>&1 | tee /tmp/cmake_config.log && \
    ninja -C build -j8 2>&1 | tee /tmp/cmake_build.log && \
    ls -lh /src/build/themis_server

# Runtime stage - minimal Ubuntu image
FROM ubuntu:22.04 AS runtime
ENV DEBIAN_FRONTEND=noninteractive

ARG THEMIS_VERSION=1.3.4-hyperscaler
LABEL org.opencontainers.image.title="ThemisDB HYPERSCALER Edition" \
    org.opencontainers.image.description="ThemisDB HYPERSCALER Edition with LLM & GPU support for distributed sharding" \
    org.opencontainers.image.version="$THEMIS_VERSION" \
    org.opencontainers.image.edition="HYPERSCALER"

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl libstdc++6 jq \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user 'themis'
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

RUN chown -R themis:themis /etc/themis && \
    chmod -R 755 /etc/themis

# Copy documentation
COPY README.md LICENSE CHANGELOG.md SECURITY.md /usr/local/share/themis/docs/

# Copy OpenAPI specification
COPY openapi/openapi.yaml /usr/local/share/themis/openapi/

# Copy runtime libraries
COPY --from=build /src/vcpkg_installed /tmp/vcpkg_installed
RUN echo "Copying shared libraries from x64-linux..." && \
    mkdir -p /usr/local/lib/themisdb && \
    if [ -d "/tmp/vcpkg_installed/x64-linux/lib" ]; then \
      find /tmp/vcpkg_installed/x64-linux/lib -name "*.so*" -exec cp -v {} /usr/local/lib/themisdb/ \; ; \
    fi && \
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

# Setup data directories with proper ownership
RUN mkdir -p /var/lib/themisdb /var/lib/themisdb/data /var/lib/themisdb/vector_indexes /var/log/themis && \
    chown -R themis:themis /var/lib/themisdb /var/log/themis && \
    chmod 755 /var/lib/themisdb /var/log/themis && \
    chmod +x /usr/local/bin/themis_server && \
    ldconfig

# Hyperscaler-specific environment variables
ENV THEMIS_CONFIG_PATH=/etc/themis/config.json
ENV THEMIS_DATA_DIR=/var/lib/themisdb
ENV THEMIS_PORT=18765
ENV THEMIS_HOST=0.0.0.0
ENV THEMIS_EDITION=HYPERSCALER

# Storage configuration optimized for RAID
ENV THEMIS_ROCKSDB_PATH=/var/lib/themisdb/data
ENV THEMIS_VECTOR_INDEX_PATH=/var/lib/themisdb/vector_indexes
ENV THEMIS_MEMTABLE_SIZE_MB=512
ENV THEMIS_BLOCK_CACHE_SIZE_MB=2048
ENV THEMIS_MAX_BACKGROUND_JOBS=16

# Server configuration for distributed workloads
ENV THEMIS_WORKER_THREADS=16
ENV THEMIS_MAX_CONNECTIONS=10000

# Logging configuration
ENV THEMIS_LOG_LEVEL=info
ENV THEMIS_LOG_DIR=/var/log/themis

# Feature flags (Hyperscaler Edition)
ENV THEMIS_ENABLE_TRACING=true
ENV THEMIS_ENABLE_SEMANTIC_CACHE=true
ENV THEMIS_ENABLE_LLM_STORE=true
ENV THEMIS_ENABLE_CDC=true
ENV THEMIS_ENABLE_TIMESERIES=true
ENV THEMIS_ENABLE_GPU=true

# Sharding configuration (overridable at runtime)
ENV SHARD_ID=shard_0
ENV SHARD_ROLE=primary
ENV CLUSTER_NAME=themis-hyperscaler
ENV CLUSTER_SIZE=10
ENV RAID_LEVEL=RAID10
ENV REPLICATION_FACTOR=2
ENV CONSISTENCY_LEVEL=QUORUM

# OpenTelemetry/Metrics
ENV THEMIS_OTLP_ENDPOINT=http://localhost:4318
ENV THEMIS_SERVICE_NAME=themis-server

ENV LD_LIBRARY_PATH=/usr/local/lib/themisdb:/usr/local/lib

VOLUME ["/var/lib/themisdb"]

# Core ports
EXPOSE 8080
EXPOSE 18765
EXPOSE 4318
EXPOSE 9090

# Health check
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
  CMD curl -f http://localhost:8080/health || exit 1

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD []
