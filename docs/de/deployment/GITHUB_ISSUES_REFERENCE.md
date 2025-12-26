# GitHub Issue & Documentation Reference für CMAKE Find_Package Probleme

## FAISS - Windows CMake Configuration Issues

### Offizielle Dokumentation
- **FAISS CMakeLists.txt**: https://github.com/facebookresearch/faiss/blob/main/CMakeLists.txt
- **FAISS cmake/ directory**: https://github.com/facebookresearch/faiss/tree/main/cmake
- **FAISS faiss-config.cmake.in**: https://github.com/facebookresearch/faiss/blob/main/cmake/faiss-config.cmake.in
- **FAISS CMake Export Targets**: https://github.com/facebookresearch/faiss/blob/main/faiss/CMakeLists.txt#L80-L120

### Verwandte GitHub Issues (FAISS)
1. **#2909** - Failed build on Windows with -DBUILD_SHARED_LIBS=ON
   - Link: https://github.com/facebookresearch/faiss/issues/2909
   - Status: ✅ Gelöst
   - Relevanz: Windows Shared Library Export Issues

2. **#3193** - AVX2 support doesn't compile with MSVC
   - Link: https://github.com/facebookresearch/faiss/issues/3193
   - Status: ✅ Gelöst
   - Relevanz: MSVC Compiler Compatibility

3. **#3499** - faiss-gpu build fails on windows 11
   - Link: https://github.com/facebookresearch/faiss/issues/3499
   - Status: ✅ Gelöst
   - Relevanz: GPU Support auf Windows

4. **#4108** - Windows OS: Linker errors when using faiss with go
   - Link: https://github.com/facebookresearch/faiss/issues/4108
   - Status: ✅ Gelöst
   - Relevanz: Static vs Dynamic Linking auf Windows

### PR für Relevante Fixes
- **#4145** - Added support for building for MinGW, in addition to MSVC
  - Link: https://github.com/facebookresearch/faiss/pull/4145
  - Status: Abandoned but relevant

---

## gRPC - Windows CMake Configuration Issues

### Offizielle Dokumentation
- **gRPC CMakeLists.txt**: https://github.com/grpc/grpc/blob/master/CMakeLists.txt (2.19 MB - sehr groß)
- **gRPC cmake/ directory**: https://github.com/grpc/grpc/tree/master/cmake
- **gRPC gRPCConfig.cmake.in**: https://github.com/grpc/grpc/blob/master/cmake/gRPCConfig.cmake.in
- **gRPC CMake Modules**: https://github.com/grpc/grpc/tree/master/cmake/modules
- **gRPC protobuf.cmake**: https://github.com/grpc/grpc/blob/master/cmake/protobuf.cmake

### Verwandte GitHub Issues (gRPC)
1. **#38623** - Fix MSVC static runtime build with CMake >= 3.15
   - Link: https://github.com/grpc/grpc/pull/38623
   - Status: ✅ Merged
   - Relevanz: **CRITICAL** für Windows MSVC Static Library Builds
   - Was: https://github.com/grpc/grpc/commit/0021f7e2f6df52143a56a2a4d75f7d049c43a5a4

2. **gRPC MSVC Build Documentation**
   - Link: https://github.com/grpc/grpc/blob/master/BUILDING.md#windows
   - Relevanz: Offizielle Windows Build-Anleitung

3. **gRPC CMake Usage Documentation**
   - Link: https://github.com/grpc/grpc/tree/master/examples/cpp
   - Relevanz: CMake find_package() Beispiele

---

## vcpkg - Port-Konfiguration & Workarounds

### vcpkg FAISS Port
- **Port URL**: https://github.com/microsoft/vcpkg/tree/master/ports/faiss
- **portfile.cmake**: https://github.com/microsoft/vcpkg/blob/master/ports/faiss/portfile.cmake
- **Letzte Update**: 2025-01-22 (v1.13.2)

**Wichtige vcpkg FAISS Patches:**
```
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
        -DFAISS_ENABLE_MKL=OFF
        -DFAISS_ENABLE_PYTHON=OFF
)
```

### vcpkg gRPC Port
- **Port URL**: https://github.com/microsoft/vcpkg/tree/master/ports/grpc
- **portfile.cmake**: https://github.com/microsoft/vcpkg/blob/master/ports/grpc/portfile.cmake
- **vcpkg-cmake-wrapper.cmake**: https://github.com/microsoft/vcpkg/blob/master/ports/grpc/vcpkg-cmake-wrapper.cmake
- **Letzte Update**: 2024-09-10 (v1.71.0)

**Wichtige Windows-spezifische Zeilen im vcpkg gRPC Port:**
```cmake
if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_check_linkage(ONLY_STATIC_LIBRARY)  # ← Windows MUSS static sein!
endif()
```

**Critical gRPC Patches für Windows:**
- 00004-link-gdi32-on-windows.patch
- 00005-fix-uwp-error.patch
- 00006-utf8-range.patch

---

## vcpkg CMake Configuration Best Practices

### vcpkg Official CMake Documentation
- **vcpkg CMake Integration**: https://github.com/microsoft/vcpkg/blob/master/docs/users/cmake-integration.md
- **vcpkg CMake Functions**: https://github.com/microsoft/vcpkg/blob/master/docs/maintainers/cmake_functions.md
- **vcpkg Global Config Mode**: https://github.com/microsoft/vcpkg/tree/master/scripts/buildsystems

### Problem: CMAKE_PREFIX_PATH und CONFIG Mode
**Issue**: vcpkg.cmake setzt CMAKE_PREFIX_PATH, aber:
1. IDE-Builds können es zurücksetzen
2. Nested CMake Aufrufe erben es nicht immer
3. CONFIG Mode erfordert explizite Pfade in manchen Fällen

