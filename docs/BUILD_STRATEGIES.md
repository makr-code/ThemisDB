# Build-Strategien: Pre-built Libraries

## Übersicht

ThemisDB unterstützt drei Build-Strategien mit unterschiedlichen Trade-offs:

| Strategie | Build-Zeit | Vorteile | Nachteile | Anwendungsfall |
|-----------|------------|----------|-----------|----------------|
| **System Libraries (apt)** | ~5 min | ⚡ Sehr schnell, keine Downloads | Versionen nicht pinned | CI, Development |
| **vcpkg Manifest** | ~30 min | 🎯 Exakte Versionen, reproduzierbar | Langsam, große Downloads | Release Builds |
| **Pre-built Binary** | ~30 sec | 🚀 Ultra-schnell | Benötigt vorkompiliertes Binary | Docker, Packaging |

## 1. System Libraries (apt) - **EMPFOHLEN FÜR CI**

### Verfügbare Pakete (Ubuntu 24.04)

#### ✅ Vollständig verfügbar via apt

| Dependency | apt Package | Version | vcpkg Alternative |
|------------|-------------|---------|-------------------|
| RocksDB | `librocksdb-dev` | 8.9.1 | ✅ Kompatibel |
| OpenSSL | `libssl-dev` | 3.0.13 | ✅ Kompatibel |
| Boost | `libboost-system-dev` | 1.83.0 | ✅ Kompatibel |
| spdlog | `libspdlog-dev` | 1.12.0 | ✅ Kompatibel |
| nlohmann-json | `nlohmann-json3-dev` | 3.11.3 | ✅ Kompatibel |
| yaml-cpp | `libyaml-cpp-dev` | 0.8.0 | ✅ Kompatibel |
| zstd | `libzstd-dev` | 1.5.5 | ✅ Kompatibel |
| TBB | `libtbb-dev` | 2021.11.0 | ✅ Kompatibel |
| curl | `libcurl4-openssl-dev` | 8.5.0 | ✅ Kompatibel |
| GTest | `libgtest-dev` | 1.14.0 | ✅ Kompatibel |
| benchmark | `libbenchmark-dev` | 1.8.3 | ✅ Kompatibel |

#### ⚠️ Nicht verfügbar / Inkompatibel

| Dependency | Status | Lösung |
|------------|--------|--------|
| simdjson | Nicht in apt | vcpkg (klein, schnell) |
| arrow | Veraltete Version | vcpkg (wichtig für features) |
| hnswlib | Nicht in apt | vcpkg (header-only, schnell) |
| opentelemetry-cpp | Nicht in apt | vcpkg (optional für tracing) |

### Installation

```bash
# System-Bibliotheken installieren
sudo apt-get update
sudo apt-get install -y \
    librocksdb-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    libtbb-dev \
    libspdlog-dev \
    nlohmann-json3-dev \
    libboost-system-dev \
    libboost-asio-dev \
    libyaml-cpp-dev \
    libzstd-dev \
    libgtest-dev \
    libbenchmark-dev

# Fehlende Pakete via vcpkg (minimal)
git clone --depth 1 https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install simdjson arrow[parquet,compute] hnswlib opentelemetry-cpp[otlp-http]
```

### CMake Konfiguration

```bash
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DTHEMIS_USE_SYSTEM_LIBS=ON
```

### Vorteile

✅ **Geschwindigkeit**: 6x schneller als vollständiges vcpkg  
✅ **Bandbreite**: 90% weniger Downloads  
✅ **Disk Space**: Kein vcpkg-Cache erforderlich  
✅ **CI-Freundlich**: Schnellere Feedback-Zyklen  
✅ **Ubuntu-optimiert**: Getestet und stabil  

### Nachteile

⚠️ **Versionen**: Folgen Ubuntu-Releases (nicht pinned)  
⚠️ **Plattformen**: Nur Linux (keine Windows/macOS)  
⚠️ **Breaking Changes**: Bei Ubuntu-Upgrades möglich  

## 2. vcpkg Manifest Mode - **FÜR RELEASES**

### Verwendung

```bash
# Alle Dependencies via vcpkg
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build -j $(nproc)
```

### Vorteile

✅ **Reproduzierbarkeit**: Exakte Versionen  
✅ **Cross-Platform**: Windows, Linux, macOS  
✅ **Isolation**: Keine Konflikte mit System-Libs  
✅ **Features**: Volle Kontrolle über Build-Features  

### Nachteile

