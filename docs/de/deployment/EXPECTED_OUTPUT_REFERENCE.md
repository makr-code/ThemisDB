# CMake find_package - Expected Output Reference

## Erfolgreiche CMake Configure Ausgabe

Nach erfolgreichem Patch und CMake Configure sollte die Ausgabe ungefähr so aussehen:

```
C:\VCC\themis> cmake -S . -B build-msvc `
>> -G "Visual Studio 17 2022" -A x64 `
>> -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake" `
>> -DCMAKE_PREFIX_PATH="C:\VCC\themis\vcpkg_installed\x64-windows;C:\VCC\themis\vcpkg_installed\x64-windows\share" `
>> -Dfaiss_DIR="C:\VCC\themis\vcpkg_installed\x64-windows\share\faiss" `
>> -DgRPC_DIR="C:\VCC\themis\vcpkg_installed\x64-windows\share\grpc" `
>> -DTHEMIS_ENABLE_GPU=ON `
>> -DTHEMIS_ENABLE_LLM=ON

-- The CXX compiler identification is MSVC 19.42.34433.0
-- The RC compiler identification is rc
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.42.34433/bin/HostX86/x64/cl.exe - skipped
-- Detecting CXX compiler features
-- Detecting CXX compiler features - done
-- v1.3.5: CMAKE_PREFIX_PATH set to C:\VCC\themis\vcpkg_installed\x64-windows\share;C:\VCC\themis\vcpkg_installed\x64-windows

=== OpenSSL Setup ===
-- Found OpenSSL: C:/VCC/themis/vcpkg_installed/x64-windows/lib/libssl.lib (found version "3.2.0")
-- Found CURL

=== ThemisDB Build Configuration ===
-- Building for Windows x86_64 with MSVC
-- C++ Standard: C++20
-- Optimize: Release
-- Build Tests: ON
-- Build Benchmarks: ON

=== Building FAISS GPU Support ===
-- Looking for OpenMP
-- Found OpenMP_CXX: C:/VCC/themis/vcpkg_installed/x64-windows/lib/omp.lib (found version "4.0")
-- Found OpenMP: TRUE
-- Looking for BLAS
-- Found BLAS: C:/VCC/themis/vcpkg_installed/x64-windows/lib/openblas.lib
-- Found LAPACK: C:/VCC/themis/vcpkg_installed/x64-windows/lib/openblas.lib
-- Found faiss: C:/VCC/themis/vcpkg_installed/x64-windows/share/faiss/faiss-config.cmake (found version "1.8.0")
-- v1.3.5: faiss found at C:/VCC/themis/vcpkg_installed/x64-windows/share/faiss
-- THEMIS_ENABLE_GPU: ON (faiss found, GPU features enabled)

=== Building LLM Features ===
-- Found gRPC: C:/VCC/themis/vcpkg_installed/x64-windows/share/grpc/gRPCConfig.cmake (found version "1.71.0")
-- Found Protobuf: C:/VCC/themis/vcpkg_installed/x64-windows/share/protobuf/protobufConfig.cmake (found version "4.28.2")
-- v1.3.5: gRPC found at C:/VCC/themis/vcpkg_installed/x64-windows/share/grpc - enabling LLM gRPC service
-- Configuring LLM components with grpc support
-- THEMIS_ENABLE_LLM: ON (gRPC found, LLM gRPC service enabled)

=== Optional Components ===
-- Found mimalloc: C:/VCC/themis/vcpkg_installed/x64-windows/lib/mimalloc.lib
-- mimalloc found - enabling for 20-40% memory boost

