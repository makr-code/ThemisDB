Project: Themis (Database System)
Language: C++

Purpose:
- High-performance C++ vector database with RocksDB integration, AQL and MVCC.

What Copilot should help with:
- Generate idiomatic C++ code for core algorithms, concurrency control, and memory-safe structures.
- Suggest tests for correctness and performance; avoid unsafe patterns.

Coding style and constraints:
- Follow project C++ style; prefer modern C++ (C++17/20) features and RAII patterns.
- Document threading and locking choices in `docs/architecture.md`.

Documentation duties (./docs):
- Add `docs/design.md` describing AQL semantics, indexing strategy and storage layout.

Todo.md continuation:
- Add clear development tasks with benchmark targets and unit test coverage goals.

Examples for Copilot prompts:
- "Implement MVCC snapshot read path with minimal locking and add unit tests simulating concurrent writers."

Testing & CI:
- Provide unit tests and a micro-benchmark harness; CI should run static analysis and unit tests.

---

## ThemisDB Build-System Übersicht

### Kernprinzip: vcpkg Offline-First Architecture

ThemisDB nutzt eine **vcpkg Offline-First Architektur** für reproduzierbare, netzwerk-unabhängige Builds:

**vcpkg Cache Struktur:**
```
vcpkg/
├── downloads/          ← ~2.5 GB - Source Archives (alle Packages)
├── buildtrees/         ← ~3 GB - Temporäre Build-Artefakte
├── packages/           ← ~10 GB - Installierte Packages
└── scripts/buildsystems/vcpkg.cmake  ← CMake Integration
```

**Einmalig erforderlich:**
```powershell
# Windows
.\scripts\setup-vcpkg-offline.ps1

# Linux/WSL
./scripts/setup-vcpkg-offline.sh
```

### Build-Plattformen & Triplets

| Plattform | Triplet | Build-Verzeichnis | Empfohlener Generator |
|-----------|---------|-------------------|---------------------|
| **Windows MSVC** | x64-windows | `build-msvc` | Visual Studio 17 2022 |
| **Linux/WSL x64** | x64-linux | `build-wsl` oder `build-linux` | Ninja |
| **Linux ARM64** | arm64-linux | `build-arm` | Ninja |
| **Docker Multi-Arch** | x64-linux, arm64-linux | Container | Multi-stage |
| **QNAP NAS** | x64-linux | `build-qnap` | Ninja |

### Quick Start Build-Commands

#### Windows (MSVC 2022) - Empfohlen
```powershell
# CMake Preset (v4.0.0+)
$env:VCPKG_ROOT = "C:\VCC\themis\vcpkg"
cmake --preset windows-vs2022-release
cmake --build --preset windows-vs2022-release --parallel 8

# Oder manuell
cmake -B build-msvc -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DTHEMIS_CORE_SHARED=OFF `
  -DTHEMIS_ENABLE_LLM=OFF
cmake --build build-msvc --config Release --parallel 8
```

**Output:** `build-msvc/Release/themis_server.exe` (~32 MB)  
**Dauer:** 25-35 min (erste Build), 5-10 min (inkrementell)

#### Linux/WSL (GCC/Clang)
```bash
# Konfiguration
export VCPKG_ROOT=/mnt/c/VCC/themis/vcpkg  # WSL Pfad
cmake -B build-wsl -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DTHEMIS_ENABLE_LLM=OFF

# Build
cmake --build build-wsl --config Release -j$(nproc)
```

**Output:** `build-wsl/themis_server` (~32 MB)  
**Dauer:** 20-30 min (erste Build)

#### ARM64 (Raspberry Pi / Linux ARM)
```bash
# Native auf ARM
cmake -B build-arm -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DTHEMIS_QNAP_BUILD=ON \  # Baseline x86-64, kein AVX
  -DTHEMIS_ENABLE_LLM=OFF