⚠️ **Build-Zeit**: ~30 Minuten  
⚠️ **Disk Space**: 5-10 GB Cache  
⚠️ **Bandbreite**: 500+ MB Downloads  

## 3. Pre-built Binary - **FÜR DOCKER**

### Docker Fast Build

```dockerfile
# Dockerfile.simple
FROM ubuntu:24.04
RUN apt-get update && apt-get install -y ca-certificates
COPY build/themis_server /usr/local/bin/
CMD ["/usr/local/bin/themis_server"]
```

### Verwendung

```bash
# Binary lokal bauen (einmalig)
cmake -DTHEMIS_STATIC_BUILD=ON ...
cmake --build build

# Docker Image (schnell)
docker build -f Dockerfile.simple -t themisdb:latest .
```

### Vorteile

✅ **Ultra-schnell**: 30 Sekunden für Docker-Image  
✅ **Klein**: ~160 MB Image  
✅ **Offline**: Keine Downloads benötigt  

## Empfehlungen nach Anwendungsfall

### CI/CD (GitHub Actions, GitLab CI)
```yaml
# .github/workflows/ci-fast.yml verwenden
# ⚡ System Libraries + minimal vcpkg
# Zeit: ~5 Minuten
# Kosten: Minimal
```

### Lokale Entwicklung
```bash
# Option 1: System Libraries (schnell iterieren)
sudo apt-get install librocksdb-dev libboost-dev ...

# Option 2: vcpkg (exakte Reproduktion)
./vcpkg/vcpkg install --triplet x64-linux
```

### Release Builds
```bash
# Vollständiges vcpkg für Reproduzierbarkeit
cmake -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
# + Statisches Linking für Portabilität
cmake -DTHEMIS_STATIC_BUILD=ON
```

### Docker Images
```bash
# Strategie 1: Pre-built Binary (empfohlen)
./docker-build.sh --use-prebuilt

# Strategie 2: Multi-stage Build (von scratch)
docker build -f Dockerfile .
```

## Performance-Vergleich

### Build-Zeiten (GitHub Actions)

| Workflow | Dependencies | Build-Zeit | Downloads | Kosten |
|----------|--------------|------------|-----------|--------|
| ci-fast.yml | apt + vcpkg (4 pkgs) | ~5 min | ~50 MB | €0.08 |
| ci.yml | vcpkg (alle) | ~30 min | ~500 MB | €0.50 |
| docker-build.yml (fast) | Pre-built | ~30 sec | 0 MB | €0.01 |
| docker-build.yml (full) | vcpkg (alle) | ~30 min | ~500 MB | €0.50 |

*Kosten basierend auf GitHub Actions Pricing: $0.008/minute*

## Migration Guide

### Von vollständigem vcpkg zu System Libraries

```diff
# CMakeLists.txt
- find_package(RocksDB CONFIG REQUIRED)
+ find_package(RocksDB CONFIG)
+ if(NOT RocksDB_FOUND)
+   find_package(PkgConfig)
+   pkg_check_modules(RocksDB REQUIRED rocksdb)
+ endif()
```

### Workflow-Auswahl

```yaml
# Schnelles Feedback (PR checks)
name: CI Fast
uses: ./.github/workflows/ci-fast.yml

# Release Builds
name: CI Full
uses: ./.github/workflows/ci.yml
if: startsWith(github.ref, 'refs/tags/')
```

## Troubleshooting

### Problem: Inkompatible apt-Version

**Symptom**: Build-Fehler mit apt-Paket
```
error: 'feature_x' is not a member of 'rocksdb'
```

**Lösung**: Zurück zu vcpkg für dieses Paket
```bash
# Entferne apt-Paket
sudo apt-get remove librocksdb-dev

# Installiere via vcpkg
./vcpkg/vcpkg install rocksdb[lz4,zstd]
```

### Problem: Fehlende Header

**Symptom**: 
```
fatal error: simdjson.h: No such file or directory
```

**Lösung**: Paket fehlt in vcpkg-Liste
```bash
./vcpkg/vcpkg install simdjson
```

## Best Practices

1. **CI**: Nutze `ci-fast.yml` für Pull Requests
2. **Releases**: Nutze vollständiges vcpkg für Tags
3. **Docker**: Nutze pre-built binary Strategie
4. **Lokal**: System Libraries für schnelle Iteration
5. **Cross-Platform**: vcpkg für Windows/macOS Support

## Fazit

**Für 90% der CI-Builds**: System Libraries (apt) + minimal vcpkg
- ⚡ 6x schneller
- 💰 90% Kosten-Ersparnis
- 🎯 Ausreichend für Tests und Validierung

