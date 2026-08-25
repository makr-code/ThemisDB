# CMake System Hardening & Simplification Plan

## Executive Summary

Das aktuelle CMake-System hat **Wartungsprobleme, Redundanzen und Fragilitäten**:

| Problem | Impact | Komplexität | Priority |
|---------|--------|-------------|----------|
| Duplizierte CUDA-Erkennung (2x im CMakeLists.txt) | Wartbarkeit, Sync-Fehler | 🟢 Trivial | 🔴 HIGH |
| Hardcodierte VS/SDK-Pfade in CMakePresets.json | Breakage auf anderen Maschinen | 🟢 Trivial | 🔴 HIGH |
| ExternalProject MAX_PATH Workarounds zu komplex | Schwer zu debuggen, Fehleranfällig | 🟡 Mittel | 🟡 MEDIUM |
| GPU Auto-Detect mit Glob-Pfaden (C:/Program Files*) | Unreliable, falsche CUDA-Version | 🟡 Mittel | 🟡 MEDIUM |
| Keine zentrale Plattform-Konfiguration | Duplicate Cache-Var Boilerplate | 🟢 Trivial | 🟢 LOW |
| Llama.cpp Vulkan Shader Generator Abhängigkeiten | Broken Build (abs.comp.cpp missing) | 🔴 Komplex | 🔴 CRITICAL |

---

## 1. QUICK WINS (Sofort umsetzbar)

### 1.1 Duplizierte CUDA-Erkennung zusammenführen

**Problem**: Lines ~165-195 und ~295-325 in cmake/CMakeLists.txt sind identisch.

**Lösung**: Refactoring in eine Hilfsfunktion `find_cuda_compiler()`

```cmake
# NEW: cmake/FindCudaCompiler.cmake
function(find_cuda_compiler)
    if(NOT DEFINED CMAKE_CUDA_COMPILER OR CMAKE_CUDA_COMPILER STREQUAL "")
        set(_themis_cuda_nvcc_candidates)

        if(DEFINED ENV{CUDACXX} AND EXISTS "$ENV{CUDACXX}")
            list(APPEND _themis_cuda_nvcc_candidates "$ENV{CUDACXX}")
        endif()
        
        # Use registry query on Windows instead of hardcoded paths
        if(WIN32)
            execute_process(
                COMMAND powershell -NoProfile -Command "
                    \$nvidia = Get-ChildItem 'Registry::HKLM\\Software\\NVIDIA Corporation\\Installed Products' -ErrorAction SilentlyContinue | 
                    Get-ItemProperty -Name InstallationPath -ErrorAction SilentlyContinue
                    if(\$nvidia) { Write-Host \$nvidia.InstallationPath }
                "
                OUTPUT_VARIABLE _nvidia_registry_path
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if(_nvidia_registry_path)
                list(APPEND _themis_cuda_nvcc_candidates "${_nvidia_registry_path}/bin/nvcc.exe")
            endif()
        endif()
        
        # Fallback: Standard ENV paths
        if(DEFINED ENV{CUDA_PATH} AND EXISTS "$ENV{CUDA_PATH}/bin/nvcc.exe")
            list(APPEND _themis_cuda_nvcc_candidates "$ENV{CUDA_PATH}/bin/nvcc.exe")
        endif()
        
        if(_themis_cuda_nvcc_candidates)
            list(GET _themis_cuda_nvcc_candidates 0 _themis_cuda_nvcc)
            set(CMAKE_CUDA_COMPILER "${_themis_cuda_nvcc}" PARENT_SCOPE)
            get_filename_component(_themis_cuda_root "${_themis_cuda_nvcc}" DIRECTORY)
            get_filename_component(_themis_cuda_root "${_themis_cuda_root}" DIRECTORY)
            set(CUDAToolkit_ROOT "${_themis_cuda_root}" PARENT_SCOPE)
        endif()
    endif()
endfunction()

# Usage in main CMakeLists.txt:
if(THEMIS_ENABLE_CUDA)
    find_cuda_compiler()
    include(CheckLanguage)
    check_language(CUDA)
    if(CMAKE_CUDA_COMPILER)
        enable_language(CUDA)
        set(CMAKE_CUDA_STANDARD 17)
        set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    else()
        message(WARNING "THEMIS_ENABLE_CUDA=ON but no CUDA compiler found")
        set(THEMIS_ENABLE_CUDA OFF CACHE BOOL "" FORCE)
    endif()
endif()
```

