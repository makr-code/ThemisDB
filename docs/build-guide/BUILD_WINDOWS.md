# Windows Build Guide - v1.4.0+

**Letzte Aktualisierung**: v1.4.0 mit modularer CMake-Architektur  
**Kompatibilität**: Windows 10+ (Build 2004+)  
**Empfohlene Build-Toolchain**: Ninja + MSVC/Clang

---

## 🚀 Schnellstart (3 Minuten)

### Methode 1: Ninja + MSVC (EMPFOHLEN - Schnellster Build)

```powershell
cd C:\VCC\themis

# Automatisches Build-Skript mit vcpkg Binary-Cache
.\scripts\quick-build.ps1 -Jobs 8

# Binary: build-ninja\themis_server.exe
# Build-Zeit: ~5 Minuten (erste Build), ~1 Minute (incremental)
```

### Methode 2: Ninja + Clang/LLVM (Bessere Diagnostik)

```powershell
cd C:\VCC\themis

# Clang/LLVM Build mit optimierter Konfiguration
.\scripts\build-clang.ps1 -Jobs 8

# Binary: build-clang\themis_server.exe
# Vorteil: Bessere Fehlermeldungen, striktere Compiler-Checks
```

### Methode 3: Visual Studio (Fallback für GUI-Debugging)

```powershell
cd C:\VCC\themis

# Nur wenn Visual Studio IDE benötigt wird
cmake --preset windows-vs2022-release
cmake --build --preset windows-vs2022-release --parallel 8

# Binary: build-msvc\Release\themis_server.exe
# Öffnen: build-msvc\themis.sln
```

---

## 📋 Voraussetzungen

### 1. Build-Tools (Wähle eine Option)

#### Option A: Ninja + MSVC (EMPFOHLEN)
```powershell
# Ninja installieren
winget install Ninja-build.Ninja
# oder: choco install ninja

# Visual Studio 2022 Build Tools (ohne IDE)
winget install Microsoft.VisualStudio.2022.BuildTools --silent --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools;includeRecommended"

# Verifikation
ninja --version    # ✅ 1.11.0+
cl /v              # ✅ MSVC v19.44+
```

#### Option B: Clang/LLVM
```powershell
# Clang/LLVM 18+ installieren
winget install LLVM.LLVM

# Verifikation
clang --version    # ✅ 18.0.0+
```

#### Option C: Visual Studio 2022 (Full IDE)
```powershell
# Erforderliche Komponenten:
# - C++ Development Tools
# - MSVC v19.44+
# - Windows SDK 10.0.19041+
# - Clang/LLVM (optional, aber empfohlen)

# Verifikation
cl /v              # ✅ MSVC v19.44+
```

### 2. CMake 3.20+
```powershell
winget install Kitware.CMake
cmake --version    # ✅ mindestens 3.20.x
```

### 3. vcpkg mit Binary Cache (KRITISCH für schnelle Builds)

```powershell
cd C:\VCC\themis\vcpkg

# Einmalige Initialisierung
.\bootstrap-vcpkg.bat

# Binary-Cache aktivieren (REDUZIERT BUILD-ZEIT UM 80%!)
$env:VCPKG_BINARY_SOURCES = "clear;files,C:\VCC\vcpkg-cache,readwrite"

# Permanente Konfiguration
[System.Environment]::SetEnvironmentVariable("VCPKG_BINARY_SOURCES", "clear;files,C:\VCC\vcpkg-cache,readwrite", [System.EnvironmentVariableTarget]::User)

# Cache-Verzeichnis erstellen
New-Item -ItemType Directory -Path "C:\VCC\vcpkg-cache" -Force

# Erste Build: ~30 Minuten (94 Dependencies)
# Mit Cache: ~2 Minuten (nur Linking)
```

**Tipp**: Teile `C:\VCC\vcpkg-cache` im Netzwerk für Team-Builds!

### 4. Python 3.11+ (Optional)
Nur für Dokumentationsgenerierung benötigt.

```powershell
winget install Python.Python.3.11
python --version   # ✅ 3.11+
```

---

