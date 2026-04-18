# ThemisDB Build Guide

## 📁 New Directory Structure

Als Teil der Build-Pipeline Modernisierung wurden die Dateien reorganisiert:

```
themis/
├── CMakeLists.txt                    # Root CMake (delegiert zu cmake/)
├── cmake/                            # ✨ NEW: Alle CMAKE-Dateien
│   ├── CMakeLists.txt                # Hauptkonfiguration
│   ├── CMakeLists_debug.txt
│   ├── CMakePresets.json             # Build-Profile (MSVC, WSL, Docker)
│   ├── config/                       # Feature-Konfiguration
│   ├── modules/                      # CMake Module
│   └── ...
├── docker/                           # ✨ NEW: Alle Docker-Dateien
│   ├── Dockerfile.themis-server      # Production Build
│   ├── Dockerfile.minimal            # Minimal Edition
│   ├── docker-compose-minimal.yml
│   └── .dockerignore
├── docs/
│   └── build-guide/                  # ✨ NEW: Build-Dokumentation
│       ├── README.md                 # Index
│       ├── BUILD_WINDOWS.md          # MSVC Build
│       ├── BUILD_LINUX.md            # WSL/Linux Build
│       ├── BUILD_DOCKER.md           # Docker Build
│       └── TROUBLESHOOTING.md        # Fehlersuche
└── scripts/
    ├── build-windows.ps1
    ├── build-linux.sh
    ├── build-docker.sh
    └── ...
```

## � Nächste Schritte nach dem Build

Nach erfolgreichem Build folgen **Deployment** und **Release** - diese sind bereits in `docs/de/` dokumentiert:

### 📦 Deployment-Strategie
- [deployment_strategy.md](../de/deployment/deployment_strategy.md) - **Umfassende Deployment-Strategie** für alle Editionen
- [EDITION_DEPLOYMENT_STRATEGY.md](../de/deployment/EDITION_DEPLOYMENT_STRATEGY.md) - Edition-spezifische Deployment (Community, Professional, Enterprise, Hyperscaler)
- [deployment_docker_multiarch.md](../de/deployment/deployment_docker_multiarch.md) - Multi-Arch Docker Deployment (x86_64, ARM64, ARMv7)
- [deployment_qnap.md](../de/deployment/deployment_qnap.md) - QNAP NAS Deployment (Docker + Native Build)
- [deployment_raspberry_tuning.md](../de/deployment/deployment_raspberry_tuning.md) - Raspberry Pi Tuning & Deployment
- [deployment_arm_build.md](../de/deployment/deployment_arm_build.md) - ARM-spezifische Build & Deployment

### 🔖 Release-Strategie  
- [updates_distribution_strategy.md](../de/releases/updates_distribution_strategy.md) - **Distributions-Strategie** für Releases
- [updates_release_manifest.md](../de/releases/updates_release_manifest.md) - Release Manifest Struktur
- [RELEASE_NOTES_v1.4.md](../de/RELEASE_NOTES_v1.4.md) - Release Notes v1.4

### 🏗️ Versions-Management
- [v1.3.5_RELEASE_BUILD_STRATEGY.md](../de/deployment/v1.3.5_RELEASE_BUILD_STRATEGY.md) - Release Build-Strategie
- [EDITION_CONTROL_MECHANISMS.md](../de/deployment/EDITION_CONTROL_MECHANISMS.md) - Edition Control & Feature Flags

## ✅ Verification

Nach den Umzügen mussten folgende Dateien angepasst werden:

### Option 1: CMake Presets (Empfohlen)

**CMakePresets.json** ist jetzt in `cmake/CMakePresets.json` - CMake findet sie automatisch via `${sourceDir}`:

#### Windows (MSVC)
```bash
cd C:\VCC\themis
cmake --preset windows-vs2022-release
cmake --build --preset windows-vs2022-release --parallel 8
```

#### Linux / WSL
```bash
cd /path/to/themis
cmake --preset linux-gcc-release
cmake --build build-wsl --parallel 8
```

#### Raspberry Pi / ARM
```bash
cd /path/to/themis
cmake -S . -B build-rpi \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_BUILD_TESTS=OFF

cmake --build build-rpi --parallel 2  # Weniger Cores auf RPi
```

#### Docker
```bash
docker build -f docker/Dockerfile.themis-server \
  -t themis-server:hyperscaler-llm \
  --build-arg THEMIS_ENABLE_LLM=ON \
  --build-arg THEMIS_ENABLE_GPU=ON \
  .
```

### Option 2: Manuelle CMake Commands

