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