**Lösung nach vcpkg-Dokumentation:**
```cmake
# Nach vcpkg toolchain laden
if(DEFINED ENV{VCPKG_ROOT})
    list(APPEND CMAKE_PREFIX_PATH 
        "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share"
        "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}")
endif()

# Und IMMER CMAKE_FIND_PACKAGE_PREFER_CONFIG=ON
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
```

---

## Workarounds für CMAKE_PREFIX_PATH Issues

### Workaround 1: Explizite find_package() Hints
```cmake
find_package(faiss CONFIG 
    HINTS "${CMAKE_PREFIX_PATH}"
    REQUIRED)
```

### Workaround 2: Direkter gRPC_DIR/faiss_DIR
```cmake
# Wenn find_package fehlschlägt, explizit setzen
if(NOT DEFINED gRPC_DIR AND EXISTS "${VCPKG_INSTALLED}/share/grpc")
    set(gRPC_DIR "${VCPKG_INSTALLED}/share/grpc")
endif()
```

### Workaround 3: VCPKG_MANIFEST Mode
```cmake
# Nutze vcpkg.json für automatische Installation statt manueller Pfade
# Aber: Für Windows gRPC MUSS VCPKG_TARGET_TRIPLET=x64-windows sein!
```

### Workaround 4: pkg-config Fallback
```cmake
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(FAISS faiss)  # Liest faiss.pc
    pkg_check_modules(GRPC gRPC++)  # Liest grpc++.pc
endif()
```

---

## ThemisDB Spezifische Konfiguration

### Unsere Current Setup
- **CMake Version**: ≥3.20 ✅
- **vcpkg Toolchain**: C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ✅
- **VCPKG_TARGET_TRIPLET**: x64-windows ✅
- **CMAKE_FIND_PACKAGE_PREFER_CONFIG**: ON ✅
- **FAISS Version**: v1.8.0 ✅
- **gRPC Version**: v1.71.0 (static on Windows) ✅

### Known Workarounds Applied
1. ✅ FAISS graceful degradation wenn GPU=OFF (Line 505-510 in CMakeLists.txt)
2. ✅ gRPC optional bei LLM Feature (Line 928-938 in CMakeLists.txt)
3. ✅ Protobuf automatic dependency resolution (grpc -> protobuf)

### Issues to Track
- Monitor: https://github.com/microsoft/vcpkg/issues?q=grpc+cmake
- Monitor: https://github.com/microsoft/vcpkg/issues?q=faiss+windows
- Monitor: https://github.com/grpc/grpc/issues?q=cmake+windows

---

## Debugging CMake find_package() Failures

### Verbose CMake Output
```bash
# Enables find_package verbose output
cmake -S . -B build-msvc --debug-output
# Oder:
cmake -S . -B build-msvc -DCMAKE_FIND_DEBUG_MODE=ON
```

### Package Config Files Debug
```bash
# Zeige was CMake sucht
cmake -S . -B build-msvc -DCMAKE_MESSAGE_LOG_LEVEL=VERBOSE
# Auch:
cmake --debug-output
```

### Manuelle Config-Datei Check
```powershell
# Überprüfe ob find_package() die Config findet
$CMAKE_PREFIX_PATH = "C:\VCC\themis\vcpkg_installed\x64-windows"

# Test FAISS
Test-Path "$CMAKE_PREFIX_PATH\share\faiss\faiss-config.cmake"

# Test gRPC
Test-Path "$CMAKE_PREFIX_PATH\share\grpc\gRPCConfig.cmake"

# Read Config to debug dependencies
Get-Content "$CMAKE_PREFIX_PATH\share\faiss\faiss-config.cmake" | Select-Object -First 20

Get-Content "$CMAKE_PREFIX_PATH\share\grpc\gRPCConfig.cmake" | Select-Object -First 20
```

---

## Zusammenfassung der Kritischen Erkenntnisse

| Punkt | FAISS | gRPC |
|-------|-------|------|
| **vcpkg verfügbar** | ✅ Ja | ✅ Ja |
| **Config-Datei existiert** | ✅ Ja | ✅ Ja |
| **CMAKE_PREFIX_PATH** | ❌ Nicht immer propagiert | ❌ Nicht immer propagiert |
| **Windows Static** | Ja (supported) | **Ja REQUIRED** |
| **Known MSVC Issues** | #3193, #2909, #4145 | #38623 **CRITICAL** |
| **Lösung** | Explizite faiss_DIR | Explizite gRPC_DIR |
| **Backup Lösung** | pkg-config Fallback | pkg-config Fallback |

---

## Commands zum Sofort-Testen

```powershell
# 1. Diagnose durchführen
.\scripts\fix-cmake-prefix-path.ps1 -Action diagnose

# 2. Mit Fixes bauen
.\scripts\fix-cmake-prefix-path.ps1 -Action build -EnableGPU $true -EnableLLM $true

# 3. Oder manuell mit explizitem CMAKE_PREFIX_PATH:
$VCPKG = "C:\VCC\themis\vcpkg_installed\x64-windows"
cmake -S . -B build-msvc `
    -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake" `
    -DCMAKE_PREFIX_PATH="$VCPKG;$VCPKG\share" `
    -Dfaiss_DIR="$VCPKG\share\faiss" `
    -DgRPC_DIR="$VCPKG\share\grpc" `
    -DTHEMIS_ENABLE_GPU=ON

# 4. Build
cmake --build build-msvc --config Release --target themis_server --parallel 8
```

---

**Datum**: 2025-12-26  
**Zuletzt überprüft**: gRPC v1.71.0, FAISS v1.8.0, vcpkg latest  
**Status**: 🟢 Ready für Implementation
