# Deployment Documentation

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Deployment

---

## Übersicht

ThemisDB unterstützt Multi-Architektur-Deployment (x86_64, ARM64) mit Docker und nativen Binaries.

## Unterstützte Plattformen

| Platform | Architecture | Status | Docker |
|----------|--------------|--------|--------|
| Linux (Ubuntu) | x86_64 | ✅ Production | ✅ |
| Linux (Ubuntu) | ARM64 | ✅ Production | ✅ |
| macOS | ARM64 (M1/M2) | ✅ Production | ❌ |
| Windows | x86_64 | ✅ Production | ❌ |
| Raspberry Pi 4/5 | ARM64 | ✅ Supported | ✅ |
| QNAP NAS | ARM64 | ✅ Supported | ✅ |

## Docker Deployment

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
