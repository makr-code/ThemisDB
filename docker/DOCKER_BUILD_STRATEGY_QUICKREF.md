# Docker Build Strategy - Quick Reference
**Erstellt:** 10. Januar 2026  
**ThemisDB Version:** v1.4.0  
**Aktualisiert:** Siehe [DOCKER_BUILD_CORRECTIONS.md](DOCKER_BUILD_CORRECTIONS.md) für Korrekturen

> **✅ Korrekturen:** Ubuntu 26.04, lokales llama.cpp, MINIMAL Edition, Multi-Arch Support  
> Siehe Details in [DOCKER_BUILD_CORRECTIONS.md](DOCKER_BUILD_CORRECTIONS.md)

> **⚡ vcpkg+aria2 Optimierung:** Download-Stabilität & Performance  
> Siehe Details in [VCPKG_ARIA2_STRATEGY.md](VCPKG_ARIA2_STRATEGY.md)

---

## 🎯 Strategie-Übersicht

### Download-Strategie: aria2 für vcpkg ⚡

**Status:** ✅ Implementiert in `Dockerfile.unified`

```dockerfile
# aria2 aktiviert für schnellere & stabilere Downloads
export VCPKG_DOWNLOAD_TOOL=aria2
export VCPKG_USE_ARIA2=1
export VCPKG_DOWNLOADER=aria2
```

**Vorteile:**
- ⚡ **10-16x schneller** (16 parallele Verbindungen vs. 1 bei curl)
- 🔄 **Auto-Retry** (5 Versuche bei Fehlern)
- 💾 **Resume-fähig** (fortsetzbare Downloads)
- 🛡️ **Robust** gegen Timeouts

**Performance:**
- Boost 1.86.0 (71 Pakete): ~6-10 Min statt ~12-18 Min
- Mit Cache: ~1-2 Min statt ~2-4 Min

📖 Siehe [VCPKG_ARIA2_STRATEGY.md](VCPKG_ARIA2_STRATEGY.md) für Details

---

### vcpkg Triple-Cache-Strategie 💾

**Status:** ✅ Implementiert (11. Januar 2026)

```dockerfile
# 1. BuildKit Container-Cache (persistent zwischen Builds)
--mount=type=cache,target=/opt/vcpkg/downloads,sharing=locked
--mount=type=cache,target=/opt/vcpkg/packages,sharing=locked

# 2. Host-Cache Bind-Mounts (Ihre lokalen Verzeichnisse)
--mount=type=bind,source=vcpkg/downloads,target=/tmp/host-vcpkg-downloads,readonly
--mount=type=bind,source=vcpkg/packages,target=/tmp/host-vcpkg-packages,readonly

# 3. vcpkg Binary Caching (kompilierte Packages als .zip)
export VCPKG_BINARY_SOURCES="clear;files,/opt/vcpkg/packages,readwrite"

# 4. Kopier-Logik (Host → Container vor vcpkg install)
find /tmp/host-vcpkg-downloads ... | xargs cp -n /opt/vcpkg/downloads/
find /tmp/host-vcpkg-packages -name "*_x64-linux" | xargs cp -rn /opt/vcpkg/packages/
```

**Vorteile:**
- 🚀 **Null Downloads** wenn Packages bereits im Host-Cache existieren
- 💾 **~144 Packages** (~2.8GB) sofort verfügbar aus `./vcpkg/packages/`
- ⚡ **Keine Rebuilds** für bereits kompilierte Dependencies
- 🔄 **Automatisches Fallback** zu Download nur bei fehlenden Packages
- 🎯 **Binary Caching** speichert kompilierte Packages als .zip (effizienter als Verzeichnisse)

**Ablauf:**
1. Host-Packages (`./vcpkg/packages/*_x64-linux`) → `/opt/vcpkg/packages/`
2. Host-Downloads (`./vcpkg/downloads/*.tar.gz`) → `/opt/vcpkg/downloads/`
3. vcpkg Binary Cache aktiviert → Packages als .zip gespeichert
4. `vcpkg install` findet alle Dependencies bereits vor → **Skip Build**
5. Nur fehlende Packages werden heruntergeladen/gebaut

**Performance:**
- Mit vollem Cache: **~30 Sekunden** statt ~10-15 Minuten
- Kein GitHub-Netzwerk erforderlich für gecachte Packages
- Robustheit gegen GitHub-Download-Timeouts

