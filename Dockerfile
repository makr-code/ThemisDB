# syntax=docker/dockerfile:1.7
# ThemisDB production Docker build
# Root Dockerfile: canonical entry point for Docker Desktop and buildx.
# Docker assets (config, manifests, compose files) live under ./docker.

ARG THEMIS_EDITION=COMMUNITY
ARG ENABLE_LLM=ON
ARG ENABLE_GPU=ON
ARG FORCE_CPU_ONLY=OFF
ARG BUILD_TESTS=OFF
ARG BUILD_BENCHMARKS=OFF
ARG TARGETARCH=amd64
ARG LLAMA_CPP_REF=1e8924fd65ad349d1d838412a2172292618f3bbf

FROM ubuntu:24.04 AS base

ENV DEBIAN_FRONTEND=noninteractive \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    VCPKG_ROOT=/opt/vcpkg \
    VCPKG_FORCE_SYSTEM_BINARIES=1 \
    VCPKG_DISABLE_METRICS=1 \
    VCPKG_DOWNLOADS=/opt/vcpkg/downloads \
    VCPKG_ALLOWED_DOWNLOADER_TOOLS=aria2 \
    VCPKG_USE_ARIA2=ON

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git curl ca-certificates pkg-config \
        zip unzip tar wget flex bison python3 perl nasm autoconf automake libtool \
        aria2 libssl-dev zlib1g-dev libkrb5-dev libvulkan-dev && \
    apt-get clean && rm -rf /var/lib/apt/lists/* && \
    if [ ! -d "${VCPKG_ROOT}/.git" ]; then \
        rm -rf "${VCPKG_ROOT}" && \
        git clone --depth=1 https://github.com/microsoft/vcpkg.git "${VCPKG_ROOT}"; \
    fi && \
    mkdir -p "${VCPKG_DOWNLOADS}" "${VCPKG_ROOT}/buildtrees" "${VCPKG_ROOT}/packages" && \
    cd "${VCPKG_ROOT}" && ./bootstrap-vcpkg.sh -disableMetrics

FROM base AS deps

ARG THEMIS_EDITION
ARG TARGETARCH
WORKDIR /build

COPY docker/vcpkg-*.json ./docker/
RUN set -eux; \
    EDITION=$(echo "${THEMIS_EDITION}" | tr '[:upper:]' '[:lower:]'); \
    if [ -f "docker/vcpkg-${EDITION}.json" ]; then \
        cp "docker/vcpkg-${EDITION}.json" /build/vcpkg.json; \
    elif [ -f "docker/vcpkg.json" ]; then \
        cp "docker/vcpkg.json" /build/vcpkg.json; \
    else \
        echo "ERROR: no vcpkg manifest found for edition ${THEMIS_EDITION}"; exit 1; \
    fi; \
    case "${TARGETARCH}" in \
        amd64) echo "x64-linux" > /tmp/triplet.txt ;; \
        arm64) echo "arm64-linux" > /tmp/triplet.txt ;; \
        arm) echo "arm-linux" > /tmp/triplet.txt ;; \
        *) echo "ERROR: Unsupported arch ${TARGETARCH}"; exit 1 ;; \
    esac

RUN --mount=type=cache,target=/opt/vcpkg/downloads,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/buildtrees,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/packages,sharing=locked \
    --mount=type=cache,target=/root/.cache,sharing=locked \
    set -eux; \
    TRIPLET=$(cat /tmp/triplet.txt); \
    export VCPKG_BINARY_SOURCES="clear;files,/opt/vcpkg/packages,readwrite"; \
    ${VCPKG_ROOT}/vcpkg install \
        --triplet="${TRIPLET}" \
        --x-manifest-root=/build \
        --x-install-root=/build/vcpkg_installed \
        --allow-unsupported \
        --debug \
        --downloads-root=/opt/vcpkg/downloads; \
    ln -sfn /build/vcpkg_installed/${TRIPLET}/include /build/include; \
    ln -sfn /build/vcpkg_installed/${TRIPLET}/lib /build/lib

FROM base AS llama

ARG ENABLE_LLM
ARG LLAMA_CPP_REF
WORKDIR /opt

RUN if [ "${ENABLE_LLM}" = "ON" ]; then \
        git clone https://github.com/ggerganov/llama.cpp.git /opt/llama.cpp && \
        cd /opt/llama.cpp && \
        git checkout "${LLAMA_CPP_REF}" && \
        cmake -S . -B build -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DLLAMA_BUILD_TESTS=OFF \
            -DLLAMA_BUILD_EXAMPLES=OFF \
            -DLLAMA_BUILD_SERVER=OFF \
            -DLLAMA_BUILD_SHARED_LIB=ON \
            -DLLAMA_NATIVE=OFF && \
        cmake --build build --parallel $(nproc); \
    else \
        mkdir -p /opt/llama.cpp/build/bin; \
    fi

FROM deps AS build

ARG THEMIS_EDITION
ARG ENABLE_LLM
ARG ENABLE_GPU
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
COPY plugins ./plugins
COPY tests ./tests
COPY artifacts ./artifacts
COPY docker ./docker
COPY --from=deps /build/vcpkg.json ./vcpkg.json
COPY --from=deps /tmp/triplet.txt /tmp/triplet.txt
COPY --from=llama /opt/llama.cpp /opt/llama.cpp

RUN --mount=type=cache,target=/opt/vcpkg/downloads,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/buildtrees,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/packages,sharing=locked \
    set -eux; \
    TRIPLET=$(cat /tmp/triplet.txt); \
    EDITION_UPPER=$(echo "${THEMIS_EDITION}" | tr '[:lower:]' '[:upper:]'); \
    cmake -S /src -B /src/build -G Ninja \
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
        -DTHEMIS_MODELS_MODE=SKIP \
        -DTHEMIS_BUILD_BENCHMARKS=${BUILD_BENCHMARKS} \
        -DTHEMIS_ENABLE_TRACING=OFF \
        -DTHEMIS_STRICT_BUILD=OFF \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && \
    cmake --build /src/build --parallel $(nproc) --target themis_server && \
    test -f /src/build/bin/themis_server

FROM ubuntu:24.04 AS runtime

ARG THEMIS_EDITION
LABEL org.opencontainers.image.title="ThemisDB" \
    org.opencontainers.image.description="ThemisDB ${THEMIS_EDITION} runtime image" \
    org.opencontainers.image.source="https://github.com/makr-code/ThemisDB" \
    org.opencontainers.image.vendor="ThemisDB Project" \
    org.opencontainers.image.version="1.4.0"

ENV DEBIAN_FRONTEND=noninteractive \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    THEMIS_EDITION=${THEMIS_EDITION} \
    LD_LIBRARY_PATH=/opt/themis/lib \
    THEMIS_CONFIG_DIR=/etc/themis/config \
    THEMIS_DATA_DIR=/var/lib/themis/data \
    THEMIS_LOG_DIR=/var/log/themis \
    THEMIS_MODEL_DIR=/opt/themis/models

WORKDIR /opt/themis

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates libssl3t64 zlib1g libstdc++6 libgomp1 curl libsodium23 libvulkan1 && \
    apt-get clean && rm -rf /var/lib/apt/lists/* && \
    mkdir -p /opt/themis/lib /etc/themis/config /var/log/themis /var/lib/themis/data /opt/themis/models

COPY --from=build /src/build/bin/themis_server /opt/themis/themis_server
COPY --from=build /src/build/lib/ /opt/themis/lib/
COPY --from=build /src/build/data/ /opt/themis/data/
COPY docker/config /etc/themis/config

RUN chmod 755 /opt/themis /opt/themis/lib /var/log/themis /var/lib/themis /var/lib/themis/data && \
    useradd -r -u 1000 -d /opt/themis -s /bin/false themis 2>/dev/null || true && \
    chown -R themis:themis /opt/themis /etc/themis /var/lib/themis /var/log/themis && \
    mkdir -p /data && chown -R root:root /opt/themis /data && \
    ldconfig

USER themis
EXPOSE 8080 18765
ENTRYPOINT ["/opt/themis/themis_server"]
CMD ["--config=/etc/themis/config.yml", "--data-dir=/data"]
