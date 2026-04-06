---
category: "🔨 Build/Deployment"
version: "v1.3.0"
status: "✅"
date: "22.12.2025"
---

# 🔨 ThemisDB Build, Packaging & Deployment Strategy

Comprehensive build strategy across all platforms and deployment scenarios.

## 📋 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features](#-features)
- [🚀 Build-Strategien](#-build-strategien)
- [📖 Versionierung & Packaging](#-versionierung--packaging)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

---

## 📋 Übersicht

Konsistente Build-Toolchain für alle Plattformen mit eindeutiger Versionierung und abgestimmtem Packaging/CI-CD.

**Stand:** 6. April 2026  
**Version:** 1.3.0  
**Kategorie:** 🔨 Build/Deployment

---

## ✨ Features

- 🏗️ **Multi-Plattform** - Windows, Linux, Docker, ARM64
- 📦 **Versionierung** - Semantic Versioning, Git Tags, Docker Tags
- 🔐 **LLM Integration** - llama.cpp, GPU/CUDA, Continuous Batching
- 🔌 **RPC Framework** - gRPC plugin, Inter-Shard Communication
- 🎯 **Modular Builds** - LLM, RPC, GPU können unabhängig aktiviert werden

---

## 🚀 Build-Strategien nach Plattform

**WICHTIG - NEU in Version 1.3.0 (v1.3.0):**
- ✅ **LLM Integration**: llama.cpp, GPU/CUDA, PagedAttention, Continuous Batching
- ✅ **RPC Framework**: gRPC plugin, Inter-Shard Communication, TLS/mTLS
- ✅ **New Dependencies**: gRPC, Protobuf, CUDA Toolkit (optional)
- ✅ GPU Acceleration mit 10 Backends (CUDA, Vulkan, HIP, DirectX, OpenCL, OneAPI, ZLUDA, Faiss)
- ✅ 7 Client SDKs Build-Integration (Python, JS, Rust, Go, Java, C#, Swift)
- ✅ Modular Build-Matrix: LLM, RPC, GPU können unabhängig aktiviert werden

---


## Ziel
Konsistente Build-Toolchain für alle Plattformen, eindeutige Versionierung und abgestimmtes Packaging/CI-CD.

## Build-Strategie nach Plattform

### Linking-Strategie

| Plattform | Linking | Begründung |
|-----------|---------|------------|
| **Docker/QNAP** | **Monolithisch (Statisch)** | Maximale Portabilität, keine GLIBC-Abhängigkeiten |
| **Windows** | **Dynamisch (DLL)** | Native Windows-Konventionen, kleinere Binary |
| **Linux (Native)** | Dynamisch oder Statisch | Je nach Deployment-Szenario |

### CMake-Flags

```bash
# Docker/QNAP: Monolithisch (statisch)
cmake -DTHEMIS_STATIC_BUILD=ON -DBUILD_SHARED_LIBS=OFF ...

# Windows: Dynamisch (DLL)
cmake -DTHEMIS_STATIC_BUILD=OFF -DBUILD_SHARED_LIBS=ON ...
```

## Unterstützte Plattformen

| Plattform | Architektur | Build-Methode | Linking | Binary-Kompatibilität |
|-----------|-------------|---------------|---------|----------------------|
| **Windows** | x64 | MSVC/ClangCL + vcpkg | **DLL** | Native .exe + .dll |
| **Docker (Standard)** | x64 | Ubuntu 24.04 | **Statisch** | Monolithische Binary |
| **Docker (QNAP)** | x64 | Ubuntu 20.04 | **Statisch** | Monolithische Binary |
| **Raspberry Pi** | ARM64 | GCC + vcpkg | **Statisch** | Monolithische Binary |
| **WSL/Linux** | x64 | GCC/Clang + vcpkg | Dynamisch/Statisch | Je nach Bedarf |

## Versionierungs-Strategie

### Semantic Versioning
Format: `MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]`

**Beispiele:**
- `0.1.0` - Initial Public Release
- `0.2.0-beta.1` - Beta-Release
- `1.0.0` - Stable Release
- `1.0.1+qnap` - QNAP-spezifischer Build

### Version-Quellen
1. **CMakeLists.txt** - Single Source of Truth
   ```cmake
   project(ThemisDB VERSION 0.1.0)
   ```

2. **Git Tags** - Release-Tagging
   ```bash
   git tag -a v0.1.0 -m "Release 0.1.0"
   ```

3. **Docker Tags**
   - `latest` - Neueste stabile Version
   - `0.1.0` - Spezifische Version
   - `0.1.0-qnap` - Plattform-spezifisch
   - `dev` - Development builds

## Konsolidierte Build-Struktur

### 1. Native Builds (CMakePresets.json)

#### Standard Builds
```bash
# Windows MSVC Release
cmake --preset windows-ninja-msvc-release
cmake --build --preset windows-ninja-msvc-release

# Linux/WSL Clang Release  
cmake --preset linux-ninja-clang-release
cmake --build --preset linux-ninja-clang-release

# Raspberry Pi ARM64
cmake --preset rpi-arm64-gcc-release
cmake --build --preset rpi-arm64-gcc-release
```

#### Enterprise Builds (mit zusätzlichen Features)

**Enterprise Features:**
- Token Bucket Rate Limiter (Priority-basiert)
- Per-Client Rate Limiter
- Adaptive Load Shedder (Multi-Metrik)
- HTTP Client Pool (Boost.Beast)

**Build-Befehle:**
```bash
# Windows Enterprise Build
cmake --preset windows-ninja-msvc-release
cmake --build --preset windows-ninja-msvc-release
# Enterprise Features sind automatisch aktiviert

# Linux Enterprise Build
cmake --preset linux-ninja-clang-release
cmake --build --preset linux-ninja-clang-release

# Tests ausführen (Enterprise Features)
./build-msvc-ninja-release/themis_tests --gtest_filter="*Enterprise*:TokenBucket*:PerClient*:LoadShed*"
```

**Enterprise Build Scripts:**
```powershell
# Windows
.\scripts\build_enterprise.cmd

# PowerShell Alternative
.\scripts\enable_enterprise_features.ps1

# Linux/WSL
./scripts/build_enterprise.sh
```

**Enterprise Dependencies (via vcpkg):**
- `boost-beast` - HTTP Client Pool
- `boost-asio` - Asynchrone Netzwerk-Operationen
- `openssl` - SSL/TLS für HTTPS
- Alle Standard-Dependencies

**CMake Optionen:**
```cmake
# Enterprise Features sind standardmäßig aktiviert
# Explizite Konfiguration in CMakeLists.txt:
# - THEMIS_ENABLE_RATE_LIMITING (immer ON)
# - THEMIS_ENABLE_LOAD_SHEDDING (immer ON)
# - THEMIS_ENABLE_HTTP_POOL (immer ON)
```

---

## Dependencies & Libraries (v1.3.0)

### Core Dependencies (immer erforderlich)

| Library | Version | Purpose | vcpkg Package |
|---------|---------|---------|---------------|
| **RocksDB** | Latest | Storage Engine | `rocksdb[lz4,zstd]` |
| **OpenSSL** | 3.x | TLS/Encryption | `openssl` |
| **simdjson** | Latest | Fast JSON parsing | `simdjson` |
| **TBB** | Latest | Parallel algorithms | `tbb` |
| **Apache Arrow** | Latest | Columnar data | `arrow[parquet,compute]` |
| **HNSWlib** | Latest | Vector search | `hnswlib` |
| **Boost.Asio** | 1.82+ | Async I/O | `boost-asio` |
| **Boost.Beast** | 1.82+ | HTTP server | `boost-beast` |
| **spdlog** | Latest | Logging | `spdlog` |
| **nlohmann-json** | Latest | JSON handling | `nlohmann-json` |
| **OpenTelemetry** | Latest | Observability | `opentelemetry-cpp[otlp-http]` |
| **cURL** | Latest | HTTP client | `curl` |
| **yaml-cpp** | Latest | YAML config | `yaml-cpp` |
| **zstd** | Latest | Compression | `zstd` |
| **mimalloc** | Latest | Fast allocator | `mimalloc` |
| **Google Test** | Latest | Unit testing | `gtest` |
| **Google Benchmark** | Latest | Performance testing | `benchmark` |

**Total Core Size:** ~150 MB binary

### LLM Dependencies (v1.3.0, optional)

**CMake Flag:** `-DTHEMIS_ENABLE_LLM=ON`

| Component | Purpose | Implementation | Size Impact |
|-----------|---------|----------------|-------------|
| **llama.cpp** | LLM inference engine | Bundled (src/llm/) | +50 MB |
| **GGUF Loader** | Model format support | Custom implementation | Included |
| **Tokenizer** | Text tokenization | llama.cpp | Included |
| **KV Cache** | PagedAttention | Custom (paged_kv_cache.h) | Included |
| **Scheduler** | Continuous batching | Custom (continuous_batch_scheduler.h) | Included |

**Additional Files:** +96 files (24 headers, 23 implementations, 21 tests, 28 docs)
**Total LLM Build Size:** ~250 MB binary

### RPC Dependencies (v1.3.0, optional)

**CMake Flag:** `-DTHEMIS_BUILD_RPC_FRAMEWORK=ON`

| Library | Purpose | vcpkg Package |
|---------|---------|---------------|
| **gRPC** | RPC framework | `grpc` |
| **Protobuf** | Serialization | `protobuf` |

**Additional Files:** +26 files (6 headers, 4 implementations, 7 docs, 9 proto/config)
**Size Impact:** +30 MB

### GPU Dependencies (v1.3.0, optional)

#### GPU Vector Search

**CMake Flag:** `-DTHEMIS_ENABLE_GPU=ON`

| Library | Purpose | vcpkg Package |
|---------|---------|---------------|
| **FAISS** | GPU vector search | `faiss` |

**Size Impact:** +20 MB

#### CUDA for LLM (requires CUDA Toolkit installed)

**CMake Flag:** `-DTHEMIS_ENABLE_CUDA=ON`

| Component | Purpose | Requirement |
|-----------|---------|-------------|
| **CUDA Toolkit** | GPU acceleration | System install (11.x or 12.x) |
| **cuBLAS** | Matrix operations | Included in CUDA |
| **Kernel Fusion** | Custom CUDA kernels | src/llm/kernel_fusion.cu |

**Size Impact:** +50 MB (with CUDA runtime)

**Supported GPU Architectures:**
- Pascal (sm_60, sm_61) - GTX 10xx
- Volta (sm_70) - Tesla V100
- Turing (sm_75) - RTX 20xx
- Ampere (sm_80, sm_86) - RTX 30xx, A100
- Ada Lovelace (sm_89) - RTX 40xx

### Build Combinations & Sizes

| Configuration | Flags | Size | Use Case |
|---------------|-------|------|----------|
| **Minimal** | None | ~150 MB | Basic database only |
| **LLM** | `ENABLE_LLM=ON` | ~250 MB | With LLM inference (CPU) |
| **LLM+GPU** | `ENABLE_LLM=ON ENABLE_CUDA=ON` | ~300 MB | LLM with GPU acceleration |
| **LLM+RPC** | `ENABLE_LLM=ON BUILD_RPC=ON` | ~280 MB | LLM with distributed ops |
| **Full** | All ON | ~350 MB | All features enabled |

---

### 2. Docker Builds (Hybrid Pre-built Binary Ansatz)

Der empfohlene Ansatz für Docker-Builds ist der **Hybrid Pre-built Binary** Workflow:

1. Binary lokal mit vcpkg bauen (einmalig, ~30-40 Minuten)
2. Docker-Image mit `Dockerfile.simple` erstellen (schnell, ~30 Sekunden)
3. Kleine Images (~100-200 MB) und 100% offline-fähig

#### Unified Docker Build Script

```powershell
# Standard Build (mit existierender Binary)
.\docker-build.ps1

# Binary zuerst in WSL bauen, dann Docker-Image erstellen
.\docker-build.ps1 -BuildBinary

# QNAP-Variante
.\docker-build.ps1 -Variant qnap

# Build und Push zu Registry
.\docker-build.ps1 -Push
```

```bash
# Bash-Äquivalent (Linux/macOS)
./docker-build.sh
./docker-build.sh --build-binary
./docker-build.sh -b qnap
./docker-build.sh --push
```

#### Unterstützte Plattformen

| Plattform | Architektur | Use Case |
|-----------|-------------|----------|
| `linux/amd64` | x86_64 | Server, Desktop, QNAP NAS |
| `linux/arm64` | ARM64 | Raspberry Pi 4/5, ARM Server, Apple Silicon |

#### Manueller Docker Build (Alternative)

```powershell
# Pre-built Binary muss in build/ vorhanden sein
docker build -f Dockerfile.simple -t themisdb/themisdb:1.0.1 --platform linux/amd64 .
```

#### Push zu Registry
```powershell
docker push themisdb/themisdb:1.0.1
docker push themisdb/themisdb:latest
docker push themisdb/themisdb:1.0.1-qnap
docker push themisdb/themisdb:qnap
```

### 3. Packaging

#### Portable Archives (Linux/QNAP/Windows)
```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package_releases.ps1
```
Erzeugt unter `dist/`: `themisdb-1.0.0-qnap-x64.tar.gz`, optional `themisdb-1.0.0-linux-x64.tar.gz` und `themisdb-1.0.0-windows-x64.zip` (wenn Binaries vorhanden). Enthält:
- `bin/` Binaries, `lib/` (gebündelte Shared Libs), `config/`, `openapi/`, `clients/`, `examples/`, `tools/`, `docs/`.

#### Debian/Ubuntu (.deb)
Build in Linux-Umgebung, siehe `themisdb.spec` Pendant und CI-Job (optional).

#### Red Hat/CentOS (.rpm)
```bash
rpmbuild -ba themisdb.spec
```

## CI/CD & Deployment

### 1. GitHub Releases
**Automated via GitHub Actions:**
```yaml
# .github/workflows/release.yml
on:
  push:
    tags:
      - 'v*.*.*'
```

**Artifacts:**
- `themis_server-{version}-linux-x64.tar.gz`
- `themis_server-{version}-windows-x64.zip`
- `themis_server-{version}-linux-arm64.tar.gz`
- `themis_server-{version}-enterprise-linux-x64.tar.gz` *(mit Enterprise Features)*
- `themis_server-{version}-enterprise-windows-x64.zip` *(mit Enterprise Features)*
- `themis_server-{version}.deb`
- `themis_server-{version}.rpm`

### Docker Push
```powershell
docker push themisdb/themisdb:1.0.1
docker push themisdb/themisdb:latest
docker push themisdb/themisdb:1.0.1-qnap
docker push themisdb/themisdb:qnap
```

### 3. GitHub Container Registry (ghcr.io)
```bash
docker tag themisdb:latest ghcr.io/makr-code/themisdb:latest
docker push ghcr.io/makr-code/themisdb:latest
```

## Vereinfachungs-Maßnahmen

### Zu Entfernen (Duplikate/Veraltet)
- [ ] `Dockerfile.old` - Veraltete Docker-Konfiguration
- [ ] `build-msvc/` - Veraltete MSVC-Build-Artefakte
- [ ] `build-wsl/` - Redundant (nutze CMakePresets)
- [ ] `build_final.txt`, `build_log.txt` - Veraltete Logs
- [ ] Diverse `tmp_*.json` - Temporäre Test-Dateien
- [ ] `server.pid`, `server.err`, `*.log` - Laufzeit-Artefakte (gitignore)

### Zu Konsolidieren
1. **Build-Scripts:**
   - ✅ `build.ps1` - Haupt-Windows-Build (DLL-basiert)
   - ✅ `build.sh` - Haupt-Linux-Build (behalten)
   - ✅ `docker-build.ps1` - Unified Docker Build (Hybrid Pre-built, monolithisch)
   - ✅ `build-unified.ps1` - Unified Build Script (behalten)
   - ✅ `build-qnap.sh` - Native QNAP Build ohne Docker (monolithisch)
   - ✅ `build-deb.sh` / `build-rpm.sh` - Packaging-Skripte (behalten)
   - ✅ `scripts/build_enterprise.cmd` - Enterprise Build für Windows (DLL)
   - ✅ `scripts/enable_enterprise_features.ps1` - Enterprise Build PowerShell
   - ❌ ~~`build-docker-qnap.ps1`~~ - Entfernt (ersetzt durch docker-build.ps1 -Variant qnap)
   - ❌ ~~`build-docker-simple.ps1`~~ - Entfernt (ersetzt durch docker-build.ps1)
   - ❌ ~~`build-rpi.ps1`~~ / ~~`build-rpi.sh`~~ - Entfernt (lokal mit -DTHEMIS_STATIC_BUILD=ON bauen)
   - ❌ ~~`docker-build-push.ps1`~~ - Entfernt (ersetzt durch docker-build.ps1 -Push)
   - ❌ ~~`docker-build-multiarch.ps1/.sh`~~ - Entfernt (vereinfacht zu `docker-build.ps1/.sh`)

2. **Docker-Dateien:**
   - ✅ `Dockerfile.simple` - **EMPFOHLEN**: Pre-built Binary Deployment (monolithisch)
   - ✅ `Dockerfile` - Multi-Stage Standard-Build (für CI/CD, langsam)
   - ✅ `Dockerfile.qnap` - QNAP-kompatibel (SSE4.2; GLIBC 2.31, monolithisch)
   - ❌ `Dockerfile.runtime` - Redundant
   - ❌ `Dockerfile.old` - Veraltet

3. **Docker Compose:**
   - ✅ `docker-compose.yml` - Standard
   - ✅ `docker-compose-arm.yml` - ARM-spezifisch
   - ⚠️ `docker-compose.qnap.yml` - Behalten, aber fix required
   - ❌ `docker-compose.pull.qnap.yml` - Redundant

## QNAP-Deployment

### Problem
- WSL/Docker Ubuntu 24.04 → GLIBC 2.38, GLIBCXX 3.4.32
- QNAP benötigt → GLIBC 2.31, GLIBCXX 3.4.28

### Lösungsoptionen

#### Option 1: Static Linking (EMPFOHLEN)
```cmake
# CMakeLists.txt
option(THEMIS_STATIC_BUILD "Build fully static binary" OFF)

if(THEMIS_STATIC_BUILD)
  set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
  set(VCPKG_TARGET_TRIPLET "x64-linux-static")
endif()
```

Build:
```bash
cmake -DTHEMIS_STATIC_BUILD=ON ...
```

#### Option 2: Ubuntu 20.04 Runtime-Image
Dockerfile.qnap nutzt 20.04 und SSE4.2 Basis.

#### Option 3: Cross-Compilation
```bash
# Auf Ubuntu 24.04 für Ubuntu 20.04 kompilieren
docker run -v $(pwd):/src ubuntu:20.04 bash -c "cd /src && ./build.sh"
```

## v1.3.0 Modular Build-Matrix (NEU - Dezember 2025)

### Übersicht

ThemisDB v1.3.0 bietet eine **modulare Build-Architektur** mit unabhängig aktivierbaren Features:

| Feature | Build-Flag | Dependencies | Status | Impact |
|---------|-----------|--------------|--------|--------|
| **Core Database** | *(immer aktiv)* | RocksDB, TBB, OpenSSL | ✅ Stable | Base |
| **LLM Integration** | `THEMIS_ENABLE_LLM=ON` | llama.cpp, CUDA* | ✅ v1.3.0 | +96 files |
| **RPC Framework** | `THEMIS_BUILD_RPC_FRAMEWORK=ON` | gRPC, Protobuf | ✅ v1.3.0 | +26 files |
| **GPU/CUDA** | `THEMIS_ENABLE_CUDA=ON` | CUDA Toolkit 11+ | ✅ v1.3.0 | Optional |
| **Vector Search** | `THEMIS_ENABLE_GPU=ON` | Faiss, HNSWLIB | ✅ Stable | Performance |
| **Monitoring** | `THEMIS_ENABLE_TRACING=ON` | OpenTelemetry | ✅ Stable | Observability |
| **Content Processors** | `THEMIS_ENABLE_CONTENT_PROCESSORS=ON` | LibAV, GDAL | ⚠️ Optional | Heavy deps |
| **Enterprise** | `THEMIS_BUILD_ENTERPRISE=ON` | - | ⚠️ Optional | Enterprise-only |

**\* CUDA ist optional** - LLM funktioniert auch mit CPU (100x langsamer)

### Build-Konfigurationen

#### 1. Minimal Build (Core nur)
```bash
cmake -S . -B build-minimal \
  -DTHEMIS_ENABLE_LLM=OFF \
  -DTHEMIS_BUILD_RPC_FRAMEWORK=OFF \
  -DTHEMIS_ENABLE_CUDA=OFF \
  -DCMAKE_BUILD_TYPE=Release

# Binary Size: ~150 MB
# Dependencies: 16 libraries (RocksDB, TBB, OpenSSL, etc.)
```

#### 2. Standard Build (Core + Vector)
```bash
cmake -S . -B build-standard \
  -DTHEMIS_ENABLE_LLM=OFF \
  -DTHEMIS_BUILD_RPC_FRAMEWORK=OFF \
  -DTHEMIS_ENABLE_CUDA=OFF \
  -DTHEMIS_ENABLE_GPU=ON \
  -DCMAKE_BUILD_TYPE=Release

# Binary Size: ~180 MB
# Dependencies: +2 (Faiss, HNSWLIB)
```

#### 3. LLM Build (Core + LLM + GPU)
```bash
cmake -S . -B build-llm \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DTHEMIS_BUILD_RPC_FRAMEWORK=OFF \
  -DCMAKE_BUILD_TYPE=Release

# Binary Size: ~250 MB
# Dependencies: +2 (CUDA Toolkit, llama.cpp wird inline gebaut)
```

#### 4. Full Build (Core + LLM + RPC + GPU)
```bash
cmake -S . -B build-full \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DTHEMIS_BUILD_RPC_FRAMEWORK=ON \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_TRACING=ON \
  -DCMAKE_BUILD_TYPE=Release

# Binary Size: ~300 MB
# Dependencies: +4 (CUDA, gRPC, Protobuf, OpenTelemetry)
```

#### 5. Enterprise Build (All Features)
```bash
cmake -S . -B build-enterprise \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DTHEMIS_BUILD_RPC_FRAMEWORK=ON \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_TRACING=ON \
  -DTHEMIS_BUILD_ENTERPRISE=ON \
  -DTHEMIS_ENABLE_CONTENT_PROCESSORS=ON \
  -DCMAKE_BUILD_TYPE=Release

# Binary Size: ~400 MB
# Dependencies: +8 (LibAV, GDAL, CAD libs, etc.)
```

### Dependencies Matrix (v1.3.0)

#### Core Dependencies (Required)
- OpenSSL 3.0+
- RocksDB 8.x (with LZ4, ZSTD)
- simdjson
- TBB (Threading Building Blocks)
- Boost (ASIO, Beast)
- spdlog
- fmt
- nlohmann-json
- yaml-cpp
- zstd
- mimalloc (optional, 20-40% memory boost)

#### LLM Dependencies (THEMIS_ENABLE_LLM=ON)
- **llama.cpp** (embedded, built from source)
- **CUDA Toolkit 11.0+** (optional, for GPU acceleration)
  - Compute Capability 7.0+ (Volta/Turing/Ampere/Ada/Hopper)
  - Minimum 8GB VRAM (recommended 16GB+)

#### RPC Dependencies (THEMIS_BUILD_RPC_FRAMEWORK=ON)
- **gRPC 1.50+**
- **Protobuf 3.21+**

#### Vector Search Dependencies (THEMIS_ENABLE_GPU=ON)
- **Faiss** (with GPU support if CUDA enabled)
- **HNSWLIB**

#### Monitoring Dependencies (THEMIS_ENABLE_TRACING=ON)
- **OpenTelemetry C++** (OTLP HTTP exporter)

#### Testing Dependencies (THEMIS_BUILD_TESTS=ON)
- **Google Test**
- **Google Benchmark**

## GPU Acceleration Build Options (NEU - Dezember 2025)

### Übersicht

ThemisDB unterstützt 10 GPU-Backends für beschleunigte Workloads (10-50x Speedup für Vector Search):

| Backend | Plattform | Build-Flag | Use Case |
|---------|-----------|------------|----------|
| **CUDA** | NVIDIA GPUs | `-DTHEMIS_ENABLE_CUDA=ON` | Vector Search, Geo Operations |
| **Vulkan** | Cross-Platform | `-DTHEMIS_ENABLE_GPU=ON` | Universal GPU Compute |
| **HIP** | AMD ROCm | `-DTHEMIS_ENABLE_HIP=ON` | AMD GPU Acceleration |
| **OpenCL** | Cross-Platform | `-DTHEMIS_ENABLE_OPENCL=ON` | Fallback GPU Compute |
| **DirectX 12** | Windows | `-DTHEMIS_ENABLE_DIRECTX=ON` | DirectML, Compute Shaders |
| **OneAPI** | Intel | `-DTHEMIS_ENABLE_ONEAPI=ON` | Intel GPU/CPU |
| **ZLUDA** | AMD via CUDA | `-DTHEMIS_ENABLE_ZLUDA=ON` | CUDA on AMD (compatibility) |
| **Faiss GPU** | NVIDIA | `-DTHEMIS_ENABLE_FAISS_GPU=ON` | Optimized Vector Search |

### Build-Beispiele

#### CUDA Build (NVIDIA)
```bash
# Voraussetzungen:
# - CUDA Toolkit 11.0+
# - GPU: Compute Capability 7.0+ (Volta/Turing/Ampere/Hopper)
# - VRAM: Mindestens 8GB (empfohlen 16GB+)

cmake -S . -B build-cuda \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DTHEMIS_ENABLE_FAISS_GPU=ON \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-cuda --target themis_server -j$(nproc)
```

#### Vulkan Build (Cross-Platform)
```bash
# Voraussetzungen:
# - Vulkan SDK 1.2+
# - GPU mit Vulkan 1.2+ Support

cmake -S . -B build-vulkan \
  -DTHEMIS_ENABLE_GPU=ON \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-vulkan --target themis_server -j$(nproc)
```

#### HIP Build (AMD ROCm)
```bash
# Voraussetzungen:
# - ROCm 5.0+
# - AMD GPU (RDNA2+)

cmake -S . -B build-hip \
  -DTHEMIS_ENABLE_HIP=ON \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-hip --target themis_server -j$(nproc)
```

#### DirectX 12 Build (Windows)
```powershell
# Voraussetzungen:
# - Windows 10/11
# - DirectX 12 SDK
# - GPU mit DirectX 12 Support

cmake -S . -B build-dx12 `
  -G "Visual Studio 17 2022" -A x64 `
  -DTHEMIS_ENABLE_DIRECTX=ON `
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-dx12 --config Release --target themis_server
```

#### Alle GPU-Backends (Development)
```bash
# Baut alle verfügbaren GPU-Backends
cmake -S . -B build-all-gpu \
  -DTHEMIS_ENABLE_ALL_GPU_BACKENDS=ON \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-all-gpu --target themis_server -j$(nproc)
```

### Runtime-Konfiguration

GPU-Backends werden zur Laufzeit automatisch ausgewählt:

```yaml
# config.yaml
gpu:
  enabled: true
  backend: "auto"  # auto, cuda, vulkan, hip, opencl, directx, oneapi
  fallback_chain: ["cuda", "hip", "vulkan", "opencl", "cpu"]
  
vector_search:
  use_gpu: true
  batch_size: 1000
  
geo_operations:
  use_gpu: true
```

### Performance-Vergleich

**Test-System:**
- CPU: AMD Ryzen 9 5950X (16C/32T)
- GPU: NVIDIA RTX 3090 (24GB VRAM, CUDA 11.8)
- RAM: 64GB DDR4-3600
- Storage: NVMe SSD (PCIe 4.0)

| Workload | CPU | CUDA GPU | Vulkan GPU | Speedup |
|----------|-----|----------|------------|---------|
| Vector Search (k=100, 1M vectors) | 1,800 q/s | 50,000 q/s | 35,000 q/s | 10-50x |
| Geo Distance Calc (1M points) | 5,000 ops/s | 50,000 ops/s | 40,000 ops/s | 5-20x |
| OLAP Aggregation (100M rows) | 1,000 q/s | 10,000 q/s | 8,000 q/s | 5-10x |

**Hinweis:** Performance kann je nach Hardware und Workload variieren. Benchmarks dienen als Richtwerte.

### Dokumentation

- **GPU Plan:** `docs/performance/performance_gpu_plan.md`
- **CUDA Setup:** `docs/performance/cuda_setup.md`
- **Vector Search GPU:** `docs/performance/gpu_vector_search.md`

#### Option 3: Cross-Compilation
```bash
# Auf Ubuntu 24.04 für Ubuntu 20.04 kompilieren
docker run -v $(pwd):/src ubuntu:20.04 bash -c "cd /src && ./build.sh"
```

## Automatisierung (CI/CD)

### GitHub Actions Workflow
```yaml
# .github/workflows/build-release.yml
name: Build & Release

on:
  push:
    tags: ['v*']

jobs:
  build-matrix:
    strategy:
      matrix:
        include:
          # Standard Builds
          - os: ubuntu-24.04
            preset: linux-ninja-clang-release
            artifact: linux-x64
            variant: standard
          - os: windows-latest
            preset: windows-ninja-msvc-release
            artifact: windows-x64
            variant: standard
          - os: ubuntu-24.04
            preset: linux-arm64-gcc-release
            artifact: linux-arm64
            variant: standard
          
          # Enterprise Builds
          - os: ubuntu-24.04
            preset: linux-ninja-clang-release
            artifact: enterprise-linux-x64
            variant: enterprise
            extra_flags: "-DTHEMIS_BUILD_TESTS=ON"
          - os: windows-latest
            preset: windows-ninja-msvc-release
            artifact: enterprise-windows-x64
            variant: enterprise
            extra_flags: "-DTHEMIS_BUILD_TESTS=ON"
    
    steps:
      - name: Build
        run: |
          cmake --preset ${{ matrix.preset }} ${{ matrix.extra_flags }}
          cmake --build build
      
      - name: Test Enterprise Features
        if: matrix.variant == 'enterprise'
        run: |
          ./build/themis_tests --gtest_filter="*Enterprise*:TokenBucket*:PerClient*:LoadShed*"
      
      - name: Package Portable Archives
        run: |
          pwsh -File scripts/package_releases.ps1
          ls dist
```

### Vereinfachtes Release-Script
```powershell
# scripts/release.ps1
param(
    [string]$Version,
    [switch]$Enterprise
)

# 1. Tag erstellen
$tagSuffix = if ($Enterprise) { "-enterprise" } else { "" }
git tag -a "v$Version$tagSuffix" -m "Release $Version$tagSuffix"

# 2. Builds auslösen (GitHub Actions)
git push origin "v$Version$tagSuffix"

# 3. Docker Images
if ($Enterprise) {
    # Enterprise Build
    .\scripts\enable_enterprise_features.ps1
    .\build-docker-simple.ps1 -Tag "themisdb:enterprise-$Version"
    docker tag "themisdb:enterprise-$Version" "themisdb/themisdb:$Version-enterprise"
    docker tag "themisdb:enterprise-$Version" "themisdb/themisdb:enterprise-latest"
    docker push "themisdb/themisdb:$Version-enterprise"
    docker push "themisdb/themisdb:enterprise-latest"
} else {
    # Standard Build
    .\build-docker-simple.ps1 -Tag "themisdb:$Version"
    docker tag "themisdb:$Version" "themisdb/themisdb:$Version"
    docker tag "themisdb:$Version" "themisdb/themisdb:latest"
    docker push "themisdb/themisdb:$Version"
    docker push "themisdb/themisdb:latest"
}

# 4. Summary
Write-Host "✅ Released ThemisDB $Version$(if($Enterprise){' (Enterprise)'})" -ForegroundColor Green
```

**Verwendung:**
```powershell
# Standard Release
.\scripts\release.ps1 -Version "0.2.0"

# Enterprise Release
.\scripts\release.ps1 -Version "0.2.0" -Enterprise
```

## Migration-Plan

### Phase 1: Cleanup (Sofort)
1. Entferne veraltete Dateien
2. Aktualisiere .gitignore
3. Commit: "chore: Clean up build artifacts and obsolete files"

### Phase 2: QNAP-Fix (Priorität Hoch)
1. Implementiere statisches Linking
2. Teste QNAP-Deployment
3. Commit: "feat: Add static build option for QNAP compatibility"

### Phase 3: Automation (Kurzfristig)
1. Erweitere GitHub Actions
2. Automatisiere Docker Hub Push
3. Commit: "ci: Automate multi-platform builds and releases"

### Phase 4: Packaging (Mittelfristig)
1. Verbessere .deb/.rpm Packaging
2. Arch AUR Package
3. Homebrew Formula

## Verwendete Tools

| Tool | Zweck | Status |
|------|-------|--------|
| **CMake 3.28+** | Build-System | ✅ Produktiv |
| **vcpkg** | Dependency Management | ✅ Produktiv |
| **Ninja** | Build-Generator | ✅ Produktiv |
| **Docker** | Containerization | ✅ Produktiv |
| **GitHub Actions** | CI/CD | 🔧 Teilweise |
| **CPack** | Packaging | 📋 Geplant |

## Best Practices

1. **Immer CMakePresets verwenden** - Keine manuellen cmake-Aufrufe
2. **Version in CMakeLists.txt pflegen** - Single Source of Truth
3. **Git Tags für Releases** - Automatische CI/CD Trigger
4. **Docker: Pre-built Binary bevorzugen** - Schneller, stabiler
5. **QNAP: Baseline SSE4.2** - Vermeidet FMA/AVX-Inkompatibilität
6. **Artefakt-Fluss** - Build → Artefakt → Packaging → Release/Docker

## Nächste Schritte

1. ✅ Konsolidiere Build-Scripts
2. ✅ Enterprise Build-Variante implementiert
3. ✅ Enterprise Tests erfolgreich (13/13 passed)
4. ⏸️ Implementiere statisches Linking für QNAP
5. ⏸️ Automatisiere Docker Hub Deployment (Standard + Enterprise)
6. ⏸️ Erweitere GitHub Actions für Multi-Platform (Standard + Enterprise)
7. ⏸️ Erstelle Release-Automation Script mit Enterprise-Support

## Enterprise Edition - Überblick

### Zusätzliche Features
- **Token Bucket Rate Limiter**: Priority-basierte Request-Limitierung (HIGH, NORMAL, LOW)
- **Per-Client Rate Limiter**: Unabhängige Quotas pro Client/API-Key
- **Adaptive Load Shedder**: Multi-Metrik Lastüberwachung (CPU 50%, Memory 30%, Queue 20%)
- **HTTP Client Pool**: Production-ready Boost.Beast Implementation mit SSL/TLS

### Code-Statistiken
- **Production Code**: 1.047 LOC (Rate Limiter: 407, Load Shedder: 116, HTTP Pool: 524)
- **Tests**: 478 LOC (13 Unit Tests, alle bestanden)
- **Dokumentation**: ~2.000 LOC (4 Markdown-Dateien)

### Build-Status
- ✅ Windows MSVC 19.44 - Build erfolgreich
- ✅ Unit Tests: 13/13 bestanden
- ✅ Integration: Bereit für HTTP Server Middleware
- 📋 Performance Tests: Ausstehend (k6 Load Testing)

### Deployment
Enterprise Features sind standardmäßig in allen Builds enthalten und können über Konfiguration aktiviert/deaktiviert werden:

```json
// config.json
{
  "rate_limiting": {
    "enabled": true,
    "capacity": 1000,
    "refill_rate": 100
  },
  "load_shedding": {
    "enabled": true,
    "cpu_threshold": 0.95,
    "memory_threshold": 0.90
  },
  "http_client_pool": {
    "max_connections": 100,
    "timeout_ms": 5000
  }
}
```
