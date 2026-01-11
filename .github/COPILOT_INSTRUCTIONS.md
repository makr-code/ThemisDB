Project: Themis (Database System)
Language: C++
> **📋 WICHTIG: Build-Pipeline Modernisierung (Jan 2026)**

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

## Git Branching Strategy (Git Flow)

ThemisDB folgt einer **Git Flow Branching Strategy**:

### Branch-Struktur

| Branch | Zweck | Schutz | Merges von |
|--------|-------|--------|------------|
| **`main`** | Production-ready releases (Tagged: v1.4.0, etc.) | 🔒 Vollständig geschützt | `release/*`, `hotfix/*` |
| **`develop`** | Aktive Entwicklung und Integration | 🔒 Geschützt | `feature/*`, `bugfix/*`, `release/*` |
| **`feature/*`** | Neue Features entwickeln | - | - |
| **`bugfix/*`** | Bugfixes für develop | - | - |
| **`hotfix/*`** | Kritische Production-Fixes | - | - |
| **`release/*`** | Release-Vorbereitung | - | - |

### Workflow für Copilot

**Bei Docker-Builds:**
- 🔗 **vcpkg Cache-Strategie**: Siehe [docker/DOCKER_BUILD_STRATEGY_QUICKREF.md](../docker/DOCKER_BUILD_STRATEGY_QUICKREF.md#vcpkg-triple-cache-strategie-)
- Triple-Cache nutzt Host-Packages aus `./vcpkg/packages/` + BuildKit Cache
- Keine unnötigen Downloads/Rebuilds für existierende Dependencies
- Bei Cache-Problemen: Prüfe Bind-Mounts in `Dockerfile.unified`

**Bei neuen Features:**
```bash
# Immer von develop branchen
git checkout develop
git pull origin develop
git checkout -b feature/xyz
```

**Bei PRs:**
- **Target Branch**: `develop` (NICHT `main`!)
- **Ausnahme**: Hotfixes targeten `main`

**Bei Release-Vorbereitung:**
```bash
# Von develop zu release branch
git checkout -b release/1.4.0 develop
# Nach Testing: Merge zu main + Tag + Merge zurück zu develop
```

### Wichtige Regeln

- ✅ Feature-Branches immer von `develop` erstellen
- ✅ PRs standardmäßig zu `develop` erstellen
- ✅ `main` Branch ist NUR für Production Releases
- ❌ Nie direkt auf `main` oder `develop` committen
- ❌ Keine Feature-PRs direkt zu `main`

**Dokumentation**: Siehe `BRANCHING_STRATEGY.md` für Details

---

## Merge Strategy für Pull Requests

ThemisDB verwendet unterschiedliche Merge-Methoden abhängig vom Branch-Typ:

### Merge-Strategie Übersicht

| Branch-Typ | Ziel | Merge-Methode | Begründung |
|-----------|------|---------------|------------|
| **`feature/*`** | `develop` | **Squash and merge** ✅ | Saubere Historie, ein Commit pro Feature |
| **`bugfix/*`** | `develop` | **Squash and merge** ✅ | Saubere Historie, ein Commit pro Fix |
| **`release/*`** | `main` | **Merge commit** | Vollständige Release-Historie erhalten |
| **`hotfix/*`** | `main` | **Merge commit** | Vollständige Hotfix-Historie für Audit |

### Wichtig für Copilot bei PR-Erstellung

Wenn du Pull Requests erstellst:

1. **PR-Titel ist kritisch**: Wird zur Commit-Message bei Squash Merge
   - Format: `<type>(<scope>): <description>`
   - Beispiel: `feat(storage): Add vector search optimization`

2. **PR-Beschreibung ist wichtig**: Wird zum Commit-Body bei Squash Merge
   - Erkläre was sich geändert hat
   - Erkläre warum die Änderung notwendig war
   - Referenziere Issues: `Closes #123`

3. **Einzelne Commits im Branch**: Unwichtig bei Feature/Bugfix
   - WIP commits sind OK
   - Werden nicht in `develop` erscheinen
   - Nur PR-Titel und -Beschreibung zählen

**Dokumentation**:
- Vollständige Anleitung: `docs/MERGE_STRATEGY_MIGRATION.md`
- Quick Reference: `docs/MERGE_STRATEGY_QUICK_REF.md`
- Siehe auch: `CONTRIBUTING.md` → "Merge Strategy Guidelines"

---

## GitHub Labels System

### Label Verwendung für Issues und PRs

ThemisDB verwendet ein strukturiertes Label-System zur Kategorisierung von Issues und Pull Requests.

**Wichtig für Copilot:**

Wenn du Issues erstellst, PRs vorschlägst, oder Skripte zur Issue-Erstellung überprüfst/erstellst:

1. **Verwende NUR Labels aus `.github/labels.yml`**
   - Diese Datei ist die einzige Quelle der Wahrheit für gültige Labels
   - Erfinde keine neuen Labels - schlage dem Maintainer vor, `labels.yml` zu erweitern

2. **Konsultiere die Label-Dokumentation**
   - Vollständiger Leitfaden: `.github/LABELS_GUIDE.md`
   - Label-Definitionen: `.github/labels.yml`

3. **Label-Kategorien und Beispiele**
   - **Priorität (erforderlich):** `priority:P0`, `priority:P1`, `priority:P2`, `priority:P3`
   - **Typ (erforderlich):** `type:bug`, `type:feature`, `type:enhancement`, `type:documentation`, `type:security`, `type:performance`, etc.
   - **Bereich (optional, mehrere möglich):** `area:llm`, `area:storage`, `area:aql`, `area:api`, `area:networking`, `area:build`, `area:docker`, etc.
   - **Status (optional):** `status:ready`, `status:in-progress`, `status:needs-review`, `status:blocked`, etc.
   - **Aufwand (optional):** `effort:small`, `effort:medium`, `effort:large`, `effort:x-large`
   - **Spezial (optional):** `good first issue`, `help wanted`, `breaking-change`, `regression`, etc.

4. **Issue-Erstellungs-Beispiel**
   ```yaml
   ---
   title: "Fix RocksDB memory leak in snapshot cleanup"
   labels: priority:P1, type:bug, area:storage, regression
   ---
   ```
   
   Oder als Array:
   ```yaml
   ---
   title: "Fix RocksDB memory leak in snapshot cleanup"
   labels: ['priority:P1', 'type:bug', 'area:storage', 'regression']
   ---
   ```

5. **Bei Scripts zur Issue-Erstellung**
   - Das primäre Skript ist: `.github/scripts/create_issues_from_templates.py`
   - Dieses Skript erstellt Issues aus Templates in `.github/ISSUE_TEMPLATE/`
   - Alle Labels in Templates sollten gegen `labels.yml` validiert werden
   - Gib klare Fehlermeldungen wenn ungültige Labels verwendet werden

6. **Label-Validierung in Python**
   ```python
   # Beispiel: Labels aus labels.yml laden und validieren
   import yaml
   
   with open('.github/labels.yml', 'r') as f:
       valid_labels = {label['name'] for label in yaml.safe_load(f)}
   
   # Validiere Labels
   for label in issue_labels:
       if label not in valid_labels:
           print(f"ERROR: Invalid label '{label}'. Check .github/labels.yml")
   ```

**Best Practices:**
- Jedes Issue sollte mindestens ein Priority- und ein Type-Label haben
- Area-Labels helfen Maintainern, Issues zu routen
- Effort-Labels unterstützen Sprint-Planung
- Status-Labels werden typischerweise von Maintainern gesetzt

**Referenzen:**
- Label-Konfiguration: `.github/labels.yml`
- Vollständiger Guide: `.github/LABELS_GUIDE.md`
- Quick Reference: `.github/LABELS_QUICK_REF.md` (falls vorhanden)

---

## ThemisDB Build-System Übersicht

## ThemisDB Build-System (v2.1 - Moderne Pipeline)

### ✨ Neue Struktur (Jan 2026)

Die Build-Pipeline wurde reorganisiert für bessere Wartbarkeit:

```
themis/
├── CMakeLists.txt                    # Root CMake (delegiert zu cmake/)
├── cmake/                            # ✨ Zentralisiert
│   ├── CMakeLists.txt                # Hauptkonfiguration (2600+ Zeilen)
│   ├── CMakePresets.json             # Alle Build-Profile (MSVC, WSL, Docker)
│   ├── CMakeLists_debug.txt
│   ├── config/                       # Feature-Konfiguration
│   ├── modules/                      # CMake Module
│   └── ModularBuild.cmake
├── docker/                           # ✨ Zentralisiert
│   ├── Dockerfile.themis-server      # Production (LLM + GPU)
│   ├── Dockerfile.minimal            # Minimal Edition
│   ├── Dockerfile.qnap               # QNAP NAS
│   ├── docker-compose-minimal.yml
│   └── .dockerignore
├── docs/build-guide/                 # ✨ Neue Dokumentation
│   ├── README.md                     # Build-Guide Index
│   ├── BUILD_WINDOWS.md              # MSVC Build
│   ├── BUILD_LINUX.md                # WSL/Linux Build
│   ├── BUILD_DOCKER.md               # Docker Multi-Stage
│   ├── BUILD_ARM.md                  # ARM Cross-Compilation
│   ├── BUILD_RASPBERRY_PI.md         # Raspberry Pi (ARMv8)
│   └── BUILD_QNAP.md                 # QNAP NAS (x86_64/ARM)
└── docs/de/                          # Existierende Strategien
    ├── deployment/                   # Deployment-Strategien
    └── releases/                     # Release Management
```

### 🎯 Für Copilot: Pfade aktualisieren

**ALT (Root-Level):**
```cmake
# ❌ Diese existieren nicht mehr im Root!
CMakeLists.txt          → jetzt cmake/CMakeLists.txt
CMakePresets.json       → jetzt cmake/CMakePresets.json
Dockerfile.themis-server → jetzt docker/Dockerfile.themis-server
```

**NEU (Moderne Struktur):**
```cmake
# ✅ Root CMakeLists.txt (einfach)
cmake_minimum_required(VERSION 3.20)
add_subdirectory(cmake)  # Delegiert

# ✅ cmake/CMakeLists.txt (Hauptkonfiguration)
project(Themis VERSION ${_ver} LANGUAGES CXX)
# 2600+ Zeilen Konfiguration

# ✅ cmake/CMakePresets.json (alle Presets)
{
  "configurePresets": [
    { "name": "windows-vs2022-release", ... },
    { "name": "linux-gcc-release", ... },
    { "name": "docker-ninja-release", ... }
  ]
}
```

### Kernprinzip: vcpkg Offline-First Architecture

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

### Quick Start Build-Commands (2026)

Alle Builds verwenden jetzt **CMake Presets** (zentral in `cmake/CMakePresets.json`):

#### Windows (MSVC 2022) - Empfohlen
```powershell
cd C:\VCC\themis

# Mit Preset (empfohlen)
cmake --preset windows-vs2022-release
cmake --build --preset windows-vs2022-release --parallel 8

# Oder manuell (Preset wird automatisch geladen)
cmake -S . -B build-msvc \
  -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GPU=ON

cmake --build build-msvc --config Release --parallel 8
```

**Dokumentation:** [docs/build-guide/BUILD_WINDOWS.md](docs/build-guide/BUILD_WINDOWS.md)  
**Output:** `build-msvc/Release/themis_server.exe`

#### Linux/WSL (GCC/Ninja)
```bash
cd /path/to/themis

# Mit Preset
cmake --preset linux-gcc-release
cmake --build build-wsl --parallel 8

# Oder manuell
cmake -S . -B build-wsl \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=ON

cmake --build build-wsl --parallel $(nproc)
```

**Dokumentation:** [docs/build-guide/BUILD_LINUX.md](docs/build-guide/BUILD_LINUX.md)  
**Output:** `build-wsl/themis_server`

#### Docker (Multi-Arch: x86_64 + ARM64)
```bash
# Dockerfile liegt nun in docker/ Verzeichnis
docker build -f docker/Dockerfile.themis-server \
  -t themis-server:hyperscaler-llm \
  --build-arg THEMIS_ENABLE_LLM=ON \
  --build-arg THEMIS_ENABLE_GPU=ON \
  .
```

**Dokumentation:** [docs/build-guide/BUILD_DOCKER.md](docs/build-guide/BUILD_DOCKER.md)  
**Besonderheit:** Multi-Stage Build (Builder 2.5GB → Runtime 200MB)

#### Raspberry Pi (ARM64)
```bash
cd /path/to/themis

# Native Build auf RPi 4+ mit reduziertem Parallelismus
cmake -S . -B build-rpi \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_BUILD_TESTS=OFF

cmake --build build-rpi --parallel 2
```

**Dokumentation:** [docs/build-guide/BUILD_RASPBERRY_PI.md](docs/build-guide/BUILD_RASPBERRY_PI.md)  
**Output:** `build-rpi/themis_server`

#### QNAP NAS (x86_64 Docker)
```bash
# Docker Build auf QNAP (via docker-compose)
docker build -f docker/Dockerfile.qnap \
  -t themis-server:qnap \
  --build-arg THEMIS_ENABLE_LLM=OFF \
  .
```

**Dokumentation:** [docs/build-guide/BUILD_QNAP.md](docs/build-guide/BUILD_QNAP.md)

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