=== Configuring done (14.5s) ===
=== Generating done (1.2s) ===
-- Build files have been generated to: C:\VCC\themis\build-msvc
```

---

## Was bedeuten die Schlüssel-Zeilen?

### ✅ FAISS wurde gefunden
```
-- Found faiss: C:/VCC/themis/vcpkg_installed/x64-windows/share/faiss/faiss-config.cmake (found version "1.8.0")
-- v1.3.5: faiss found at C:/VCC/themis/vcpkg_installed/x64-windows/share/faiss
```
**Bedeutung**: FAISS wird für GPU-Unterstützung verwendet  
**Status**: 🟢 OK

### ✅ gRPC wurde gefunden
```
-- Found gRPC: C:/VCC/themis/vcpkg_installed/x64-windows/share/grpc/gRPCConfig.cmake (found version "1.71.0")
-- v1.3.5: gRPC found at C:/VCC/themis/vcpkg_installed/x64-windows/share/grpc - enabling LLM gRPC service
```
**Bedeutung**: gRPC wird für LLM RPC-Services verwendet  
**Status**: 🟢 OK

### ✅ CMAKE_PREFIX_PATH gesetzt
```
-- v1.3.5: CMAKE_PREFIX_PATH set to C:\VCC\themis\vcpkg_installed\x64-windows\share;C:\VCC\themis\vcpkg_installed\x64-windows
```
**Bedeutung**: Der CMAKE_PREFIX_PATH Patch funktioniert  
**Status**: 🟢 OK

### ✅ Protobuf gefunden (gRPC Abhängigkeit)
```
-- Found Protobuf: C:/VCC/themis/vcpkg_installed/x64-windows/share/protobuf/protobufConfig.cmake (found version "4.28.2")
```
**Bedeutung**: gRPC kann erfolgreich gebaut werden  
**Status**: 🟢 OK

---

## Fehler-Output - Was ist FALSCH?

### ❌ FAISS NICHT gefunden
```
CMake Error at CMakeLists.txt:504 (find_package):
  By not providing "faissConfig.cmake" in CMAKE_PREFIX_PATH this project has
  asked CMake to find a package configuration file provided by "faiss", but
  CMake could not find one.

Could NOT find faiss (missing: faiss_DIR)
```

**Ursache**: CMAKE_PREFIX_PATH wurde nicht gesetzt  
**Lösung**:
1. Überprüfe dass `.\scripts\fix-cmake-prefix-path.ps1 -Action diagnose` grüne Checks zeigt
2. Verwende explizit: `-Dfaiss_DIR="C:\VCC\themis\vcpkg_installed\x64-windows\share\faiss"`

### ❌ gRPC NICHT gefunden (Primary)
```
CMake Error at plugins/rpc/grpc/CMakeLists.txt:9 (find_package):
  By not providing "gRPCConfig.cmake" in CMAKE_PREFIX_PATH this project has
  asked CMake to find a package configuration file provided by "gRPC", but
  CMake could not find one.

Could NOT find gRPC (missing: gRPC_DIR)
```

**Ursache**: gRPC_DIR nicht gesetzt  
**Lösung**: 
1. `-DgRPC_DIR="C:\VCC\themis\vcpkg_installed\x64-windows\share\grpc"`
2. Überprüfe dass Protobuf auch gefunden wird

### ❌ Protobuf NICHT gefunden (Secondary - bricht gRPC)
```
CMake Error at C:/VCC/themis/vcpkg_installed/x64-windows/share/grpc/gRPCConfig.cmake:5 (find_dependency):
  Could not find a package configuration file provided by "Protobuf"
```

**Ursache**: Protobuf nicht installiert oder PATH falsch  
**Lösung**:
```powershell
# Protobuf neu bauen
cd C:\VCC\themis
.\vcpkg\vcpkg install protobuf:x64-windows

# Dann erneut CMake
cmake -S . -B build-msvc ...
```

### ❌ BLAS/LAPACK NICHT gefunden (Secondary - bricht FAISS)
```
CMake Error at C:/VCC/themis/vcpkg_installed/x64-windows/share/faiss/faiss-config.cmake:7 (find_dependency):
  Could not find a package configuration file provided by "BLAS"
```

**Ursache**: FAISS Dependencies nicht installiert  
**Lösung**:
```powershell
# FAISS neu bauen (zieht BLAS/LAPACK automatisch)
cd C:\VCC\themis
.\vcpkg\vcpkg install faiss:x64-windows