cmake --build build-arm -j4
```

**Performance:** ARM NEON SIMD automatisch aktiviert

### Docker Artefakte & Compose-Pfade
- Dockerfiles liegen unter `docker/` (Build: `docker/Dockerfile`, Runtime: `docker/Dockerfile.release`)
- Release-Archiv für Docker-Images: `docker/themis-linux.tar.gz` (aus `release/v1.3.4` gespiegelt)
- Compose-Files: `docker/compose/docker-compose.yml` und `docker/compose/docker-compose-vllm.yml`
- Build (Release Image): `docker buildx build -f docker/Dockerfile.release --platform linux/amd64,linux/arm64 -t <tag> --push .`

### Edition-spezifische Builds

| Edition | Script | CMake Flag | Features |
|---------|--------|-----------|----------|
| **Community** (Open Source) | `build-community-release.ps1` | `-DTHEMIS_EDITION=COMMUNITY` | Core, 24GB GPU, Single-Node |
| **Enterprise** | `build-enterprise-release.ps1` | `-DTHEMIS_EDITION=ENTERPRISE` | + Sharding, 256GB GPU, 100 Nodes |
| **Hyperscaler** | `build-hyperscaler-release.ps1` | `-DTHEMIS_EDITION=HYPERSCALER` | Unlimited |

### Feature Flags

```bash
# Minimal Build (Core only, ~150 MB)
cmake -B build -DTHEMIS_ENABLE_LLM=OFF -DTHEMIS_BUILD_RPC_FRAMEWORK=OFF

# LLM Build (Core + llama.cpp, ~250 MB)
cmake -B build -DTHEMIS_ENABLE_LLM=ON

# Full Build (Core + LLM + RPC + GPU, ~350 MB)
cmake -B build -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_BUILD_RPC_FRAMEWORK=ON \
  -DTHEMIS_ENABLE_GPU=ON
```

### Wichtige CMake-Optionen

| Option | Default | Beschreibung |
|--------|---------|-------------|
| `THEMIS_CORE_SHARED` | OFF | Build themis_core als DLL/SO statt statisch |
| `THEMIS_STATIC_BUILD` | OFF | Vollständig statisches Binary (Docker/QNAP) |
| `THEMIS_ENABLE_LLM` | OFF | llama.cpp Integration |
| `THEMIS_ENABLE_GPU` | OFF | CUDA/Vulkan/ROCm Support |
| `THEMIS_BUILD_TESTS` | ON | Google Test Suite |
| `THEMIS_BUILD_BENCHMARKS` | ON | Google Benchmark Suite |
| `THEMIS_QNAP_BUILD` | OFF | QNAP NAS Optimierungen (SSE4.2 baseline) |

### Troubleshooting

**CMake find_package Probleme (FAISS/gRPC):**
```powershell
# Quick Fix
.\scripts\fix-cmake-prefix-path.ps1 -Action build -EnableGPU $true

