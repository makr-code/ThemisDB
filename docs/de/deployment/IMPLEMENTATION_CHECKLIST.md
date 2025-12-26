# CMake PREFIX_PATH Fix - Implementierungsanleitung für ThemisDB v1.3.5

## 📋 Übersicht

Diese Anleitung behebt die CMake find_package() Fehler für FAISS und gRPC auf Windows (x64).

**Status**: 🟢 Bereit zur Implementierung  
**Betroffene CMakeLists.txt Dateien**: 2  
**Geschätzte Implementierungszeit**: 10 Minuten  
**Test-Zeit**: 15 Minuten  

---

## 🔍 Phase 1: Diagnose (5 min)

### 1.1 Aktuellen Status überprüfen

```powershell
# Terminal öffnen und ins Projektverzeichnis navigieren
cd C:\VCC\themis

# Diagnose durchführen
.\scripts\fix-cmake-prefix-path.ps1 -Action diagnose
```

**Erwartet Output wenn alles falsch ist:**
```
❌ FAISS config NOT found: ...
❌ gRPC config NOT found: ...
⚠️  CMake Cache missing faiss_DIR entry
⚠️  CMake Cache missing gRPC_DIR entry
```

### 1.2 Manuelle Überprüfung

```powershell
# FAISS Check
Test-Path "C:\VCC\themis\vcpkg_installed\x64-windows\share\faiss\faiss-config.cmake"
# Erwartet: True

# gRPC Check
Test-Path "C:\VCC\themis\vcpkg_installed\x64-windows\share\grpc\gRPCConfig.cmake"
# Erwartet: True

# Libraries Check
Test-Path "C:\VCC\themis\vcpkg_installed\x64-windows\lib\faiss.lib"    # True erwartet
Test-Path "C:\VCC\themis\vcpkg_installed\x64-windows\lib\grpc.lib"     # True erwartet
```

---

## 🛠️ Phase 2: CMakeLists.txt Patchen (5 min)

### 2.1 Hauptdatei patchen: CMakeLists.txt

**Datei**: [CMakeLists.txt](CMakeLists.txt)  
**Zeile zu patchen**: 40-50 (nach `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`)

**Vor (Zeile 40-47):**
```cmake
# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# vcpkg integration
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
```

**Nach (Zeile 40-60):**
```cmake
# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# v1.3.5: Fix CMAKE_PREFIX_PATH for vcpkg packages (faiss, gRPC, etc)
# Explanation: vcpkg.cmake sets PREFIX_PATH but it may be cleared by IDE builds
# SOLUTION: Explicitly prepend vcpkg installed directory to PREFIX_PATH
# GitHub Issues: microsoft/vcpkg#xxxxx, grpc/grpc#38623
# ============================================================================
if(DEFINED ENV{VCPKG_ROOT})
    list(PREPEND CMAKE_PREFIX_PATH 
        "$ENV{VCPKG_ROOT}/installed/x64-windows/share"
        "$ENV{VCPKG_ROOT}/installed/x64-windows")
    message(STATUS "v1.3.5: CMAKE_PREFIX_PATH set to ${CMAKE_PREFIX_PATH}")
endif()

# vcpkg integration
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
```

**Implementation**:
1. VS Code öffnen: Datei `CMakeLists.txt` (Line 1)
2. Zu Zeile 40 navigieren (Ctrl+G → "40")
3. Nach Zeile 46 (nach `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`) neue Zeilen einfügen
4. Obigen Code einfügen
5. Speichern (Ctrl+S)

---

### 2.2 FAISS find_package patchen (Zeile 502-510)

**Vor:**
```cmake
# Optional: Faiss for GPU support (graceful degradation if not found)
if(THEMIS_ENABLE_GPU)
    find_package(faiss CONFIG)
    if(NOT faiss_FOUND)
        message(WARNING "Faiss nicht gefunden – GPU-Unterstützung wird automatisch deaktiviert.")
        set(THEMIS_ENABLE_GPU OFF CACHE BOOL "Enable GPU acceleration for vector search" FORCE)
    endif()
endif()
```

