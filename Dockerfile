# syntax=docker/dockerfile:1.6
# ThemisDB Production Dockerfile

ARG THEMIS_EDITION=COMMUNITY
ARG ENABLE_LLM=ON
ARG ENABLE_GPU=ON
ARG INCLUDE_DOCS_DB=ON
ARG INCLUDE_MINI_LLM=ON
ARG FORCE_CPU_ONLY=OFF
ARG BUILD_TESTS=ON
ARG BUILD_BENCHMARKS=ON
ARG THEMIS_ENABLE_ENCRYPTED_STORAGE=OFF
ARG TARGETARCH=amd64
ARG LLAMA_CPP_REF=1e8924fd65ad349d1d838412a2172292618f3bbf

# Dummy stages for optional artifacts
FROM scratch AS prebuilt
FROM scratch AS vcpkg-cache

# Stage 1: base - Build environment
FROM ubuntu:24.04 AS base

ENV DEBIAN_FRONTEND=noninteractive \
    VCPKG_ROOT=/opt/vcpkg \
    VCPKG_FORCE_SYSTEM_BINARIES=1 \
    VCPKG_DISABLE_METRICS=1 \
    VCPKG_BINARY_SOURCES="clear;x-azblob,https://vcpkgcache.blob.core.windows.net/public,read" \
    VCPKG_BUILD_TYPE=release

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    rm -f /etc/apt/apt.conf.d/docker-clean && \
    apt-get update && apt-get -y upgrade && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git curl ca-certificates pkg-config \
        zip unzip tar wget flex bison python3 perl nasm \
        autoconf automake libtool aria2 \
        libssl-dev zlib1g-dev libkrb5-dev && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} && \
    cd ${VCPKG_ROOT} && \
    ./bootstrap-vcpkg.sh -disableMetrics

# Stage 2: deps - Install dependencies
FROM base AS deps

ARG THEMIS_EDITION
ARG TARGETARCH

WORKDIR /build

COPY docker/vcpkg-*.json ./
RUN mkdir -p ./ports

RUN set -eux; \
    EDITION=$(echo "${THEMIS_EDITION}" | tr '[:upper:]' '[:lower:]'); \
    if [ -f "vcpkg-${EDITION}.json" ]; then \
        cp "vcpkg-${EDITION}.json" vcpkg.json; \
    else \
        echo "ERROR: vcpkg-${EDITION}.json not found"; exit 1; \
    fi; \
    case "${TARGETARCH}" in \
        amd64) echo "x64-linux" > /tmp/triplet.txt ;; \
        arm64) echo "arm64-linux" > /tmp/triplet.txt ;; \
        arm)   echo "arm-linux" > /tmp/triplet.txt ;; \
        *)     echo "ERROR: Unsupported arch ${TARGETARCH}"; exit 1 ;; \
    esac

RUN --mount=type=bind,from=prebuilt,target=/vcpkg-prebuilt,readonly \
    --mount=type=cache,target=/opt/vcpkg/downloads,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/packages,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/buildtrees,sharing=locked \
    set -eux; \
    TRIPLET=$(cat /tmp/triplet.txt); \
    mkdir -p /build/vcpkg_installed/${TRIPLET}; \
    export VCPKG_BINARY_SOURCES="clear;files,/opt/vcpkg/archives,readwrite"; \
    ${VCPKG_ROOT}/vcpkg install \
        --triplet="${TRIPLET}" \
        --x-manifest-root=/build \
        --x-install-root=/build/vcpkg_installed \
        --allow-unsupported \
        --clean-after-build || exit 1; \
    ln -sf /build/vcpkg_installed/${TRIPLET}/include /build/include; \
    ln -sf /build/vcpkg_installed/${TRIPLET}/lib /build/lib

# Stage 3: llama - Build llama.cpp
FROM base AS llama

ARG ENABLE_LLM
ARG LLAMA_CPP_REF
WORKDIR /opt

RUN if [ "$ENABLE_LLM" = "ON" ]; then \
    git clone https://github.com/ggerganov/llama.cpp.git /opt/llama.cpp && \
    cd /opt/llama.cpp && \
    git checkout "${LLAMA_CPP_REF}" && \
        cd /opt/llama.cpp && \
        rm -rf build && mkdir -p build && cd build && \
        cmake .. -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DLLAMA_BUILD_TESTS=OFF \
            -DLLAMA_BUILD_EXAMPLES=OFF \
            -DLLAMA_BUILD_SERVER=OFF \
            -DLLAMA_BUILD_SHARED_LIB=ON \
            -DLLAMA_NATIVE=OFF \
            -DLLAMA_AVX2=ON && \
        ninja -j4; \
    else \
        mkdir -p /opt/llama.cpp/build/bin; \
    fi