**Für Production Releases**: Vollständiges vcpkg
- 🔒 Reproduzierbare Builds
- 🌐 Cross-Platform Support
- ✅ Exakte Versionskontrolle

---

## Deployment-Strategie und Branching

### Branch-basierte Build & Deployment Strategy

ThemisDB nutzt eine **Git Flow Branching Strategy** mit unterschiedlichen Build- und Deployment-Strategien pro Branch:

#### 1. `develop` Branch - Continuous Integration

**Zweck**: Schnelles Feedback für Feature-Integration

**Build-Strategie:**
```yaml
# CI auf develop: Fast Builds mit System Libraries
trigger: push/PR to develop
strategy: System Libraries (apt) + minimal vcpkg
build-time: ~5-10 min
artifacts: Keine Releases, nur Test-Reports
```

**CI Workflow:**
- ✅ Build-Validierung (Linux + Windows)
- ✅ Unit Tests
- ✅ Code Quality (clang-tidy, cppcheck)
- ✅ Security Scans (Gitleaks)
- ❌ Keine Docker Images
- ❌ Keine Release-Artefakte

**Verwendete Workflow-Dateien:**
- `.github/workflows/develop-ci.yml` (oder ähnlich)
- Schnelle Feedbackzyklen (5-10 min)

#### 2. `release/*` Branch - Release Preparation

**Zweck**: Release-Testing und Stabilisierung

**Build-Strategie:**
```yaml
# CI auf release branches: Full vcpkg Builds
trigger: push to release/*
strategy: vcpkg Manifest (vollständig)
build-time: ~30 min
artifacts: Pre-release binaries für Testing
```

**CI Workflow:**
- ✅ Vollständige Builds (alle Plattformen)
- ✅ Komplette Test-Suite
- ✅ Performance Benchmarks
- ✅ Pre-release Docker Images (optional)
- ✅ Release Notes Validierung

**Artefakte:**
- Linux: `themis_server` (statisch)
- Windows: `themis_server.exe`
- Docker: `themisdb/server:v1.4.0-rc1`

#### 3. `main` Branch - Production Releases

**Zweck**: Stabile Production-Deployments

**Build-Strategie:**
```yaml
# CI auf main: Production Builds + Deployment
trigger: push to main (mit Tag v*.*.*)
strategy: vcpkg Manifest + Static Linking
build-time: ~30-40 min (mit Multi-Arch)
artifacts: Alle Release-Artefakte + Docker Images
```

**CI Workflow:**
- ✅ Production Builds (statisch, optimiert)
- ✅ Multi-Architektur Docker Images (amd64, arm64)
- ✅ GitHub Release erstellen
- ✅ Docker Hub Push (`themisdb/server:latest`, `themisdb/server:v1.4.0`)
- ✅ Dokumentation deployen (GitHub Pages)
- ✅ Release-Ankündigungen

**Deployment Targets:**
```bash
# Docker Hub
docker.io/themisdb/server:latest
docker.io/themisdb/server:v1.4.0
docker.io/themisdb/server:v1.4.0-community

# GitHub Releases
- themis-server-linux-amd64.tar.gz
- themis-server-linux-arm64.tar.gz
- themis-server-windows-amd64.zip
- themis-server-macos-universal.tar.gz
```

**Verwendete Workflow-Dateien:**
- `.github/workflows/release-build.yml`
- `.github/workflows/docker-build.yml`

#### 4. `hotfix/*` Branch - Emergency Fixes

**Zweck**: Kritische Produktions-Fixes

**Build-Strategie:**
```yaml
# CI auf hotfix: Wie Production aber schneller
trigger: push to hotfix/*
strategy: vcpkg Manifest (cached)
build-time: ~10-15 min (mit Cache)
artifacts: Patch-Release Artefakte
priority: HIGH
```

**Fast-Track Workflow:**
- ✅ Essenzielle Tests nur (schnell)
- ✅ Production Build
- ✅ Sofortiges Deployment nach Approval
- ✅ Automatisches Tagging (v1.4.1)

### Branch-to-Workflow Mapping