**Nach:**
```cmake
# Optional: Faiss for GPU support (graceful degradation if not found)
if(THEMIS_ENABLE_GPU)
    find_package(faiss CONFIG QUIET)
    if(NOT faiss_FOUND)
        # v1.3.5: Fallback with HINTS if CMAKE_PREFIX_PATH wasn't propagated
        find_package(faiss CONFIG 
            HINTS "${CMAKE_PREFIX_PATH}"
            QUIET)
    endif()
    if(NOT faiss_FOUND)
        message(WARNING "Faiss nicht gefunden – GPU-Unterstützung wird automatisch deaktiviert.")
        set(THEMIS_ENABLE_GPU OFF CACHE BOOL "Enable GPU acceleration for vector search" FORCE)
    else()
        message(STATUS "v1.3.5: faiss found at ${faiss_DIR}")
    endif()
endif()
```

**Implementation**:
1. Zu Zeile 502 navigieren
2. Die if-block mit den Fallback-find_package Calls ersetzen
3. Speichern

---

### 2.3 gRPC find_package patchen (Zeile 925-940)

**Vor:**
```cmake
    find_package(gRPC CONFIG QUIET)
    if(gRPC_FOUND)
        message(STATUS "gRPC found - enabling LLM gRPC service")
        list(APPEND THEMIS_CORE_SOURCES
            src/server/llm_grpc_service.cpp
            proto/llm_service.proto
        )
        # Note: Proto generation should be configured in a separate section when needed
    else()
        message(STATUS "gRPC not found - disabling LLM gRPC service and proto generation")
    endif()
```

**Nach:**
```cmake
    find_package(gRPC CONFIG QUIET)
    if(NOT gRPC_FOUND)
        # v1.3.5: Fallback with HINTS if CMAKE_PREFIX_PATH wasn't propagated
        find_package(gRPC CONFIG 
            HINTS "${CMAKE_PREFIX_PATH}"
            QUIET)
    endif()
    
    if(gRPC_FOUND)
        message(STATUS "v1.3.5: gRPC found at ${gRPC_DIR} - enabling LLM gRPC service")
        list(APPEND THEMIS_CORE_SOURCES
            src/server/llm_grpc_service.cpp
            proto/llm_service.proto
        )
        # Note: Proto generation should be configured in a separate section when needed
    else()
        message(WARNING "gRPC not found - disabling LLM gRPC service and proto generation")
    endif()
```

**Implementation**:
1. Zu Zeile 925 navigieren
2. Die find_package Calls ersetzen
3. Speichern

---

### 2.4 gRPC Plugin CMakeLists patchen

**Datei**: [plugins/rpc/grpc/CMakeLists.txt](plugins/rpc/grpc/CMakeLists.txt)  
**Zeile**: 8-20

**Vor:**
```cmake
# Find gRPC package
find_package(gRPC CONFIG)
if(NOT gRPC_FOUND)
    find_package(gRPC)
endif()

if(NOT gRPC_FOUND)
    message(WARNING "gRPC not found - skipping gRPC plugin build")
    message(WARNING "Install with: apt install libgrpc++-dev or brew install grpc or vcpkg install grpc")
    return()
endif()
```

**Nach:**
```cmake
# v1.3.5 Hotfix: Ensure gRPC_DIR is set from vcpkg
if(NOT DEFINED gRPC_DIR AND DEFINED ENV{VCPKG_ROOT})
    set(gRPC_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows/share/grpc" CACHE PATH "gRPC directory")
endif()

# Find gRPC package
find_package(gRPC CONFIG QUIET)
if(NOT gRPC_FOUND)
    # Fallback with HINTS
    find_package(gRPC CONFIG 
        HINTS "${CMAKE_PREFIX_PATH}"
        QUIET)
endif()

if(NOT gRPC_FOUND)
    message(WARNING "gRPC not found - skipping gRPC plugin build")
    message(WARNING "To fix: Set gRPC_DIR to <vcpkg>/installed/x64-windows/share/grpc")
    message(WARNING "Or install with: apt install libgrpc++-dev | brew install grpc | vcpkg install grpc")
    return()
else()
    message(STATUS "v1.3.5: gRPC found at ${gRPC_DIR}")
endif()
```

**Implementation**:
1. Datei öffnen: [plugins/rpc/grpc/CMakeLists.txt](plugins/rpc/grpc/CMakeLists.txt)
2. Zu Zeile 8 navigieren
3. Code ersetzen
4. Speichern

---

## ✅ Phase 3: CMake Configure durchführen (5 min)

### 3.1 Automatisiertes Script verwenden (EMPFOHLEN)

