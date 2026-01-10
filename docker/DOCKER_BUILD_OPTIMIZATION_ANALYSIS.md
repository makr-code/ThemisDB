# Docker Build Optimization Analysis
**Datum:** 10. Januar 2026  
**Version:** ThemisDB v1.4.0  
**Ziel:** Schnelle und effiziente Docker-Builds für verschiedene Editionen

---

## 🔍 Aktuelle Situation

### Build-Struktur Analyse

#### 1. **Zu viele Dockerfiles** (25+ Dateien)
```
Dockerfile                      # Main build (368 lines)
Dockerfile.hyperscaler          # Hyperscaler Edition (133 lines)
Dockerfile.fast                 # Fast pre-built copy
Dockerfile.prebuilt             # Pre-built binary runtime
Dockerfile.minimal              # Minimal build
Dockerfile.minimal-fast         # Minimal + fast
Dockerfile.quick                # Quick build
Dockerfile.quick-linux          # Quick Linux
Dockerfile.optimized-local      # Local optimization
Dockerfile.themis-server        # Server specific
Dockerfile.benchmark            # Benchmark specific
... und 14+ weitere
```

**Problem:** 
- Duplikate von Build-Logik
- Schwer wartbar
- Inkonsistente Optimierungen

#### 2. **vcpkg Installation bei jedem Build** (~10-15 Minuten)

**Aktueller Flow:**
```dockerfile
# In JEDEM Build wird vcpkg komplett neu installiert:
RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT}
RUN ${VCPKG_ROOT}/vcpkg install --triplet=x64-linux --clean-after-build
```

**Typische Installationszeiten:**
- vcpkg clone: 30-40s
- vcpkg bootstrap: 20-30s
- Dependency compilation: 8-15 Minuten (112 Pakete)
  - zstd: ~30s
  - boost (40+ modules): 5-8 Minuten
  - rocksdb: 1-2 Minuten
  - Arrow/Parquet: 2-3 Minuten
  - grpc/protobuf: 1-2 Minuten

**Gesamte Build-Zeit:** ~15-25 Minuten pro Build

#### 3. **Keine Layer-Caching-Strategie**

**Aktuelles Problem:**
```dockerfile
# Jede Code-Änderung invalidiert alle nachfolgenden Layers
COPY . ./src
RUN cmake ... && ninja build
```

**Folge:** Selbst bei kleinen Änderungen → kompletter Rebuild

#### 4. **llama.cpp wird immer neu geclont**

```dockerfile
RUN git clone https://github.com/ggerganov/llama.cpp.git /src/llama.cpp
RUN cmake -S /src/llama.cpp ...
RUN ninja -C /src/llama.cpp/build
```

**Zeit:** ~2-4 Minuten zusätzlich

---

## 🎯 Optimierungsvorschläge

### **1. Pre-built vcpkg Base Image Strategy** ⭐⭐⭐

#### Konzept: Mehrstufige Base Images

```dockerfile
# ============================================================================
# BASE IMAGE 1: vcpkg-base (cached, rarely changed)
# ============================================================================
FROM ubuntu:24.04 AS vcpkg-base

ENV VCPKG_ROOT=/opt/vcpkg
RUN apt-get update && apt-get install -y build-essential cmake ninja-build git curl ca-certificates pkg-config zip unzip tar wget flex bison
RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} && \
    cd ${VCPKG_ROOT} && git checkout 2024.10.21 && \
    ./bootstrap-vcpkg.sh -disableMetrics

# ============================================================================
# BASE IMAGE 2: vcpkg-deps-base (Edition-spezifisch, pre-compiled)
# ============================================================================
FROM vcpkg-base AS vcpkg-deps-community
COPY docker/vcpkg-community.json /tmp/vcpkg.json
RUN ${VCPKG_ROOT}/vcpkg install --triplet=x64-linux --x-manifest-root=/tmp

FROM vcpkg-base AS vcpkg-deps-enterprise
COPY docker/vcpkg-enterprise.json /tmp/vcpkg.json
RUN ${VCPKG_ROOT}/vcpkg install --triplet=x64-linux --x-manifest-root=/tmp

FROM vcpkg-base AS vcpkg-deps-hyperscaler
COPY docker/vcpkg-hyperscaler.json /tmp/vcpkg.json
RUN ${VCPKG_ROOT}/vcpkg install --triplet=x64-linux --x-manifest-root=/tmp

# ============================================================================
# BASE IMAGE 3: llama-base (optional, LLM-builds)
# ============================================================================
FROM vcpkg-base AS llama-base
RUN git clone --depth=1 https://github.com/ggerganov/llama.cpp.git /opt/llama.cpp && \
    cmake -S /opt/llama.cpp -B /opt/llama.cpp/build -G Ninja \
        -DLLAMA_BUILD_TESTS=OFF \
        -DLLAMA_BUILD_EXAMPLES=OFF \
        -DLLAMA_BUILD_SHARED_LIB=ON && \
    ninja -C /opt/llama.cpp/build -j$(nproc) && \
    ninja -C /opt/llama.cpp/build install
```

