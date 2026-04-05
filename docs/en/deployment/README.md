# ThemisDB Deployment Documentation

**Date:** April 5, 2026  
**Version:** v1.8.1-rc1  
**Category:** 🚀 Deployment  
**Status:** Production-Ready

---

## 📑 Table of Contents

- [Overview](#-overview)
- [Quick Start](#-quick-start)
- [Supported Platforms](#-supported-platforms)
- [Build Variants](#-build-variants)
- [Docker Deployment](#-docker-deployment)
- [Native Binary Deployment](#-native-binary-deployment)
- [Documentation in This Directory](#documentation-in-this-directory)
- [Related Documentation](#related-documentation)

## 📋 Overview

ThemisDB uses an **Offline-First vcpkg build strategy** for reproducible deployments across all platforms.

### Core Documents

1. **[Deployment Strategy](../../de/deployment/deployment_strategy.md)** - Overall build & deployment strategy
2. **[vcpkg Offline Strategy](../../de/deployment/VCPKG_OFFLINE_STRATEGY.md)** ⭐ **NEW** - Offline-first build system
3. **[Docker Build](../../de/deployment/docker_build.md)** - Container-based deployment
4. **[ARM/Raspberry Pi Build](../../de/deployment/deployment_arm_build.md)** - ARM64/ARMv7 builds

---

## 🚀 Quick Start

### Option 1: Docker (Recommended)

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
# 1. vcpkg cache setup (one-time)
./scripts/setup-vcpkg-offline.sh

# 2. Build
cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build -j$(nproc)

# 3. Install
sudo cmake --install build
```

**See:** [vcpkg Offline Strategy](../../de/deployment/VCPKG_OFFLINE_STRATEGY.md) for details

---

## 🌍 Supported Platforms

| Platform | Architecture | Status | Docker | vcpkg Offline | Guide |
|----------|--------------|--------|--------|---------------|-------|
| **Windows 10/11** | x64 | ✅ Production | ❌ | ✅ | [Build Guide](../../de/build/README.md) |
| **Linux (Ubuntu)** | x64 | ✅ Production | ✅ | ✅ | [Deployment Strategy](../../de/deployment/deployment_strategy.md) |
| **Linux (Ubuntu)** | ARM64 | ✅ Production | ✅ | ✅ | [ARM Build](../../de/deployment/deployment_arm_build.md) |
| **Raspberry Pi 4/5** | ARM64 | ✅ Supported | ✅ | ✅ | [ARM Build](../../de/deployment/deployment_arm_build.md) |
| **QNAP NAS** | x64 | ✅ Supported | ✅ | ✅ | [QNAP Deployment](../../de/deployment/deployment_qnap.md) |
| **macOS** | ARM64 (M1/M2) | 🚧 Planned | ❌ | ✅ | TBD |

---

## 📦 Build Variants

ThemisDB offers different build configurations for various use cases:

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

**See:** [Deployment Strategy](../../de/deployment/deployment_strategy.md#build-varianten) for all options

---

## 🐳 Docker Deployment

```bash
# Pull latest image
docker pull themisdb/themisdb:latest

# Run with data volume
docker run -d \
  -p 8765:8765 \
  -v /data/themis:/var/lib/themis \
  themisdb/themisdb:latest
```

> Community releases are published to Docker Hub (`themisdb/themisdb`) via
> `.github/workflows/04-release_publish-community.yml`.

### Multi-Arch Build

```bash
# Build for multiple architectures
docker buildx build --platform linux/amd64,linux/arm64 \
  -t themisdb/themisdb:latest \
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

## Documentation in This Directory

| File | Description |
|------|-------------|
| [deployment_strategy.md](../../de/deployment/deployment_strategy.md) | Deployment strategy |
| [deployment_arm_build.md](../../de/deployment/deployment_arm_build.md) | ARM build guide |
| [deployment_arm_benchmarks.md](../../de/deployment/deployment_arm_benchmarks.md) | ARM performance |
| [deployment_arm_packages.md](../../de/deployment/deployment_arm_packages.md) | ARM packages |
| [deployment_docker_multiarch.md](../../de/deployment/deployment_docker_multiarch.md) | Multi-arch Docker |
| [deployment_cicd_multiarch.md](../../de/deployment/deployment_cicd_multiarch.md) | CI/CD pipelines |
| [deployment_qnap.md](../../de/deployment/deployment_qnap.md) | QNAP NAS deployment |
| [deployment_raspberry_tuning.md](../../de/deployment/deployment_raspberry_tuning.md) | Raspberry Pi tuning |
| [docker_build.md](../../de/deployment/docker_build.md) | Docker build guide |
| [docker_status.md](../../de/deployment/docker_status.md) | Docker status |
| [QNAP_CPU_COMPATIBILITY.md](../../de/deployment/QNAP_CPU_COMPATIBILITY.md) | QNAP CPU support |

## Related Documentation

- [Guides: Deployment](../guides/guides_deployment.md) - Deployment guide
- [Guides: Build Strategy](../../de/guides/guides_build_strategy.md) - Build toolchain
- [CI/CD](../../de/build/README.md) - CI/CD workflows

---

> **Note:** Most detailed deployment documentation is currently available in German. English translations are in progress.  
> For the most up-to-date information, please refer to the [German deployment documentation](../../de/deployment/).

**Version:** 1.8.1-rc1 | **License:** MIT | **Support:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
