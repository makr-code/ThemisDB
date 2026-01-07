# CMake find_package Probleme - FAISS & gRPC Lösungen
**ThemisDB v1.3.5** | Datum: 26.12.2025

---

## Zusammenfassung der Probleme

### Problem 1: FAISS CMake Config nicht gefunden
```
CMake Error at CMakeLists.txt:504:
  Could NOT find faiss (missing: faiss_DIR)
```
- **vcpkg Installation**: ✅ faiss v1.8.0 in `C:\VCC\themis\vcpkg_installed\x64-windows\`
- **Config-Datei existiert**: ✅ `vcpkg_installed/x64-windows/share/faiss/faiss-config.cmake`
- **CMake findet sie nicht**: ❌ faiss_DIR wird nicht automatisch propagiert

### Problem 2: gRPC CMake Config nicht gefunden  
```
CMake Error at plugins/rpc/grpc/CMakeLists.txt:9:
  gRPCConfig.cmake missing despite vcpkg installation
```
- **vcpkg Installation**: ✅ grpc v1.71.0 (statisch) in `C:\VCC\themis\vcpkg_installed\x64-windows\`
- **Config-Datei existiert**: ✅ `vcpkg_installed/x64-windows/share/grpc/gRPCConfig.cmake`
- **CMake findet sie nicht**: ❌ gRPC_DIR wird nicht automatisch propagiert

---

## ROOT CAUSE ANALYSE

### Die Diagnose:

1. **Beim CMake Configure sind CMAKE_PREFIX_PATH und CMAKE_FIND_PACKAGE_PREFER_CONFIG richtig gesetzt**
2. **vcpkg.cmake Toolchain wird korrekt geladen** (`-DCMAKE_TOOLCHAIN_FILE=...`)
3. **ABER**: Die Pfade zu `faiss_DIR` und `gRPC_DIR` werden nicht vom Toolchain übertragen

### Warum passiert das?

- **vcpkg.cmake** setzt `CMAKE_PREFIX_PATH` auf VCPKG_INSTALLED_DIR
- **ABER**: Dieses wird bei jedem CMake-Reset überschrieben
- **Bei CMakeLists.txt Lines 43-45**: `CMAKE_FIND_PACKAGE_PREFER_CONFIG ON` ist korrekt
- **ABER**: `CMAKE_PREFIX_PATH` muss bei JEDEM cmake-Schritt gesetzt sein

---

## LÖSUNGEN (in Prioritätsreihenfolge)

### 🔴 LÖSUNG 1: Explizite CMAKE_PREFIX_PATH (EMPFOHLEN)

Garantierte Lösung: Pfade direkt in CMakeLists.txt setzen.

**[CMakeLists.txt](CMakeLists.txt#L40-L50)** - Nach Zeile 39 ergänzen:

```cmake
# Set CMAKE_PREFIX_PATH for vcpkg packages (v1.3.5 hotfix)
if(DEFINED ENV{VCPKG_ROOT})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-windows")
endif()