**Unified Build Dockerfile:**

```dockerfile
# ============================================================================
# Unified ThemisDB Docker Build
# Supports: COMMUNITY, ENTERPRISE, HYPERSCALER
# ============================================================================

ARG THEMIS_EDITION=COMMUNITY
ARG ENABLE_LLM=OFF

# ============================================================================
# Stage 1: Select pre-built dependency base
# ============================================================================
FROM themisdb/vcpkg-deps-community:latest AS deps-community
FROM themisdb/vcpkg-deps-enterprise:latest AS deps-enterprise
FROM themisdb/vcpkg-deps-hyperscaler:latest AS deps-hyperscaler

# Select edition-specific base
FROM deps-${THEMIS_EDITION,,} AS builder

# Optional: Add llama if LLM enabled
FROM builder AS builder-nollm
FROM themisdb/llama-base:latest AS builder-llm
FROM builder-${ENABLE_LLM:+llm}${ENABLE_LLM:-nollm} AS builder-final

# ============================================================================
# Stage 2: Build ThemisDB (nur Source + CMake)
# ============================================================================
WORKDIR /src
COPY CMakeLists.txt VERSION ./
COPY include ./include
COPY cmake ./cmake
COPY src ./src

RUN cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
        -DTHEMIS_EDITION=${THEMIS_EDITION} \
        -DTHEMIS_ENABLE_LLM=${ENABLE_LLM} && \
    ninja -C build themis_server

# ============================================================================
# Stage 3: Runtime (minimal)
# ============================================================================
FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl libstdc++6 && rm -rf /var/lib/apt/lists/*

RUN groupadd -r themis --gid=999 && \
    useradd -r -g themis --uid=999 --home-dir=/var/lib/themisdb themis

COPY --from=builder-final /src/build/themis_server /usr/local/bin/
COPY --from=builder-final /src/vcpkg_installed/x64-linux/lib/*.so* /usr/local/lib/
RUN ldconfig

USER themis
EXPOSE 8080 18765
HEALTHCHECK --interval=10s --timeout=3s CMD curl -f http://localhost:8080/health || exit 1
CMD ["/usr/local/bin/themis_server"]
```

**Zeitersparnis:**
- **Erstbuild:** ~15 Minuten (wie vorher)
- **Rebuild nach Code-Änderung:** ~2-3 Minuten (nur CMake + Ninja)
- **Rebuild mit dependency-Änderung:** ~5-8 Minuten (nur vcpkg delta)

---

### **2. BuildKit Cache Mount Strategy** ⭐⭐⭐

**Docker BuildKit aktivieren:**
```bash
export DOCKER_BUILDKIT=1
export COMPOSE_DOCKER_CLI_BUILD=1
```

**Optimiertes Dockerfile mit Cache Mounts:**

```dockerfile
# syntax=docker/dockerfile:1.6

FROM vcpkg-deps-community AS builder

WORKDIR /src

# Cache CMake build directory
RUN --mount=type=cache,target=/src/build \
    --mount=type=bind,source=CMakeLists.txt,target=CMakeLists.txt \
    --mount=type=bind,source=cmake,target=cmake \
    --mount=type=bind,source=src,target=src \
    --mount=type=bind,source=include,target=include \
    cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake && \
    ninja -C build themis_server && \
    cp build/themis_server /tmp/themis_server

# Copy binary out of cache mount
COPY --from=builder /tmp/themis_server /usr/local/bin/themis_server
```

**Persistent Cache:**
```bash
# Cache auf Host speichern
docker buildx build \
    --cache-from type=local,src=/tmp/docker-cache \
    --cache-to type=local,dest=/tmp/docker-cache,mode=max \
    -t themisdb:community .

# Oder Registry Cache
docker buildx build \
    --cache-from type=registry,ref=themisdb/cache:community \
    --cache-to type=registry,ref=themisdb/cache:community,mode=max \
    -t themisdb:community .
```

**Zeitersparnis:**
- **Erstbuild:** 15 Minuten
- **Rebuild mit Cache:** 30-60 Sekunden (nur geänderte Source-Files)