## 🏗️ Build-Methoden (Nach Priorität)

### ⚡ Methode 1: Ninja + MSVC (PRIMÄR - Schnellster Build)

**Vorteile:**
- ✅ 3-5x schneller als Visual Studio Generator
- ✅ Parallele Kompilierung optimal genutzt
- ✅ Inkrementelle Builds in ~30 Sekunden
- ✅ Funktioniert mit MSVC und Clang

```powershell
cd C:\VCC\themis

# Automatisches Setup + Build (EMPFOHLEN)
.\scripts\quick-build.ps1 -Jobs 8

# Oder: Manuelle Konfiguration
cmake -S . -B build-ninja `
  -G Ninja `
  -DCMAKE_C_COMPILER=cl `
  -DCMAKE_CXX_COMPILER=cl `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake"

# Build
cmake --build build-ninja --parallel 8

# Binary: build-ninja\themis_server.exe
```

**Build-Zeit:**
- Erste Build mit vcpkg-Cache: ~5-7 Minuten
- Inkrementelle Builds: ~30-60 Sekunden
- Mit Binary-Cache: ~2 Minuten (erste Build)

---

### 🔧 Methode 2: Ninja + Clang/LLVM (SEKUNDÄR - Bessere Diagnostik)

**Vorteile:**
- ✅ Bessere Compiler-Fehlermeldungen
- ✅ Striktere C++20 Konformität
- ✅ Schnellere Template-Instantiierung
- ✅ Cross-Platform Kompatibilität (Linux-ähnliche Warnings)

```powershell
cd C:\VCC\themis

# Automatisches Clang-Build
.\scripts\build-clang.ps1 -Clean -Configure -Jobs 8

# Oder: Manuelle Konfiguration
cmake -S . -B build-clang `
  -G Ninja `
  -DCMAKE_C_COMPILER=clang `
  -DCMAKE_CXX_COMPILER=clang++ `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake"

# Build
cmake --build build-clang --parallel 8

# Binary: build-clang\themis_server.exe
```

**Wann nutzen?**
- Bei Cross-Compile-Testing (Linux-Kompatibilität prüfen)
- Für bessere Debugging-Output bei Template-Errors
- Wenn MSVC ungenaue Fehlermeldungen gibt

---

### 🖥️ Methode 3: Visual Studio Generator (FALLBACK - GUI-Debugging)

**Vorteile:**
- ✅ Visual Studio IDE Integration
- ✅ GUI-Debugger mit allen Features
- ✅ IntelliSense und Code-Navigation

**Nachteile:**
- ❌ 3-5x langsamer als Ninja
- ❌ Inkrementelle Builds langsamer

```powershell
cd C:\VCC\themis

# CMake Preset
cmake --preset windows-vs2022-release
cmake --build --preset windows-vs2022-release --parallel 8

# Oder: Manuelle Konfiguration
cmake -S . -B build-msvc `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake"

# Build
cmake --build build-msvc --config Release --parallel 8

# Visual Studio öffnen
start build-msvc\themis.sln

# Binary: build-msvc\Release\themis_server.exe
```

**Wann nutzen?**
- Nur wenn Visual Studio IDE benötigt wird
- Für professionelles Debugging mit GUI
- Wenn MSBuild-Integration erforderlich ist

---

## ⚙️ vcpkg Binary-Cache Optimierung

### Setup (Einmalig, KRITISCH für Team-Builds)

```powershell
# Lokaler Binary-Cache (SSD empfohlen)
$env:VCPKG_BINARY_SOURCES = "clear;files,C:\VCC\vcpkg-cache,readwrite"

# Permanent setzen
[System.Environment]::SetEnvironmentVariable("VCPKG_BINARY_SOURCES", "clear;files,C:\VCC\vcpkg-cache,readwrite", [System.EnvironmentVariableTarget]::User)

# Cache-Verzeichnis erstellen
New-Item -ItemType Directory -Path "C:\VCC\vcpkg-cache" -Force
```

### Netzwerk-Cache (für Teams)

```powershell
# SMB Share
$env:VCPKG_BINARY_SOURCES = "clear;files,\\\\build-server\\vcpkg-cache,readwrite"

