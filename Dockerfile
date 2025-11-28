# Multi-stage Docker build for ThemisDB
# Hybrid approach: System packages + vcpkg for missing libraries

FROM ubuntu:22.04 AS build
ENV DEBIAN_FRONTEND=noninteractive

# Install build tools and available system dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build git curl zip unzip tar pkg-config \
    ca-certificates python3 perl nasm \
    # Core system libraries
    libssl-dev libcurl4-openssl-dev zlib1g-dev \
    # Available dependencies from Ubuntu 22.04
    librocksdb-dev libtbb-dev libfmt-dev \
    nlohmann-json3-dev libboost-system-dev libboost-thread-dev \
    libyaml-cpp-dev libzstd-dev libsnappy-dev liblz4-dev \
    libbz2-dev libgflags-dev \
    && rm -rf /var/lib/apt/lists/*

# Bootstrap vcpkg for missing packages (simdjson, spdlog, hnswlib)
ENV VCPKG_ROOT=/opt/vcpkg
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} \
    && ${VCPKG_ROOT}/bootstrap-vcpkg.sh -disableMetrics

# Set up environment
ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++
ARG VCPKG_TRIPLET=x64-linux
ENV VCPKG_DEFAULT_TRIPLET=${VCPKG_TRIPLET}

WORKDIR /src

# Install only missing packages via vcpkg
RUN ${VCPKG_ROOT}/vcpkg install \
    simdjson:${VCPKG_TRIPLET} \
    spdlog:${VCPKG_TRIPLET} \
    hnswlib:${VCPKG_TRIPLET}

# Copy source code
COPY CMakeLists.txt ./
COPY include ./include
COPY src ./src

# Build ThemisDB
RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
    -DTHEMIS_BUILD_TESTS=OFF \
    -DTHEMIS_BUILD_BENCHMARKS=OFF \
    -DTHEMIS_ENABLE_TRACING=OFF \
    && cmake --build build --target themis_server -j$(nproc)

# Runtime stage - minimal Ubuntu image
FROM ubuntu:22.04 AS runtime
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

# Copy built binary
COPY --from=build /src/build/themis_server /usr/local/bin/themis_server

# Copy vcpkg installed libraries that are needed at runtime
COPY --from=build /opt/vcpkg/installed/x64-linux/lib/*.so* /usr/local/lib/ || true

# Copy configuration files
RUN mkdir -p /etc/themis /usr/local/share/themis
COPY --from=build /src/config/config.json /etc/themis/config.json || true
COPY --from=build /src/config/config.qnap.json /usr/local/share/themis/config.qnap.json || true

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
