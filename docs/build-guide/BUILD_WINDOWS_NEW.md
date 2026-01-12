# Windows Build Guide (MSVC) - v1.4.0+

**Letzte Aktualisierung**: v1.4.0 mit modularer CMake-Architektur  
**Kompatibilität**: Windows 10+ (Build 2004+), Visual Studio 2022

## Schnellstart (5 Minuten)

```powershell
cd C:\VCC\themis

# Standard Community Edition
cmake --preset windows-vs2022-release
cmake --build --preset windows-vs2022-release --parallel 8

# Binary: build-msvc\Release\themis_server.exe
```

## Voraussetzungen

### Windows 10+ (Build 2004+)

### Visual Studio 2022
```powershell
# Erforderliche Komponenten:
# - C++ Development Tools
# - MSVC v19.44+
# - Windows SDK 10.0.19041+

cl /v  # Verifikation: sollte v19.44+ zeigen
```

### CMake 3.20+
```powershell
cmake --version  # ✅ mindestens 3.20.x
```

### vcpkg
```powershell
cd C:\VCC\themis\vcpkg
.\bootstrap-vcpkg.bat  # Erste Initialisierung (~15 GB)
```

### Python 3.11+ (optional)
Nur für Dokumentation nötig.

## Architektur (NEU in v1.4.0)

Modulare CMake mit Feature Isolation:

```
CMakeLists.txt (Root - 150 Zeilen)
  ├─ cmake/Versions.cmake (Versionsverwaltung)
  ├─ cmake/CompilerOptions.cmake (C++20 + Compiler Flags)
  ├─ cmake/Dependencies.cmake (Externe Abhängigkeiten)
  └─ cmake/CMakeLists.txt (Hauptlogik)
      ├─ cmake/Features/LLM.cmake (~20 LLM-Dateien)
      ├─ cmake/Features/GPU.cmake (CUDA/HIP/FAISS)
      ├─ cmake/Features/gRPC.cmake (Verteilte Sharding)
      ├─ cmake/Features/Protocols.cmake (HTTP/2, HTTP/3, WebSocket, etc.)
      └─ cmake/Features/Tracing.cmake (OpenTelemetry)
```

**Vorteil**: Nur notwendige Komponenten werden kompiliert!

### Editionsystem

| Edition | Zweck | Standardflags |
|---------|---|---|
| **MINIMAL** | IoT/Embedded/ARM | LLM=OFF, gRPC=OFF, GPU=OFF |
| **COMMUNITY** (Standard) | LLM=ON, GPU=ON, Open Source | Alle Features optional |
| **ENTERPRISE** | Data Center | LLM=ON, GPU=ON, gRPC=ON, Tracing=ON |
| **HYPERSCALER** | Cloud/LLM-Heavy | LLM=ON, GPU=ON, gRPC=ON, Tracing=ON |

## Build-Methoden

### ✅ Methode 1: CMake Presets (EMPFOHLEN)

```powershell
cd C:\VCC\themis

# Alle verfügbaren Presets
cmake --list-presets

# Mit Preset bauen
cmake --preset windows-vs2022-release
cmake --build --preset windows-vs2022-release --parallel 8
```

**Verfügbare Presets**:
- `windows-vs2022-release` - Release Build
- `windows-vs2022-debug` - Debug mit Sanitizer
- `windows-ninja-msvc-release` - Ninja + MSVC
- `windows-ninja-clangcl-release` - Ninja + ClangCL

### ✅ Methode 2: Manuelle CMake (für Feature-Flags)

```powershell
# Hyperscaler Edition (LLM + GPU + Tracing)
cmake -S . -B build-msvc `
  -G "Visual Studio 17 2022" -A x64 `
  -DTHEMIS_EDITION=HYPERSCALER `
  -DTHEMIS_ENABLE_LLM=ON `
  -DTHEMIS_ENABLE_GPU=ON `
  -DTHEMIS_ENABLE_CUDA=ON `
  -DTHEMIS_ENABLE_TRACING=ON `
  -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake"

cmake --build build-msvc --config Release --parallel 8
```

### ✅ Methode 3: PowerShell Script

```powershell
cd C:\VCC\themis
.\scripts\run-windows-devbuild.ps1
```

## Build-Varianten

### Community Edition (Standard)
```powershell
cmake --preset windows-vs2022-release
cmake --build --preset windows-vs2022-release --parallel 8
```

### Minimal Edition (für IoT/ARM)
```powershell
cmake -S . -B build-minimal `
  -DTHEMIS_EDITION=MINIMAL `
  -DTHEMIS_BUILD_BENCHMARKS=OFF `
  -DTHEMIS_BUILD_TESTS=OFF

cmake --build build-minimal --config Release --parallel 8
# Größe: ~50 MB (statt 200 MB)
```