| Branch Pattern | Workflow | Build-Typ | Deployment | Docker Push |
|---------------|----------|-----------|------------|-------------|
| `develop` | develop-ci.yml | Fast (apt) | ❌ Nie | ❌ Nein |
| `feature/*` | develop-ci.yml | Fast (apt) | ❌ Nie | ❌ Nein |
| `bugfix/*` | develop-ci.yml | Fast (apt) | ❌ Nie | ❌ Nein |
| `release/*` | release-build.yml | Full (vcpkg) | ⚠️ Pre-release | ⚠️ Optional |
| `main` | release-build.yml + docker-build.yml | Full + Static | ✅ Production | ✅ Latest + Tag |
| `hotfix/*` | release-build.yml | Full (cached) | ✅ Hotfix | ✅ Patch Tag |

### Beispiel-Workflow Konfigurationen

**develop-ci.yml** (Fast Feedback):
```yaml
name: Develop CI
on:
  push:
    branches: [develop]
  pull_request:
    branches: [develop]

jobs:
  fast-build:
    strategy:
      matrix:
        os: [ubuntu-latest]
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v3
      - name: Install System Dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y librocksdb-dev libboost-dev ...
      - name: Build
        run: |
          cmake -B build -G Ninja -DTHEMIS_USE_SYSTEM_LIBS=ON
          cmake --build build -j$(nproc)
      - name: Test
        run: cd build && ctest --output-on-failure
```

**release-build.yml** (Production):
```yaml
name: Release Build
on:
  push:
    branches: [main]
    tags: ['v*']

jobs:
  build-release:
    strategy:
      matrix:
        include:
          - os: ubuntu-latest
            triplet: x64-linux
          - os: windows-latest
            triplet: x64-windows
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v3
      - name: Setup vcpkg
        run: |
          git clone https://github.com/microsoft/vcpkg.git
          ./vcpkg/bootstrap-vcpkg.sh
      - name: Build Production
        run: |
          cmake -B build -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DTHEMIS_STATIC_BUILD=ON \
            -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
          cmake --build build -j$(nproc)
      - name: Upload Artifacts
        uses: actions/upload-artifact@v3
        with:
          name: themis-${{ matrix.triplet }}
          path: build/themis_server*
```

### Version-Tagging Konvention

| Branch | Tag Format | Beispiel | Docker Tag |
|--------|-----------|----------|------------|
| `main` | `v{MAJOR}.{MINOR}.{PATCH}` | `v1.4.0` | `latest`, `v1.4.0`, `v1.4` |
| `main` (hotfix) | `v{MAJOR}.{MINOR}.{PATCH}` | `v1.4.1` | `latest`, `v1.4.1`, `v1.4` |
| `release/*` | `v{VERSION}-rc{N}` | `v1.4.0-rc1` | `v1.4.0-rc1` |

**Semantic Versioning:**
- **MAJOR**: Breaking Changes (v2.0.0)
- **MINOR**: Neue Features (v1.4.0)
- **PATCH**: Bugfixes (v1.4.1)

### Build-Cache Strategie

**develop Branch:**
```yaml
# Kein Docker Layer Cache (schnelle apt Builds)
cache:
  paths:
    - ~/.cache/ccache  # Nur Compiler Cache
```

**main Branch:**
```yaml
# Vollständiger vcpkg Cache
cache:
  paths:
    - vcpkg/downloads
    - vcpkg/packages
    - ~/.cache/ccache
```

### Performance-Optimierung nach Branch

| Branch | Build-Zeit Ziel | Cache Strategy | Optimierung |
|--------|-----------------|----------------|-------------|
| `develop` | < 10 min | ccache + apt | Schnelles Feedback |
| `release/*` | < 30 min | vcpkg cache | Vollständig, reproduzierbar |
| `main` | < 40 min | Full cache | + Multi-arch Docker |
| `hotfix/*` | < 15 min | Warmer Cache | Fast-track |

### Zusammenfassung

**Entwickler auf `develop`:**
- Arbeiten mit schnellen System-Library Builds
- Schnelles Feedback (5-10 min)
- Keine Release-Artefakte

**Maintainer auf `release/*`:**
- Vollständige vcpkg Builds für alle Plattformen
- Release-Testing und Stabilisierung
- Pre-release Artefakte

**Production auf `main`:**
- Optimierte, statische Builds
- Multi-Architektur Docker Images
- Automatisches Deployment zu Docker Hub
- GitHub Releases mit allen Artefakten

**Hotfixes:**
- Fast-Track Process
- Minimale Tests, maximale Geschwindigkeit
- Sofortiges Deployment nach Approval

Siehe auch:
- `BRANCHING_STRATEGY.md` - Vollständige Git Flow Dokumentation
- `MIGRATION_GUIDE.md` - Migration zu neuer Branching Strategy
- `.github/workflows/` - Konkrete Workflow-Implementierungen
