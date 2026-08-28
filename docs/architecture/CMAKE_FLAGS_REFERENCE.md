# ThemisDB CMake Flags Reference

**Version**: v1.3.0+  
**Status**: Comprehensive Build System Documentation  
**Related**: See [FEATURE_FLAGS_REFERENCE.md](FEATURE_FLAGS_REFERENCE.md) for detailed feature documentation

## Overview

This document provides a comprehensive reference for CMake flags and build system configuration in ThemisDB. It covers:

1. **Edition Selection** - Choose your deployment edition (MINIMAL, COMMUNITY, ENTERPRISE, HYPERSCALER)
2. **Feature Flags** - Enable/disable optional features
3. **Build Options** - Control compilation and testing
4. **Optimization Flags** - Performance tuning
5. **vcpkg Configuration** - Dependency management and binary caching

## Quick Start

### Basic Build
```bash
# Default build (COMMUNITY edition)
cmake -S . -B build
cmake --build build --config Release
```

### With Binary Caching (Fast Builds)
```bash
# Set up vcpkg binary cache for 93% faster builds
export VCPKG_BINARY_SOURCES="clear;files,$HOME/.cache/vcpkg/archives,readwrite"

cmake -S . -B build
cmake --build build --config Release
```

### Custom Edition
```bash
# MINIMAL: Smallest footprint (~80 MB)
cmake -S . -B build -DTHEMIS_EDITION=MINIMAL

# ENTERPRISE: Production features (~250 MB)
cmake -S . -B build -DTHEMIS_EDITION=ENTERPRISE

# HYPERSCALER: All features (~500 MB)
cmake -S . -B build -DTHEMIS_EDITION=HYPERSCALER
```

## Build System Architecture

### Module Load Order

The build system loads modules in this critical order:

```
1. Versions.cmake                      - Version management
2. CompilerOptions.cmake               - Compiler flags, LTCG/LTO
3. platforms/PlatformDetection.cmake   - OS, CPU, triplet detection
4. platforms/ArchitectureOptimizations.cmake - AVX2, NEON flags
5. PreloadTargets.cmake                - System package preloading
6. editions/EditionDefaults.cmake      - Edition selection
7. features/FeatureDefaults.cmake      - Feature configuration
8. EditionMatrix.cmake                 - Feature validation
9. VcpkgConfiguration.cmake            - vcpkg setup, binary cache
10. Dependencies.cmake                 - find_package() calls
11. validation/*.cmake                 - Final validation
```

**Why This Order Matters:**
- Features must be configured BEFORE vcpkg setup (to enable correct vcpkg features)
- Edition must be selected BEFORE features (editions set feature defaults)
- Platform detection BEFORE optimizations (to apply correct flags)

### New in v1.3.0: Build Optimizations

1. **VcpkgConfiguration.cmake** - Centralized vcpkg setup with automatic binary caching
2. **EditionMatrix.cmake** - Validates features against edition constraints
3. **LTCG/LTO** - Explicit opt-in for Release builds (`THEMIS_ENABLE_IPO=ON`)
4. **Consolidated Flags** - Removed duplicate AVX2 flags across modules

## Edition System

### THEMIS_EDITION

**Type**: `MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER`  
**Default**: `COMMUNITY`

```bash
cmake -S . -B build -DTHEMIS_EDITION=ENTERPRISE
```

### Edition Comparison

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **Binary Size** | 80 MB | 200 MB | 250 MB | 500 MB |
| **Build Time** | 5 min | 10 min | 15 min | 30 min |
| **LLM Integration** | ❌ | ✅ Optional | ✅ Optional | ✅ Required |
| **GPU Acceleration** | ❌ | ✅ Optional | ✅ Optional | ✅ Required |
| **gRPC (Sharding)** | ❌ | ✅ Optional | ✅ Required | ✅ Required |
| **Tracing** | ❌ | ✅ Optional | ✅ Required | ✅ Required |
| **HTTP/2** | ✅ | ✅ | ✅ | ✅ |
| **WebSocket** | ✅ | ✅ | ✅ | ✅ |
| **MQTT** | ✅ | ✅ | ✅ | ✅ |
| **Target** | IoT/Edge | Small-Medium | Production | Cloud Scale |

### Edition-Feature Matrix

The new `EditionMatrix.cmake` module validates feature flags against edition constraints:

- **REQUIRED**: Feature must be ON (will be forced ON if OFF)
- **ALLOWED**: Feature can be toggled ON/OFF by user
- **FORBIDDEN**: Feature must be OFF (will be forced OFF if ON)