# Azure Blob (Enterprise)
$env:VCPKG_BINARY_SOURCES = "clear;x-azblob,https://themis.blob.core.windows.net/vcpkg-cache,read"

# NuGet Feed (CI/CD)
$env:VCPKG_BINARY_SOURCES = "clear;nuget,https://nuget.company.com/vcpkg,readwrite"
```

### Build-Zeit Vergleich

| Szenario | Ohne Cache | Mit lokalem Cache | Mit Netzwerk-Cache |
|----------|-----------|------------------|-------------------|
| **Erste Build** | 30-45 Min | 5-7 Min | 10-12 Min |
| **Clean Rebuild** | 30-45 Min | 2-3 Min | 4-5 Min |
| **Incremental** | 1-2 Min | 30-60 Sek | 30-60 Sek |

**Tipp**: Cache auf schneller SSD speichern (C:\VCC\vcpkg-cache), nicht auf Netzlaufwerk!

### Cache-Management

```powershell
# Cache-Größe prüfen
Get-ChildItem "C:\VCC\vcpkg-cache" -Recurse | Measure-Object -Property Length -Sum

# Cache leeren (bei Problemen)
Remove-Item "C:\VCC\vcpkg-cache\*" -Recurse -Force

# Alte Cache-Einträge löschen (älter als 30 Tage)
Get-ChildItem "C:\VCC\vcpkg-cache" -Recurse | Where-Object {$_.LastWriteTime -lt (Get-Date).AddDays(-30)} | Remove-Item -Force
```

---

## 📊 Architektur & Editionen

### Modulare CMake (NEU in v1.4.0)

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
| **COMMUNITY** (Standard) | Vollständig, Open Source | Alle Features optional |
| **ENTERPRISE** | Data Center | gRPC=ON, Tracing=ON |
| **HYPERSCALER** | Cloud/LLM-Heavy | LLM=ON, GPU=ON, gRPC=ON, Tracing=ON |

---

## 🎯 Build-Varianten

### Community Edition (Standard - EMPFOHLEN)
```powershell
# Ninja + MSVC (schnellster Build)
.\scripts\quick-build.ps1 -Jobs 8

# Binary: build-ninja\themis_server.exe
# Größe: ~120 MB
# Features: Alle Core-Features
```

### Minimal Edition (IoT/Embedded)
```powershell
cmake -S . -B build-minimal `
  -G Ninja `
  -DTHEMIS_EDITION=MINIMAL `
  -DTHEMIS_BUILD_BENCHMARKS=OFF `
  -DTHEMIS_BUILD_TESTS=OFF `
  -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake"

cmake --build build-minimal --parallel 8

# Binary: build-minimal\themis_server.exe
# Größe: ~50 MB
```

### Enterprise Edition (mit gRPC & Tracing)
```powershell
cmake -S . -B build-enterprise `
  -G Ninja `
  -DTHEMIS_EDITION=ENTERPRISE `
  -DTHEMIS_ENABLE_GRPC=ON `
  -DTHEMIS_ENABLE_TRACING=ON `
  -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake"

cmake --build build-enterprise --parallel 8

# Binary: build-enterprise\themis_server.exe
# Größe: ~180 MB
```

### Hyperscaler Edition (Maximum)
```powershell
cmake -S . -B build-hyperscaler `
  -G Ninja `
  -DTHEMIS_EDITION=HYPERSCALER `
  -DTHEMIS_ENABLE_LLM=ON `
  -DTHEMIS_ENABLE_GPU=ON `
  -DTHEMIS_ENABLE_CUDA=ON `
  -DTHEMIS_ENABLE_GRPC=ON `
  -DTHEMIS_ENABLE_TRACING=ON `
  -DTHEMIS_ENABLE_HTTP2=ON `
  -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake"

cmake --build build-hyperscaler --parallel 8

# Binary: build-hyperscaler\themis_server.exe
# Größe: ~350 MB
# CUDA erforderlich: https://developer.nvidia.com/cuda-downloads
```

