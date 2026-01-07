# Windows Build Guide (MSVC)

## Voraussetzungen

- **Visual Studio 2022** (Community, Pro, oder Enterprise)
  - `C++ workload` muss installiert sein
  - Mindestens `MSVC v143` oder neuer

- **CMake >= 3.20**
  ```powershell
  cmake --version  # Sollte mindestens 3.20.x sein
  ```

- **vcpkg** (im Root-Verzeichnis)
  ```powershell
  cd C:\VCC\themis\vcpkg
  .\bootstrap-vcpkg.bat
  ```

- **Python 3.11+** (für Dokumentation)

## Quick Start

### Methode 1: CMake Presets (Empfohlen)

```powershell
cd C:\VCC\themis

# Configure mit Release Preset
cmake --preset windows-vs2022-release

# Build
cmake --build --preset windows-vs2022-release --parallel 8

# Binary findet sich in:
# C:\VCC\themis\build-msvc\Release\themis_server.exe
```

### Methode 2: Manuelle CMake Commands

```powershell
cd C:\VCC\themis

# Configure
cmake -S . -B build-msvc `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_BUILD_TYPE=Release `
  -DTHEMIS_ENABLE_LLM=ON `
  -DTHEMIS_ENABLE_GPU=ON

# Build
cmake --build build-msvc --config Release --parallel 8
```

### Methode 3: PowerShell Script

```powershell
cd C:\VCC\themis
.\scripts\run-windows-devbuild.ps1
```

## Build-Varianten

### Standard Release (Community Edition)
```powershell
cmake --preset windows-vs2022-release
cmake --build --preset windows-vs2022-release
```

### Hyperscaler Edition (LLM + GPU)
```powershell
cmake -S . -B build-msvc `
  -G "Visual Studio 17 2022" -A x64 `
  -DTHEMIS_ENABLE_LLM=ON `
  -DTHEMIS_ENABLE_GPU=ON `
  -DTHEMIS_ENABLE_TRACING=ON `
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-msvc --config Release --parallel 8
```

### Debug Build (mit Sanitizer)
```powershell
cmake --preset windows-vs2022-debug
cmake --build --preset windows-vs2022-debug
```

## Output Locations

| Artifact | Pfad |
|----------|------|
| Server Executable | `build-msvc\Release\themis_server.exe` |
| Tests Executable | `build-msvc\Release\themis_tests.exe` |
| Libraries | `build-msvc\Release\*.lib` |
| Compile Commands | `build-msvc\compile_commands.json` |

## Test ausführen

```powershell
cd C:\VCC\themis\build-msvc

# Alle Tests
ctest -C Release -j 8 --output-on-failure

# Nur ein Test
ctest -C Release -R "ThreadSafety" -V
```

## Troubleshooting

### Problem: "CMAKE_CXX_COMPILER not set"
**Lösung**: Visual Studio 2022 richtig installieren oder CMakePresets.json benutzen

### Problem: vcpkg Module nicht gefunden
**Lösung**:
```powershell
$env:VCPKG_ROOT = "C:\VCC\themis\vcpkg"
cmake --preset windows-vs2022-release
```

### Problem: "Access Denied" beim Build
**Lösung**: cl.exe mögliche noch aktiv - vorher terminieren:
```powershell
taskkill /f /im cl.exe
# Dann neuen Build starten
```

### Problem: Out of Disk Space
**Lösung**: Build-Verzeichnis löschen und neu anfangen:
```powershell
Remove-Item -Recurse build-msvc, build-msvc-debug -Force
cmake --preset windows-vs2022-release
```

## Performance-Tipps

### Parallel Build aktivieren
```powershell
cmake --build build-msvc --config Release --parallel 8
```
> 8 = CPU-Cores. Anpassen nach Ihrer Hardware.

### Incremental Build
Nach Code-Änderungen:
```powershell
cmake --build build-msvc --config Release --parallel 8
```
CMake kompiliert nur veränderte Dateien.

### Link-Zeit Optimierung (LTO)
```powershell
cmake -S . -B build-msvc `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON `
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-msvc --config Release --parallel 8
```

## ClangCL Alternative

Falls Sie Clang statt MSVC lieber mögen:

```powershell
cmake --preset windows-ninja-clangcl-release
cmake --build build-clangcl-release --parallel 8
```

## Nächste Schritte

Nach erfolgreichem Build lesen Sie:
- **Deployment**: [docs/de/deployment/deployment_strategy.md](../../de/deployment/deployment_strategy.md)
- **Releases**: [docs/de/releases/updates_distribution_strategy.md](../../de/releases/updates_distribution_strategy.md)

## Weitere Infos

- [cmake/CMakePresets.json](../../cmake/CMakePresets.json) - Alle verfügbaren Presets
- [cmake/CMakeLists.txt](../../cmake/CMakeLists.txt) - Konfigurationsdetails
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Weitere Fehlersuche