---

### **3. Edition Matrix Build System** ⭐⭐

**Zentrales Build-Script:**

```bash
#!/bin/bash
# build-all-editions.sh

EDITIONS=("community" "enterprise" "hyperscaler")
VERSION="${1:-1.4.0}"
REGISTRY="${2:-themisdb/themisdb}"

for EDITION in "${EDITIONS[@]}"; do
    echo "Building ${EDITION} edition..."
    
    docker buildx build \
        --platform linux/amd64,linux/arm64 \
        --build-arg THEMIS_EDITION=${EDITION^^} \
        --build-arg ENABLE_LLM=$([ "$EDITION" = "hyperscaler" ] && echo "ON" || echo "OFF") \
        --cache-from type=registry,ref=${REGISTRY}:cache-${EDITION} \
        --cache-to type=registry,ref=${REGISTRY}:cache-${EDITION},mode=max \
        -t ${REGISTRY}:${VERSION}-${EDITION} \
        -t ${REGISTRY}:${EDITION} \
        -f docker/Dockerfile.unified \
        --push \
        .
done

# Tag latest
docker tag ${REGISTRY}:${VERSION}-community ${REGISTRY}:latest
docker push ${REGISTRY}:latest
```

**Matrix Build in CI:**

```yaml
# .github/workflows/docker-build.yml
name: Docker Build Matrix

on:
  push:
    branches: [main, release/*]
  workflow_dispatch:

jobs:
  build-matrix:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        edition: [community, enterprise, hyperscaler]
        platform: [linux/amd64, linux/arm64]
    
    steps:
      - uses: docker/setup-buildx-action@v3
      
      - name: Build ${{ matrix.edition }}
        uses: docker/build-push-action@v5
        with:
          platforms: ${{ matrix.platform }}
          build-args: |
            THEMIS_EDITION=${{ matrix.edition }}
            ENABLE_LLM=${{ matrix.edition == 'hyperscaler' && 'ON' || 'OFF' }}
          cache-from: type=registry,ref=themisdb/cache:${{ matrix.edition }}-${{ matrix.platform }}
          cache-to: type=registry,ref=themisdb/cache:${{ matrix.edition }}-${{ matrix.platform }},mode=max
          tags: themisdb/themisdb:${{ matrix.edition }}
          push: true
```

---

### **4. Binary Pre-Build + Runtime Copy Strategy** ⭐

**Für lokale Entwicklung / Testing:**

```dockerfile
# Dockerfile.fast-local
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 ca-certificates curl && rm -rf /var/lib/apt/lists/*

# Copy from local WSL build
COPY build-wsl/themis_server /usr/local/bin/
COPY build-wsl/*.so /usr/local/lib/
RUN ldconfig

EXPOSE 8080 18765
CMD ["/usr/local/bin/themis_server"]
```

**Build:**
```bash
# 1. Build lokal (WSL/Native)
cmake --build build-wsl --target themis_server -j8

# 2. Docker Image (nur Copy, keine Compilation!)
docker build -f docker/Dockerfile.fast-local -t themisdb:dev .
```

**Zeit:** ~10-15 Sekunden (nur Image-Layer-Creation)

---

### **5. vcpkg Binary Cache** ⭐⭐

**NuGet Binary Cache Setup:**

```dockerfile
# Pre-compile vcpkg packages und cache in Registry
FROM vcpkg-base AS binary-cache-builder

# Enable binary caching
ENV VCPKG_BINARY_SOURCES="clear;nuget,https://nuget.pkg.github.com/themisdb/index.json,readwrite"

# Install all dependencies
COPY vcpkg.json ./
RUN ${VCPKG_ROOT}/vcpkg install --triplet=x64-linux

# Push to cache automatically via VCPKG_BINARY_SOURCES
```

**Nutzung in Builds:**

```dockerfile
FROM vcpkg-base AS builder

ENV VCPKG_BINARY_SOURCES="clear;nuget,https://nuget.pkg.github.com/themisdb/index.json,read"

# Packages werden aus Binary Cache geladen (keine Compilation!)
RUN ${VCPKG_ROOT}/vcpkg install --triplet=x64-linux
```

**Zeitersparnis:** ~10-12 Minuten (Download statt Compilation)

---

## 📊 Vergleich: Alt vs. Neu