### Debug Build (mit AddressSanitizer)
```powershell
cmake -S . -B build-debug `
  -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DTHEMIS_ENABLE_ASAN=ON `
  -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake"

cmake --build build-debug

# Binary: build-debug\themis_server.exe
```

---

## 🎛️ Feature Flags

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

---

## 📦 Output Locations

| Artifact | Pfad |
|----------|------|
| **Server Binary (Ninja)** | `build-ninja\themis_server.exe` |
| **Server Binary (Clang)** | `build-clang\themis_server.exe` |
| **Server Binary (MSVC)** | `build-msvc\Release\themis_server.exe` |
| **Test Binary** | `build-*/themis_tests.exe` |
| **Benchmarks** | `build-*/bench_*.exe` (72 Benchmarks) |
| **Static Libraries** | `build-*/*.lib` |
| **Compile Database** | `build-*/compile_commands.json` |

---

## 🧪 Tests ausführen

```powershell
cd C:\VCC\themis\build-ninja

# Alle Tests
ctest -j 8 --output-on-failure

# Unit Tests nur
ctest -R "^test_" -j 4

# Einzelner Test
ctest -R "ThreadSafety" -VV
```

---

## 📈 Benchmarks ausführen

```powershell
cd C:\VCC\themis\build-ninja

# Core Performance Benchmark
.\bench_core_performance.exe

# Mit speziellen Filtern
.\bench_comprehensive.exe --benchmark_filter=Vector --benchmark_time_unit=ms
```

---

## 🔧 Troubleshooting

### ❌ "CMAKE_CXX_COMPILER not set"

**Ursache**: Visual Studio nicht richtig installiert oder PATH nicht gesetzt

**Lösung**:
```powershell
# Option 1: VS2022 Developer Shell benutzen
"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\Launch-VsDevShell.ps1"

# Option 2: VCVARS ausführen
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"

# Option 3: Ninja Build benutzen (automatisch)
.\scripts\quick-build.ps1 -Jobs 8
```

### ❌ "Could not find package OpenSSL" etc.

**Ursache**: vcpkg Cache beschädigt oder nicht initialisiert

**Lösung**:
```powershell
# vcpkg neu bootstrappen
cd C:\VCC\themis\vcpkg
.\bootstrap-vcpkg.bat

# CMake mit --fresh neu konfigurieren
cmake -S . -B build-ninja -G Ninja --fresh
```

### ❌ "Access Denied" beim Build

**Ursache**: Alte Compiler-Prozesse noch aktiv

**Lösung**:
```powershell
taskkill /f /im cl.exe
taskkill /f /im clang.exe
taskkill /f /im link.exe

# Build neu starten
cmake --build build-ninja
```

### ❌ "Out of Disk Space"

**Ursache**: vcpkg benötigt ~15 GB, Build-Verzeichnis ~5 GB

**Lösung**:
```powershell
# Build-Verzeichnis löschen
Remove-Item -Recurse build-ninja -Force

# Nur Minimal Edition bauen
cmake -S . -B build-minimal -G Ninja -DTHEMIS_EDITION=MINIMAL

# Oder: Auf andere Laufwerk ausweichen
cmake -S . -B D:\build-ninja -G Ninja
```

### ❌ LLM Feature aktiviert, aber llama.cpp nicht gefunden

**Lösung**:
```powershell
# llama.cpp clonen
git clone https://github.com/ggerganov/llama.cpp.git C:\VCC\themis\llama.cpp

# CMake erneut konfigurieren
cmake -S . -B build-ninja -G Ninja --fresh
```

### ❌ GPU (CUDA) aktiviert, aber CUDA Toolkit nicht erkannt

**Lösung**:
```powershell
# CUDA 12.4+ von nvidia.com herunterladen + installieren

# Oder: Manueller CUDA Pfad
$env:CUDA_TOOLKIT_ROOT_DIR = "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4"
cmake -S . -B build-ninja -G Ninja -DTHEMIS_ENABLE_CUDA=ON

# Verifikation
"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4\bin\nvcc" --version
```

### ❌ Ninja Build langsam trotz vcpkg-Cache

