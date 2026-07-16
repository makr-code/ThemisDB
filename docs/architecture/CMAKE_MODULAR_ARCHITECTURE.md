# ThemisDB CMake Modular Architecture

**Version**: 1.4.0+  
**Date**: 2026-01-12  
**Status**: ⚠️ **PARTIALLY IMPLEMENTED - PLANNING PHASE**

> **⚠️ CRITICAL DOCUMENTATION NOTICE:**  
> This document describes the **TARGET modular architecture** for ThemisDB's build system. The modular Features/ and Targets/ structure described below **DOES NOT YET EXIST** in the codebase. This is a **roadmap document** that shows the planned future state.
>
> **For current implementation details**, see the [Current Implementation Status](#current-implementation-status) section.  
> **For implementation roadmap**, see [MODULAR_ARCHITECTURE_ROADMAP.md](MODULAR_ARCHITECTURE_ROADMAP.md).

## Overview

ThemisDB v1.4.0 is transitioning towards a **modular CMake architecture** from a monolithic 2700-line single file to a maintainable, feature-based structure.

## Current Implementation Status

| Component | Status | Notes |
|-----------|--------|-------|
| Root CMakeLists.txt | ✅ **IMPLEMENTED** | 241 lines - orchestration layer |
| cmake/Versions.cmake | ✅ **IMPLEMENTED** | Version management |
| cmake/CompilerOptions.cmake | ✅ **IMPLEMENTED** | Compiler setup |
| cmake/Dependencies.cmake | ✅ **IMPLEMENTED** | Dependency management |
| cmake/ModularBuild.cmake | ✅ **IMPLEMENTED** | Some modular logic |
| cmake/PreloadTargets.cmake | ✅ **IMPLEMENTED** | Target preloading |
| cmake/CMakeLists.txt | ⚠️ **MONOLITHIC** | **3115 lines** - NOT refactored |
| cmake/Features/ directory | ❌ **NOT IMPLEMENTED** | **PLANNED** - See roadmap |
| cmake/Features/*.cmake modules | ❌ **NOT IMPLEMENTED** | **PLANNED** - See roadmap |
| cmake/Targets/ directory | ❌ **NOT IMPLEMENTED** | **PLANNED** - See roadmap |
| cmake/Targets/*.cmake modules | ❌ **NOT IMPLEMENTED** | **PLANNED** - See roadmap |

### Before vs After (Current vs Planned)

| Aspect | **Current (v1.4.0)** | **Planned Goal** |
|--------|---------------------|------------------|
| **Structure** | Root (241 lines) + cmake/CMakeLists.txt (3115 lines - monolithic) | Root + cmake/Features/* + cmake/Targets/* (modular) |
| **Maintainability** | ⭐⭐ Moderate - Large monolithic file | ⭐⭐⭐⭐⭐ Excellent - Modular by feature |
| **Feature Management** | Conditional blocks scattered in 3115-line file | Explicit, isolated modules (goal) |
| **Path Resolution** | ✅ Fixed (CMAKE_SOURCE_DIR) | ✅ Already Fixed |
| **Build Time** | ~150s (Windows, 8 cores) | ~150s (no change expected) |
| **Extensibility** | Difficult - edit large monolithic file | Easy - add .cmake module (goal) |
| **Documentation Gap** | ❌ HIGH - Docs claim modular, reality is monolithic | ✅ Accurate documentation (goal) |

---

## Directory Structure

### Current Implementation (v1.4.0)

```
themis/
├── CMakeLists.txt                    ← Root (241 lines) ✅ IMPLEMENTED
├── cmake/
│   ├── CMakeLists.txt               ← Build logic (3115 lines - MONOLITHIC) ⚠️ NOT REFACTORED
│   ├── ModularBuild.cmake           ← Some modular logic ✅ IMPLEMENTED
│   ├── PreloadTargets.cmake         ← Target preloading ✅ IMPLEMENTED
│   ├── Versions.cmake               ← Version parsing, Edition management ✅ IMPLEMENTED
│   ├── CompilerOptions.cmake        ← C++ standards, compiler flags ✅ IMPLEMENTED
│   ├── Dependencies.cmake           ← External dependencies ✅ IMPLEMENTED
│   ├── FindWhisper.cmake            ← Custom find module ✅ IMPLEMENTED
│   ├── FindPiper.cmake              ← Custom find module ✅ IMPLEMENTED
│   └── FindKerberos.cmake           ← Custom find module ✅ IMPLEMENTED
```

### Planned Modular Architecture (ROADMAP - Not Yet Implemented)

> **⚠️ WARNING:** The structure below is a PLANNED future state. None of the cmake/Features/ or cmake/Targets/ components exist yet.  
> See [MODULAR_ARCHITECTURE_ROADMAP.md](MODULAR_ARCHITECTURE_ROADMAP.md) for implementation plan.

```
themis/
├── CMakeLists.txt                    ← Root (241 lines) ✅ ALREADY DONE
├── cmake/
│   ├── CMakeLists.txt               ← TO BE REFACTORED from 3115 lines → <500 lines 📋 PLANNED
│   ├── Versions.cmake               ← ✅ ALREADY DONE
│   ├── CompilerOptions.cmake        ← ✅ ALREADY DONE
│   ├── Dependencies.cmake           ← ✅ ALREADY DONE
│   ├── Features/                    ← 📋 PLANNED (Phase 1)
│   │   ├── LLM.cmake               ← LLM plugin sources (~20 files)
│   │   ├── GPU.cmake               ← GPU acceleration (CUDA, HIP, FAISS)
│   │   ├── gRPC.cmake              ← Inter-shard communication, proto generation
│   │   ├── Protocols.cmake         ← HTTP/2, HTTP/3, WebSocket, MQTT, PostgreSQL, MCP
│   │   └── Tracing.cmake           ← OpenTelemetry integration
│   └── Targets/                     ← 📋 PLANNED (Phase 2)
│       ├── CoreLibrary.cmake       ← themis_core library definition
│       ├── Executables.cmake       ← themis_server, themis_demo
│       ├── Tests.cmake             ← Test executables
│       └── Benchmarks.cmake        ← Benchmark targets
```

---

## CMake Module Organization

### 1. Root CMakeLists.txt (241 lines)

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

### 5. cmake/Features/* Modules (PLANNED - Not Yet Implemented)

> **⚠️ WARNING:** The feature modules described below DO NOT EXIST yet. This is the planned architecture.  
> See [MODULAR_ARCHITECTURE_ROADMAP.md](MODULAR_ARCHITECTURE_ROADMAP.md) for implementation timeline.

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

### 6. cmake/CMakeLists.txt (Currently 3115 lines - Needs Refactoring)

> **⚠️ CURRENT STATE:** This file is still monolithic at 3115 lines and has NOT been refactored yet.  
> The description below is the PLANNED state after Phase 1 and Phase 2 refactoring.

**Purpose (After Refactoring)**: Main build logic

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

**Old way (v1.3.x and current v1.4.0)**:
1. Add sources to long list in `cmake/CMakeLists.txt` (3115 lines)
2. Add conditional block with `if(THEMIS_ENABLE_FEATURE)`
3. Search for dependency code, add find_package() somewhere
4. Hard to maintain, easy to create merge conflicts

**New way (PLANNED for v1.5.0+)**:
1. Create `cmake/Features/MyFeature.cmake`
2. Add sources, dependencies, compile definitions in one place
3. Include in cmake/CMakeLists.txt: `include(cmake/Features/MyFeature.cmake)`
4. Clean, isolated, testable

> **⚠️ NOTE:** The "New way" is PLANNED but not yet implemented. Currently, all features are still managed in the monolithic cmake/CMakeLists.txt file.

**Example**: Adding a new optimization feature (PLANNED WORKFLOW)

> **⚠️ NOTE:** This workflow is for the planned modular architecture and cannot be used yet.

```cmake
# cmake/Features/MyOptimization.cmake (TO BE CREATED)
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

Then in `cmake/CMakeLists.txt` (after refactoring):
```cmake
include(cmake/Features/MyOptimization.cmake)
```

And add option in Root `CMakeLists.txt`:
```cmake
option(THEMIS_ENABLE_MY_OPTIMIZATION "Enable My Optimization" OFF)
```

---

## Future Roadmap

> **📋 For detailed implementation plan, see [MODULAR_ARCHITECTURE_ROADMAP.md](MODULAR_ARCHITECTURE_ROADMAP.md)**

### Phase 1: Feature Module Extraction (PLANNED)

**Status:** 📋 Not Started  
**Goal:** Reduce cmake/CMakeLists.txt from 3115 lines to ~1500 lines  
**Effort:** 40-60 hours  
**Target:** v1.5.0 or v1.6.0

Extract feature-specific sources to separate modules:
- Create cmake/Features/ directory
- Extract LLM sources to cmake/Features/LLM.cmake (~20 files)
- Extract GPU sources to cmake/Features/GPU.cmake
- Extract gRPC sources to cmake/Features/gRPC.cmake
- Extract Protocol sources to cmake/Features/Protocols.cmake
- Extract Tracing sources to cmake/Features/Tracing.cmake

### Phase 2: Target Module Extraction (PLANNED)

**Status:** 📋 Not Started (Depends on Phase 1)  
**Goal:** Reduce cmake/CMakeLists.txt from ~1500 lines to <500 lines  
**Effort:** 20-30 hours  
**Target:** v1.5.0 or v1.6.0

Move target definitions to separate modules:
- Create cmake/Targets/ directory
- Extract to cmake/Targets/CoreLibrary.cmake
- Extract to cmake/Targets/Executables.cmake
- Extract to cmake/Targets/Tests.cmake
- Extract to cmake/Targets/Benchmarks.cmake

### Phase 3: Plugin Architecture (PLANNED)

**Status:** 📋 Not Started (Depends on Phase 1 & 2)  
**Target:** v1.5.0+

Enable runtime-loadable plugins:
- LLM as .so/.dll plugin
- Protocol handlers as plugins
- Hardware backends as plugins

### Phase 4: Package Templates (PLANNED)

**Status:** 📋 Not Started  
**Target:** v1.5.0+

Create distribution templates:
- Docker multi-stage Dockerfile
- Kubernetes Helm charts
- Ansible deployment playbooks

---

## Current Limitations

**As of v1.4.0, the following limitations exist:**

1. **Monolithic cmake/CMakeLists.txt:** Still 3115 lines, all feature logic in one file
2. **No Feature Modules:** cmake/Features/ directory does not exist
3. **No Target Modules:** cmake/Targets/ directory does not exist
4. **Manual Feature Management:** Adding features requires editing large monolithic file
5. **Limited Modularity:** Only Versions, CompilerOptions, Dependencies, ModularBuild, and PreloadTargets are modular
6. **Documentation Gap:** This document describes a planned architecture that does not yet exist

**Migration to Modular Architecture:**
- Phase 1 and Phase 2 (above) address these limitations
- See detailed roadmap: [MODULAR_ARCHITECTURE_ROADMAP.md](MODULAR_ARCHITECTURE_ROADMAP.md)
- Expected completion: v1.5.0 or v1.6.0

**For Current Build System:**
- Use existing monolithic cmake/CMakeLists.txt
- Edit feature logic directly in the 3115-line file
- Follow existing patterns for conditional compilation
- Test builds thoroughly after any changes

---

## References

- [CMake Official Documentation](https://cmake.org/documentation/)
- [vcpkg Integration Guide](https://github.com/microsoft/vcpkg)
- [ThemisDB Build Guides](../build-guide/)
  - [BUILD_WINDOWS.md](../build-guide/BUILD_WINDOWS.md)
  - [BUILD_LINUX.md](../build-guide/BUILD_LINUX.md)
- [Feature Flags Reference](FEATURE_FLAGS_REFERENCE.md)
- [Source Directory Guide](SOURCE_DIRECTORY_GUIDE.md) - Comprehensive guide to all 35 src/ directories
- [Documentation Gap Analysis](../Audit/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md)

**For accurate current implementation details, always verify against the actual source code.**