```powershell
cd C:\VCC\themis

# Option 1: Einfach mit Standard-Optionen
.\scripts\fix-cmake-prefix-path.ps1 -Action build

# Option 2: Mit GPU-Support
.\scripts\fix-cmake-prefix-path.ps1 -Action build -EnableGPU $true

# Option 3: Mit GPU + LLM
.\scripts\fix-cmake-prefix-path.ps1 -Action build -EnableGPU $true -EnableLLM $true

# Option 4: Clean (Remove alt build dir) + Build
.\scripts\fix-cmake-prefix-path.ps1 -Action clean
```

### 3.2 Manuelles CMake Configure (Falls Script fehlschlägt)

```powershell
cd C:\VCC\themis

$VCPKG = "C:\VCC\themis\vcpkg_installed\x64-windows"

cmake -S . -B build-msvc `
    -G "Visual Studio 17 2022" `
    -A x64 `
    -DCMAKE_TOOLCHAIN_FILE="$PWD\vcpkg\scripts\buildsystems\vcpkg.cmake" `
    -DVCPKG_TARGET_TRIPLET=x64-windows `
    -DCMAKE_PREFIX_PATH="$VCPKG;$VCPKG\share" `
    -Dfaiss_DIR="$VCPKG\share\faiss" `
    -DgRPC_DIR="$VCPKG\share\grpc" `
    -DTHEMIS_ENABLE_GPU=ON `
    -DTHEMIS_ENABLE_LLM=ON `
    -DTHEMIS_BUILD_TESTS=ON
```

### 3.3 Output überprüfen

**Erfolgreiches Zeichen:**
```
...
-- v1.3.5: CMAKE_PREFIX_PATH set to ...
...
-- Found faiss: ...faiss-config.cmake (found version "1.8.0")
-- v1.3.5: faiss found at ...
...
-- v1.3.5: gRPC found at ... - enabling LLM gRPC service
...
-- Configuring done (10.5s)
-- Generating done (0.5s)
-- Build files have been generated to: C:\VCC\themis\build-msvc
```

**Fehler-Zeichen:**
```
CMake Error: Could NOT find faiss (missing: faiss_DIR)
CMake Error: Could NOT find gRPC (missing: gRPC_DIR)
```

Falls Fehler: **Gehe zu Phase 4 Debugging**.

---

## 🧪 Phase 4: Build & Test (10 min)

### 4.1 Build durchführen

```powershell
cd C:\VCC\themis

# Option 1: CMAKE
cmake --build build-msvc --config Release --target themis_server --parallel 8

# Option 2: MSBuild direkt
msbuild build-msvc\Themis.sln /p:Configuration=Release /m:8 /t:themis_server
```

### 4.2 Überprüfen dass Exe gebaut wurde

```powershell
# Sollte existieren
Test-Path "C:\VCC\themis\build-msvc\Release\themis_server.exe"  # True erwartet

# Optional: Mit GPU auch
Test-Path "C:\VCC\themis\build-msvc\Release\themis_core.lib"    # True erwartet
```

### 4.3 Runtime-Test (wenn GPU=ON)

```powershell
# Wenn GPU-Features gebaut wurden, überprüfen ob FAISS geladen wird
C:\VCC\themis\build-msvc\Release\themis_server.exe --help | Select-String -Pattern "faiss|gpu"

# Wenn keine Fehler: ✅ FAISS Integration erfolgreich
```

---

## 🔧 Phase 5: Verifikation (5 min)

### 5.1 CMake Cache überprüfen

```powershell
# Überprüfe dass Pfade im Cache korrekt sind
Select-String -Path "C:\VCC\themis\build-msvc\CMakeCache.txt" `
    -Pattern "faiss|gRPC" | Select-Object -First 20

# Sollte enthalten:
# faiss_DIR:PATH=...share\faiss
# gRPC_DIR:PATH=...share\grpc
```

### 5.2 Finale Diagnose

```powershell
# Script erneut laufen lassen für Bestätigung
.\scripts\fix-cmake-prefix-path.ps1 -Action diagnose

# Sollte ALLE grünen ✅ Checks zeigen
```

---

## 🐛 Phase 6: Troubleshooting

### Problem 1: "Could NOT find faiss (missing: faiss_DIR)"

**Ursache**: CMAKE_PREFIX_PATH wurde nicht richtig propagiert

**Lösung**:
```powershell
# 1. Überprüfe VCPKG_ROOT
$env:VCPKG_ROOT

# 2. Wenn leer, setzen
$env:VCPKG_ROOT = "C:\VCC\themis\vcpkg"

