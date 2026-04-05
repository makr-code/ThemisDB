# Docker Build Guide

## Voraussetzungen

- **Docker** 20.10+
  ```bash
  docker --version
  ```

- **Docker Buildkit** aktiviert (für Multi-Stage Builds)
  ```bash
  export DOCKER_BUILDKIT=1
  ```

- **Disk Space**: ~5GB für Build, ~2GB für finale Image

## Quick Start

### Build Production Image (Recommended)

```bash
cd /path/to/themis

docker build -f docker/Dockerfile.themis-server \
  -t themis-server:hyperscaler-llm \
  --build-arg THEMIS_ENABLE_LLM=ON \
  --build-arg THEMIS_ENABLE_GPU=ON \
  --build-arg CMAKE_BUILD_TYPE=Release \
  .
```

### Build Minimal Image

```bash
docker build -f docker/Dockerfile.minimal \
  -t themis-server:minimal \
  --build-arg CMAKE_BUILD_TYPE=Release \
  .
```

## Build Arguments

### Hyperscaler (LLM + GPU)
```dockerfile
--build-arg THEMIS_ENABLE_LLM=ON       # LLM Support (llama.cpp)
--build-arg THEMIS_ENABLE_GPU=ON       # GPU Acceleration
--build-arg CMAKE_BUILD_TYPE=Release   # Release Config
```

### Community (Standard)
```dockerfile
--build-arg THEMIS_ENABLE_LLM=OFF      # LLM disabled
--build-arg THEMIS_ENABLE_GPU=OFF      # GPU disabled
--build-arg CMAKE_BUILD_TYPE=Release   # Release Config
```

### Encrypted Storage (optional, OFF by default)

```dockerfile
--build-arg THEMIS_ENABLE_ENCRYPTED_STORAGE=ON   # gocryptfs + fuse einschließen
```

> [!WARNING]
> `THEMIS_ENABLE_ENCRYPTED_STORAGE` ist standardmäßig `OFF`. Das Aktivieren installiert
> `gocryptfs` (Go-Binär) und `fuse` ins Runtime-Image und bringt Go-stdlib-Abhängigkeiten
> mit, die bekannte CVEs tragen können. Nur aktivieren, wenn At-Rest-Verschlüsselung via
> gocryptfs explizit benötigt wird. Alternativer Ansatz: Verschlüsselung auf Storage-Ebene
> (dm-crypt, LUKS) außerhalb des Containers.
>
> Standard-Community-Image (DockerHub `themisdb/themisdb:latest`) wird immer mit
> `THEMIS_ENABLE_ENCRYPTED_STORAGE=OFF` gebaut.

## Image Structure

Der `Dockerfile.themis-server` verwendet **Multi-Stage Build**:

```dockerfile
# Stage 1: BUILDER
FROM ubuntu:24.04 AS builder
  - Install build tools (gcc, g++, cmake, ninja)
  - Install vcpkg
  - Copy source code
  - Configure CMake
  - Build all targets
  - Size: ~2.5GB

# Stage 2: RUNTIME
FROM ubuntu:24.04
  - Copy compiled binary only
  - Runtime dependencies only (libssl, libcurl, etc)
  - Create themis user
  - Health checks
  - Final size: ~200MB
```

## Running the Container

### Start Server
```bash
docker run -d \
  --name themis-prod \
  -p 18765:18765 \
  -p 8080:8080 \
  -v themis-data:/var/lib/themisdb \
  themis-server:hyperscaler-llm
```

### Check Status
```bash
docker logs themis-prod
docker exec themis-prod /usr/local/bin/themis_server --version
```

### Health Check
```bash
curl http://localhost:8080/health
```

## Docker Compose

Für schnelle Entwicklung mit `docker-compose`:

```bash
cd docker/

# Start mit compose
docker-compose -f docker-compose-minimal.yml up -d

# Logs
docker-compose logs -f themis

# Stop
docker-compose down
```