### Enterprise Edition (mit gRPC & Tracing)
```powershell
cmake -S . -B build-enterprise `
  -DTHEMIS_EDITION=ENTERPRISE `
  -DTHEMIS_ENABLE_GRPC=ON `
  -DTHEMIS_ENABLE_TRACING=ON

cmake --build build-enterprise --config Release --parallel 8
```

### Hyperscaler Edition (Maximum)
```powershell
cmake -S . -B build-hyperscaler `
  -DTHEMIS_EDITION=HYPERSCALER `
  -DTHEMIS_ENABLE_LLM=ON `
  -DTHEMIS_ENABLE_GPU=ON `
  -DTHEMIS_ENABLE_CUDA=ON `
  -DTHEMIS_ENABLE_GRPC=ON `
  -DTHEMIS_ENABLE_TRACING=ON `
  -DTHEMIS_ENABLE_HTTP2=ON

cmake --build build-hyperscaler --config Release --parallel 8
```

### Debug Build (mit AddressSanitizer)
```powershell
cmake --preset windows-vs2022-debug
cmake --build --preset windows-vs2022-debug
```

## Feature Flags (v1.4.0+)

### Core Features

| Flag | Beschreibung | Größeneffekt |
|------|---|---|
| `THEMIS_ENABLE_GRPC` | Inter-Shard Communication | +50 MB |
| `THEMIS_ENABLE_LLM` | LLM-Integrationsmodul | +150 MB |
| `THEMIS_ENABLE_GPU` | GPU-Beschleunigung | +200 MB |
| `THEMIS_ENABLE_CUDA` | NVIDIA CUDA Kernels | +100 MB |
| `THEMIS_ENABLE_TRACING` | OpenTelemetry | +30 MB |

### Protocol Flags

```powershell
-DTHEMIS_ENABLE_HTTP2=ON          # HTTP/2 (nghttp2)
-DTHEMIS_ENABLE_HTTP3=ON          # HTTP/3 (experimental)
-DTHEMIS_ENABLE_WEBSOCKET=ON      # WebSocket Support
-DTHEMIS_ENABLE_MQTT=ON           # MQTT Protocol
-DTHEMIS_ENABLE_POSTGRES_WIRE=ON  # PostgreSQL Wire Compatibility
-DTHEMIS_ENABLE_MCP=ON            # Model Context Protocol
-DTHEMIS_ENABLE_SSE=ON            # Server-Sent Events
```

### Optimization Flags

```powershell
-DTHEMIS_ENABLE_DISKANN=ON        # DiskANN GPU-Indexing
-DTHEMIS_ENABLE_WISCKEY=ON        # WiscKey Log Separation
-DTHEMIS_ENABLE_ARM_SIMD=ON       # ARM SIMD Support (AArch64)
-DTHEMIS_ENABLE_QNAP_ARM=ON       # QNAP ARM Baseline
-DTHEMIS_ENABLE_MIMALLOC=ON       # Memory Optimization
```

### Build Flags

```powershell
-DTHEMIS_BUILD_TESTS=ON           # Kompiliere ~180 Unit Tests
-DTHEMIS_BUILD_BENCHMARKS=ON      # Kompiliere ~72 Benchmarks
-DTHEMIS_STRICT_BUILD=ON          # Compiler Warnings als Fehler
-DTHEMIS_ENABLE_ASAN=ON           # Address Sanitizer (Debug)
```

## Output Locations

| Artifact | Pfad |
|----------|------|
| **Server Binary** | `build-msvc\Release\themis_server.exe` |
| **Test Binary** | `build-msvc\Release\themis_tests.exe` |
| **Benchmarks** | `build-msvc\Release\bench_*.exe` (72 Benchmarks) |
| **Static Libraries** | `build-msvc\Release\*.lib` |
| **Compile Database** | `build-msvc\compile_commands.json` |

## Tests ausführen

```powershell
cd C:\VCC\themis\build-msvc

# Alle Tests
ctest -C Release -j 8 --output-on-failure

# Unit Tests nur
ctest -C Release -R "^test_" -j 4

# Einzelner Test
ctest -C Release -R "ThreadSafety" -VV
```

## Benchmarks ausführen

```powershell
cd C:\VCC\themis\build-msvc

# Core Performance Benchmark
.\Release\bench_core_performance.exe

# Mit speziellen Filtern
.\Release\bench_comprehensive.exe --benchmark_filter=Vector --benchmark_time_unit=ms
```

