# ThemisDB CMake Modular Architecture

**Version**: 1.4.0+  
**Date**: 2026-01-07  
**Status**: ⚠️ **Partially Implemented - Under Development**

> **⚠️ IMPORTANT NOTE:**  
> This document describes the **planned modular architecture** for ThemisDB's build system. Some features described here are **not yet fully implemented**. Please refer to the [Current Implementation Status](#current-implementation-status) section below for accurate information about what exists today.

## Overview

ThemisDB v1.4.0 is transitioning towards a **modular CMake architecture** from a monolithic 2700-line single file to a maintainable, feature-based structure.

## Current Implementation Status

| Component | Status | Notes |
|-----------|--------|-------|
| Root CMakeLists.txt (150 lines) | ✅ **Implemented** | Clean orchestration |
| cmake/Versions.cmake | ✅ **Implemented** | Version management |
| cmake/CompilerOptions.cmake | ✅ **Implemented** | Compiler setup |
| cmake/Dependencies.cmake | ✅ **Implemented** | Dependency management |
| cmake/ModularBuild.cmake | ✅ **Implemented** | Some modular logic |
| cmake/CMakeLists.txt | ⚠️ **Monolithic** | Still 3076 lines (not refactored) |
| cmake/Features/ directory | ❌ **Not Implemented** | Planned feature |
| cmake/Features/*.cmake modules | ❌ **Not Implemented** | Planned feature |
| cmake/Targets/ directory | ❌ **Not Implemented** | Planned feature |
| cmake/Targets/*.cmake modules | ❌ **Not Implemented** | Planned feature |

### Before vs After (Planned)

| Aspect | Current (v1.4.0) | Planned Goal |
|--------|------------------|--------------|
| **Structure** | Root + cmake/Versions + cmake/Dependencies + cmake/CMakeLists.txt (3076 lines) | Root + cmake/Versions + cmake/Dependencies + cmake/Features/* + cmake/Targets/* |
| **Maintainability** | ⭐⭐ Moderate | ⭐⭐⭐⭐⭐ Excellent (goal) |
| **Feature Management** | Conditional blocks in monolithic file | Explicit, modular (goal) |
| **Path Resolution** | ✅ Fixed (CMAKE_SOURCE_DIR) | ✅ Already Fixed |
| **Build Time** | ~150s (Windows) | ~150s (Windows, same) |
| **Extensibility** | Moderate - edit large file | Easy - add .cmake module (goal) |

---

## Directory Structure

### Current Implementation (v1.4.0)

```
themis/
├── CMakeLists.txt                    ← Root (150 lines) ✅ IMPLEMENTED
├── cmake/
│   ├── CMakeLists.txt               ← Build logic (3076 lines - still monolithic) ⚠️
│   ├── ModularBuild.cmake           ← Some modular logic ✅ IMPLEMENTED
│   ├── Versions.cmake               ← Version parsing, Edition management ✅ IMPLEMENTED
│   ├── CompilerOptions.cmake        ← C++ standards, compiler flags ✅ IMPLEMENTED
│   ├── Dependencies.cmake           ← External dependencies ✅ IMPLEMENTED
│   ├── FindWhisper.cmake            ← Custom find module ✅ IMPLEMENTED
│   └── FindPiper.cmake              ← Custom find module ✅ IMPLEMENTED
```

### Planned Modular Architecture (Future)

```
themis/
├── CMakeLists.txt                    ← Root (150 lines) ✅ ALREADY DONE
├── cmake/
│   ├── CMakeLists.txt               ← Refactored to <500 lines 📋 PLANNED
│   ├── Versions.cmake               ← ✅ ALREADY DONE
│   ├── CompilerOptions.cmake        ← ✅ ALREADY DONE
│   ├── Dependencies.cmake           ← ✅ ALREADY DONE
│   ├── Features/                    ← 📋 PLANNED
│   │   ├── LLM.cmake               ← LLM plugin sources (~20 files)
│   │   ├── GPU.cmake               ← GPU acceleration (CUDA, HIP, FAISS)
│   │   ├── gRPC.cmake              ← Inter-shard communication, proto generation
│   │   ├── Protocols.cmake         ← HTTP/2, HTTP/3, WebSocket, MQTT, PostgreSQL, MCP
│   │   └── Tracing.cmake           ← OpenTelemetry integration
│   └── Targets/                     ← 📋 PLANNED
│       ├── CoreLibrary.cmake       ← themis_core library definition
│       ├── Executables.cmake       ← themis_server, themis_demo
│       ├── Tests.cmake             ← Test executables
│       └── Benchmarks.cmake        ← Benchmark targets
│
├── src/
│   ├── storage/                     ← RocksDB wrapper, key-value storage
│   ├── index/                       ← Vector, graph, spatial indexes
│   ├── query/                       ← AQL parser, query engine
│   ├── server/                      ← HTTP server, API handlers
│   ├── security/                    ← Encryption, HSM, PKI
│   ├── sharding/                    ← Distributed sharding, replication
│   ├── llm/                         ← LLM plugin (included if THEMIS_ENABLE_LLM=ON)
│   ├── acceleration/                ← GPU backend abstraction (CUDA, HIP stubs)
│   └── ...
├── tests/                           ← ~180+ test executables
├── benchmarks/                      ← ~72 benchmark executables
└── proto/                           ← Protocol Buffer definitions
    └── shard_rpc.proto             ← gRPC service definition
```

---

## CMake Module Organization

### 1. Root CMakeLists.txt (150 lines)

**Purpose**: Minimal orchestration, single point for all feature flags

```cmake
cmake_minimum_required(VERSION 3.20)

# Read VERSION from file
# Define project()
# Set all feature flags (THEMIS_ENABLE_*)
# Include modular submodules
# Configure CPack

add_subdirectory(cmake)  # ← Delegates actual build logic
```

**All feature options centralized**:
- Build: `THEMIS_BUILD_TESTS`, `THEMIS_BUILD_BENCHMARKS`
- Edition: `THEMIS_EDITION` (MINIMAL/COMMUNITY/ENTERPRISE/HYPERSCALER)
- Core: `THEMIS_ENABLE_GRPC`, `THEMIS_ENABLE_LLM`, `THEMIS_ENABLE_GPU`
- Protocols: `THEMIS_ENABLE_HTTP2`, `THEMIS_ENABLE_WEBSOCKET`, etc.
- Optimization: `THEMIS_ENABLE_DISKANN`, `THEMIS_ENABLE_WISCKEY`, etc.

### 2. cmake/Versions.cmake (50 lines)

**Purpose**: Single source of truth for versioning and edition management

```cmake
# Reads VERSION file: "1.4.0-alpha"
# Parses semantic version
# Sets edition defaults:
#   MINIMAL   → LLM=OFF, gRPC=OFF
#   COMMUNITY → All optional
#   ENTERPRISE → Advanced features
#   HYPERSCALER → GPU, LLM, gRPC enabled by default
```

### 3. cmake/CompilerOptions.cmake (70 lines)

**Purpose**: All compiler-specific setup

```cmake
# C++20 standard
# Platform-specific flags (MSVC vs GCC/Clang)
# AVX2/SIMD optimization
# AddressSanitizer
# Strict warnings
```

### 4. cmake/Dependencies.cmake (250+ lines)

**Purpose**: Centralized external dependency management

**Structure**:
- **Required**: OpenSSL, RocksDB, gRPC, Protobuf, GTest (hard fails if missing)
- **Optional**: CURL, Arrow/Parquet, mimalloc, OpenTelemetry (warnings if missing)
- **Protocol-specific**: nghttp2 (HTTP/2), nghttp3+ngtcp2 (HTTP/3)
- **Hardware**: CUDAToolkit, HIP, faiss
- **LLM**: llama.cpp, whisper.cpp, piper-tts

Each dependency has clear find_package() with fallbacks and messaging.

### 5. cmake/Features/* Modules (5 files)

**Purpose**: Conditionally include sources for optional features

Each feature module:
1. Returns early if feature disabled (`if(NOT THEMIS_ENABLE_*) return() endif()`)
2. Appends source files to `THEMIS_CORE_SOURCES`
3. Adds compile definitions
4. Links required libraries

**Files**:

| Module | Sources | Condition | Linked Libs |
|--------|---------|-----------|-------------|
| **LLM.cmake** | ~20 llm/*.cpp + kernel fusion | `THEMIS_ENABLE_LLM=ON` | llama, CUDA (opt) |
| **GPU.cmake** | acceleration/*.cpp, *.cu | `THEMIS_ENABLE_GPU=ON` | CUDA::cudart, faiss |
| **gRPC.cmake** | server/wal_grpc_service.cpp | `THEMIS_ENABLE_GRPC=ON` | gRPC++, Protobuf |
| **Protocols.cmake** | HTTP2, HTTP3, WebSocket, MQTT, etc. | Feature-specific | nghttp2, etc. |
| **Tracing.cmake** | observability/*.cpp | `THEMIS_ENABLE_TRACING=ON` | opentelemetry-cpp |

### 6. cmake/CMakeLists.txt (refactored from 2679 lines)

**Purpose**: Main build logic

- Defines `THEMIS_CORE_SOURCES` variable (base sources)
- Includes feature modules (LLM, GPU, gRPC, etc.)
- Creates library targets: `add_library(themis_core ...)`
- Creates executable targets: `add_executable(themis_server ...)`
- Creates test targets (if `THEMIS_BUILD_TESTS=ON`)
- Creates benchmark targets (if `THEMIS_BUILD_BENCHMARKS=ON`)

---

## Feature Flag System

### Edition-Based Defaults

```cmake
if(THEMIS_EDITION STREQUAL "MINIMAL")
    set(THEMIS_ENABLE_LLM OFF FORCE)
    set(THEMIS_ENABLE_GRPC OFF FORCE)

elseif(THEMIS_EDITION STREQUAL "COMMUNITY")
    # All features optional

elseif(THEMIS_EDITION STREQUAL "ENTERPRISE")
    set(THEMIS_ENABLE_GRPC ON FORCE)

elseif(THEMIS_EDITION STREQUAL "HYPERSCALER")
    set(THEMIS_ENABLE_LLM ON FORCE)
    set(THEMIS_ENABLE_GRPC ON FORCE)
    set(THEMIS_ENABLE_GPU ON FORCE)
    set(THEMIS_ENABLE_TRACING ON FORCE)
endif()
```

### Example Build Commands

**Community Edition (default)**:
```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel 8
```

**Hyperscaler with LLM + GPU**:
```bash
cmake -S . -B build-hyperscaler \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_CUDA=ON
cmake --build build-hyperscaler --parallel 8
```

**Minimal for embedded**:
```bash
cmake -S . -B build-minimal \
  -DTHEMIS_EDITION=MINIMAL \
  -DTHEMIS_BUILD_BENCHMARKS=OFF
cmake --build build-minimal --parallel 4
```

---

## Path Resolution (Key Fix)

### Problem (v1.3.x)

Root `CMakeLists.txt` delegated to `cmake/` subdirectory:
```cmake
add_subdirectory(cmake)  # CMAKE_CURRENT_SOURCE_DIR becomes "cmake/"
```

`cmake/CMakeLists.txt` used relative paths:
```cmake
set(THEMIS_CORE_SOURCES
    src/storage/rocksdb_wrapper.cpp  # Looked for cmake/src/storage/rocksdb_wrapper.cpp ❌
    ...
)
```

**Result**: `CMake Error: Cannot find source file: src/storage/rocksdb_wrapper.cpp`

### Solution (v1.4.0+)

**Root CMakeLists.txt now calls `project()`** before subdirectory:

```cmake
project(Themis VERSION 1.4.0 LANGUAGES CXX)  # ← CMAKE_SOURCE_DIR = /themis (root)
add_subdirectory(cmake)  # ← Works correctly now
```

**cmake/CMakeLists.txt uses absolute paths**:

```cmake
# PowerShell script automated this replacement:
set(THEMIS_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/storage/rocksdb_wrapper.cpp  # ✅ Finds correct file
    ...
)
```

**Automation script** (applied during refactor):

```powershell
# For each line in cmake/CMakeLists.txt:
$line = $line -replace '(\s+)src/', '${1}${CMAKE_SOURCE_DIR}/src/'
$line = $line -replace '(\s+)tests/', '${1}${CMAKE_SOURCE_DIR}/tests/'
# ... same for benchmarks/, proto/, include/
```

---

## Build System Commands

### Windows (MSVC)

```bash
# Using provided script
.\scripts\build-windows.ps1 -NoCache -Config Release

# Or direct CMake
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
cmake --build build-msvc --config Release --parallel 8
```

### Linux (GCC/WSL)

```bash
# Using build preset
cmake --preset linux-gcc-release
cmake --build build-wsl --parallel 8

# Or direct CMake
cmake -S . -B build-wsl -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build-wsl --parallel 8
```

### Distribution Packages

```bash
# Windows packages (ZIP + WIX installer)
cmake --build build-msvc --target package --config Release

# Linux packages (TGZ + DEB + RPM)
cd build-wsl && cpack -G "TGZ;DEB;RPM" -C Release
```

---

## Migration Guide (v1.3.x → v1.4.0)

### For Build System Users

**No changes needed!** Build commands remain the same:

```bash
# Still works exactly as before
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 8
```

Feature flags also unchanged:
```bash
cmake -DTHEMIS_ENABLE_LLM=ON -DTHEMIS_ENABLE_GRPC=ON
```

### For Developers Adding Features

**Old way (v1.3.x)**:
1. Add sources to long list in `cmake/CMakeLists.txt` (~2700 lines)
2. Add conditional block with `if(THEMIS_ENABLE_FEATURE)`
3. Search for dependency code, add find_package() somewhere
4. Hard to maintain, easy to create conflicts

**New way (v1.4.0+)**:
1. Create `cmake/Features/MyFeature.cmake`
2. Add sources, dependencies, compile definitions in one place
3. Include in cmake/CMakeLists.txt: `include(cmake/Features/MyFeature.cmake)`
4. Clean, isolated, testable

**Example**: Adding a new optimization feature

```cmake
# cmake/Features/MyOptimization.cmake
if(NOT THEMIS_ENABLE_MY_OPTIMIZATION)
    return()
endif()

message(STATUS "[Feature] My Optimization enabled")
add_compile_definitions(THEMIS_MY_OPTIMIZATION=1)

list(APPEND THEMIS_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/optimization/my_optimization.cpp
)

# Link required libraries
target_link_libraries(themis_core PUBLIC some_dependency::lib)
```

Then in `cmake/CMakeLists.txt`:
```cmake
include(cmake/Features/MyOptimization.cmake)
```

And add option in Root `CMakeLists.txt`:
```cmake
option(THEMIS_ENABLE_MY_OPTIMIZATION "Enable My Optimization" OFF)
```

---

## Future Roadmap

### Phase 1: Feature Module Extraction (Planned)

Extract feature-specific sources to separate modules:
- Create cmake/Features/ directory
- Extract LLM sources to cmake/Features/LLM.cmake (~20 files)
- Extract GPU sources to cmake/Features/GPU.cmake
- Extract gRPC sources to cmake/Features/gRPC.cmake
- Extract Protocol sources to cmake/Features/Protocols.cmake
- Extract Tracing sources to cmake/Features/Tracing.cmake

**Goal:** Reduce cmake/CMakeLists.txt from 3076 lines to ~1500 lines  
**Effort:** 40-60 hours  
**Status:** 📋 Planned

### Phase 2: Target Module Extraction (Planned)

Move target definitions to separate modules:
- Create cmake/Targets/ directory
- Extract to cmake/Targets/CoreLibrary.cmake
- Extract to cmake/Targets/Executables.cmake
- Extract to cmake/Targets/Tests.cmake
- Extract to cmake/Targets/Benchmarks.cmake

**Goal:** Reduce cmake/CMakeLists.txt from ~1500 lines to <500 lines  
**Effort:** 20-30 hours  
**Status:** 📋 Planned

### Phase 3: Plugin Architecture (v1.5.0)

Enable runtime-loadable plugins:
- LLM as .so/.dll plugin
- Protocol handlers as plugins
- Hardware backends as plugins

**Status:** 📋 Planned for v1.5.0

### Phase 4: Package Templates (v1.5.0+)

Create distribution templates:
- Docker multi-stage Dockerfile
- Kubernetes Helm charts
- Ansible deployment playbooks

**Status:** 📋 Planned for v1.5.0+

---

## Current Limitations

**As of v1.4.0, the following limitations exist:**

1. **Monolithic cmake/CMakeLists.txt:** Still 3076 lines, all feature logic in one file
2. **No Feature Modules:** cmake/Features/ directory does not exist
3. **No Target Modules:** cmake/Targets/ directory does not exist
4. **Manual Feature Management:** Adding features requires editing large monolithic file
5. **Limited Modularity:** Only Versions, CompilerOptions, and Dependencies are modular

**Migration to Modular Architecture:**
- Phase 1 and Phase 2 (above) address these limitations
- Tracking issue: TBD (to be created)
- Expected completion: v1.5.0 or v1.6.0

---

## References

- [CMake Official Documentation](https://cmake.org/documentation/)
- [vcpkg Integration Guide](https://github.com/microsoft/vcpkg)
- [ThemisDB Build Guides](../build-guide/)
  - [BUILD_WINDOWS.md](../build-guide/BUILD_WINDOWS.md)
  - [BUILD_LINUX.md](../build-guide/BUILD_LINUX.md)
- [Feature Flags Reference](FEATURE_FLAGS_REFERENCE.md)
- [Documentation Gap Analysis](../DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md)

**For accurate current implementation details, always verify against the actual source code.**