**Referenz:**
- [Reddit: vcpkg Docker Caching](https://www.reddit.com/r/cpp_questions/comments/1nb1cwo/how_to_solve_the_problem_of_vcpkg_needlessly/)
- [vcpkg Binary Caching Docs](https://learn.microsoft.com/vcpkg/users/binarycaching)

---

### 3-Stufige Build-Architektur

```
┌─────────────────────────────────────────────────────────────┐
│ STUFE 1: Base Images (build once, cache forever)           │
├─────────────────────────────────────────────────────────────┤
│ • vcpkg-base          → Bootstrapped vcpkg + build tools   │
│ • vcpkg-deps-*        → Pre-compiled dependencies/edition  │
│ • llama-base          → Pre-built llama.cpp (LLM)          │
│ Build: Weekly or on dependency changes                     │
└─────────────────────────────────────────────────────────────┘
                                ↓
┌─────────────────────────────────────────────────────────────┐
│ STUFE 2: Application Build (fast, cacheable)               │
├─────────────────────────────────────────────────────────────┤
│ FROM vcpkg-deps-{edition}  → Select pre-built deps        │
│ COPY source code           → Only changed on code updates  │
│ RUN cmake + ninja          → Incremental builds            │
│ Build: On every code change (~2-3 min)                     │
└─────────────────────────────────────────────────────────────┘
                                ↓
┌─────────────────────────────────────────────────────────────┐
│ STUFE 3: Runtime Image (minimal, production-ready)         │
├─────────────────────────────────────────────────────────────┤
│ FROM ubuntu:24.04          → Minimal base                  │
│ COPY binary + libs         → Only runtime dependencies     │
│ Result: ~400-600 MB production image                       │
└─────────────────────────────────────────────────────────────┘
```

---

## 🚀 Quick Start Commands

### Lokale Entwicklung (10-15 Sekunden)
```bash
# 1. Build lokal (WSL oder Windows)
cmake --build build-wsl --target themis_server -j8
# ODER: cmake --build build-msvc --config Release --target themis_server

# 2. Docker Image aus Binary
docker build -f docker/Dockerfile.dev -t themisdb:dev . > logs/docker-build-dev.log 2>&1
docker run -d -p 8080:8080 -p 18765:18765 themisdb:dev
```

### Unified Build (alle Editionen, 2-3 Minuten mit Cache)
**Logs**: Alle Build-Logs werden in `./logs/` gespeichert.

> Tipp: Nach Änderungen an `vcpkg-*.json` einen Hash übergeben, damit der deps-Layer neu gebaut wird: `--build-arg VCPKG_MANIFEST_HASH=$(sha256sum docker/vcpkg-community.json | cut -d' ' -f1)` (Linux) bzw. `$(Get-FileHash docker/vcpkg-community.json).Hash` (PowerShell).
```bash
# Minimal (IoT/Embedded)
docker buildx build --build-arg THEMIS_EDITION=MINIMAL \
  --build-arg VCPKG_MANIFEST_HASH=$(sha256sum docker/vcpkg-minimal.json | cut -d' ' -f1) \
  -t themisdb:minimal -f docker/Dockerfile.unified .

# Community
docker buildx build --build-arg THEMIS_EDITION=COMMUNITY \
  --build-arg VCPKG_MANIFEST_HASH=$(sha256sum docker/vcpkg-community.json | cut -d' ' -f1) \
  -t themisdb:community -f docker/Dockerfile.unified .

# Enterprise
docker buildx build --build-arg THEMIS_EDITION=ENTERPRISE \
  --build-arg VCPKG_MANIFEST_HASH=$(sha256sum docker/vcpkg-enterprise.json | cut -d' ' -f1) \
  -t themisdb:enterprise -f docker/Dockerfile.unified .

# Hyperscaler mit LLM
docker buildx build --build-arg THEMIS_EDITION=HYPERSCALER \
  --build-arg ENABLE_LLM=ON \
  --build-arg VCPKG_MANIFEST_HASH=$(sha256sum docker/vcpkg-hyperscaler.json | cut -d' ' -f1) \
  -t themisdb:hyperscaler \
  -f docker/Dockerfile.unified .
```

### Base Images bauen (einmalig, ~15-20 Minuten)
```bash
# Lokal testen
./docker/build-base-images.sh themisdb

# Push to registry
./docker/build-base-images.sh themisdb push
```

### Alle Editionen bauen
```bash
# Mit lokalem Cache
./docker/build-all-editions.sh 1.4.0 themisdb/themisdb linux/amd64

# Mit Push zu Registry
./docker/build-all-editions.sh 1.4.0 themisdb/themisdb linux/amd64 --push

# Multi-platform
./docker/build-all-editions.sh 1.4.0 themisdb/themisdb "linux/amd64,linux/arm64" --push
```

---

## 📁 Neue Dateistruktur

```
docker/
├── Dockerfile.unified          ⭐ Main build (all editions)
├── Dockerfile.dev              ⭐ Fast dev (pre-built binary)
├── Dockerfile.vcpkg-base       🏗️ Base vcpkg image
├── Dockerfile.vcpkg-deps       🏗️ Pre-compiled dependencies
├── Dockerfile.llama-base       🏗️ Pre-built llama.cpp
│
├── vcpkg-community.json        📦 Community dependencies
├── vcpkg-enterprise.json       📦 Enterprise dependencies
├── vcpkg-hyperscaler.json      📦 Hyperscaler dependencies
│
├── build-all-editions.sh       🔧 Build all editions
├── build-base-images.sh        🔧 Build base images
│
├── BUILD_STAGES_GUIDE.md       📚 Stage documentation
├── DOCKER_BUILD_OPTIMIZATION_ANALYSIS.md  📊 Analysis
└── DOCKER_BUILD_STRATEGY_QUICKREF.md      📖 This file
```

---

## ⚡ Performance-Vergleich

| Build-Typ | Vorher | Nachher | Verbesserung |
|-----------|--------|---------|--------------|
| **Erstbuild (kein Cache)** | 15-25 Min | 12-18 Min | ✅ 20-30% |
| **Code-Änderung** | 15-25 Min | 1-3 Min | ⚡ **90-95%** |
| **Dependency-Änderung** | 15-25 Min | 5-8 Min | ✅ 60-70% |
| **Edition-Wechsel** | 15-25 Min | 3-5 Min | ⚡ **75-85%** |
| **Lokales Testing** | 15-25 Min | 10-15 Sek | ⚡ **99%** |

---

## 🔧 CI/CD Integration

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