Example:
```bash
# This will WARN and force THEMIS_ENABLE_LLM=OFF
cmake -S . -B build -DTHEMIS_EDITION=MINIMAL -DTHEMIS_ENABLE_LLM=ON

# This will WARN and force THEMIS_ENABLE_GRPC=ON
cmake -S . -B build -DTHEMIS_EDITION=ENTERPRISE -DTHEMIS_ENABLE_GRPC=OFF
```

## Core Feature Flags

For detailed feature documentation, see [FEATURE_FLAGS_REFERENCE.md](FEATURE_FLAGS_REFERENCE.md).

### THEMIS_ENABLE_LLM

**Purpose**: Large Language Model integration  
**Type**: `ON | OFF`  
**Default**: Edition-dependent (HYPERSCALER: ON, others: OFF)  
**Dependencies**: llama.cpp, optional CUDA Toolkit

```bash
cmake -S . -B build -DTHEMIS_ENABLE_LLM=ON
```

**vcpkg Feature**: `llm`  
**Binary Impact**: +150 MB  
**Build Time**: +60 seconds

### THEMIS_ENABLE_GPU

**Purpose**: GPU acceleration for vector search  
**Type**: `ON | OFF`  
**Default**: Edition-dependent (HYPERSCALER: ON, others: OFF)  
**Dependencies**: FAISS, OpenBLAS, LAPACK

```bash
cmake -S . -B build -DTHEMIS_ENABLE_GPU=ON
```

**vcpkg Feature**: `gpu`  
**Binary Impact**: +200 MB  
**Build Time**: +40 seconds

### THEMIS_ENABLE_CUDA

**Purpose**: NVIDIA CUDA support  
**Type**: `ON | OFF`  
**Default**: `OFF`  
**Dependencies**: CUDA Toolkit 12.4+, Compute Capability 6.1+

```bash
cmake -S . -B build \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DCUDA_TOOLKIT_ROOT_DIR="/usr/local/cuda"
```

**vcpkg Feature**: `cuda`  
**Binary Impact**: +100 MB  
**Build Time**: +80 seconds

### THEMIS_ENABLE_GRPC

**Purpose**: gRPC for inter-shard communication  
**Type**: `ON | OFF`  
**Default**: Edition-dependent (ENTERPRISE/HYPERSCALER: ON, others: OFF)  
**Dependencies**: gRPC++, protobuf

```bash
cmake -S . -B build -DTHEMIS_ENABLE_GRPC=ON
```

**vcpkg Feature**: `rpc`  
**Binary Impact**: +50 MB  
**Build Time**: +15 seconds

### THEMIS_ENABLE_TRACING

**Purpose**: OpenTelemetry observability  
**Type**: `ON | OFF`  
**Default**: Edition-dependent (ENTERPRISE/HYPERSCALER: ON, others: OFF)  
**Dependencies**: opentelemetry-cpp

```bash
cmake -S . -B build -DTHEMIS_ENABLE_TRACING=ON
```

**Binary Impact**: +30 MB  
**Build Time**: +10 seconds

## Protocol Flags

### THEMIS_ENABLE_HTTP2

**Purpose**: HTTP/2 protocol support  
**Type**: `ON | OFF`  
**Default**: `ON`  
**Dependencies**: nghttp2

**vcpkg Feature**: `http2`

### THEMIS_ENABLE_HTTP3

**Purpose**: HTTP/3 (QUIC) protocol support  
**Type**: `ON | OFF`  
**Default**: `OFF` (experimental)  
**Dependencies**: nghttp3, ngtcp2

**vcpkg Feature**: `http3`

### THEMIS_ENABLE_WEBSOCKET

**Purpose**: WebSocket protocol support  
**Type**: `ON | OFF`  
**Default**: `ON`  
**Dependencies**: Boost.Beast (included)

**vcpkg Feature**: `websocket`

### THEMIS_ENABLE_MQTT

**Purpose**: MQTT protocol for IoT  
**Type**: `ON | OFF`  
**Default**: `ON`  
**Dependencies**: paho-mqttpp3

**vcpkg Feature**: `mqtt`

### THEMIS_ENABLE_POSTGRES_WIRE

**Purpose**: PostgreSQL wire protocol compatibility  
**Type**: `ON | OFF`  
**Default**: `ON`  
**Dependencies**: None (built-in)

**vcpkg Feature**: `postgres-wire`

### THEMIS_ENABLE_MCP

**Purpose**: Model Context Protocol for LLM integration  
**Type**: `ON | OFF`  
**Default**: `OFF`  
**Dependencies**: None (built-in)

**vcpkg Feature**: `mcp`

## Build Configuration Flags