# Oder manuell CMAKE_PREFIX_PATH setzen
$VCPKG = "C:\VCC\themis\vcpkg_installed\x64-windows"
cmake -DCMAKE_PREFIX_PATH="$VCPKG;$VCPKG\share" ...
```

**WSL Build-Fehler (MSVC Flags auf Linux):**
- Problem: `/O1`, `/bigobj`, `/Bt+` auf Linux
- Fix: In CMakeLists.txt mit `if(MSVC)` gaten

**Docker Build-Context Probleme:**
- Problem: vcpkg/buildtrees zu groß
- Fix: .dockerignore aktualisieren, nur downloads/ inkludieren

---

## Build-Dokumentation

Weitere Details in:
- `docs/de/deployment/deployment_strategy.md` - Gesamtstrategie
- `docs/de/deployment/deployment_arm_build.md` - ARM/Raspberry Pi Builds
- `docs/de/deployment/BUILD_OPTIONEN_REFERENZ.md` - Alle CMake Flags
- `docs/de/deployment/QUICK_REFERENCE.md` - CMake find_package Fixes

---

## Build-Pfade (aktive Konfigurationen):

1. build-qnap/
	- Ziel: Statischer Release-Build für QNAP / Ubuntu 20.04 (GLIBC 2.31)
	- Generator: Ninja
	- Toolchain: /opt/vcpkg (Manifest-Modus, Triplet x64-linux, manuelle Prefix-Hints)
	- Flags: -DTHEMIS_STATIC_BUILD=ON, -DTHEMIS_ENABLE_TRACING=ON, Tests & Benchmarks ON
	- Prefix: CMAKE_PREFIX_PATH=/src/vcpkg_installed/x64-linux
	- Wichtige Pakete: RocksDB, Arrow, OpenSSL 3.6.0, Curl (statisch), Zstd
	- Artefakte: libthemis_core.a, themis_server (Release, etwa ~35MB)
	- Besonderheiten: -DVCPKG_MANIFEST_INSTALL=OFF (Manifest schon vor Configure installiert); statische Abhängigkeiten (BUILD_SHARED_LIBS=OFF)

2. build-linux-gcc-release/
	- Ziel: Linux Release (GCC) mit statischem Build (THEMIS_STATIC_BUILD=ON)
	- Generator: Standard (vermutlich Unix Makefiles oder Ninja, Cache zeigt /usr/local/bin/cmake Erzeugung)
	- Toolchain: /opt/vcpkg (Manifest INSTALL ON)
	- Build-Type: Release
	- Artefakte: Entspricht regulärem Server/Linux Release Build (dynamische/teilweise statische Libs je nach vcpkg Port)

3. build-msvc-ninja-release/
	- Ziel: Windows Release Build (MSVC, Ninja)
	- Compiler: MSVC 14.44 (cl.exe)
	- Build-Type: Release
	- Toolchain: vcpkg x64-windows (Arrow_DIR, Boost_DIR etc. im lokalen vcpkg_installed)
	- Artefakte: themis_server.exe, libthemis_core.lib
	- Install-Prefix: C:/Program Files (x86)/Themis vorgesehen

4. Weitere vorhandene Verzeichnisse (Status: nicht im Fokus/QNAP):
	- build-msvc-ninja-debug/ (Windows Debug)
	- build-linux-clang-debug/, build-linux-clang-release/ (Clang Builds)
	- build-linux-clang-debug-bench/ (Clang + Benchmarks)
	- build-linux-gcc-release/ (siehe oben)
	- build-ninja/ (Allgemeiner Ninja Build – ggf. Standard Preset Test)
	- build/ (Legacy/Initial CMake Build-Ausgabe)

Pfad- und Strukturregeln für neue Build-Verzeichnisse:
	- Nutze eindeutige Namenskonvention: build-<platform>-<compiler>-<config>[-optionalTag]
	- Immer separates vcpkg_installed Unterverzeichnis, falls Manifest-Modus genutzt (automatisch durch CMake/vcpkg)
	- Für portable statische Builds: BUILD_SHARED_LIBS=OFF + THEMIS_STATIC_BUILD=ON + explizites CMAKE_PREFIX_PATH auf vcpkg_installed/<triplet>
	- Keine doppelte Manifest-Installation: Vor dem Configure einmal `vcpkg install`, dann `-DVCPKG_MANIFEST_INSTALL=OFF`

QNAP Release Rebuild Ablauf (Referenz):
	1. Manifest kopieren (vcpkg.qnap.json -> vcpkg.json)
	2. `bootstrap-vcpkg.sh`
	3. `vcpkg install --triplet x64-linux --clean-after-build` (mit Retry)
	4. `cmake -S . -B build-qnap -G Ninja -DCMAKE_BUILD_TYPE=Release -DTHEMIS_STATIC_BUILD=ON -DVCPKG_MANIFEST_INSTALL=OFF -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_PREFIX_PATH="$(pwd)/vcpkg_installed/x64-linux" -DRocksDB_DIR="$(pwd)/vcpkg_installed/x64-linux/share/rocksdb" -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake`
	5. `cmake --build build-qnap --target themis_server --parallel`
	6. Optional: GLIBC Prüfung via Ubuntu 20.04 Container (`ldd build-qnap/themis_server | grep GLIBC`)

Erwartungen an Copilot bei Build-Themen:
	- Bei Änderung an `build-qnap.sh`: Nur notwendige Flags anpassen, keine Preset-Einführung.
	- Statische Build-Optimierungsvorschläge nur nach Prüfung auf symbolische Duplikate (nm -C | sort | uniq -d).
	- Bei Paket-Find-Problemen zuerst CMAKE_PREFIX_PATH / *_DIR prüfen, erst danach find_package Log erweitern.
	- Docker-Builds immer mit `docker-build.ps1/.sh` (Hybrid Pre-built Binary Ansatz)

Bitte neue Build-Profile konsistent mit obiger Matrix ergänzen und Unterschiede klar dokumentieren.