# Stage 3b: mini-llm - Prepare mini model
FROM base AS mini-llm

ARG ENABLE_LLM
ARG INCLUDE_MINI_LLM
WORKDIR /opt/themis-mini-llm

RUN mkdir -p /opt/themis-mini-llm/models

# Stage 4: build - Compile ThemisDB
FROM deps AS build

ARG THEMIS_EDITION
ARG ENABLE_LLM
ARG ENABLE_GPU
ARG INCLUDE_DOCS_DB
ARG FORCE_CPU_ONLY
ARG BUILD_TESTS
ARG BUILD_BENCHMARKS
ARG TARGETARCH

WORKDIR /src

COPY CMakeLists.txt VERSION LICENSE ./
COPY cmake ./cmake
COPY include ./include
COPY src ./src
COPY proto ./proto
COPY internal ./internal
COPY compendium ./compendium
COPY examples ./examples
COPY plugins ./plugins

RUN mkdir -p ./tests ./artifacts/docs-db
COPY tests/ ./tests/
COPY artifacts/docs-db/ ./artifacts/docs-db/

COPY --from=deps /build/vcpkg.json ./vcpkg.json
COPY --from=deps /tmp/triplet.txt /tmp/triplet.txt
COPY --from=llama /opt/llama.cpp /opt/llama.cpp

RUN --mount=type=bind,from=deps,source=/build/vcpkg_installed,target=/build/vcpkg_installed,readonly \
    set -eux; \
    TRIPLET=$(cat /tmp/triplet.txt); \
    EDITION_UPPER=$(echo "${THEMIS_EDITION}" | tr '[:lower:]' '[:upper:]'); \
    cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_TARGET_TRIPLET=${TRIPLET} \
        -DVCPKG_INSTALLED_DIR=/build/vcpkg_installed \
        -DVCPKG_MANIFEST_MODE=OFF \
        -DTHEMIS_EDITION=${EDITION_UPPER} \
        -DTHEMIS_DOCS_DB_MODE=$([ "${INCLUDE_DOCS_DB}" = "ON" ] && echo "PREBUILT" || echo "OFF") \
        -DTHEMIS_DOCS_DB_PREBUILT_PATH=/src/artifacts/docs-db \
        -DTHEMIS_ENABLE_LLM=${ENABLE_LLM} \
        -DTHEMIS_ENABLE_GPU=${ENABLE_GPU} \
        -DTHEMIS_ENABLE_VULKAN=$([ "${FORCE_CPU_ONLY}" = "ON" ] && echo "OFF" || echo "${ENABLE_GPU}") \
        -DTHEMIS_ENABLE_CUDA=OFF \
        -DTHEMIS_BUILD_TESTS=${BUILD_TESTS} \
        -DTHEMIS_MODELS_MODE=SKIP \
        -DTHEMIS_BUILD_BENCHMARKS=${BUILD_BENCHMARKS} \
        -DTHEMIS_ENABLE_TRACING=OFF \
        -DTHEMIS_STRICT_BUILD=OFF \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && \
    mkdir -p build/data && \
    ninja -C build -j$(nproc) themis_server && \
    if [ ! -f build/bin/themis_server ]; then \
        echo "ERROR: themis_server binary not found"; exit 1; \
    fi && \
    if [ "${BUILD_TESTS}" = "ON" ]; then \
        ninja -C build -j$(nproc) all_tests 2>/dev/null || ninja -C build -j$(nproc) || true; \
    fi && \
    if [ "${BUILD_BENCHMARKS}" = "ON" ]; then \
        ninja -C build -j$(nproc) all_benchmarks 2>/dev/null || ninja -C build -j$(nproc) || true; \
    fi

# Stage 4b: test - Run tests and benchmarks
FROM build AS test

ARG BUILD_TESTS=ON
ARG BUILD_BENCHMARKS=ON

WORKDIR /src/build

RUN if [ "${BUILD_TESTS}" = "ON" ]; then \
        echo "Running CTest suite..."; \
        ctest --output-on-failure --verbose 2>&1 | tee /tmp/ctest-results.log || true; \
        echo ""; \
        echo "CTest Summary:"; \
        ctest --output-on-failure -V 2>&1 | tail -20 || true; \
    fi

RUN if [ "${BUILD_BENCHMARKS}" = "ON" ]; then \
        echo "Listing available benchmarks..."; \
        find . -name "*benchmark*" -type f -executable 2>/dev/null | head -20 || true; \
    fi