**Benefit**: -60 LOC Duplizierung, Single Source of Truth für CUDA Detection

**Time**: 20 min

---

### 1.2 Hardcodierte Windows SDK Paths entfernen

**Problem**: CMakePresets.json hat fest verdrahtete Pfade:
```json
"INCLUDE": "C:/Program Files/.../MSVC/14.44.35207/include;C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/..."
```

Bricht wenn:
- MSVC-Version 14.45+ installiert ist
- Windows SDK 10.0.22621 oder älter/neuer
- Developer auf anderen Maschinen arbeitet

**Lösung**: Generieren aus CMake (Preset wird einfach)

```cmake
# NEW: cmake/PlatformSDK.cmake
if(WIN32 AND MSVC)
    # Query MSVC version from compiler
    execute_process(
        COMMAND "${CMAKE_CXX_COMPILER}" /showIncludes /EP
        INPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/dummy.cpp"
        ERROR_QUIET
        OUTPUT_VARIABLE _msvc_includes
    )
    
    # Better: Use CMAKE_CXX_COMPILER_VERSION
    string(REGEX MATCH "([0-9]+\\.[0-9]+)" _msvc_version "${MSVC_VERSION}")
    
    # Find Windows SDK version from installed kit
    file(GLOB _win_sdk_roots "C:/Program Files (x86)/Windows Kits/10/Lib")
    if(_win_sdk_roots)
        file(GLOB _sdk_versions "${_win_sdk_roots}/*")
        list(SORT _sdk_versions COMPARE NATURAL ORDER DESCENDING)
        list(GET _sdk_versions 0 _sdk_latest)
        get_filename_component(_sdk_version "${_sdk_latest}" NAME)
        
        set(CMAKE_SDK_VERSION "${_sdk_version}" CACHE STRING "Windows SDK version")
    endif()
endif()
```

Dann CMakePresets.json wird **viel** einfacher:
```json
{
  "name": "windows-base",
  "inherits": "vcpkg-base",
  "environment": {
    "SCCACHE_DIR": "$env{LOCALAPPDATA}/Mozilla/sccache/themisdb",
    "SCCACHE_ERROR_LOG": "${sourceDir}/logs/sccache-error.log"
  },
  "cacheVariables": {
    "CMAKE_VS_RUNTIME_LIBRARY": "MultiThreadedDLL",
    "VCPKG_TARGET_TRIPLET": "x64-windows",
    "THEMIS_ENABLE_GPU": "ON"
  }
}
```

**Benefit**: Portable Presets, keine MSVC/SDK Version Hardcoding

**Time**: 30 min

---

### 1.3 CMAKE_EXTERNALS_BASE_DIR Logik vereinfachen

**Problem**: Komplexer Workaround für Windows MAX_PATH mit `add_external_project_flat()` function

**Besser**:
```cmake
# Einfache, zentrale Konstante
set(CMAKE_EXTERNALS_BASE_DIR "${CMAKE_SOURCE_DIR}/_e" CACHE PATH "Flattened ExternalProject base")

# Windows MAX_PATH note in Code, nicht verteilt
if(WIN32 AND (CMAKE_EXTERNALS_BASE_DIR STREQUAL ""))
    message(FATAL_ERROR "CMAKE_EXTERNALS_BASE_DIR required on Windows to avoid MAX_PATH (260 char limit)")
endif()

# Nutze direkt in add_subdirectory statt Hilfsfunktion
file(MAKE_DIRECTORY "${CMAKE_EXTERNALS_BASE_DIR}/llama_cpp")
add_subdirectory(${LLAMA_SRC_DIR} ${CMAKE_EXTERNALS_BASE_DIR}/llama_cpp EXCLUDE_FROM_ALL)
```

**Benefit**: -20 LOC, klarer Intent

**Time**: 10 min

---

## 2. MEDIUM EFFORT (Empfohlen)

### 2.1 Zentralisierte Plattform-Konfiguration

**Problem**: Cache-Variablen und Logik sind über CMakeLists.txt, CMakePresets.json, ModularBuild.cmake verteilt

