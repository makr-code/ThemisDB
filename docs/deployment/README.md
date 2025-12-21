# ThemisDB Deployment Documentation

**Version:** 2.0.0 (Offline-First vcpkg)  
**Last Updated:** 18. Dezember 2025  
**Status:** Production-Ready

---

## 📋 Übersicht

ThemisDB nutzt eine **Offline-First vcpkg Build-Strategie** für reproduzierbare Deployments auf allen Plattformen.

### Kern-Dokumente

1. **[Deployment Strategy](deployment_strategy.md)** - Übergeordnete Build & Deployment Strategie
2. **[vcpkg Offline Strategy](VCPKG_OFFLINE_STRATEGY.md)** ⭐ **NEU** - Offline-First Build-System
3. **[Docker Build](docker_build.md)** - Container-basiertes Deployment
4. **[ARM/Raspberry Pi Build](deployment_arm_build.md)** - ARM64/ARMv7 Builds

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

ThemisDB bietet verschiedene Build-Konfigurationen für unterschiedliche Use-Cases:

### Minimal Build (~150 MB)
```bash
cmake -B build \
  -DTHEMIS_ENABLE_LLM=OFF \
  -DTHEMIS_BUILD_RPC_FRAMEWORK=OFF \
  -DCMAKE_BUILD_TYPE=Release
```

### LLM Build (~250 MB)
```bash
cmake -B build \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_CORE_SHARED=OFF \
  -DCMAKE_BUILD_TYPE=Release
```

### Full Build (~350 MB)
```bash
cmake -B build \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_BUILD_RPC_FRAMEWORK=ON \
  -DTHEMIS_ENABLE_GPU=ON \
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

| Datei | Beschreibung |
|-------|--------------|
| [deployment_strategy.md](deployment_strategy.md) | Deployment-Strategie |
| [deployment_arm_build.md](deployment_arm_build.md) | ARM Build-Anleitung |
| [deployment_arm_benchmarks.md](deployment_arm_benchmarks.md) | ARM Performance |
| [deployment_arm_packages.md](deployment_arm_packages.md) | ARM Packages |
| [deployment_docker_multiarch.md](deployment_docker_multiarch.md) | Multi-Arch Docker |
| [deployment_cicd_multiarch.md](deployment_cicd_multiarch.md) | CI/CD Pipelines |
| [deployment_qnap.md](deployment_qnap.md) | QNAP NAS Deployment |
| [deployment_raspberry_tuning.md](deployment_raspberry_tuning.md) | Raspberry Pi Tuning |
| [docker_build.md](docker_build.md) | Docker Build Guide |
| [docker_status.md](docker_status.md) | Docker Status |
| [QNAP_CPU_COMPATIBILITY.md](QNAP_CPU_COMPATIBILITY.md) | QNAP CPU Support |

## Verwandte Dokumentation

- [Guides: Deployment](../guides/guides_deployment.md) - Deployment Guide
- [Guides: Build Strategy](../guides/guides_build_strategy.md) - Build Toolchain
- [CI/CD](../cicd/README.md) - CI/CD Workflows