# Fallback: Lokaler Installed-Pfad
if(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows")
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows")
endif()
```

**Oder kompakt (Best Practice für Windows):**

```cmake
# Ensure vcpkg packages are discoverable (v1.3.5)
if(DEFINED ENV{VCPKG_ROOT})
    list(PREPEND CMAKE_PREFIX_PATH 
        "$ENV{VCPKG_ROOT}/installed/x64-windows"
        "$ENV{VCPKG_ROOT}/installed/x64-windows/share")
endif()
```

### ✅ LÖSUNG 2: Explizite DIR-Variablen bei CMake Configure

**Am einfachsten für SOFORT-FIX (PowerShell):**

```powershell
# FAISS + gRPC Problem sofort beheben
$VCPKG_INSTALLED = "C:\VCC\themis\vcpkg_installed\x64-windows"

cmake -S . -B build-msvc `
    -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake" `
    -DVCPKG_TARGET_TRIPLET=x64-windows `
    -DCMAKE_PREFIX_PATH="$VCPKG_INSTALLED;$VCPKG_INSTALLED\share" `
    -Dfaiss_DIR="$VCPKG_INSTALLED\share\faiss" `
    -DgRPC_DIR="$VCPKG_INSTALLED\share\grpc" `
    -DTHEMIS_ENABLE_GPU=ON
```

### ⚡ LÖSUNG 3: vcpkg manifest mode mit Toolchain (MITTEL-TERM)

**In CMakeLists.txt (Lines 40-45 ersetzen):**

```cmake
# vcpkg integration with proper PREFIX_PATH propagation
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    # CRITICAL: Set PREFIX_PATH after toolchain to ensure config packages are found
    list(APPEND CMAKE_PREFIX_PATH 
        "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}"
        "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share")
endif()

# Prefer CONFIG packages (vcpkg) - MUSS nach PREFIX_PATH kommen!
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
set(CMAKE_FIND_PACKAGE_CONFIG_DIR 
    "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/share"
    CACHE PATH "vcpkg cmake configs")
```

---

## PATCH FÜR CMakeLists.txt (EMPFOHLEN)

**Betroffene Dateien:**
- [CMakeLists.txt](CMakeLists.txt#L40-L50)
- [plugins/rpc/grpc/CMakeLists.txt](plugins/rpc/grpc/CMakeLists.txt#L8-L20)

### 1. Hauptdatei anpassen:

```diff
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -42,9 +42,20 @@ set(CMAKE_CXX_EXTENSIONS OFF)
 endif()
 
 # Export compile commands for IDE support
 set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
 
+# ============================================================================
+# v1.3.5: Fix CMAKE_PREFIX_PATH for vcpkg packages (faiss, gRPC, etc)
+# Explanation: vcpkg.cmake sets PREFIX_PATH but it may be cleared by IDE builds
+# SOLUTION: Explicitly prepend vcpkg installed directory to PREFIX_PATH
+# ============================================================================
+if(DEFINED ENV{VCPKG_ROOT})
+    list(PREPEND CMAKE_PREFIX_PATH 
+        "$ENV{VCPKG_ROOT}/installed/x64-windows/share"
+        "$ENV{VCPKG_ROOT}/installed/x64-windows")
+endif()
+
 # vcpkg integration
 if(DEFINED ENV{VCPKG_ROOT})
     set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
         CACHE STRING "Vcpkg toolchain file")
 endif()
 # Prefer CONFIG packages (vcpkg) over FindXXX modules
 set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
```

### 2. FAISS find_package anpassen (Lines 500-510):

```diff
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -503,7 +503,15 @@ find_package(hnswlib CONFIG)
 
 # Optional: Faiss for GPU support (graceful degradation if not found)
 if(THEMIS_ENABLE_GPU)
-    find_package(faiss CONFIG)
+    find_package(faiss CONFIG QUIET)
     if(NOT faiss_FOUND)
+        # Fallback: Try to find via direct DIR hint
+        if(NOT DEFINED faiss_DIR AND EXISTS "${CMAKE_PREFIX_PATH}")
+            find_package(faiss CONFIG 
+                HINTS "${CMAKE_PREFIX_PATH}"
+                QUIET)
+        endif()
+    endif()
+    if(NOT faiss_FOUND)
         message(WARNING "Faiss nicht gefunden – GPU-Unterstützung wird automatisch deaktiviert.")
         set(THEMIS_ENABLE_GPU OFF CACHE BOOL "Enable GPU acceleration for vector search" FORCE)
```

### 3. gRPC find_package anpassen (Lines 925-940):

```diff
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -927,7 +927,15 @@ endif()
 
     find_package(gRPC CONFIG QUIET)
     if(gRPC_FOUND)
         message(STATUS "gRPC found - enabling LLM gRPC service")
         list(APPEND THEMIS_CORE_SOURCES
             src/server/llm_grpc_service.cpp
             proto/llm_service.proto
         )
     else()
-        message(STATUS "gRPC not found - disabling LLM gRPC service and proto generation")
+        # Fallback: Try gRPC with HINTS
+        find_package(gRPC CONFIG 
+            HINTS "${CMAKE_PREFIX_PATH}"
+            QUIET)
+    endif()
+    if(NOT gRPC_FOUND)
+        message(WARNING "gRPC not found - LLM gRPC service disabled")
```

### 4. gRPC Plugin CMakeLists.txt anpassen:

```diff
--- a/plugins/rpc/grpc/CMakeLists.txt
+++ b/plugins/rpc/grpc/CMakeLists.txt
@@ -7,9 +7,21 @@ message(STATUS "Configuring gRPC Plugin")
 
 # Find gRPC package
