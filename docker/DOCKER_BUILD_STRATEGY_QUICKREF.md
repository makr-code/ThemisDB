# Docker Build Strategy Quick Reference

**Status:** current as of 2026-09-01  
**Canonical build entrypoint:** [../Dockerfile](../Dockerfile)  
**Supporting assets:** [../docker](../docker)

## Current architecture

- The repository root [../Dockerfile](../Dockerfile) is the canonical Docker build file used by Docker Desktop and `docker buildx`.
- Docker-related config, manifests, and compose files live in [../docker](../docker), not beside the root build file.
- The build uses multi-stage layering: `base` -> `deps` -> `llama` -> `build` -> `runtime`.
- BuildKit cache mounts are used for APT package caches and vcpkg caches.
- The vcpkg clone step is guarded so a cached build directory does not fail with “destination path ... already exists and is not an empty directory”.

## Base Image Versioning (as of 2026-09-01)

**Primary strategy:** Use floating/latest tags for cross-platform compatibility

| Dockerfile | Base Image | Policy | Rationale |
|---|---|---|---|
| `Dockerfile.unified` (Primary) | `ubuntu:latest` | Always track LTS + patches | Auto-resolves across registries; no SHA divergence |
| `Dockerfile.ethics-ai` | `python:3.11-slim` | Track Python 3.11.x patches | Allows security patches; platform-independent resolution |
| `Dockerfile.themisdb` (Legacy) | `ubuntu:22.04` | Deprecated; not updated | For backward compatibility only |

**Benefits:**
- Different Docker registries (Linux/macOS/Windows/Docker Desktop) independently resolve `ubuntu:latest` without SHA conflicts
- All LTS security patches are applied automatically
- Reduces maintenance burden of tracking minor versions
- Improves cross-compilation resilience

## Why this layout

This repository uses a single root Dockerfile so the build is consistent for:

- local development builds
- Desktop Docker builds
- CI/CD or remote buildx builds
- cache-driven, repeatable rebuilds

The folder [../docker](../docker) is reserved for supporting assets like config, manifests, compose, and references, while the root [../Dockerfile](../Dockerfile) remains the actual build entrypoint.

## Local validation command

```bash
docker buildx build --progress=plain --load \
  -f Dockerfile \
  -t themisdb:test \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --build-arg ENABLE_LLM=OFF \
  --build-arg ENABLE_GPU=OFF \
  --build-arg BUILD_TESTS=OFF \
  --build-arg BUILD_BENCHMARKS=OFF \
  .
```

This is the minimal smoke test for the current Docker pipeline.

## Cache strategy

The important part is the BuildKit cache pattern used in [../Dockerfile](../Dockerfile):

```dockerfile
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    ...

RUN --mount=type=cache,target=/opt/vcpkg/downloads,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/buildtrees,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/packages,sharing=locked \
    ...
```

This keeps apt and vcpkg state reusable between rebuilds without forcing unstable host bind mounts.

## Common failure fixed

A recurring issue with cache-backed Docker builds is a stale pre-created vcpkg directory:

```text
fatal: destination path '/opt/vcpkg' already exists and is not an empty directory.
```

The Dockerfile now handles this explicitly by checking for the repository metadata before cloning vcpkg and by creating the cache directories before bootstrap.

## Related files

- [../Dockerfile](../Dockerfile)
- [README.md](README.md)
- [../docker/config](../docker/config)
- [../docker/docker-compose.dev.yml](../docker/docker-compose.dev.yml)
- [../docker/docker-compose.yml](../docker/docker-compose.yml)

## Notes

- GPU-enabled builds require Vulkan development packages in the container.
- For minimal local validation, keep `ENABLE_GPU=OFF` unless the environment is intentionally configured for GPU builds.
- `ENABLE_LLM` and build flags should be passed explicitly when the target image should omit optional features.
### GitHub Actions Beispiel
```yaml
name: Docker Build

on:
  push:
    branches: [main]

jobs:
  build:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        edition: [community, enterprise, hyperscaler]
    
    steps:
      - uses: actions/checkout@v3
      
      - uses: docker/setup-buildx-action@v3
      
      - uses: docker/login-action@v3
        with:
          username: ${{ secrets.DOCKER_USERNAME }}
          password: ${{ secrets.DOCKER_PASSWORD }}
      
      - name: Build ${{ matrix.edition }}
        run: |
          docker buildx build \
            --build-arg THEMIS_EDITION=${{ matrix.edition }} \
            --build-arg ENABLE_LLM=${{ matrix.edition == 'hyperscaler' && 'ON' || 'OFF' }} \
            --cache-from type=registry,ref=themisdb/cache:${{ matrix.edition }} \
            --cache-to type=registry,ref=themisdb/cache:${{ matrix.edition }},mode=max \
            -t themisdb/themisdb:${{ matrix.edition }} \
            -f docker/Dockerfile.unified \
            --push \
            .
```

---

## 📦 Edition-spezifische Builds

### Community (Minimal, Open Source)
```bash
docker build --build-arg THEMIS_EDITION=COMMUNITY \
             -t themisdb:community \
             -f docker/Dockerfile.unified .
```
**Features:** Core DB, REST API, Basic Replication  
**Dependencies:** rocksdb, boost, spdlog, nlohmann-json

