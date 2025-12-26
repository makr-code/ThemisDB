# ThemisDB Deployment Documentation

**Stand:** 26. Dezember 2025  
**Version:** v1.3.1  
**Kategorie:** 🚀 Deployment  
**Status:** Production-Ready

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Quick Start](#-quick-start)
- [Unterstützte Plattformen](#-unterstützte-plattformen)
- [Build-Varianten](#-build-varianten)

## 📋 Übersicht

ThemisDB nutzt eine **Offline-First vcpkg Build-Strategie** für reproduzierbare Deployments auf allen Plattformen.

### Kern-Dokumente

1. **[Deployment Strategy](deployment_strategy.md)** - Übergeordnete Build & Deployment Strategie
2. **[Bibliotheken-Übersicht](BIBLIOTHEKEN_UBERSICHT.md)** ⭐ **NEU** - Alle Dependencies mit Vendor-Links
3. **[Build-Optionen Referenz](BUILD_OPTIONEN_REFERENZ.md)** ⭐ **NEU** - Alle 61 CMake Schalter
4. **[vcpkg Offline Strategy](VCPKG_OFFLINE_STRATEGY.md)** - Offline-First Build-System
5. **[Docker Build](DOCKER_DEPLOYMENT.md)** - Container-basiertes Deployment
6. **[ARM/Raspberry Pi Build](deployment_arm_build.md)** - ARM64/ARMv7 Builds

---

## 🚀 Quick Start

### Option 1: Docker (Empfohlen)

```bash
# Pull latest multi-arch image (amd64/arm64)
docker pull themisdb/themisdb:latest

# Run with data volume
docker run -d \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themis_data:/data \
  themisdb/themisdb:latest
```

### Option 2: From Source (Offline-First)

```bash
# 1. vcpkg cache setup (einmalig)
./scripts/setup-vcpkg-offline.sh

# 2. Build
cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build -j$(nproc)

# 3. Install
sudo cmake --install build
```

**Siehe:** [vcpkg Offline Strategy](VCPKG_OFFLINE_STRATEGY.md) für Details

---

## 🌍 Unterstützte Plattformen

| Platform | Architecture | Status | Docker | vcpkg Offline | Guide |
|----------|--------------|--------|--------|---------------|-------|
| **Windows 10/11** | x64 | ✅ Production | ❌ | ✅ | [Build Guide](../build/README.md) |
| **Linux (Ubuntu)** | x64 | ✅ Production | ✅ | ✅ | [Deployment Strategy](deployment_strategy.md) |
| **Linux (Ubuntu)** | ARM64 | ✅ Production | ✅ | ✅ | [ARM Build](deployment_arm_build.md) |
| **Raspberry Pi 4/5** | ARM64 | ✅ Supported | ✅ | ✅ | [ARM Build](deployment_arm_build.md) |
| **QNAP NAS** | x64 | ✅ Supported | ✅ | ✅ | [QNAP Deployment](deployment_qnap.md) |
| **macOS** | ARM64 (M1/M2) | 🚧 Planned | ❌ | ✅ | TBD |

---

## 📦 Build-Varianten

ThemisDB bietet verschiedene Build-Konfigurationen für unterschiedliche Use-Cases.

**Siehe:** [Build-Optionen Referenz](BUILD_OPTIONEN_REFERENZ.md) für alle 61 CMake Schalter

### Minimal Build (~150 MB)
```bash
cmake -B build \
  -DTHEMIS_ENABLE_LLM=OFF \
  -DTHEMIS_ENABLE_GRPC=OFF \
  -DCMAKE_BUILD_TYPE=Release
```

### Standard Build mit LLM (~250 MB)
```bash
cmake -B build \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_CORE_SHARED=OFF \
  -DCMAKE_BUILD_TYPE=Release
```

### Performance-Optimiert
```bash
cmake -B build \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_MIMALLOC=ON \
  -DTHEMIS_ENABLE_HUGE_PAGES=ON \
  -DTHEMIS_ENABLE_RCU_INDEX=ON \
  -DCMAKE_BUILD_TYPE=Release
```

### Full Build (~350 MB)
```bash
cmake -B build \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GRPC=ON \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_HTTP2=ON \
  -DCMAKE_BUILD_TYPE=Release
```

**Siehe:** [Deployment Strategy](deployment_strategy.md#build-varianten) für alle Optionen

---

## 🐳 Docker Deployment

```bash
# Pull latest image
docker pull ghcr.io/makr-code/themisdb:latest

# Run with data volume
docker run -d \
  -p 8765:8765 \
  -v /data/themis:/var/lib/themis \
  ghcr.io/makr-code/themisdb:latest
```

### Multi-Arch Build

```bash
# Build for multiple architectures
docker buildx build --platform linux/amd64,linux/arm64 \
  -t ghcr.io/makr-code/themisdb:latest \
  --push .
```

## Native Binary Deployment

```bash
# Download release
wget https://github.com/makr-code/ThemisDB/releases/latest/download/themis-linux-x86_64.tar.gz

# Extract and run
tar -xzf themis-linux-x86_64.tar.gz
./themis_server --config config.yaml
```

## Dokumentation in diesem Ordner

### Kern-Dokumentation (v1.3.1)

| Datei | Beschreibung | Status |
|-------|--------------|--------|
| **[deployment_strategy.md](deployment_strategy.md)** | **Hauptdokument:** Build & Deployment Strategie | ✅ Aktuell |
| **[BIBLIOTHEKEN_UBERSICHT.md](BIBLIOTHEKEN_UBERSICHT.md)** | **Alle Dependencies** mit Vendor-Links | ⭐ NEU |
| **[BUILD_OPTIONEN_REFERENZ.md](BUILD_OPTIONEN_REFERENZ.md)** | **Alle 61 CMake Schalter** mit Beispielen | ⭐ NEU |
| **[VCPKG_OFFLINE_STRATEGY.md](VCPKG_OFFLINE_STRATEGY.md)** | Offline-First Build System | ✅ Aktuell |
| **[DOCKER_DEPLOYMENT.md](DOCKER_DEPLOYMENT.md)** | Docker Container Deployment | ✅ Aktuell |

### Plattform-Spezifisch

| Datei | Beschreibung | Status |
|-------|--------------|--------|
| [deployment_arm_build.md](deployment_arm_build.md) | ARM64/ARMv7 Build-Anleitung | ✅ Aktuell |
| [deployment_arm_benchmarks.md](deployment_arm_benchmarks.md) | ARM Performance-Daten | ✅ Aktuell |
| [deployment_arm_packages.md](deployment_arm_packages.md) | ARM Package-Management | ✅ Aktuell |
| [deployment_qnap.md](deployment_qnap.md) | QNAP NAS Deployment | ✅ Aktuell |
| [deployment_raspberry_tuning.md](deployment_raspberry_tuning.md) | Raspberry Pi Performance Tuning | ✅ Aktuell |
| [QNAP_CPU_COMPATIBILITY.md](QNAP_CPU_COMPATIBILITY.md) | QNAP CPU Support Matrix | ✅ Aktuell |

### CI/CD & Multi-Arch

| Datei | Beschreibung | Status |
|-------|--------------|--------|
| [deployment_docker_multiarch.md](deployment_docker_multiarch.md) | Multi-Arch Docker Builds | ✅ Aktuell |
| [deployment_cicd_multiarch.md](deployment_cicd_multiarch.md) | CI/CD Pipeline-Konfiguration | ✅ Aktuell |
| [docker_build.md](docker_build.md) | Docker Build Details | ✅ Aktuell |
| [docker_status.md](docker_status.md) | Docker Implementation Status | ✅ Aktuell |

### Edition & Strategie

| Datei | Beschreibung | Status |
|-------|--------------|--------|
| [EDITION_DEPLOYMENT_STRATEGY.md](EDITION_DEPLOYMENT_STRATEGY.md) | Multi-Edition Deployment | ✅ Aktuell |
| [EDITION_CONTROL_MECHANISMS.md](EDITION_CONTROL_MECHANISMS.md) | Edition Control Implementation | ✅ Aktuell |
| [v1.3.5_RELEASE_BUILD_STRATEGY.md](v1.3.5_RELEASE_BUILD_STRATEGY.md) | v1.3.5 Release-Planung | 📋 Geplant |
| [80PERCENT_COVERAGE_STRATEGY.md](80PERCENT_COVERAGE_STRATEGY.md) | Community Edition Strategie | ✅ Aktuell |
| [PRICING_MODEL_v1.3.5.md](PRICING_MODEL_v1.3.5.md) | Pricing Model | ✅ Aktuell |

### Implementierungs-Reports

| Datei | Beschreibung | Status |
|-------|--------------|--------|
| [EXECUTIVE_SUMMARY.md](EXECUTIVE_SUMMARY.md) | Executive Summary v1.3.0 | ✅ Archiviert |
| [IMPLEMENTATION_COMPLETED.md](IMPLEMENTATION_COMPLETED.md) | Implementation Status | ✅ Archiviert |
| [PORT_REFERENCE.md](PORT_REFERENCE.md) | Port-Mapping Referenz | ✅ Aktuell |
| [PORT_STANDARDIZATION.md](PORT_STANDARDIZATION.md) | Port-Standardisierung | ✅ Aktuell |

## Verwandte Dokumentation

- [Guides: Deployment](../guides/guides_deployment.md) - Deployment Guide
- [Guides: Build Strategy](../guides/guides_build_strategy.md) - Build Toolchain
- [CI/CD](../build/README.md) - CI/CD Workflows