# Oder mit GPU
.\vcpkg\vcpkg install faiss[gpu]:x64-windows
```

### ❌ OpenMP NICHT gefunden (Secondary - bricht FAISS GPU)
```
-- Looking for OpenMP
-- Looking for OpenMP - found
-- Found OpenMP_CXX: NOTFOUND
```

**Ursache**: OpenMP (für MSVC) nicht korrekt verlinkt  
**Lösung**:
```powershell
# MSVC OpenMP sollte automatisch gefunden werden
# Falls nicht: Explizit in CMakeLists.txt
find_package(OpenMP REQUIRED)
target_link_libraries(themis_core PRIVATE OpenMP::OpenMP_CXX)
```

---

## Build-Output - Nach erfolgreicher Configuration

### ✅ Erfolgreicher Build

```
C:\VCC\themis> cmake --build build-msvc --config Release --target themis_server --parallel 8

Microsoft (R) Build Engine version 17.9.1+41af1a3e2 for .NET Framework
Copyright (C) Microsoft Corporation. All rights reserved.

Build started 2025-12-26 10:15:32.

Project "C:\VCC\themis\build-msvc\Themis.sln" on node 1 (themis_server target(s)).
  ...
  Building themis_core.lib...
  Building themis_server.exe... [linking FAISS, gRPC, Protobuf libraries]
  ...
  2 files to modify in total.

Build succeeded.
    0 Warning(s)
    0 Error(s)

Time Elapsed 00:02:45.62

C:\VCC\themis> ls build-msvc\Release\themis_server.exe

    Directory: C:\VCC\themis\build-msvc\Release

Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a---         26.12.2025     10:18       45632000 themis_server.exe
```

**Zeichen**: themis_server.exe existiert ✅

### ❌ Build fehlgeschlagen - Linker Error mit gRPC

```
error LNK2019: unresolved external symbol "..." referenced in function "..."
error LNK1120: ... unresolved externals

LINK: fatal error LNK1181: cannot open input file 'grpc.lib'
```

**Ursache**: gRPC Library nicht verlinkt oder falsch statisch/dynamisch  
**Lösung**:
1. Überprüfe dass gRPC statisch ist (MUSS Windows):
   ```powershell
   Test-Path "C:\VCC\themis\vcpkg_installed\x64-windows\lib\grpc.lib"  # TRUE
   Test-Path "C:\VCC\themis\vcpkg_installed\x64-windows\lib\grpc.dll"  # FALSE!
   ```
2. Falls DLL vorhanden: gRPC mit staticlib neu bauen
   ```powershell
   .\vcpkg\vcpkg remove grpc:x64-windows
   .\vcpkg\vcpkg install grpc:x64-windows
   ```

---

## Diagnose-Output - Skript

### ✅ Erfolgreiche Diagnose

```
C:\VCC\themis> .\scripts\fix-cmake-prefix-path.ps1 -Action diagnose

=== DIAGNOSE: FAISS + gRPC CMAKE CONFIG STATUS ===

[1/5] Checking FAISS...
✅ FAISS config found: C:\VCC\themis\vcpkg_installed\x64-windows\share\faiss\faiss-config.cmake
✅ FAISS library found: C:\VCC\themis\vcpkg_installed\x64-windows\lib\faiss.lib

[2/5] Checking gRPC...
✅ gRPC config found: C:\VCC\themis\vcpkg_installed\x64-windows\share\grpc\gRPCConfig.cmake
✅ gRPC library found: C:\VCC\themis\vcpkg_installed\x64-windows\lib\grpc.lib

[3/5] Checking Protobuf (gRPC dependency)...
✅ Protobuf config found

[4/5] Checking CMake Cache...
✅ CMake Cache has faiss_DIR: ...
✅ CMake Cache has gRPC_DIR: ...

[5/5] Checking environment...
✅ VCPKG_ROOT is set: C:\VCC\themis\vcpkg

ℹ️  Diagnosis complete. Next: .\fix-cmake-prefix-path.ps1 -Action build