### THEMIS_BUILD_TESTS

**Purpose**: Compile unit tests  
**Type**: `ON | OFF`  
**Default**: `Debug: ON / Release: OFF`

```bash
cmake -S . -B build -DTHEMIS_BUILD_TESTS=OFF
```

**Binary Impact**: +200 MB  
**Build Time**: +30 seconds

### THEMIS_BUILD_BENCHMARKS

**Purpose**: Compile performance benchmarks  
**Type**: `ON | OFF`  
**Default**: `OFF`

```bash
cmake -S . -B build -DTHEMIS_BUILD_BENCHMARKS=OFF
```

**Binary Impact**: +300 MB  
**Build Time**: +60 seconds

### THEMIS_MODULES_ENABLE_UNITY

**Purpose**: Enable Unity Build for modular MSVC targets  
**Type**: `ON | OFF`  
**Default**: `OFF`

```bash
cmake -S . -B build -DTHEMIS_MODULES_ENABLE_UNITY=ON
```

**Note**: Unity should be enabled deliberately (for selected CI/build profiles), not by default for local dev.

### THEMIS_ENABLE_IPO

**Purpose**: Enable IPO/LTO in Release builds  
**Type**: `ON | OFF`  
**Default**: `OFF`

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTHEMIS_ENABLE_IPO=ON
```

**Note**: `THEMIS_ENABLE_IPO=ON` has effect only for `CMAKE_BUILD_TYPE=Release`.

### THEMIS_STRICT_BUILD

**Purpose**: Treat warnings as errors  
**Type**: `ON | OFF`  
**Default**: `OFF`

```bash
cmake -S . -B build -DTHEMIS_STRICT_BUILD=ON
```

**Compiler Flags**:
- MSVC: `/WX`
- GCC/Clang: `-Werror`

### THEMIS_ENABLE_ASAN

**Purpose**: AddressSanitizer for memory debugging  
**Type**: `ON | OFF`  
**Default**: `OFF`

```bash
cmake -S . -B build -DTHEMIS_ENABLE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug
```

**Binary Impact**: +50 MB  
**Runtime**: 3x slower (for debugging)

## Optimization Flags

### THEMIS_ENABLE_AVX2

**Purpose**: Enable AVX2 SIMD instructions  
**Type**: `ON | OFF`  
**Default**: `OFF` (auto-detected in Release mode)

```bash
cmake -S . -B build -DTHEMIS_ENABLE_AVX2=ON -DCMAKE_BUILD_TYPE=Release
```

**Compiler Flags**:
- MSVC: `/arch:AVX2`
- GCC/Clang: `-mavx2 -mfma`

**Note**: Only applies to x86_64. Automatically disabled for ARM.

### User Override Semantics

- Defaults are applied only when an option is not explicitly provided by user/preset.
- `THEMIS_BENCHMARK_MODE=ON` prefers enabling measurement flags, but explicit user `OFF` overrides remain valid.

### THEMIS_QNAP_BUILD

**Purpose**: Optimize for QNAP NAS (Celeron N5095)  
**Type**: `ON | OFF`  
**Default**: `OFF`

```bash
cmake -S . -B build -DTHEMIS_QNAP_BUILD=ON
```

**Effect**: Forces SSE4.2 baseline, disables AVX2.

### THEMIS_ENABLE_ARM_NEON

**Purpose**: Enable ARM NEON SIMD (ARMv7 only)  
**Type**: `ON | OFF`  
**Default**: `ON` (auto-enabled on ARMv7)

**Compiler Flags**: `-march=armv7-a -mfpu=neon -mfloat-abi=hard`

**Note**: ARM64 (AArch64) always has NEON.

## New: Link-Time Code Generation (LTCG/LTO)

### CMAKE_INTERPROCEDURAL_OPTIMIZATION

**Purpose**: Enable Link-Time Code Generation for whole-program optimization  
**Type**: Automatic in Release mode  
**Performance**: 15% speedup, 10% binary size reduction

Automatically enabled in `CompilerOptions.cmake` for Release builds:

**MSVC**: `/GL` (compile) + `/LTCG:INCREMENTAL` (link)  
**GCC/Clang**: `-flto`

**Build Time Impact**: +20% (first build), incremental builds faster with `/LTCG:INCREMENTAL`

To disable:
```bash
cmake -S . -B build -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF
```

## vcpkg Configuration

### New: VcpkgConfiguration.cmake

Centralized vcpkg setup with automatic features:

1. **Binary Cache Auto-Configuration**: Speeds up builds by 93% (30 min → 2 min)
2. **Feature Dependency Mapping**: Automatically enables vcpkg features based on CMake flags
3. **Dependency Validation**: Warns if required packages are missing

### VCPKG_BINARY_SOURCES

**Purpose**: Enable binary package caching  
**Type**: Environment variable  
**Default**: Local filesystem cache

```bash
# Local filesystem cache (default)
export VCPKG_BINARY_SOURCES="clear;files,$HOME/.cache/vcpkg/archives,readwrite"