## Build-Customization

### Custom Port
```dockerfile
ENV THEMIS_PORT=9000
ENV THEMIS_HTTP_PORT=9001
```

### Custom Data Directory
```bash
docker run -e THEMIS_DATA_DIR=/opt/data themis-server:hyperscaler-llm
```

### GPU Support

Falls GPU verfügbar ist:

```bash
docker run --gpus all \
  -e THEMIS_ENABLE_GPU=1 \
  themis-server:hyperscaler-llm
```

## Dockerfile Änderungen für neue Struktur

Nach der Umstrukturierung wurden diese Pfade angepasst:

```dockerfile
# Alt (Root-Level):
# COPY CMakeLists.txt .
# Neu (cmake/ subfolder):
COPY cmake/ cmake/
COPY CMakeLists.txt .  # Root-delegator

# CMakePresets.json Pfad:
# Alt: cmake/ findet presets in root
# Neu: cmake/ findet presets in cmake/ (automatisch über ${sourceDir})
```

## Image Inspection

```bash
# Image Details
docker inspect themis-server:hyperscaler-llm

# Layers
docker history themis-server:hyperscaler-llm

# Size
docker images themis-server:hyperscaler-llm
```

## Push to Registry

```bash
# Tag für Registry
docker tag themis-server:hyperscaler-llm \
  registry.example.com/themis:v2.1.0

# Push
docker push registry.example.com/themis:v2.1.0
```

## CI/CD Integration

### GitHub Actions
```yaml
name: Build Docker Image
on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Set up Docker Buildx
        uses: docker/setup-buildx-action@v2
      - name: Build
        run: |
          docker build -f docker/Dockerfile.themis-server \
            -t themis-server:${{ github.sha }} \
            --build-arg THEMIS_ENABLE_LLM=ON \
            .
```

## Troubleshooting

### Problem: "CMAKE_CXX_COMPILER not set"
**Lösung**: Dockerfile hat jetzt explizite Compiler-Umgebungsvariablen:
```dockerfile
ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++
ENV CMAKE_C_COMPILER=/usr/bin/gcc
ENV CMAKE_CXX_COMPILER=/usr/bin/g++
```

### Problem: Build überschreitet Memory
**Lösung**: Builder Stage parallelisieren:
```dockerfile
RUN cmake --build build --config Release --parallel 4  # Weniger Cores
```

### Problem: "Image too large"
**Lösung**: Multi-Stage Build stellt sicher, dass nur Runtime-Dependencies im final image sind (~200MB statt 2.5GB)

## Performance

### Build Cache
```bash
# Cache verwenden (schneller)
docker build --build-arg BUILDKIT_INLINE_CACHE=1 ...

# Cache ignorieren (clean build)
docker build --no-cache ...
```

### Layer Caching
Der `Dockerfile.themis-server` ist optimiert für Caching:
1. System packages early (geringe Änderungen)
2. vcpkg middle (vcpkg.json gelegentlich geändert)
3. Source code late (häufig geändert)

## Next Steps

1. ✅ Lokales Image bauen
2. ✅ Container mit `docker run` testen
3. ✅ In Registry pushen (Docker Hub, ECR, GCR)
4. ✅ Kubernetes deployen (falls vorhanden)

## Nächste Schritte

Nach erfolgreichem Docker-Build lesen Sie:
- **Deployment**: [docs/de/deployment/deployment_docker_multiarch.md](../../de/deployment/deployment_docker_multiarch.md) - Multi-Arch Docker Deployment
- **Releases**: [docs/de/releases/updates_distribution_strategy.md](../../de/releases/updates_distribution_strategy.md)

## Weitere Infos

- [docker/Dockerfile.themis-server](../../docker/Dockerfile.themis-server) - Dockerfile Quelle
- [docker/docker-compose-minimal.yml](../../docker/docker-compose-minimal.yml) - Compose Config
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Weitere Fehlersuche
