#!/bin/bash
# Simple syntax check for modular build configuration
# This validates CMake configuration without requiring full dependencies

set -e

echo "=========================================="
echo "ThemisDB Modular Build Syntax Check"
echo "=========================================="

# Create temporary test directory
TEST_DIR=$(mktemp -d)
echo "Test directory: $TEST_DIR"

# Create minimal CMakeLists.txt that only checks ModularBuild.cmake syntax
cat > "$TEST_DIR/CMakeLists.txt" << 'EOF'
cmake_minimum_required(VERSION 3.20)
project(ThemisDB_ModularCheck VERSION 1.4.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set required variables that ModularBuild.cmake expects
set(THEMIS_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(THEMIS_ROCKSDB_TARGET "RocksDB::rocksdb")
set(THEMIS_ARROW_TARGET "Arrow::arrow_shared")
set(THEMIS_PARQUET_TARGET "Parquet::parquet_shared")
set(THEMIS_YAML_TARGET "yaml-cpp::yaml-cpp")

# Feature flags
option(THEMIS_ENABLE_HTTP_SERVER "Enable HTTP server" ON)
option(THEMIS_ENABLE_GRPC "Enable gRPC" OFF)
option(THEMIS_ENABLE_LLM "Enable LLM" OFF)
option(THEMIS_ENABLE_CONTENT "Enable content processing" OFF)
option(THEMIS_ENABLE_OFFICE "Enable office formats" OFF)
option(THEMIS_ENABLE_SSE "Enable SSE" OFF)
option(THEMIS_ENABLE_HTTP2 "Enable HTTP/2" OFF)
option(THEMIS_ENABLE_HTTP3 "Enable HTTP/3" OFF)
option(THEMIS_ENABLE_WEBSOCKET "Enable WebSocket" OFF)
option(THEMIS_ENABLE_MQTT "Enable MQTT" OFF)
option(THEMIS_ENABLE_POSTGRES_WIRE "Enable PostgreSQL wire protocol" OFF)
option(THEMIS_ENABLE_MCP "Enable MCP" OFF)
option(THEMIS_ENABLE_GRAPHQL "Enable GraphQL" OFF)
option(THEMIS_ENABLE_CUDA "Enable CUDA" OFF)
option(THEMIS_ENABLE_OPENCL "Enable OpenCL" OFF)

# Enable modular build
option(THEMIS_BUILD_MODULAR "Build as modular libraries" ON)

# Include the modular build configuration
include(cmake/ModularBuild.cmake)

message(STATUS "✓ ModularBuild.cmake syntax check passed")
EOF

# Copy ModularBuild.cmake
mkdir -p "$TEST_DIR/cmake"
cp cmake/ModularBuild.cmake "$TEST_DIR/cmake/"

# Create dummy src directories (CMake checks if source files exist during configuration)
mkdir -p "$TEST_DIR/src/utils" "$TEST_DIR/src/storage" "$TEST_DIR/src/query" \
         "$TEST_DIR/src/security" "$TEST_DIR/src/transaction" "$TEST_DIR/src/network" \
         "$TEST_DIR/src/sharding" "$TEST_DIR/src/llm" "$TEST_DIR/src/content" \
         "$TEST_DIR/src/timeseries" "$TEST_DIR/src/geo" "$TEST_DIR/src/graph" \
         "$TEST_DIR/src/base" "$TEST_DIR/src/core/concerns" "$TEST_DIR/src/acceleration" \
         "$TEST_DIR/src/plugins"

# Create dummy .cpp files for sources referenced in ModularBuild.cmake
# (CMake won't error on missing files during configuration, only during build)
echo "Syntax check only - files don't need to exist for CMake configuration"

# Run CMake configuration
cd "$TEST_DIR"
cmake . -DCMAKE_BUILD_TYPE=Release 2>&1 | tee cmake_output.log

# Check for success
if grep -q "ModularBuild.cmake syntax check passed" cmake_output.log; then
    echo "=========================================="
    echo "✓ SUCCESS: Modular build configuration valid"
    echo "=========================================="
    exit 0
else
    echo "=========================================="
    echo "✗ FAILED: Modular build configuration has errors"
    echo "=========================================="
    cat cmake_output.log
    exit 1
fi
