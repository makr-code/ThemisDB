---
category: "🛠️ Developer/Technical"
version: "v1.3.0"
status: "✅"
date: "22.12.2025"
---

# 🛠️ RocksDB Windows Build Issues - Knowledge Base

Knowledge base for RocksDB-related build issues on Windows.

## 📋 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Common Issues](#-common-issues)
- [🚀 Solutions](#-solutions)
- [📖 Root Cause Analysis](#-root-cause-analysis)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

---

## 📋 Übersicht

Comprehensive knowledge base for RocksDB build issues on Windows platforms.

**Stand:** 6. April 2026  
**Version:** 1.3.0  
**Kategorie:** 🛠️ Developer/Technical

---

## ✨ Common Issues

---

## Problem: "unrecognized file format in rocksdb_wrapper.obj"

### Symptome

Bei Windows MSVC Release Builds mit `THEMIS_CORE_SHARED=ON` (DLL-Build) tritt folgender Fehler auf:

```
Auto build dll exports
unrecognized file format in 'C:/VCC/themis/build-msvc/themis_core.dir/Release/rocksdb_wrapper.obj, 0'
error MSB3073: Der Befehl ... cmake.exe -E __create_def ... exports.def ... objects.txt wurde mit dem Code 1 beendet.
```

### Root Cause Analysis

1. **vcpkg RocksDB-Installation**:
   - vcpkg installiert RocksDB mit zwei Varianten:
     - `rocksdb.lib` (1.2 GB) - Statische Bibliothek mit allen Object-Dateien
     - `rocksdb-shared.lib` (305 KB) + `rocksdb-shared.dll` (6.6 MB) - Dynamic Library

2. **CMake Target-Auswahl**:
   - Standard-Target ist `RocksDB::rocksdb` (statisch, STATIC IMPORTED)
   - Bei DLL-Builds (`THEMIS_CORE_SHARED=ON`) versucht CMake, DLL-Exporte aus allen Object-Dateien zu generieren

3. **Linker-Fehler**:
   - `rocksdb_wrapper.obj` wird als 11.25 MB Objektdatei generiert
   - Beim DLL-Export-Generierungsschritt (`cmake -E __create_def`) kann CMake die massive Objektdatei nicht korrekt verarbeiten
   - Fehler: "unrecognized file format"

4. **Warum tritt das Problem auf**:
   - Die statische `rocksdb.lib` ist zu groß für DLL-Symbol-Export-Mechanismus
   - MSVC DLL-Export-Tool (`__create_def`) kann die große Objektdatei nicht parsen
   - Problem tritt NUR bei `THEMIS_CORE_SHARED=ON` auf

### Betroffene Konfigurationen

- ✅ **Funktioniert**: `THEMIS_CORE_SHARED=OFF` (statisches Build)
- ❌ **Fehlschlag**: `THEMIS_CORE_SHARED=ON` mit statischem `RocksDB::rocksdb`
- ⚠️ **Ungetestet**: `THEMIS_CORE_SHARED=ON` mit `RocksDB::rocksdb-shared`

### Lösung: Automatische RocksDB-Target-Auswahl

#### 1. CMakeLists.txt Anpassung

```cmake
# Find required packages
find_package(RocksDB CONFIG)

# Select appropriate RocksDB target based on build type
# THEMIS_CORE_SHARED=ON requires rocksdb-shared (DLL) to avoid linker issues with huge static .lib
# THEMIS_CORE_SHARED=OFF can use the static rocksdb library
set(THEMIS_ROCKSDB_TARGET "RocksDB::rocksdb")
if(TARGET RocksDB::rocksdb-shared AND THEMIS_CORE_SHARED)
    # Use shared library for Windows DLL build (avoids "unrecognized file format" linker error)
    set(THEMIS_ROCKSDB_TARGET "RocksDB::rocksdb-shared")
    message(STATUS "RocksDB: Using shared library (rocksdb-shared.dll) for THEMIS_CORE_SHARED=ON")
elseif(TARGET RocksDB::rocksdb)
    message(STATUS "RocksDB: Using static library (rocksdb.lib)")
endif()

# Use ${THEMIS_ROCKSDB_TARGET} in target_link_libraries
target_link_libraries(themis_core
    PUBLIC
        ${THEMIS_ROCKSDB_TARGET}
        # ... andere Dependencies
)
```

#### 2. Build-Script Anpassung (Empfohlener Workaround)

Für produktive Windows Release Builds: **Statisches Build verwenden**

```powershell
# scripts/build-windows.ps1
cmake -S $rootDir -B $buildDir `
    -G "Visual Studio 17 2022" `
    -A x64 `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_TOOLCHAIN_FILE="$rootDir\vcpkg\scripts\buildsystems\vcpkg.cmake" `
    -DVCPKG_TARGET_TRIPLET=x64-windows `
    -DTHEMIS_BUILD_TESTS=OFF `
    -DTHEMIS_BUILD_BENCHMARKS=OFF `
    -DTHEMIS_ENABLE_TRACING=OFF `
    -DTHEMIS_CORE_SHARED=OFF  # Statisch für zuverlässigen Build
```

### Workarounds & Best Practices

#### Option A: Statisches Build (Empfohlen für Windows)

```bash
cmake -DTHEMIS_CORE_SHARED=OFF ...
```

**Vorteile**:
- Zuverlässig, kein Linker-Problem
- Keine DLL-Abhängigkeiten
- Einfachere Deployment (single executable)

**Nachteile**:
- Größere Binary (~1.3 GB themis_core.lib)
- Längere Link-Zeit

#### Option B: RocksDB-Shared verwenden (Experimentell)

```bash
cmake -DTHEMIS_CORE_SHARED=ON ...
# CMake wählt automatisch rocksdb-shared.dll
```

**Vorteile**:
- Kleinere Binaries
- Schnellere Link-Zeit

**Nachteile**:
- Erfordert rocksdb-shared.dll zur Laufzeit
- Komplexere Deployment-Struktur
- Nicht vollständig getestet

### Technische Details

#### vcpkg RocksDB Targets

**RocksDB::rocksdb** (Statisch):
```cmake
add_library(RocksDB::rocksdb STATIC IMPORTED)
set_target_properties(RocksDB::rocksdb PROPERTIES
  IMPORTED_LOCATION "${_IMPORT_PREFIX}/lib/rocksdb.lib"  # 1.2 GB
)
```

**RocksDB::rocksdb-shared** (Dynamisch):
```cmake
add_library(RocksDB::rocksdb-shared SHARED IMPORTED)
set_target_properties(RocksDB::rocksdb-shared PROPERTIES
  IMPORTED_IMPLIB "${_IMPORT_PREFIX}/lib/rocksdb-shared.lib"    # 305 KB
  IMPORTED_LOCATION "${_IMPORT_PREFIX}/bin/rocksdb-shared.dll"  # 6.6 MB
)
```

#### Objektdatei-Größen

```
rocksdb_wrapper.obj: 11.25 MB  (zu groß für DLL-Export-Parser)
themis_core.lib:     1.27 GB   (statisches Build)
themis_server.exe:   10.1 MB   (finales Executable)
```

### Debugging-Schritte

Falls das Problem wieder auftritt:

1. **Überprüfe CMake-Konfiguration**:
   ```bash
   cmake ... 2>&1 | Select-String "RocksDB"
   # Erwartete Ausgabe:
   # "RocksDB: Using static library (rocksdb.lib)"  # für THEMIS_CORE_SHARED=OFF
   # "RocksDB: Using shared library (rocksdb-shared.dll)"  # für THEMIS_CORE_SHARED=ON
   ```

2. **Überprüfe Object-Datei**:
   ```powershell
   Get-ChildItem "build-msvc\themis_core.dir\Release\rocksdb_wrapper.obj"
   # Größe sollte ~11 MB sein
   ```

3. **Überprüfe vcpkg-Installation**:
   ```powershell
   Get-ChildItem "vcpkg_installed\x64-windows\lib\rocksdb*"
   Get-ChildItem "vcpkg_installed\x64-windows\bin\rocksdb*"
   ```

4. **Test mit vollständigem Clean-Build**:
   ```powershell
   Remove-Item -Path "build-msvc" -Recurse -Force
   cmake -S . -B "build-msvc" -DTHEMIS_CORE_SHARED=OFF ...
   cmake --build "build-msvc" --config Release --parallel 4
   ```

### Verwandte Issues

- CMake DLL-Export mit großen Static Libraries
- MSVC `/bigobj` Flag (bereits aktiviert, hilft nicht)
- vcpkg RocksDB Packaging

### Referenzen

- RocksDB vcpkg Port: `rocksdb@10.4.2`
- CMake `__create_def` Dokumentation
- Microsoft MSVC Link-Time Code Generation (LTCG)

### Status

- **Identifiziert**: 14.12.2025
- **Gelöst**: Statisches Build als Standard für Windows
- **Alternative Lösung**: Automatische rocksdb-shared Auswahl (CMakeLists.txt Update)
- **Production Ready**: ✅ Ja (mit THEMIS_CORE_SHARED=OFF)

### Change History

| Datum | Änderung | Author |
|-------|----------|--------|
| 14.12.2025 | Initial Knowledge Base erstellt | System |
| 14.12.2025 | CMakeLists.txt Fix implementiert | System |
| 14.12.2025 | build-windows.ps1 auf statisch umgestellt | System |