# GitHub Actions cache
export VCPKG_BINARY_SOURCES="clear;x-gha,readwrite"

# NuGet cache
export VCPKG_BINARY_SOURCES="clear;nuget,https://nuget.example.com/v3/index.json,readwrite"
```

**Impact**: First build with cache: **2 minutes** vs **30 minutes** without

### vcpkg Feature Mapping

CMake flags automatically map to vcpkg features:

| CMake Flag | vcpkg Feature | Packages |
|------------|---------------|----------|
| `THEMIS_ENABLE_GPU=ON` | `gpu` | faiss, openblas, lapack |
| `THEMIS_ENABLE_LLM=ON` | `llm` | (llama.cpp is external) |
| `THEMIS_ENABLE_GRPC=ON` | `rpc` | grpc, protobuf |
| `THEMIS_ENABLE_HTTP2=ON` | `http2` | nghttp2 |
| `THEMIS_ENABLE_HTTP3=ON` | `http3` | nghttp3, ngtcp2 |
| `THEMIS_ENABLE_MQTT=ON` | `mqtt` | paho-mqttpp3 |
| `THEMIS_ENABLE_GDAL=ON` | `gdal` | gdal |

### vcpkg Baseline

**Current**: `01f602195983451bc83e72f4214af2cbc495aa94` (2025-01-15)  
**Previous**: `bee7b66f0219eeb463dc1ff77ad6ad0211f94f48` (2024-12-14)

**Changed**: Updated to latest stable baseline for security patches and bug fixes.

### Version Constraints

New in v1.3.0: Explicit version constraints for core dependencies:

```json
{
  "dependencies": [
    { "name": "openssl", "version>=": "3.0.0" },
    { "name": "simdjson", "version>=": "3.0.0" },
    { "name": "boost-asio", "version>=": "1.83.0" },
    { "name": "grpc", "version>=": "1.50.0" }
  ]
}
```

**Benefit**: Prevents accidental downgrades, ensures security patches.

## Performance Impact Summary

### Build Time Improvements

| Optimization | First Build | Incremental | Benefit |
|--------------|-------------|-------------|---------|
| **Binary Cache** | 2 min (was 30 min) | 1 min (was 5 min) | **93% / 80%** |
| **Consolidated Flags** | -10 sec | -5 sec | Cleaner output |
| **LTCG/LTO** | +20% (one-time) | Faster | **15% runtime speedup** |
| **Feature Validation** | +5 sec | +2 sec | Prevents errors |
| **Total Gain** | **~28 min saved** | **~4 min saved** | **85-93%** |

### Binary Size Improvements

| Configuration | Before | After | Savings |
|---------------|--------|-------|---------|
| **Minimal Build** | 100 MB | 80 MB | **20%** |
| **Community** | 250 MB | 200 MB | **20%** |
| **Enterprise** | 300 MB | 250 MB | **17%** |
| **Hyperscaler (with unused features)** | 700 MB | 500 MB | **29%** |

**Cause**: Removed transitive dependencies, LTCG dead code elimination.

## Common Configurations

### Development Build (Fast Iteration)
```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_BUILD_BENCHMARKS=OFF \
  -DTHEMIS_STRICT_BUILD=OFF
```

### Production Build (Standard)
```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=COMMUNITY \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_BUILD_BENCHMARKS=OFF
```

### Production Build (Full Features)
```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_CUDA=ON
```

### IoT/Edge Build (Minimal)
```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DTHEMIS_EDITION=MINIMAL \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_BUILD_BENCHMARKS=OFF
```

### CI/CD Build (Fast + Coverage)
```bash
export VCPKG_BINARY_SOURCES="clear;x-gha,readwrite"

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_STRICT_BUILD=ON
```

## Troubleshooting

### Build Takes Too Long

**Problem**: Build exceeds 30 minutes

**Solutions**:
1. ✅ Enable binary cache: `export VCPKG_BINARY_SOURCES="clear;files,$HOME/.cache/vcpkg,readwrite"`
2. ✅ Disable tests: `-DTHEMIS_BUILD_TESTS=OFF`
3. ✅ Disable benchmarks: `-DTHEMIS_BUILD_BENCHMARKS=OFF`
4. ✅ Use Ninja generator: `-G Ninja`
5. ✅ Increase parallel jobs: `-j 16`

### Feature Not Available at Runtime

**Problem**: CMake flag was ON, but feature not working

**Check**:
```bash
# 1. Verify compile-time flags
./themis_server --version --features

