# ThemisDB Docker Deployment Guide

**Stand:** 5. April 2026  
**Version:** v1.8.1-rc1  
**Kategorie:** 🚀 Deployment  
**Status:** Production-Ready

---

## 📑 Inhaltsverzeichnis

- [Quick Start](#quick-start)
- [Docker Images](#docker-images)
- [Build Strategies](#build-strategies)
- [Configuration](#configuration)
- [Docker Compose](#docker-compose)
- [Production Deployment](#production-deployment)
- [Platform-Specific](#platform-specific-deployment)
- [Troubleshooting](#troubleshooting)
- [Related Documentation](#related-documentation)

## Quick Start

### Pull & Run (Docker Hub)

> Aktiver Community-Release-Pfad: Docker Hub `themisdb/themisdb`
> (publiziert durch `.github/workflows/04-release_publish-community.yml`).

```bash
# Latest version
docker pull themisdb/themisdb:latest
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Specific release tag (example: 1.8.1-rc1)
docker pull themisdb/themisdb:1.8.1-rc1
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themis_data:/data \
  themisdb/themisdb:1.8.1-rc1
```

### Quick Start with GPU Support

GPU-spezifische Tags koennen release-abhaengig bereitgestellt werden.

```bash
# Pull GPU-enabled image (example tag)
docker pull themisdb/themisdb:<version>-gpu

# Run with NVIDIA GPU support
docker run -d \
  --name themis-gpu \
  --gpus all \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themis_data:/data \
  -v themis_models:/models \
  themisdb/themisdb:<version>-gpu

# Verify GPU access
docker exec themis-gpu nvidia-smi
```

**Requirements:**
- NVIDIA GPU with CUDA support
- [NVIDIA Container Toolkit](https://github.com/NVIDIA/nvidia-docker) installed
- Docker 19.03+ with GPU support

**Performance:** 100x faster inference vs CPU-only mode


### Verify Running

```bash
# Check container status
docker ps | grep themis

# Health check
curl http://localhost:18765/health
# Expected: 200 OK with JSON response

# View logs
docker logs -f themis
```

---

## Docker Images

### Available Tags

| Tag | Architecture | Status | Use Case |
|-----|--------------|--------|----------|
| `latest` | amd64 + arm64 | ✅ Production | Empfohlener Community-Tag |
| `1.8.1-rc1` | amd64 + arm64 | ✅ Production | Aktueller Release-Candidate |
| `<version>-gpu` | amd64 | ✅ Optional | Mit CUDA-Unterstuetzung (falls publiziert) |
| `qnap` | amd64 | ✅ Production | QNAP NAS optimized (Ubuntu 20.04, SSE4.2 baseline) |
| `v1.3.0`, `v1.2.0`, `v1.0.2`, `v1` | amd64 + arm64 | 🗂️ Legacy | Historische Kompatibilitaets-Tags |

### Image Specs

**Registry:** `docker.io/themisdb/themisdb`

**Multi-Architecture Support:**
- `linux/amd64` - Intel/AMD x64 processors
- `linux/arm64` - ARM v8 processors (RPi, Apple Silicon, AWS Graviton, etc.)

**Image Size:** ~150MB (compressed)

**Build Configuration:**
```dockerfile
FROM ubuntu:22.04
VCPKG_ENABLE_ONLINE=OFF          # No internet access during build
VCPKG_TRIPLET=x64-linux          # For amd64
VCPKG_TRIPLET=arm64-linux        # For arm64
```
**Hinweis:** Die lokale Quelle `llama.cpp/` im Projekt‑Root ist per `.dockerignore` ausgeschlossen und wird nicht in das Build‑Context kopiert. Die LLM‑Funktionalität wird über die kompilierten Artefakte (ggml/llama) bereitgestellt; Modelle sollten als Volume (`/models`) gemountet werden.

---

## Build Strategies

ThemisDB verwendet einen **Hybrid Pre-built Binary** Workflow für Docker-Builds:

### Empfohlene Strategie: Pre-Built Binaries

1. **Binary lokal bauen** (einmalig, ~30-40 Minuten mit vcpkg)
2. **Docker-Image erstellen** mit `Dockerfile.simple` (schnell, ~30 Sekunden)
3. **Ergebnis**: Kleine Images (~100-200 MB), 100% offline-fähig

**Vorteile:**
- ✅ Schnelle Build-Zeiten (Sekunden statt Minuten)
- ✅ Kleine Image-Größe (~150 MB komprimiert)
- ✅ 100% Offline-fähig nach initialem Setup
- ✅ Monolithische Binary (keine Library-Abhängigkeiten)
- ✅ Health-Check für Container-Orchestrierung

### Multi-Architecture Support

ThemisDB bietet native Unterstützung für mehrere Architekturen:

| Plattform | Architektur | vcpkg Triplet | Primärer Use Case |
|-----------|-------------|---------------|-------------------|
| **AMD64** | x86_64 | `x64-linux` | Server, Cloud (AWS/Azure/GCP) |
| **ARM64** | aarch64 | `arm64-linux` | Raspberry Pi 4/5, ARM-Server (Graviton) |
| **QNAP** | x86_64 | `x64-linux` | QNAP NAS (optimiert für ältere CPUs) |

**Details:** Siehe [Multi-Arch Build Strategy](deployment_docker_multiarch.md)

### Build Scripts

#### Schnellstart (Empfohlen)

```powershell
# Windows: Schnelles Build & Push
.\scripts\quick-docker-deploy.ps1

# Mit Binär-Rebuild
.\scripts\quick-docker-deploy.ps1 -BuildBinary -Push

# Spezifische Version
.\scripts\quick-docker-deploy.ps1 -Tag "1.3.4" -Push
```

#### Detailliertes Docker Build

```powershell
# Windows: Standard Docker Build
.\scripts\build-docker.ps1 -Tag "1.3.4"

# Mit Binary-Build (vollständig)
.\scripts\build-docker.ps1 -BuildBinary -Tag "1.3.4"

# Multi-Arch Build & Push zu Docker Hub
.\scripts\build-docker.ps1 -Platforms "linux/amd64,linux/arm64" -Tag "1.3.4" -Push

# Mit vorgebautem Dockerfile (schneller)
.\scripts\build-docker.ps1 -Dockerfile "Dockerfile.simple" -Tag "1.3.4" -Push
```

```bash
# Linux/macOS: Docker Build
TAG=1.3.4 ./scripts/build-docker.sh

# Mit Binary-Build
TAG=1.3.4 BUILD_BINARY=true ./scripts/build-docker.sh

# Multi-arch + Push
TAG=1.3.4 PLATFORMS="linux/amd64,linux/arm64" PUSH=true ./scripts/build-docker.sh
```

#### Vollständiger Release-Pipeline

```powershell
# Alle Plattformen bauen (Windows, Linux, Docker)
.\scripts\build.ps1 -Target all -Push

# Nur Docker
.\scripts\build.ps1 -Target docker -Push -Platforms "linux/amd64,linux/arm64"
```

---

## Configuration

### Environment Variables

```bash
docker run -e THEMIS_CONFIG_PATH=/etc/themis/config.json \
           -e THEMIS_PORT=18765 \
           -e LD_LIBRARY_PATH=/usr/local/lib/themisdb:/usr/local/lib \
           themisdb/themisdb:latest
```

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_CONFIG_PATH` | `/etc/themis/config.json` | Configuration file path |
| `THEMIS_PORT` | `18765` | Internal port (mapped via -p) |
| `LD_LIBRARY_PATH` | `/usr/local/lib/themisdb:/usr/local/lib` | Runtime library path |

### Custom Config

```bash
# Mount custom config
docker run -d \
  -v /path/to/config.json:/etc/themis/config.json:ro \
  themisdb/themisdb:latest
```

---

## Volumes

### Data Persistence

```bash
# Named volume (recommended)
docker volume create themis_data
docker run -d \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Bind mount (for development)
docker run -d \
  -v /local/data/path:/data \
  themisdb/themisdb:latest
```

### Directories in Container

| Path | Purpose | Persistence |
|------|---------|-------------|
| `/data` | Database storage | ✅ Volume |
| `/etc/themis` | Configuration | ✅ Config mount |
| `/var/log/themis` | Application logs | ✅ Volume |

---

## Networking

### Port Mapping

```bash
docker run -d \
  -p 8080:8080 \                # REST API (HTTP/HTTP/2)
  -p 18765:18765 \              # Internal protocol
  -p 9090:9090 \                # WebSocket
  -p 1883:1883 \                # MQTT
  -p 5432:5432 \                # PostgreSQL Wire Protocol
  themisdb/themisdb:latest
```

| Port | Protocol | Purpose | Default | Version |
|------|----------|---------|---------|---------|
| `8080` | HTTP/HTTP/2 | REST API, Web UI, Server Push | Required | v1.0+ |
| `18765` | Custom | Binary protocol | Required | v1.0+ |
| `9090` | WebSocket | Real-time CDC streaming | Optional | v1.3.0+ |
| `1883` | MQTT | Broker with WebSocket transport | Optional | v1.3.0+ |
| `5432` | PostgreSQL | Wire Protocol (SQL-to-Cypher) | Optional | v1.3.0+ |
| `3000` | MCP | Model Context Protocol | Optional | v1.3.0+ |

### Network Modes

```bash
# Host network (performance, less isolation)
docker run --network host themisdb/themisdb:latest

# Bridge network (default, recommended)
docker run --network bridge themisdb/themisdb:latest

# Custom network
docker network create themis_net
docker run --network themis_net themisdb/themisdb:latest
```

---

## Docker Compose (Dateien unter `docker/compose/`)

### Basic Setup

```yaml
# docker-compose.yml
version: '3.8'

services:
  themis:
    image: themisdb/themisdb:latest
    container_name: themis
    ports:
      - "8080:8080"
      - "18765:18765"
    volumes:
      - themis_data:/data
      - ./config/config.json:/etc/themis/config.json:ro
    environment:
      THEMIS_PORT: 18765
      THEMIS_CONFIG_PATH: /etc/themis/config.json
    restart: unless-stopped
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:18765/health"]
      interval: 10s
      timeout: 5s
      retries: 3

volumes:
  themis_data:
    driver: local
```

**Start (Repo-Path beachten):**
```bash
docker compose -f docker/compose/docker-compose.yml up -d
docker compose -f docker/compose/docker-compose.yml logs -f
```

### Multi-Service Stack

```yaml
version: '3.8'

services:
  themis:
    image: themisdb/themisdb:latest
    ports:
      - "8080:8080"
      - "18765:18765"
    volumes:
      - themis_data:/data
    networks:
      - themis_net

  # Optional: monitoring
  prometheus:
    image: prom/prometheus:latest
    ports:
      - "9090:9090"
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml:ro
    networks:
      - themis_net

networks:
  themis_net:
    driver: bridge

volumes:
  themis_data:
```

---

## Production Deployment

### Resource Allocation

```bash
docker run -d \
  --cpus="4" \                          # 4 CPU cores
  --memory="8g" \                       # 8GB RAM
  --memory-swap="10g" \                 # 10GB with swap
  -v themis_data:/data \
  themisdb/themisdb:latest
```

### Restart Policies

```bash
# Always restart
docker run --restart=always themisdb/themisdb:latest

# Restart unless manually stopped
docker run --restart=unless-stopped themisdb/themisdb:latest

# Restart with max retry count
docker run --restart=on-failure:5 themisdb/themisdb:latest
```

### Logging

```bash
# JSON file driver (max size limit)
docker run -d \
  --log-driver json-file \
  --log-opt max-size=10m \
  --log-opt max-file=3 \
  themisdb/themisdb:latest

# Syslog driver
docker run -d \
  --log-driver syslog \
  --log-opt syslog-address=udp://localhost:514 \
  themisdb/themisdb:latest

# View logs
docker logs --tail 100 --follow themis
```

---

## Platform-Specific Deployment

### Linux (x64)

```bash
# Ubuntu 22.04+
docker run -d \
  --platform linux/amd64 \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Debian
apt-get install docker.io
docker pull themisdb/themisdb:latest
```

### ARM/Raspberry Pi

```bash
# Auto-detects ARM64
docker pull themisdb/themisdb:latest

# Explicit pull
docker pull --platform linux/arm64 themisdb/themisdb:latest

# Run
docker run -d \
  --platform linux/arm64 \
  -v themis_data:/data \
  themisdb/themisdb:latest
```

### QNAP NAS

```bash
# Pull QNAP-optimized image (Ubuntu 20.04, SSE4.2 baseline)
docker pull themisdb/themisdb:qnap

# Run on QNAP (port 18765 to avoid conflicts)
docker run -d \
  --name themis \
  -p 18765:18765 \
  -v /share/Container/themis/data:/data \
  -v /share/Container/themis/config/config.qnap.json:/etc/themis/config.json:ro \
  themisdb/themisdb:qnap

# Or use docker-compose.qnap.yml
# See docker/docker-compose.qnap.yml for full setup
```

**QNAP Notes:**
- Use `qnap` or `v1.0.2-qnap` tags (optimized for older CPUs)
- Default port 18765 avoids QNAP service conflicts
- Mount volumes to `/share/Container/themis/`
- Requires GLIBC 2.31+ (QNAP QTS 5.0+)

### macOS (Apple Silicon/Intel)

```bash
# Auto-selects correct architecture
docker pull themisdb/themisdb:latest

# Explicitly specify
docker pull --platform linux/arm64 themisdb/themisdb:latest  # M-series
docker pull --platform linux/amd64 themisdb/themisdb:latest  # Intel
```

### Windows (Docker Desktop)

```powershell
# Pull image
docker pull themisdb/themisdb:latest

# Run
docker run -d `
  -p 8080:8080 `
  -p 18765:18765 `
  -v themis_data:C:\data `
  themisdb/themisdb:latest

# View logs
docker logs -f themis
```

---

## Troubleshooting

### Container Fails to Start

```bash
# Check logs
docker logs themis

# Common issues:
# 1. Port already in use
docker ps  # Find conflicting container
docker stop <container_id>

# 2. Insufficient disk space
docker system df  # Check usage

# 3. Broken config
docker exec themis cat /etc/themis/config.json
```

### Performance Issues

```bash
# Monitor container stats
docker stats themis

# Check resource limits
docker inspect themis | grep -i memory

# Increase memory allocation
docker update --memory 16g themis
docker restart themis
```

### Network Connectivity

```bash
# Test from host
curl http://localhost:8080/api/health

# Test from within container
docker exec themis curl http://localhost:18765/health

# Check port binding
docker port themis
```

### Library Path Issues

```bash
# Verify libraries are loaded
docker exec themis ldd /usr/local/bin/themis_server

# Check library path
docker exec themis echo $LD_LIBRARY_PATH

# Rebuild with updated lib path if needed
docker pull themisdb/themisdb:latest --force
```

---

## Build Your Own Image (Advanced)

### Build from Source

```bash
# Clone repo
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Build multi-arch (requires buildx setup)
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t themis:custom:latest .

# Build single-arch
docker build \
  -f Dockerfile \
  -t themis:custom:latest .
```

### Build Arguments

```dockerfile
# Customize build
docker build \
  --build-arg VCPKG_ENABLE_ONLINE=OFF \
  --build-arg THEMIS_VERSION=1.0.2 \
  -t themis:custom .
```

### QNAP-Specific Build

```bash
# Build QNAP-optimized image (Ubuntu 20.04, baseline CPU)
docker build \
  -f docker/Dockerfile.qnap \
  -t themis:qnap \
  .

# Tag and push
docker tag themis:qnap themisdb/themisdb:qnap
docker push themisdb/themisdb:qnap
```

---

## Best Practices

✅ **DO:**
- Use named volumes for data persistence
- Set resource limits (CPU, memory)
- Use health checks
- Enable restart policies
- Log to syslog or json-file with rotation
- Use specific version tags (not just `latest`)
- Run as non-root (built-in)
- Mount config as read-only

❌ **DON'T:**
- Run containers with `--privileged`
- Use `latest` tag in production (use specific versions)
- Store secrets in environment variables
- Ignore health check failures
- Disable restart policies
- Map unnecessary ports

---

## Support & Issues

**Docker Hub:** https://hub.docker.com/r/themisdb/themisdb

**GitHub Issues:** https://github.com/makr-code/ThemisDB/issues

**Deployment Strategy:** [Full Deployment Strategy](deployment_strategy.md)

---

## Related Documentation

- [Deployment Strategy](deployment_strategy.md) - Overall build & deployment strategy
- [Multi-Arch Docker Strategy](deployment_docker_multiarch.md) - Detailed multi-architecture build guide
- [vcpkg Offline Strategy](VCPKG_OFFLINE_STRATEGY.md) - Offline-first dependency management
- [QNAP Deployment](deployment_qnap.md) - QNAP NAS specific deployment
- [ARM/Raspberry Pi Build](deployment_arm_build.md) - ARM-specific build instructions
- [README.md](README.md) - Main deployment documentation index

### Archived Documentation

Historical Docker build documentation (status reports, alternative approaches) available in [archive/](archive/).