-find_package(gRPC CONFIG)
+# v1.3.5 Hotfix: Ensure gRPC_DIR is set
+if(NOT DEFINED gRPC_DIR AND DEFINED ENV{VCPKG_ROOT})
+    set(gRPC_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows/share/grpc" CACHE PATH "gRPC directory")
+endif()
+
+find_package(gRPC CONFIG QUIET)
 if(NOT gRPC_FOUND)
-    find_package(gRPC)
+    # Fallback with hints
+    find_package(gRPC CONFIG 
+        HINTS "$ENV{VCPKG_ROOT}/installed/x64-windows/share/grpc"
+        QUIET)
 endif()
+
 if(NOT gRPC_FOUND)
-    message(WARNING "gRPC not found - skipping gRPC plugin build")
+    message(WARNING "gRPC not found - skipping gRPC plugin build. "
+        "Set gRPC_DIR to <vcpkg>/installed/x64-windows/share/grpc")
     message(WARNING "Install with: apt install libgrpc++-dev or brew install grpc or vcpkg install grpc")
```

---

## VOR & NACH: Korrekte CMake-Befehle

### ❌ VOR (Fehler):
```powershell
cmake -S . -B build-msvc `
    -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake"
# → Fehler: faiss_DIR nicht gefunden, gRPC_DIR nicht gefunden
```

### ✅ NACH (Fix 1 - CMAKE_PREFIX_PATH):
```powershell
cmake -S . -B build-msvc `
    -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake" `
    -DCMAKE_PREFIX_PATH="C:\VCC\themis\vcpkg_installed\x64-windows;C:\VCC\themis\vcpkg_installed\x64-windows\share" `
    -DTHEMIS_ENABLE_GPU=ON
# → ✅ FAISS gefunden
```

### ✅ NACH (Fix 2 - Explizite DIR):
```powershell
cmake -S . -B build-msvc `
    -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake" `
    -Dfaiss_DIR="C:\VCC\themis\vcpkg_installed\x64-windows\share\faiss" `
    -DgRPC_DIR="C:\VCC\themis\vcpkg_installed\x64-windows\share\grpc" `
    -DTHEMIS_ENABLE_GPU=ON
# → ✅ FAISS + gRPC gefunden
```

---

## DIAGNOSE & DEBUG

### Status überprüfen nach CMake Configure:

```powershell
# 1. FAISS Config
$VCPKG = "C:\VCC\themis\vcpkg_installed\x64-windows"
Test-Path "$VCPKG\share\faiss\faiss-config.cmake"  # Muss TRUE sein
Test-Path "$VCPKG\lib\faiss.lib"                   # Muss TRUE sein

# 2. gRPC Config
Test-Path "$VCPKG\share\grpc\gRPCConfig.cmake"    # Muss TRUE sein
Test-Path "$VCPKG\lib\grpc.lib"                    # Muss TRUE sein

# 3. CMake-Cache überprüfen
Get-Content "build-msvc\CMakeCache.txt" | Select-String "faiss|gRPC" | Select-Object -First 10
```

### Im CMake configure-Output prüfen:

```bash
# Vor dem Build sollte folgendes stehen:
-- Found faiss: <path>/share/faiss/faiss-config.cmake (found version "1.8.0")
-- Found gRPC: <path>/share/grpc/gRPCConfig.cmake (found version "1.71.0")
```

---

## OFFIZIELLE DOKUMENTATION & GITHUB ISSUES

### FAISS CMake References:
1. **FAISS Official CMake**: https://github.com/facebookresearch/faiss/blob/main/cmake/faiss-config.cmake.in
2. **FAISS CMakeLists**: https://github.com/facebookresearch/faiss/blob/main/CMakeLists.txt
3. **vcpkg FAISS Port**: https://github.com/microsoft/vcpkg/blob/master/ports/faiss/portfile.cmake
4. **Known Windows Issues**: https://github.com/facebookresearch/faiss/issues?q=cmake+windows

### gRPC CMake References:
1. **gRPC Official CMake**: https://github.com/grpc/grpc/blob/master/cmake/gRPCConfig.cmake.in
2. **gRPC CMakeLists**: https://github.com/grpc/grpc/blob/master/CMakeLists.txt#L200-L250
3. **vcpkg gRPC Port**: https://github.com/microsoft/vcpkg/blob/master/ports/grpc/portfile.cmake
4. **vcpkg CMake Wrapper Fix**: https://github.com/microsoft/vcpkg/blob/master/ports/grpc/vcpkg-cmake-wrapper.cmake
5. **MSVC Static Build Fix**: https://github.com/grpc/grpc/pull/38623

### Relevante Workarounds:
- **FAISS Windows Build Issue #2909**: Shared libs problem on Windows
- **gRPC MSVC Static Runtime #38623**: CMake >= 3.15 fix für MSVC
- **vcpkg CONFIG Mode**: Ensure `CMAKE_FIND_PACKAGE_PREFER_CONFIG=ON`

---

## VCPKG REPARATUR BEFEHLE

Falls die vcpkg Installation beschädigt ist:

```powershell
# 1. Faiss neu bauen (mit GPU optional)
cd C:\VCC\themis
.\vcpkg\vcpkg install faiss:x64-windows

# 2. gRPC neu bauen (statisch erforderlich auf Windows)
.\vcpkg\vcpkg install grpc:x64-windows

# 3. Protobuf (abhängig von gRPC, wird automatisch gezogen)
.\vcpkg\vcpkg install protobuf:x64-windows

# 4. Alle abhängigen Pakete überprüfen
.\vcpkg\vcpkg list | Select-String "faiss|grpc|protobuf"

# 5. Ganze vcpkg_installed löschen und neubuild (NUCLEAR)
Remove-Item -Recurse -Force "vcpkg_installed"
.\vcpkg\vcpkg install
```

---

## IMPLEMENTATION CHECKLIST

- [ ] **Step 1**: CMakeLists.txt mit CMAKE_PREFIX_PATH Patch patchen (Zeile 40-50)
- [ ] **Step 2**: gRPC Plugin CMakeLists.txt mit gRPC_DIR Fallback patchen
- [ ] **Step 3**: CMake Configure mit `-DCMAKE_PREFIX_PATH` durchführen
- [ ] **Step 4**: Überprüfen dass `faiss_DIR` und `gRPC_DIR` in CMakeCache.txt gesetzt sind
- [ ] **Step 5**: `cmake --build . --target themis_server` testen
- [ ] **Step 6**: Test durchführen mit `-DTHEMIS_ENABLE_GPU=ON` für FAISS-Integration
- [ ] **Step 7**: Test durchführen mit LLM-Feature für gRPC-Integration

---

## QUICK REFERENCE: Pfade unter Windows (x64)

| Package | Konfiguration | Installed Path |
|---------|---------------|-----------------|
| **FAISS** | faiss-config.cmake | `vcpkg_installed\x64-windows\share\faiss\` |
| **FAISS** | Library | `vcpkg_installed\x64-windows\lib\faiss.lib` |
| **gRPC** | gRPCConfig.cmake | `vcpkg_installed\x64-windows\share\grpc\` |
| **gRPC** | Library | `vcpkg_installed\x64-windows\lib\grpc*.lib` |
| **Protobuf** | protobufConfig.cmake | `vcpkg_installed\x64-windows\share\protobuf\` |
| **OpenMP** | (MSVC builtin) | Via `/openmp` compiler flag |
| **BLAS/LAPACK** | FAISS dependency | vcpkg-abhängig (Intel MKL oder openblas) |

---

## Version-Info
- **ThemisDB**: v1.3.5+
- **FAISS**: v1.8.0 (vcpkg)
- **gRPC**: v1.71.0 (vcpkg)
- **CMake**: ≥3.20
- **MSVC**: Visual Studio 17 2022
- **vcpkg**: Latest stable