| Szenario | Aktuell | Mit Optimierungen | Zeitersparnis |
|----------|---------|-------------------|---------------|
| **Erstbuild (kein Cache)** | 15-25 Min | 12-18 Min (binary cache) | 20-30% |
| **Rebuild (Code-Änderung)** | 15-25 Min | 1-3 Min (cache mount) | **90-95%** |
| **Rebuild (Dependency-Änderung)** | 15-25 Min | 5-8 Min (vcpkg delta) | 60-70% |
| **Edition Switch** | 15-25 Min | 3-5 Min (pre-built base) | **75-85%** |
| **Lokal Testing** | 15-25 Min | 10-15 Sek (fast-local) | **99%** |

---

## 🚀 Implementierungs-Roadmap

### Phase 1: Quick Wins (Sofort umsetzbar) ✅
1. **Dockerfile Konsolidierung**
   - [ ] Merge `Dockerfile.hyperscaler`, `Dockerfile.minimal`, `Dockerfile.quick` → `Dockerfile.unified`
   - [ ] Edition via Build-Arg statt separate Files
   - [ ] LLM-Support via Build-Arg conditional

2. **BuildKit Cache Mounts**
   - [ ] `syntax=docker/dockerfile:1.6` aktivieren
   - [ ] `--mount=type=cache` für CMake build directory
   - [ ] `--mount=type=bind` für Source-Files

3. **Fast Local Development**
   - [ ] `Dockerfile.dev` für lokale WSL-Binary-Copy
   - [ ] `docker-compose.dev.yml` mit Volume-Mounts

### Phase 2: Pre-built Base Images (1-2 Wochen) 🔄
1. **Base Image Pipeline**
   - [ ] `Dockerfile.vcpkg-base` erstellen
   - [ ] `Dockerfile.vcpkg-deps-{edition}` templates
   - [ ] `Dockerfile.llama-base` für LLM-Builds
   - [ ] CI: Wöchentliche Base-Image-Builds

2. **Registry Setup**
   - [ ] GitHub Container Registry (ghcr.io)
   - [ ] Tagging-Schema: `vcpkg-base:2024.10.21`, `vcpkg-deps-community:v1.4.0`

### Phase 3: Binary Cache (Optional, 2-3 Wochen) 🎯
1. **NuGet Binary Cache**
   - [ ] GitHub Packages NuGet Feed
   - [ ] vcpkg Binary Cache CI Job
   - [ ] Authentication Setup

2. **Alternative: File-based Cache**
   - [ ] S3/MinIO Binary Cache
   - [ ] Cache-Management-Scripts

---

## 📝 Empfohlene Dateistruktur

```
docker/
├── Dockerfile.unified          # Main unified build (all editions)
├── Dockerfile.dev              # Fast local dev (pre-built binary)
├── Dockerfile.vcpkg-base       # Base image mit vcpkg
├── Dockerfile.vcpkg-deps       # Dependency pre-compilation
├── Dockerfile.llama-base       # llama.cpp pre-built
├── build-all-editions.sh       # Build-Script für alle Editionen
├── vcpkg-community.json        # Dependencies: Community
├── vcpkg-enterprise.json       # Dependencies: Enterprise
├── vcpkg-hyperscaler.json      # Dependencies: Hyperscaler
└── compose/
    ├── docker-compose.yml      # Production compose
    └── docker-compose.dev.yml  # Development compose
```

---

## 🎯 Nächste Schritte

1. **Immediate:** Dockerfile.unified erstellen mit Edition-Matrix
2. **Today:** BuildKit Cache Mounts aktivieren
3. **This Week:** Pre-built Base Images Pipeline
4. **This Month:** Binary Cache Setup (optional)

---

## 💡 Best Practices

### ✅ DO:
- Multi-stage builds mit klaren Stage-Namen
- BuildKit Cache Mounts für Build-Artifacts
- Edition als Build-Arg, nicht separate Dockerfiles
- Pre-built Base Images für Dependencies
- Shallow git clones (`--depth=1`)
- `.dockerignore` nutzen (Build-Context minimieren)

### ❌ DON'T:
- `COPY . .` vor dependency installation
- vcpkg bei jedem Build neu clonen
- Duplicate Build-Logic in mehreren Dockerfiles
- Große Layer ohne Cache-Strategie
- `apt-get install` ohne `--no-install-recommends`
- `RUN` ohne Cleanup (`rm -rf /var/lib/apt/lists/*`)

---

**Geschätzte Gesamtzeitersparnis nach vollständiger Implementierung:**
- **Development Builds:** 15 Min → 30 Sek (**96% schneller**)
- **CI Builds:** 20 Min → 3 Min (**85% schneller**)
- **Production Release:** 25 Min → 12 Min (**52% schneller**)