```

**Zeichen**: Alle grünen ✅ Checks → 🟢 Ready to build

### ❌ Fehlerhafte Diagnose

```
❌ FAISS config NOT found: C:\VCC\themis\vcpkg_installed\x64-windows\share\faiss\faiss-config.cmake
❌ FAISS library NOT found: C:\VCC\themis\vcpkg_installed\x64-windows\lib\faiss.lib
❌ gRPC config NOT found: C:\VCC\themis\vcpkg_installed\x64-windows\share\grpc\gRPCConfig.cmake
❌ gRPC library NOT found: C:\VCC\themis\vcpkg_installed\x64-windows\lib\grpc.lib
```

**Bedeutung**: vcpkg Pakete nicht installiert  
**Lösung**:
```powershell
cd C:\VCC\themis
.\vcpkg\vcpkg install faiss:x64-windows grpc:x64-windows protobuf:x64-windows
```

---

## CMake Cache Überprüfung

### ✅ Richtige Cache-Einträge

```powershell
Select-String -Path "build-msvc\CMakeCache.txt" -Pattern "faiss|gRPC"

# Sollte zeigen:
faiss_FOUND:BOOL=TRUE
faiss_DIR:PATH=C:/VCC/themis/vcpkg_installed/x64-windows/share/faiss

gRPC_FOUND:BOOL=TRUE
gRPC_DIR:PATH=C:/VCC/themis/vcpkg_installed/x64-windows/share/grpc
```

### ❌ Falsche Cache-Einträge (Problem!)

```
faiss_FOUND:BOOL=FALSE
gRPC_FOUND:BOOL=FALSE

# Cache Datei gar nicht vorhanden
```

**Bedeutung**: CMake Configure wurde nicht erfolgreich durchgeführt  
**Lösung**: Führe CMake Configure erneut durch

---

## Testing-Checkliste mit Expected Output

```powershell
# 1. Diagnose durchführen
.\scripts\fix-cmake-prefix-path.ps1 -Action diagnose
# ✅ Erwartet: Alle grünen Checks

# 2. CMake Configure
.\scripts\fix-cmake-prefix-path.ps1 -Action build
# ✅ Erwartet: "Configuring done", "Generating done", keine Fehler

# 3. Build durchführen
cmake --build build-msvc --config Release --target themis_server --parallel 8
# ✅ Erwartet: "Build succeeded", themis_server.exe existiert

# 4. Überprüfe Executable
Test-Path "build-msvc\Release\themis_server.exe"
# ✅ Erwartet: True

# 5. Überprüfe Abhängigkeiten geladen
Get-ChildItem "build-msvc\Release\*.lib" | Where-Object {$_.Name -match "faiss|grpc"}
# ✅ Erwartet: Libs sind vorhanden
```

---

## Zusammenfassung Expected Outputs

| Punkt | Erfolg ✅ | Fehler ❌ |
|-------|-----------|-----------|
| **Diagnose** | Alle grüne Checks | Rote ❌ Marks |
| **CMake Output** | "Found faiss", "Found gRPC" | "Could NOT find" |
| **CMAKE_PREFIX_PATH** | "v1.3.5: CMAKE_PREFIX_PATH set to ..." | Zeile nicht vorhanden |
| **faiss_DIR** | "v1.3.5: faiss found at ..." | Nicht in Output |
| **gRPC_DIR** | "v1.3.5: gRPC found at ..." | Nicht in Output |
| **Protobuf** | "Found Protobuf" | "Could NOT find Protobuf" |
| **CMake Cache** | faiss_DIR und gRPC_DIR Einträge | Einträge fehlen |
| **Build** | "Build succeeded" | Link-Fehler |
| **Executable** | themis_server.exe existiert | Datei nicht vorhanden |

---

**Datum**: 2025-12-26  
**Ziel**: Schnelle Verifikation dass alles funktioniert  
**Nutzung**: Vergleiche deine Ausgabe mit diesen erwarteten Outputs
