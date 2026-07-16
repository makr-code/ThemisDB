---
category: "🔨 Build/Deployment"
version: "v1.4.0"
status: "✅"
date: "23.01.2026"
---

# 🔨 ThemisDB Build Guide

Schnelleinstieg für das Bauen von ThemisDB auf allen Plattformen.

## 📋 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Choose Your Edition First](#-choose-your-edition-first)
- [Quick Start (5 min)](#-quick-start-5-min)
- [Platform-Specific Builds](#-platform-specific-builds)
- [Build Scripts](#-build-scripts)
- [Architecture & Details](#-architecture--details)
- [Troubleshooting](#-troubleshooting)
- [Siehe auch](#-siehe-auch)

---

## 📋 Übersicht

ThemisDB bietet flexible Build-Optionen für alle Plattformen:

**Plattformen:**
- 🪟 **Windows** - MSVC/ClangCL (VS 2022)
- 🐧 **Linux** - GCC/Clang
- 🐳 **Docker** - Multi-arch builds
- 🦾 **ARM** - Raspberry Pi, Graviton

**Stand:** 23. April 2026  
**Version:** v1.4.0  

---

## 🎯 Choose Your Edition First!

**WICHTIG:** Wählen Sie zuerst Ihre Edition aus!

### Quick Edition Comparison

| Edition | Max Nodes | GPU VRAM | License | Use Case |
|---------|-----------|----------|---------|----------|
| **MINIMAL** | 1 | 0 GB | Optional | Embedded, IoT |
| **COMMUNITY** | **5** ✅ | 24 GB | Optional | Dev, Test, Startups |
| **ENTERPRISE** | **100** ✅ | 256 GB | Required (Release) ⚠️ | Production |
| **HYPERSCALER** | ∞ | ∞ | Mandatory ❌ | Enterprise Scale |

**Siehe:** [Edition Limits Matrix](../deployment/EDITION_LIMITS_MATRIX.md) für vollständigen Vergleich.

### License Requirements

**Brauche ich eine Lizenz?**

- ✅ **MINIMAL/COMMUNITY:** Keine Lizenz erforderlich (Open Source)
- ⚠️ **ENTERPRISE:** Lizenz erforderlich für **Release** Builds (Debug optional)
- ❌ **HYPERSCALER:** Lizenz **immer** erforderlich (auch Debug)

**Siehe:** [License Requirements](../deployment/LICENSE_REQUIREMENTS.md) für Details.

---

## 🚀 Quick Start (5 min)

### Community Edition (Most Common)

```bash
# Windows
.\scripts\build-community-release.ps1 -Platform windows -Configuration Release

# Linux
./scripts/build-linux.sh --config release --edition COMMUNITY --llm --gpu

# Docker
.\scripts\build-docker.ps1 -Tag themisdb:latest -Edition COMMUNITY
```

**Output:** `themis_server.exe` (Windows) oder `themis_server` (Linux)

### Enterprise Edition (License Required for Release)

```bash
# Windows - Release (License Required!)
.\scripts\build-enterprise-release.ps1 `
  -Environment production `
  -Configuration Release `
  -LicenseFile "C:\licenses\enterprise-license.json"

# Windows - Debug (No License Required)
.\scripts\build-enterprise-release.ps1 `
  -Environment development `
  -Configuration Debug
```

**Siehe:** [License Requirements](../deployment/LICENSE_REQUIREMENTS.md)

---

## 🖥️ Platform-Specific Builds

### Windows Build (MSVC 2022)

```powershell
# Option 1: Use Build Script (Recommended)
.\scripts\build-community-release.ps1 -Platform windows -Configuration Release

# Option 2: Manual CMake
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DTHEMIS_EDITION=COMMUNITY `
  -DCMAKE_BUILD_TYPE=Release `
  -DTHEMIS_ENABLE_LLM=ON `
  -DTHEMIS_ENABLE_GPU=ON

cmake --build build --config Release --parallel 8
```

**Output:** `build/Release/themis_server.exe`

**Siehe:** [Platform-specific docs](BUILD_WINDOWS.md)

### Linux Build

```bash
# Option 1: Use Build Script (Recommended)
./scripts/build-linux.sh --config release --edition COMMUNITY

# Option 2: Manual CMake
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DTHEMIS_EDITION=COMMUNITY \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GPU=ON

cmake --build build -j$(nproc)
```

**Output:** `build/themis_server`

**Siehe:** [Linux Build Guide](BUILD_LINUX.md)

### Docker Build

```bash
# Single platform
.\scripts\build-docker.ps1 -Tag themisdb:latest -Edition COMMUNITY

# Multi-arch (x86_64 + ARM64)
.\scripts\build-docker.ps1 `
  -Tag themisdb:1.4.0 `
  -Edition COMMUNITY `
  -Platform "linux/amd64,linux/arm64" `
  -Push
```

**Siehe:** [Docker Guide](BUILD_DOCKER.md)

### ARM64 / Raspberry Pi (Cross-Compile)

```bash
# Cross-compile from x86_64 host
cmake -B build-arm64 -S . \
  -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=cmake/toolchains/arm64-linux-gnu.cmake \
  -DVCPKG_TARGET_TRIPLET=arm64-linux \
  -DTHEMIS_EDITION=MINIMAL \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-arm64 --parallel 8
```

**Siehe:** [Cross-Compile Complete Guide](CROSS_COMPILE_COMPLETE.md)

---

## 🔧 Build Scripts

ThemisDB bietet automatisierte Build-Scripts für alle Szenarien:

### Edition-Specific Scripts

| Script | Edition | License Required |
|--------|---------|-----------------|
| `build-community-release.ps1` | COMMUNITY | ❌ No |
| `build-enterprise-release.ps1` | ENTERPRISE | ⚠️ Yes (Release only) |
| `build-hyperscaler-release.ps1` | HYPERSCALER | ❌ Yes (always) |

**Siehe:** [Build Scripts Reference](BUILD_SCRIPTS_REFERENCE.md) für vollständige Dokumentation.

---

## 📚 Architecture & Details

### Build System Architecture

ThemisDB nutzt ein **3-Tier CMake Build System**:

1. **Platform Detection** - OS, CPU, Cross-Compile
2. **Edition System** - MINIMAL, COMMUNITY, ENTERPRISE, HYPERSCALER
3. **Feature System** - 60+ CMake Options

**Siehe:** [CMake Build System Overview](../deployment/CMAKE_BUILD_SYSTEM_OVERVIEW.md)

### Build Options Reference

60+ CMake-Optionen für granulare Konfiguration:

- **Editions:** THEMIS_EDITION
- **Features:** THEMIS_ENABLE_LLM, THEMIS_ENABLE_GPU, THEMIS_ENABLE_GRPC
- **Performance:** THEMIS_ENABLE_MIMALLOC, THEMIS_ENABLE_HUGE_PAGES
- **License:** THEMIS_LICENSE_FILE

**Siehe:** [Build Options Reference](../deployment/BUILD_OPTIONEN_REFERENZ.md)

---

## 🔧 Troubleshooting

### Problem: License File Not Found

```
ERROR: License file not found: /path/to/license.json
```

**Solution:**
```bash
# Use absolute path
cmake -B build -S . -DTHEMIS_LICENSE_FILE=/absolute/path/to/license.json
```

**Siehe:** [License Requirements](../deployment/LICENSE_REQUIREMENTS.md) - Troubleshooting

### Problem: CMake Configuration Fails

```
ERROR: vcpkg not found
```

**Solution:**
```bash
# Set VCPKG_ROOT
export VCPKG_ROOT=/path/to/vcpkg
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
```

### Problem: Build Too Slow

**Solution:**
```bash
# Use Ninja (faster than Make)
cmake -B build -S . -GNinja
ninja -C build -j$(nproc)

# Enable ccache
cmake -B build -S . -DCMAKE_C_COMPILER_LAUNCHER=ccache
```

**Siehe:** [Cross-Compile Guide](CROSS_COMPILE_COMPLETE.md) - Performance Optimization

---

## 📚 Siehe auch

### Build & Deployment
- [CMake Build System Overview](../deployment/CMAKE_BUILD_SYSTEM_OVERVIEW.md) - Zentrale Architektur-Referenz
- [Build Scripts Reference](BUILD_SCRIPTS_REFERENCE.md) - Automatisierte Build-Scripts
- [Cross-Compile Complete Guide](CROSS_COMPILE_COMPLETE.md) - Cross-Compilation für alle Szenarien
- [Build Options Reference](../deployment/BUILD_OPTIONEN_REFERENZ.md) - 60+ CMake-Optionen
- [Deployment Strategy](../deployment/deployment_strategy.md) - Deployment-Strategien

### Editions & Licensing
- [Edition Limits Matrix](../deployment/EDITION_LIMITS_MATRIX.md) - **Single Source of Truth** für Limits
- [License Requirements](../deployment/LICENSE_REQUIREMENTS.md) - Wann ist Lizenz erforderlich?
- [Edition Control Mechanisms](../deployment/EDITION_CONTROL_MECHANISMS.md) - Technische Details

### Platform-Specific
- [Windows Build](BUILD_WINDOWS.md) - Windows-spezifische Details
- [Linux Build](BUILD_LINUX.md) - Linux-spezifische Details
- [Docker Build](BUILD_DOCKER.md) - Docker & Container
- [ARM Deployment](../deployment/deployment_arm_build.md) - Raspberry Pi & ARM

### Advanced Topics
- [LLM Complete Setup](LLM_COMPLETE_SETUP_GUIDE.md) - LLM Integration
- [GPU Support](../DOCKER_GPU_SUPPORT.md) - GPU Konfiguration
- [vcpkg Offline Strategy](../deployment/VCPKG_OFFLINE_STRATEGY.md) - Offline Builds

---

**Letzte Aktualisierung:** 23. April 2026  
**Version:** v1.4.0  
**Kategorie:** 🔨 Build/Deployment  
**Kontakt:** service@themisdb.org