# 2. Check CMake configuration
grep "THEMIS_ENABLE_LLM" build/CMakeCache.txt

# 3. Review feature validation output
cat build/CMakeFiles/CMakeOutput.log | grep "THEMIS_"
```

### Edition Matrix Conflict

**Problem**: Warning about feature being FORBIDDEN or REQUIRED

**Example**:
```
WARNING: THEMIS_ENABLE_LLM is FORBIDDEN for MINIMAL edition, forcing OFF
```

**Solution**: Either:
1. Change edition: `-DTHEMIS_EDITION=COMMUNITY`
2. Accept the constraint (LLM will be disabled)

### vcpkg Feature Not Enabled

**Problem**: CMake flag ON, but vcpkg feature not installed

**Check**:
```bash
# View active vcpkg features
grep "VCPKG_MANIFEST_FEATURES" build/CMakeCache.txt

# Verify vcpkg.json features
cat vcpkg.json | grep -A 5 "features"
```

**Solution**: Reconfigure CMake (features are set at configure-time):
```bash
cmake --fresh -S . -B build
```

## References

- [Feature Flags Reference](FEATURE_FLAGS_REFERENCE.md) - Detailed feature documentation
- [CMake Modular Architecture](CMAKE_MODULAR_ARCHITECTURE.md) - Build system design
- [vcpkg Documentation](https://vcpkg.io/) - Package manager reference
- [OpenTelemetry](https://opentelemetry.io/) - Observability documentation

## Appendix: All CMake Flags

### Edition
- `THEMIS_EDITION` (MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER)

### Core Features
- `THEMIS_ENABLE_LLM` (ON | OFF)
- `THEMIS_ENABLE_GPU` (ON | OFF)
- `THEMIS_ENABLE_CUDA` (ON | OFF)
- `THEMIS_ENABLE_HIP` (ON | OFF)
- `THEMIS_ENABLE_ONEAPI` (ON | OFF)
- `THEMIS_ENABLE_OPENCL` (ON | OFF)
- `THEMIS_ENABLE_GRPC` (ON | OFF)
- `THEMIS_ENABLE_TRACING` (ON | OFF)

### Protocols
- `THEMIS_ENABLE_HTTP2` (ON | OFF)
- `THEMIS_ENABLE_HTTP3` (ON | OFF)
- `THEMIS_ENABLE_WEBSOCKET` (ON | OFF)
- `THEMIS_ENABLE_MQTT` (ON | OFF)
- `THEMIS_ENABLE_POSTGRES_WIRE` (ON | OFF)
- `THEMIS_ENABLE_MCP` (ON | OFF)
- `THEMIS_ENABLE_SSE` (ON | OFF)

### Optimizations
- `THEMIS_ENABLE_DISKANN` (ON | OFF)
- `THEMIS_ENABLE_WISCKEY` (ON | OFF)
- `THEMIS_ENABLE_MIMALLOC` (ON | OFF)
- `THEMIS_ENABLE_AVX2` (ON | OFF)
- `THEMIS_ENABLE_ARM_NEON` (ON | OFF)
- `THEMIS_ENABLE_HUGE_PAGES` (ON | OFF)
- `THEMIS_QNAP_BUILD` (ON | OFF)

### Content Processing
- `THEMIS_ENABLE_CONTENT` (ON | OFF)
- `THEMIS_ENABLE_CONTENT_PROCESSORS` (ON | OFF)
- `THEMIS_ENABLE_VISION` (ON | OFF)

### Build Options
- `THEMIS_BUILD_TESTS` (ON | OFF)
- `THEMIS_BUILD_BENCHMARKS` (ON | OFF)
- `THEMIS_STRICT_BUILD` (ON | OFF)
- `THEMIS_ENABLE_ASAN` (ON | OFF)
- `THEMIS_STATIC_BUILD` (ON | OFF)

### Advanced
- `CMAKE_BUILD_TYPE` (Debug | Release | RelWithDebInfo | MinSizeRel)
- `CMAKE_INTERPROCEDURAL_OPTIMIZATION` (ON | OFF)
- `CMAKE_TOOLCHAIN_FILE` (path to vcpkg toolchain)
- `VCPKG_TARGET_TRIPLET` (x64-windows | x64-linux | arm64-linux | etc.)

**Total Flags**: 45+ configurable options