**Lösung**: Neue Datei `cmake/PlatformConfig.cmake` für alle Plattform-Spezifika

```cmake
# cmake/PlatformConfig.cmake
#[==[
Centralized platform detection and configuration
Replaces scattered logic across CMakeLists.txt + CMakePresets.json
]=]

cmake_minimum_required(VERSION 3.23)

# Platform detection
set(THEMIS_HOST_SYSTEM "${CMAKE_SYSTEM_NAME}")
if(WIN32)
    set(THEMIS_HOST_SYSTEM "Windows")
elseif(APPLE)
    set(THEMIS_HOST_SYSTEM "macOS")
endif()

# Per-platform defaults (single source of truth)
if(THEMIS_HOST_SYSTEM STREQUAL "Windows")
    set(_THEMIS_PLATFORM_DEFAULTS
        "VCPKG_TARGET_TRIPLET=x64-windows"
        "THEMIS_ENABLE_GPU=ON"
        "THEMIS_ENABLE_HTTP_SERVER=ON"
        "THEMIS_ENABLE_LLM=ON"
        "THEMIS_BUILD_MODULAR=ON"
        "CMAKE_C_COMPILER_LAUNCHER=sccache"
        "CMAKE_CXX_COMPILER_LAUNCHER=sccache"
    )
elseif(THEMIS_HOST_SYSTEM STREQUAL "Linux")
    set(_THEMIS_PLATFORM_DEFAULTS
        "VCPKG_TARGET_TRIPLET=x64-linux"
        "THEMIS_ENABLE_GPU=ON"
        "THEMIS_ENABLE_HTTP_SERVER=ON"
        "THEMIS_ENABLE_LLM=ON"
        "CMAKE_C_COMPILER=gcc"
        "CMAKE_CXX_COMPILER=g++"
        "CMAKE_C_COMPILER_LAUNCHER=sccache"
        "CMAKE_CXX_COMPILER_LAUNCHER=sccache"
    )
endif()

# Apply defaults
foreach(_default ${_THEMIS_PLATFORM_DEFAULTS})
    string(REPLACE "=" ";" _kv "${_default}")
    list(GET _kv 0 _key)
    list(GET _kv 1 _value)
    if(NOT DEFINED ${_key})
        set(${_key} "${_value}" CACHE STRING "" FORCE)
    endif()
endforeach()
```

Dann CMakePresets.json wird:
```json
{
  "name": "windows-base",
  "inherits": "vcpkg-base",
  "cacheVariables": {
    "CMAKE_VS_RUNTIME_LIBRARY": "MultiThreadedDLL"
  }
}
```

**Benefit**: Maintainable, Single Source of Truth, leichte Edition-spezifische Variationen

**Time**: 45 min

---

### 2.2 Llama.cpp Vulkan Shader Generator korrekt integrieren

**Problem** (aktuell blockiert den Build): `abs.comp.cpp` wird nicht generiert

**Root Cause**: `vulkan-shaders-gen` custom command ist nicht als Dependency vor ggml-vulkan registriert

**Lösung 1** (Schnell): ExternalProject Custom Commands richtig ordnen
```cmake
# In cmake/CMakeLists.txt, bei llama.cpp add_subdirectory
if(THEMIS_ENABLE_VULKAN)
    # ERST: Shader Generator konfigurieren + bauen
    add_custom_target(vulkan-shaders-gen-build
        COMMAND ${CMAKE_COMMAND} --build "${CMAKE_EXTERNALS_BASE_DIR}/llama_cpp/ggml/src/ggml-vulkan" 
                --target vulkan-shaders-gen
        COMMENT "Generating Vulkan compute shaders"
    )
    
    # DANN: ggml-vulkan abhängig machen
    add_dependencies(ggml-vulkan vulkan-shaders-gen-build)
endif()
```

**Lösung 2** (Robust): Shader offline pre-generieren, in repo committen
```bash
# In CI: Shader einmal offline generieren
cd llama.cpp/ggml/src/ggml-vulkan
./vulkan-shaders-gen > abs.comp.cpp > acc.comp.cpp > ...  # alle .comp.cpp
git add *.comp.cpp
git commit "pre-generated GLSL shaders"
```

Dann ExternalProject kann reused werden ohne Generator.