**Ursache**: Binary-Cache nicht aktiviert oder auf Netzlaufwerk

**Lösung**:
```powershell
# Cache auf lokale SSD setzen
$env:VCPKG_BINARY_SOURCES = "clear;files,C:\VCC\vcpkg-cache,readwrite"

# Permanent setzen
[System.Environment]::SetEnvironmentVariable("VCPKG_BINARY_SOURCES", "clear;files,C:\VCC\vcpkg-cache,readwrite", [System.EnvironmentVariableTarget]::User)

# Build neu starten
Remove-Item -Recurse build-ninja -Force
.\scripts\quick-build.ps1 -Jobs 8
```

---

## 🚀 Performance-Tipps

### 1. Parallele Builds optimal nutzen

```powershell
# CPU-Cores - 2 (für System-Reservierung)
$cores = (Get-CimInstance Win32_Processor).NumberOfLogicalProcessors
$jobs = $cores - 2

cmake --build build-ninja --parallel $jobs
```

### 2. Inkrementelle Builds beschleunigen

```powershell
# Nur geänderte Dateien kompilieren
cmake --build build-ninja

# Mit Ninja-Status-Ausgabe
cmake --build build-ninja -- -v
```

### 3. Link-Time Optimization (LTO)

```powershell
cmake -S . -B build-lto `
  -G Ninja `
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON `
  -DCMAKE_BUILD_TYPE=Release

# +10-15% Runtime Performance, +30% Build Time
```

### 4. ccache für Compilation-Cache (Optional)

```powershell
# ccache installieren
winget install ccache

# CMake konfigurieren
cmake -S . -B build-ninja -G Ninja -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

# Build-Zeit Reduktion: ~50% bei Clean Rebuild
```

### 5. Precompiled Headers nutzen (MSVC)

```powershell
# Bereits aktiviert in CMake
# Prüfen: build-ninja\CMakeCache.txt | Select-String "CMAKE_CXX_PRECOMPILE"
```

---

## 🌐 WSL2 Integration (Linux-Builds unter Windows)

```powershell
# Aus Windows PowerShell:
wsl bash -c "cd /mnt/c/VCC/themis && cmake -S . -B build-wsl -G Ninja && cmake --build build-wsl -j8"

# Binary: C:\VCC\themis\build-wsl\themis_server (Linux ELF)
```

---

## 📚 Weiterführende Ressourcen

- 📖 [Cross-Compile Requirements](../../CROSS_COMPILE_REQUIREMENTS.md) - **NEU**: Platform-agnostischer Code
- 📚 [CMake Modular Architecture](../architecture/CMAKE_MODULAR_ARCHITECTURE.md)
- 🔧 [Feature Flags Reference](../architecture/FEATURE_FLAGS_REFERENCE.md)
- 🐧 [Linux Build Guide](BUILD_LINUX.md)
- 🐳 [Docker Build Guide](BUILD_DOCKER.md)
- 🧪 [Testing Guide](TESTING.md)
- 📦 [Packaging Guide](PACKAGING.md)

---

## 🔄 Migration v1.3.x → v1.4.0

**Gute Nachrichten**: Build-Befehle sind kompatibel!

```powershell
# Funktioniert in v1.3.x und v1.4.0+
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 8
```

**NEU in v1.4.0:**
- Ninja als primärer Build-Generator (3-5x schneller)
- vcpkg Binary-Cache Support
- Clang/LLVM als sekundärer Compiler
- Cross-Compile Validation (siehe CROSS_COMPILE_REQUIREMENTS.md)

---

## 💬 Feedback & Support

- 🐛 **Bugs**: https://github.com/YourOrg/themis/issues
- 💬 **Diskussionen**: https://github.com/YourOrg/themis/discussions
- 📖 **Vollständige Docs**: https://themisdb.readthedocs.io/

---

**Build-Priorität**: Ninja > Clang/LLVM > Visual Studio  
**Cache-Priorität**: vcpkg Binary-Cache > lokales Repository  
**Compiler-Priorität**: MSVC (Production), Clang (Development), Visual Studio (Debugging)