```bash
# WINDOWS - Explizit
cmake -S . -B build-msvc \
  -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GPU=ON

cmake --build build-msvc --config Release --parallel 8

# LINUX - Explizit
cmake -S . -B build-wsl \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GPU=ON

cmake --build build-wsl --parallel 8
```

### Option 3: PowerShell Scripts

```powershell
# Windows
.\scripts\run-windows-devbuild.ps1

# Build mit Release-Konfiguration
.\scripts\build-windows.ps1
```

### Option 4: Shell Scripts

```bash
# Linux/WSL
./scripts/build-linux.sh
./scripts/build-docker.sh
```

## 📋 Important: Path Changes

### Root-Level Changes
- ❌ `CMakeLists.txt` → ✅ jetzt Delegations-Datei zu `cmake/CMakeLists.txt`
- ❌ `CMakePresets.json` → ✅ jetzt `cmake/CMakePresets.json`
- ❌ `Dockerfile.*` → ✅ jetzt `docker/Dockerfile.*`

### Why This Structure?
1. **Clean Root**: Nur ~20 Dateien statt 40+ im Root
2. **Clear Purpose**: Jedes Verzeichnis hat eine spezifische Rolle
3. **Easier Navigation**: Verwandte Dateien zusammen
4. **Better Onboarding**: Neue Contributor finden Dinge schneller
5. **CI/CD Friendly**: Scripts können gezielt Verzeichnisse referenzieren

## 🔍 Key Files

| File | Purpose |
|------|---------|
| `cmake/CMakePresets.json` | Zentrale Build-Konfiguration (alle Presets) |
| `cmake/CMakeLists.txt` | Hauptkonfiguration (2600+ Zeilen) |
| `cmake/config/` | Feature Flags, Compiler Settings |
| `cmake/modules/` | CMake Module für externe Dependencies |
| `docker/Dockerfile.themis-server` | Production Container (LLM + GPU) |
| `docker/docker-compose-minimal.yml` | Dev Environment |
| `docs/build-guide/` | Build Documentation |

## ✅ Verification

Nach den Umzügen mussten folgende Dateien angepasst werden:

```cmake
# Root CMakeLists.txt (neu)
cmake_minimum_required(VERSION 3.20)
add_subdirectory(cmake)  # ← Delegiert
```

```cmake
# cmake/CMakeLists.txt (angepasst)
# Alter Pfad: include(cmake/ModularBuild.cmake)
# Neuer Pfad:
include(${CMAKE_CURRENT_SOURCE_DIR}/ModularBuild.cmake)
```

CMakePresets.json verwendet `${sourceDir}` (Root), daher keine Änderungen nötig.

## 🚀 Next Steps

1. ✅ Build mit neuem Preset testen:
   ```bash
   cmake --preset windows-vs2022-release
   cmake --build --preset windows-vs2022-release
   ```

2. ✅ Docker Image bauen:
   ```bash
   docker build -f docker/Dockerfile.themis-server \
     -t themis-server:test .
   ```

3. ✅ CI/CD Pipelines updaten (`.github/workflows/*`)

4. ✅ Dokumentation für Contributors aktualisiert in `docs/build-guide/`

## 📖 Weitere Ressourcen

| Platform | Guide | Besonderheiten |
|----------|-------|----------------|
| **Windows** | [BUILD_WINDOWS.md](BUILD_WINDOWS.md) | MSVC, Visual Studio 2022 |
| **Linux / WSL** | [BUILD_LINUX.md](BUILD_LINUX.md) | GCC, Ninja, ccache |
| **Docker** | [BUILD_DOCKER.md](BUILD_DOCKER.md) | Multi-Stage, x86_64 + ARM64 |
| **ARM (Generisch)** | [BUILD_ARM.md](BUILD_ARM.md) | Cross-Compilation, Toolchain |
| **Raspberry Pi** | [BUILD_RASPBERRY_PI.md](BUILD_RASPBERRY_PI.md) | ARMv8, 64-bit, Systemd |
| **QNAP NAS** | [BUILD_QNAP.md](BUILD_QNAP.md) | Docker-preferred, x86_64 + ARM |
| **Troubleshooting** | [../COMPILER_TROUBLESHOOTING.md](../COMPILER_TROUBLESHOOTING.md) | Häufige Build- und Toolchain-Fehler |

- [cmake/CMakePresets.json](../../cmake/CMakePresets.json) - Alle verfügbaren Presets
- [cmake/CMakeLists.txt](../../cmake/CMakeLists.txt) - Konfigurationsdetails