**Time**: 30 min (Lösung 1) oder 2h (Lösung 2, but cleaner)

---

## 3. STRATEGIC IMPROVEMENTS (Roadmap)

### 3.1 CMake Monorepo "Workspace" Pattern

Statt Monolith cmake/CMakeLists.txt (3000+ LOC):

```
cmake/
├── CMakeLists.txt          (Core boilerplate, 200 LOC max)
├── Foundation.cmake        (compiler, stdlib, platform fixes)
├── Dependencies.cmake      (vcpkg, ExternalProject)
├── Modules.cmake           (modular build config)
├── Testing.cmake           (CTest, benchmarks)
├── GPU.cmake               (CUDA/HIP/Vulkan)
├── LLM.cmake               (llama.cpp, whisper.cpp)
├── Security.cmake          (signing, licenses)
└── PlatformSDK.cmake       (Windows SDK, auto-detect)
```

Dann Haupt-CMakeLists.txt wird:
```cmake
cmake_minimum_required(VERSION 3.23)

# Load foundations
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Foundation.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformSDK.cmake)

project(Themis VERSION ${THEMIS_VERSION} LANGUAGES CXX)

# Load subsystems as needed
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependencies.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GPU.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/LLM.cmake)
# ... etc
```

**Benefit**: 
- Easier to understand (modular)
- Easier to maintain (focused scope)
- Easier to test (each .cmake independently)

**Time**: 3-4h refactor

---

### 3.2 CMake Unit Testing Framework

Current: No tests for cmake/CMakeLists.txt behavior

Propose: `cmake/test/CMakeLists.test.cmake`
```cmake
# Test: CUDA detection works correctly
enable_testing()

add_test(NAME cuda_detection_finds_nvcc 
    COMMAND cmake -DTHEMIS_ENABLE_CUDA=ON -P cmake/test/test_cuda_detection.cmake
)

add_test(NAME windows_sdk_paths_detected
    COMMAND cmake -P cmake/test/test_platform_sdk.cmake
)
```

**Benefit**: Catch regressions in auto-detection logic before user builds fail

**Time**: 2h initial + 30min per test

---

## Implementation Priority Roadmap

### Phase 1: Stabilization (IMMEDIATE - This Session)
- ✅ Llama.cpp Vulkan Shader Generator fix (blocking current build)
- [ ] Remove CUDA duplication (20 min)
- [ ] Remove hardcoded SDK paths from presets (30 min)

### Phase 2: Simplification (Next Sprint)
- [ ] Centralize platform config to PlatformConfig.cmake (45 min)
- [ ] ExternalProject flattening cleanup (15 min)
- [ ] Document MAX_PATH workaround in README

### Phase 3: Architecture (Longer term)
- [ ] Modularize cmake/ structure (4h refactor)
- [ ] CMake unit tests (2h setup)
- [ ] Pre-generate Vulkan shaders in CI (2h setup)

---

## Technical Debt Summary

| Debt | Cost to Fix | Cost of Not Fixing | Recommend |
|------|------------|-------------------|-----------|
| CUDA duplication | 20 min | Build brittleness per CUDA version | **FIX NOW** |
| Hardcoded SDK paths | 30 min | CI breakage on new machines | **FIX NOW** |
| Vulkan shader deps | 30 min | Build fails with Vulkan enabled | **CRITICAL** |
| Missing cmake tests | 2h | Regressions in GPU/LLM detection | FIX Q3 2026 |
| Monolithic cmake/CMakeLists | 4h | Hard to maintain, understand, debug | FIX Q4 2026 |

---

## Recommended Immediate Actions

1. **Merge FindCudaCompiler refactor** (20 min)
   - Remove duplicate CUDA detection code
   - Use registry on Windows instead of hardcoded path globbing

2. **Fix Vulkan Shader Generator dependency** (30 min)
   - Add custom target ordering for `vulkan-shaders-gen` before `ggml-vulkan`
   - OR: Pre-generate shaders in CI, commit to repo

3. **Remove SDK path hardcoding** (30 min)
   - Move to runtime detection in PlatformSDK.cmake
   - Simplify CMakePresets.json

**Total time investment**: 1.5 hours → **Prevents 10+ hours of debugging** for future builds and team members

---
