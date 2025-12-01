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

# Bootstrap vcpkg - use stable 2024.12.16 release
ENV VCPKG_ROOT=/opt/vcpkg
# Required on non-amd64 platforms when building under emulation (ARM, s390x, ppc64le, riscv)
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} \
    && cd ${VCPKG_ROOT} \
    && git checkout 2024.10.21 \
    && ${VCPKG_ROOT}/bootstrap-vcpkg.sh -disableMetrics \
    && ./vcpkg update \
    && VCPKG_BASELINE=$(git rev-parse HEAD) \
    && echo "Using vcpkg baseline: $VCPKG_BASELINE"

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

# Copy vcpkg manifest files first (for better layer caching)
# Use simplified vcpkg.docker.json for faster builds
COPY vcpkg.docker.json ./vcpkg.json
COPY vcpkg-configuration.json ./

## Pin vcpkg registry baseline to checked out commit to avoid version db mismatches
RUN cd ${VCPKG_ROOT} \
    && VCPKG_BASELINE=$(git rev-parse HEAD) \
    && echo "{\"default-registry\":{\"kind\":\"builtin\",\"baseline\":\"$VCPKG_BASELINE\"}}" > /src/vcpkg-configuration.json

# Install dependencies via vcpkg manifest mode
# Disable compiler tracking and metrics for faster, more stable builds
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_DISABLE_METRICS=1
ENV VCPKG_USE_ARIA2=1
ENV VCPKG_DOWNLOADER=aria2
ENV VCPKG_MAX_CONCURRENCY=2
ENV VCPKG_INSTALLED_DIR=/src/vcpkg_installed

RUN . /etc/profile.d/vcpkg.sh && \
    echo "Installing dependencies for ${VCPKG_TRIPLET} (vcpkg --debug)..." && \
    set -eux; \
    for i in 1 2 3; do \
        ${VCPKG_ROOT}/vcpkg install --debug --triplet=${VCPKG_TRIPLET} && break || (echo "vcpkg install failed (attempt $i), retrying..."; sleep 10); \
    done || ( \
        echo "vcpkg install failed after retries; dumping most recent logs"; \
        RECENT_LOGS=$(find /opt/vcpkg/buildtrees -maxdepth 3 -type f -name "*.log" -printf '%T@ %p\n' 2>/dev/null | sort -nr | head -n 80 | cut -d' ' -f2-); \
        for f in $RECENT_LOGS; do echo "===== $f ====="; tail -n 300 "$f" || true; done; \
        exit 1 \
    )

# Copy source code
COPY CMakeLists.txt ./
COPY include ./include
COPY src ./src

# All vcpkg manifest dependencies are installed above (with retries)

# Build ThemisDB
RUN apt-get update && apt-get install -y ninja-build build-essential && \
        echo "=== STEP 1: Packages installed ===" && \
        export VCPKG_TRIPLET=${VCPKG_TRIPLET:-x64-linux} && \
        echo "=== STEP 2: VCPKG_TRIPLET=${VCPKG_TRIPLET} ===" && \
        echo "=== STEP 3: Listing vcpkg installed packages ===" && \
        ls -lah /src/vcpkg_installed/${VCPKG_TRIPLET}/share 2>&1 | head -50 && \
        echo "=== STEP 4: Starting CMake configure ===" && \
        cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=${VCPKG_TRIPLET} \
    -DVCPKG_MANIFEST_DIR=/src \
    -DVCPKG_INSTALLED_DIR=/src/vcpkg_installed \
    -DVCPKG_FEATURE_FLAGS=manifests,versions,binarycaching \
    -DTHEMIS_BUILD_TESTS=OFF \
    -DTHEMIS_BUILD_BENCHMARKS=OFF \
    -DTHEMIS_ENABLE_TRACING=OFF 2>&1 | tee /tmp/cmake_config.log || \
        (echo "=== CMake configure FAILED ==="; \
        echo "=== Last 100 lines of CMake output ==="; \
        tail -100 /tmp/cmake_config.log; \
        echo "=== CMakeError.log ==="; \
        cat build/CMakeFiles/CMakeError.log 2>/dev/null || echo "Not found"; \
        echo "=== CMakeCache.txt (first 200 lines) ==="; \
        head -200 build/CMakeCache.txt 2>/dev/null || echo "Not found"; \
        exit 1) && \
        echo "=== STEP 5: CMake configure SUCCESS, starting build ===" && \
        cmake --build build --target themis_server -j$(nproc)

# Runtime stage - minimal Ubuntu image
FROM ubuntu:22.04 AS runtime
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

# Copy built binary
COPY --from=build /src/build/themis_server /usr/local/bin/themis_server

# Copy vcpkg installed libraries that are needed at runtime
# Auto-detect triplet from build stage
ARG TARGETARCH
RUN VCPKG_TRIPLET_COPY="x64-linux"; \
    case "${TARGETARCH}" in \
      amd64) VCPKG_TRIPLET_COPY="x64-linux" ;; \
      arm64) VCPKG_TRIPLET_COPY="arm64-linux" ;; \
      arm) VCPKG_TRIPLET_COPY="arm-linux" ;; \
    esac && \
    echo "Copying libraries from ${VCPKG_TRIPLET_COPY}..." && \
    cp -v /opt/vcpkg/installed/${VCPKG_TRIPLET_COPY}/lib/*.so* /usr/local/lib/ 2>/dev/null || true

# Setup runtime environment
RUN mkdir -p /etc/themis /usr/local/share/themis

# Setup runtime environment
RUN mkdir -p /data /var/log/themis && \
    chmod +x /usr/local/bin/themis_server && \
    ldconfig

ENV THEMIS_CONFIG_PATH=/etc/themis/config.json
ENV THEMIS_PORT=18765
ENV LD_LIBRARY_PATH=/usr/local/lib

VOLUME ["/data"]
EXPOSE 8080 18765

ENTRYPOINT ["/usr/local/bin/themis_server"]
CMD ["--config", "/etc/themis/config.json"]