# 3. Erneut CMake Config mit explizitem faiss_DIR
cmake -S . -B build-msvc `
    ... `
    -Dfaiss_DIR="C:\VCC\themis\vcpkg_installed\x64-windows\share\faiss" `
    ...
```

### Problem 2: "Could NOT find gRPC (missing: gRPC_DIR)"

**Ursache**: Protobuf Abhängigkeit nicht gefunden

**Lösung**:
```powershell
# vcpkg protobuf neu bauen
cd C:\VCC\themis
.\vcpkg\vcpkg install protobuf:x64-windows

# dann erneut gRPC
.\vcpkg\vcpkg install grpc:x64-windows

# Dann CMake Config neu mit explizitem gRPC_DIR
cmake -S . -B build-msvc `
    ... `
    -DgRPC_DIR="C:\VCC\themis\vcpkg_installed\x64-windows\share\grpc" `
    ...
```

### Problem 3: Build schlägt mit Linker-Fehler fehl

**Ursache**: gRPC auf Windows MUSS statisch sein

**Überprüfung**:
```powershell
# MUSS für Windows sein:
Select-String -Path "C:\VCC\themis\vcpkg_installed\x64-windows\lib\grpc.lib"

# Sollte statische Library sein, nicht .dll
Test-Path "C:\VCC\themis\vcpkg_installed\x64-windows\lib\grpc.dll"  # FALSE erwartet!
```

**Fix wenn DLL vorhanden**:
```powershell
# gRPC mit statischen Libraries neu bauen
cd C:\VCC\themis
Remove-Item -Recurse -Force "vcpkg_installed\x64-windows\*" -Force
.\vcpkg\vcpkg install grpc:x64-windows --triplet x64-windows
```

### Problem 4: "missing: FAISS_FOUND variable set to FALSE"

**Ursache**: FAISS Config-Datei hat Abhängigkeitsfehler

**Debug**:
```powershell
# Lese die config datei
Get-Content "C:\VCC\themis\vcpkg_installed\x64-windows\share\faiss\faiss-config.cmake"

# Sollte mit find_dependency(OpenMP) und find_dependency(BLAS) starten
# Wenn Fehler: Überprüfe OpenMP und BLAS installation

# Neu bauen wenn nötig
.\vcpkg\vcpkg install faiss:x64-windows
```

---

## ✨ Checkliste zum Abhaken

### Vor Implementierung
- [ ] Workspace geöffnet in VS Code
- [ ] Terminal verfügbar
- [ ] Diagnose durchgeführt und Probleme identifiziert

### Während Implementierung
- [ ] [CMakeLists.txt](CMakeLists.txt) Line 40-60 gepatchot ✅
- [ ] FAISS find_package (Line 502-510) gepatchot ✅
- [ ] gRPC find_package (Line 925-940) gepatchot ✅
- [ ] gRPC Plugin [CMakeLists.txt](plugins/rpc/grpc/CMakeLists.txt) gepatchot ✅
- [ ] Alle Dateien gespeichert (Ctrl+S) ✅

### CMake Build
- [ ] CMake Configure erfolgreich: `configure done` message ✅
- [ ] Build erfolgreich: `themis_server.exe` existiert ✅
- [ ] Keine Linker-Fehler ✅

### Verifikation
- [ ] Diagnose-Script zeigt alle ✅ Checks ✅
- [ ] CMake Cache enthält faiss_DIR und gRPC_DIR ✅
- [ ] themis_server.exe lädt FAISS/gRPC ohne Fehler ✅

---

## 📞 Support

Falls Probleme nach Implementierung:

1. **Logs sammeln**:
   ```powershell
   cmake --build build-msvc --config Release 2>&1 | Tee-Object -FilePath "build_errors.log"
   ```

2. **Diagnose durchführen**:
   ```powershell
   .\scripts\fix-cmake-prefix-path.ps1 -Action diagnose > diagnostics.txt
   ```

3. **CMake verbose**:
   ```powershell
   cmake -S . -B build-msvc --debug-output
   ```

4. **Ref. Dokumente konsultieren**:
   - [CMAKE_FIND_PACKAGE_SOLUTIONS.md](CMAKE_FIND_PACKAGE_SOLUTIONS.md)
   - [GITHUB_ISSUES_REFERENCE.md](GITHUB_ISSUES_REFERENCE.md)

---

**Version**: 1.3.5  
**Datum**: 2025-12-26  
**Status**: 🟢 Ready  
**Geschätzte Implementierungszeit**: 20 Minuten total