## Packages erstellen (CPack)

```powershell
cd C:\VCC\themis\build-msvc

# ZIP Archive
cpack -G ZIP -C Release

# Windows Installer (.msi mit WiX Toolset)
cpack -G WIX -C Release
```

## Troubleshooting

### ❌ "CMAKE_CXX_COMPILER not set"

**Ursache**: Visual Studio nicht richtig installiert oder PATH nicht gesetzt

**Lösung**:
```powershell
# Option 1: VS2022 Developer Shell benutzen
"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\Launch-VsDevShell.ps1"

# Option 2: VCVARS ausführen
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"

# Option 3: CMakePresets benutzen (automatisch)
cmake --preset windows-vs2022-release
```

### ❌ "Could not find package OpenSSL" etc.

**Ursache**: vcpkg Cache beschädigt oder nicht initialisiert

**Lösung**:
```powershell
cd C:\VCC\themis\vcpkg
Remove-Item * -Recurse -Force  # Cache löschen
.\bootstrap-vcpkg.bat

# CMake mit --fresh neu konfigurieren
cmake --preset windows-vs2022-release --fresh
```

### ❌ "Access Denied" beim Build

**Ursache**: Alte Compiler-Prozesse noch aktiv

**Lösung**:
```powershell
taskkill /f /im cl.exe
taskkill /f /im link.exe

# Build neu starten
cmake --build build-msvc --config Release
```

### ❌ "Out of Disk Space"

**Ursache**: vcpkg benötigt ~15 GB

**Lösung**:
```powershell
# Build-Verzeichnis löschen
Remove-Item -Recurse build-msvc -Force

# Nur Minimal Edition bauen
cmake -S . -B build-minimal -DTHEMIS_EDITION=MINIMAL

# Oder: Auf andere Laufwerk ausweichen
cmake -S . -B D:\build-msvc
```

### ❌ LLM Feature aktiviert, aber llama.cpp nicht gefunden

**Lösung**:
```powershell
# llama.cpp clonen
git clone https://github.com/ggerganov/llama.cpp.git C:\VCC\themis\llama.cpp

# CMake erneut konfigurieren
cmake --preset windows-vs2022-release --fresh
```

### ❌ GPU (CUDA) aktiviert, aber CUDA Toolkit nicht erkannt

**Lösung**:
```powershell
# CUDA 12.4+ von nvidia.com herunterladen + installieren

# Oder: Manueller CUDA Pfad
$env:CUDA_TOOLKIT_ROOT_DIR = "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4"
cmake -S . -B build-msvc -DTHEMIS_ENABLE_CUDA=ON

# Verifikation
"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4\bin\nvcc" --version
```

## Performance-Tipps

### Parallel Build
```powershell
# CPU-Cores / 2
cmake --build build-msvc --config Release --parallel 8
```

### Ninja Statt Visual Studio Generator
```powershell
# Schneller als Visual Studio Generator
cmake --preset windows-ninja-msvc-release
cmake --build --preset windows-ninja-msvc-release --parallel 16
```

### Link-Time Optimization (LTO)
```powershell
cmake -S . -B build-lto `
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON

# +10-15% Runtime Performance, +30% Build Time
```

### Incremental Build
Nach Code-Änderungen:
```powershell
cmake --build build-msvc --config Release
```

## WSL2 Integration (Schnellere Builds)

```powershell
# Aus Windows PowerShell:
wsl bash -c "cd /mnt/c/VCC/themis && cmake --preset linux-gcc-release && cmake --build build-wsl"
```

## Weiterführende Ressourcen

- 📚 [CMake Modular Architecture](../architecture/CMAKE_MODULAR_ARCHITECTURE.md)
- 🔧 [Feature Flags Reference](../architecture/FEATURE_FLAGS_REFERENCE.md)
- 🐧 [Linux Build Guide](BUILD_LINUX.md)
- 🐳 [Docker Build Guide](BUILD_DOCKER.md)
- 🧪 [Testing Guide](TESTING.md)
- 📦 [Packaging Guide](PACKAGING.md)

## Migration v1.3.x → v1.4.0

**Gute Nachrichten**: Build-Befehle sind identisch!

```powershell
# Funktioniert in v1.3.x und v1.4.0+
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 8
```

Neue Features (Edition, Feature Flags) sind optional.

## Feedback & Support

- 🐛 **Bugs**: https://github.com/YourOrg/themis/issues
- 💬 **Diskussionen**: https://github.com/YourOrg/themis/discussions
- 📖 **Vollständige Docs**: https://themisdb.readthedocs.io/