# Output test and benchmark artifacts
RUN mkdir -p /test-artifacts && \
    if [ -f /tmp/ctest-results.log ]; then cp /tmp/ctest-results.log /test-artifacts/; fi && \
    find . -name "*.log" -path "*/Testing/*" 2>/dev/null | head -10 | xargs -I {} cp {} /test-artifacts/ 2>/dev/null || true

# Stage 5: runtime - Production image
FROM ubuntu:24.04 AS runtime

ARG THEMIS_EDITION
ARG THEMIS_ENABLE_ENCRYPTED_STORAGE=OFF

LABEL org.opencontainers.image.title="ThemisDB" \
      org.opencontainers.image.description="ThemisDB ${THEMIS_EDITION} Edition" \
      org.opencontainers.image.vendor="ThemisDB Project" \
      org.opencontainers.image.version="1.4.0"

ENV DEBIAN_FRONTEND=noninteractive \
    THEMIS_EDITION=${THEMIS_EDITION} \
    LD_LIBRARY_PATH=/opt/themis/bin \
    THEMIS_CONFIG_DIR=/etc/themis/config \
    THEMIS_DATA_DIR=/var/lib/themis/data \
    THEMIS_LOG_DIR=/var/log/themis \
    THEMIS_MODEL_DIR=/opt/themis/models

WORKDIR /opt/themis

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    rm -f /etc/apt/apt.conf.d/docker-clean && \
    apt-get update && apt-get -y upgrade && apt-get install -y --no-install-recommends \
        ca-certificates libssl3t64 zlib1g libstdc++6 libgomp1 curl libsodium23 && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

COPY --from=build /src/build/bin/themis_server /opt/themis/bin/themis_server
COPY --from=build /src/build/data/ /opt/themis/data/
COPY --from=build /src/artifacts/docs-db/ /opt/themis/data/
COPY --from=mini-llm /opt/themis-mini-llm/models/ /opt/themis/models/
COPY --from=llama /opt/llama.cpp/build/bin/ /opt/themis/bin/

RUN --mount=type=bind,from=deps,source=/build/vcpkg_installed,target=/deps_vcpkg,readonly \
    find /deps_vcpkg -name "*.so*" -path "*/lib/*" 2>/dev/null | head -50 | xargs -I {} cp -af {} /opt/themis/bin/ 2>/dev/null || true

COPY config/config.yaml /etc/themis/config/config.yaml
COPY config/pii_patterns.yaml /etc/themis/config/pii_patterns.yaml
RUN mkdir -p /etc/themis/config/ai_ml
COPY config/ai_ml/lora_training_config.yaml /etc/themis/config/ai_ml/lora_training_config.yaml

RUN mkdir -p /var/log/themis /var/lib/themis/data /opt/themis/models /opt/themis/data && \
    ln -sfn /etc/themis/config /opt/themis/config && \
    ln -sfn /var/log/themis /opt/themis/logs && \
    chmod 755 /var/log/themis /var/lib/themis /var/lib/themis/data && \
    useradd -r -u 1000 -d /opt/themis -s /bin/false themis 2>/dev/null || true && \
    chown -R themis:themis /opt/themis /etc/themis /var/lib/themis /var/log/themis

USER themis
EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

ENTRYPOINT ["/opt/themis/bin/themis_server"]
CMD ["--config=/etc/themis/config/config.yaml", "--data-dir=/var/lib/themis/data"]

# Stage 6: debug - Development image
FROM ubuntu:24.04 AS debug

ARG THEMIS_EDITION

ENV DEBIAN_FRONTEND=noninteractive \
    THEMIS_EDITION=${THEMIS_EDITION} \
    LD_LIBRARY_PATH=/usr/local/lib:/opt/themis/lib

WORKDIR /opt/themis

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get -y upgrade && apt-get install -y --no-install-recommends \
        ca-certificates libssl3t64 zlib1g libstdc++6 curl \
        gdb valgrind strace ltrace lsof htop vim less \
        netcat-openbsd telnet iproute2 dnsutils && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

COPY --from=build /src/build/bin/themis_server /opt/themis/bin/themis_server
COPY --from=build /src/build/compile_commands.json /opt/themis/
COPY --from=llama /opt/llama.cpp/build/lib*.so* /usr/local/lib/
RUN --mount=type=bind,from=deps,source=/build/vcpkg_installed,target=/deps_vcpkg,readonly \
    mkdir -p /opt/themis/lib && \
    cp -a /deps_vcpkg/*/lib/*.so* /opt/themis/lib/ 2>/dev/null || true

COPY --from=build /src /src

RUN ldconfig && mkdir -p /data && chown -R root:root /opt/themis /data

EXPOSE 8080
ENTRYPOINT ["/opt/themis/bin/themis_server"]
CMD ["--config=/etc/themis/config.yml", "--data-dir=/data"]