### Enterprise (Business, erweitert)
```bash
docker build --build-arg THEMIS_EDITION=ENTERPRISE \
             -t themisdb:enterprise \
             -f docker/Dockerfile.unified .
```
**Features:** + Sharding, Advanced Monitoring, gRPC Replication  
**Dependencies:** + grpc, protobuf, prometheus-cpp

### Hyperscaler (Full-Featured, LLM)
```bash
docker build --build-arg THEMIS_EDITION=HYPERSCALER \
             --build-arg ENABLE_LLM=ON \
             -t themisdb:hyperscaler \
             -f docker/Dockerfile.unified .
```
**Features:** + LLM, Multi-DC, RAID, GraphQL  
**Dependencies:** + llama.cpp, tbb, hnswlib

---

## 🎨 BuildKit Cache Optimierungen

### Aktivierung
```bash
export DOCKER_BUILDKIT=1
export COMPOSE_DOCKER_CLI_BUILD=1
```

### Registry Cache (empfohlen für CI)
```bash
docker buildx build \
  --cache-from type=registry,ref=themisdb/cache:community \
  --cache-to type=registry,ref=themisdb/cache:community,mode=max \
  -t themisdb:community .
```

### Lokaler Cache (schnell für Entwicklung)
```bash
docker buildx build \
  --cache-from type=local,src=/tmp/docker-cache \
  --cache-to type=local,dest=/tmp/docker-cache,mode=max \
  -t themisdb:community .
```

---

## 🔄 Update-Strategien

### Base Images aktualisieren (wöchentlich/monatlich)
```bash
# 1. Pull latest vcpkg
cd vcpkg && git pull && cd ..

# 2. Rebuild base images
./docker/build-base-images.sh themisdb push

# 3. Update Edition builds
./docker/build-all-editions.sh 1.4.1 themisdb --push
```

### Dependency hinzufügen
```bash
# 1. vcpkg-{edition}.json editieren
vim docker/vcpkg-community.json

# 2. Rebuild nur betroffenes vcpkg-deps Image
docker buildx build --target community \
  -f docker/Dockerfile.vcpkg-deps \
  -t themisdb/vcpkg-deps:community-v1.4.1 --push .

# 3. Rebuild Edition
docker buildx build --build-arg THEMIS_EDITION=COMMUNITY \
  -t themisdb:community-v1.4.1 \
  -f docker/Dockerfile.unified --push .
```

---

## 🐛 Debugging

### Debug Build aktivieren
```bash
docker build --target debug \
  --build-arg THEMIS_EDITION=COMMUNITY \
  -t themisdb:debug \
  -f docker/Dockerfile.unified .

docker run -it --rm \
  --cap-add=SYS_PTRACE \
  --security-opt seccomp=unconfined \
  themisdb:debug /bin/bash

# Im Container:
gdb /src/build/themis_server
valgrind --leak-check=full /src/build/themis_server
```

### Build-Probleme analysieren
```bash
# Verbose output
docker build --progress=plain -f docker/Dockerfile.unified .

# Cache-Probleme beheben
docker builder prune -af

# Buildx reset
docker buildx rm themis-builder
docker buildx create --name themis-builder --use
```

---

## 📊 Image-Größen

| Image-Typ | Größe | Inhalt |
|-----------|-------|--------|
| **vcpkg-base** | ~2 GB | Build-Tools + vcpkg |
| **vcpkg-deps-community** | ~3 GB | + Dependencies |
| **vcpkg-deps-hyperscaler** | ~4 GB | + Alle Dependencies |
| **llama-base** | ~500 MB | llama.cpp libs |
| **themisdb:community (runtime)** | ~400 MB | ✅ Production |
| **themisdb:hyperscaler (runtime)** | ~600 MB | ✅ Production + LLM |
| **themisdb:debug** | ~5 GB | Development |

---

## ✅ Best Practices Checkliste

- [x] BuildKit aktiviert (`DOCKER_BUILDKIT=1`)
- [x] Multi-stage builds mit Cache Mounts
- [x] Edition als Build-Arg (nicht separate Dockerfiles)
- [x] Pre-built Base Images für Dependencies
- [x] `.dockerignore` für minimalen Build-Context
- [x] `--no-install-recommends` bei apt-get
- [x] Cleanup nach Package-Installation
- [x] Non-root User im Runtime-Image
- [x] Healthcheck definiert
- [x] Layer-Optimierung (große Layers früh, Code-Changes spät)

---

## 🎯 Nächste Schritte

1. **Heute:** `Dockerfile.unified` verwenden statt alte Dockerfiles
2. **Diese Woche:** Base Images bauen und pushen
3. **Nächste Woche:** CI/CD auf neue Build-Matrix umstellen
4. **Optional:** Binary Cache Setup (NuGet/S3)

---

**Fragen? Siehe:**
- [DOCKER_BUILD_OPTIMIZATION_ANALYSIS.md](DOCKER_BUILD_OPTIMIZATION_ANALYSIS.md) - Detaillierte Analyse
- [BUILD_STAGES_GUIDE.md](BUILD_STAGES_GUIDE.md) - Stage-Dokumentation
