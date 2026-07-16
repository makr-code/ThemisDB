# ThemisDB Deployment Documentation

**Stand:** 6. April 2026  
**Version:** v1.3.1  
**Kategorie:** 🚀 Deployment  
**Status:** Production-Ready

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Quick Start](#-quick-start)
- [Unterstützte Plattformen](#-unterstützte-plattformen)
- [Build-Varianten](#-build-varianten)

## 📋 Übersicht

ThemisDB nutzt eine **Offline-First vcpkg Build-Strategie** für reproduzierbare Deployments auf allen Plattformen.

### Kern-Dokumente

1. **[Deployment Strategy](deployment_strategy.md)** - Übergeordnete Build & Deployment Strategie
   - **NEU:** Edition-spezifische Build-Wege (Community, Enterprise, Hyperscaler)
   - **NEU:** Lizenz-bedingte Build-Konfigurationen
   - **NEU:** Edition-spezifische Metriken & Monitoring
2. **[ThemisDB mit llama.cpp auf SoC (Raspberry Pi & Co.)](THEMIS_LLAMA_CPP_SOC_GUIDE.md)** ⭐ **NEU (v1.5.0)** - Umfassende Anleitung für LLM-Inferenz auf Edge-Geräten
   - Raspberry Pi 4/5, Orange Pi 5, NVIDIA Jetson, Rock 5B
   - llama.cpp Integration und Optimierung
   - AI-Beschleuniger-Chips (Coral TPU, Hailo, etc.)
   - Performance-Benchmarks und Best Practices
3. **[Bibliotheken-Übersicht](BIBLIOTHEKEN_UBERSICHT.md)** ⭐ **NEU** - Alle Dependencies mit Vendor-Links
4. **[Build-Optionen Referenz](BUILD_OPTIONEN_REFERENZ.md)** ⭐ **NEU** - Alle 61 CMake Schalter
   - **NEU:** Enterprise/Hyperscaler-spezifische Optionen
5. **[vcpkg Offline Strategy](VCPKG_OFFLINE_STRATEGY.md)** - Offline-First Build-System
6. **[Docker Build](DOCKER_DEPLOYMENT.md)** - Container-basiertes Deployment
7. **[ARM/Raspberry Pi Build](deployment_arm_build.md)** - ARM64/ARMv7 Builds

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
| **Raspberry Pi + llama.cpp** | ARM64 | ✅ Supported | ✅ | ✅ | [SoC LLM Guide](THEMIS_LLAMA_CPP_SOC_GUIDE.md) ⭐ |
| **Orange Pi 5 / Rock 5B** | ARM64 | ✅ Supported | ✅ | ✅ | [SoC LLM Guide](THEMIS_LLAMA_CPP_SOC_GUIDE.md) |
| **NVIDIA Jetson** | ARM64 | ✅ Supported | ✅ | ✅ | [SoC LLM Guide](THEMIS_LLAMA_CPP_SOC_GUIDE.md) |
| **QNAP NAS** | x64 | ✅ Supported | ✅ | ✅ | [QNAP Deployment](deployment_qnap.md) |
| **macOS** | ARM64 (M1/M2) | 🚧 Planned | ❌ | ✅ | TBD |

---

## 📦 Build-Varianten

ThemisDB bietet verschiedene Build-Konfigurationen für unterschiedliche Use-Cases.

**Siehe:** [Build-Optionen Referenz](BUILD_OPTIONEN_REFERENZ.md) für alle 61 CMake Schalter

### Minimal Build (~150 MB)
```bash
cmake -B build \
  -DTHEMIS_ENABLE_LLM=OFF \
  -DTHEMIS_ENABLE_GRPC=OFF \
  -DCMAKE_BUILD_TYPE=Release
```

### Standard Build mit LLM (~250 MB)
```bash
cmake -B build \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_CORE_SHARED=OFF \
  -DCMAKE_BUILD_TYPE=Release
```

### Performance-Optimiert
```bash
cmake -B build \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_MIMALLOC=ON \
  -DTHEMIS_ENABLE_HUGE_PAGES=ON \
  -DTHEMIS_ENABLE_RCU_INDEX=ON \
  -DCMAKE_BUILD_TYPE=Release
```

### Full Build (~350 MB)
```bash
cmake -B build \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GRPC=ON \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_HTTP2=ON \
  -DCMAKE_BUILD_TYPE=Release
```

**Siehe:** [Deployment Strategy](deployment_strategy.md#build-varianten) für alle Optionen

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

> Community-Releases werden nach Docker Hub (`themisdb/themisdb`) via
> `.github/workflows/04-release_publish-community.yml` veroeffentlicht.

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

## Dokumentation in diesem Ordner

### Kern-Dokumentation (v1.3.1)

| Datei | Beschreibung | Status |
|-------|--------------|--------|
| **[deployment_strategy.md](deployment_strategy.md)** | **Hauptdokument:** Build & Deployment Strategie | ✅ Aktuell |
| **[BIBLIOTHEKEN_UBERSICHT.md](BIBLIOTHEKEN_UBERSICHT.md)** | **Alle Dependencies** mit Vendor-Links | ⭐ NEU |
| **[BUILD_OPTIONEN_REFERENZ.md](BUILD_OPTIONEN_REFERENZ.md)** | **Alle 61 CMake Schalter** mit Beispielen | ⭐ NEU |
| **[VCPKG_OFFLINE_STRATEGY.md](VCPKG_OFFLINE_STRATEGY.md)** | Offline-First Build System | ✅ Aktuell |
| **[DOCKER_DEPLOYMENT.md](DOCKER_DEPLOYMENT.md)** | **Konsolidierter** Docker Container Deployment Guide | ✅ Aktuell, Konsolidiert |

### Plattform-Spezifisch

| Datei | Beschreibung | Status |
|-------|--------------|--------|
| [deployment_arm_build.md](deployment_arm_build.md) | ARM64/ARMv7 Build-Anleitung | ✅ Aktuell |
| [deployment_arm_benchmarks.md](deployment_arm_benchmarks.md) | ARM Performance-Daten | ✅ Aktuell |
| [deployment_arm_packages.md](deployment_arm_packages.md) | ARM Package-Management | ✅ Aktuell |
| [deployment_qnap.md](deployment_qnap.md) | QNAP NAS Deployment | ✅ Aktuell |
| [deployment_raspberry_tuning.md](deployment_raspberry_tuning.md) | Raspberry Pi Performance Tuning | ✅ Aktuell |
| [QNAP_CPU_COMPATIBILITY.md](QNAP_CPU_COMPATIBILITY.md) | QNAP CPU Support Matrix | ✅ Aktuell |

### CI/CD & Docker

| Datei | Beschreibung | Status |
|-------|--------------|--------|
| [deployment_docker_multiarch.md](deployment_docker_multiarch.md) | Multi-Arch Docker Builds (Details) | ✅ Aktuell |
| [deployment_cicd_multiarch.md](deployment_cicd_multiarch.md) | CI/CD Pipeline-Konfiguration | ✅ Aktuell |

### Edition & Strategie

| Datei | Beschreibung | Status |
|-------|--------------|--------|
| [EDITION_DEPLOYMENT_STRATEGY.md](EDITION_DEPLOYMENT_STRATEGY.md) | Multi-Edition Deployment | ✅ Aktuell |
| [EDITION_CONTROL_MECHANISMS.md](EDITION_CONTROL_MECHANISMS.md) | Edition Control Implementation | ✅ Aktuell |
| [v1.3.5_RELEASE_BUILD_STRATEGY.md](v1.3.5_RELEASE_BUILD_STRATEGY.md) | v1.3.5 Release-Planung | 📋 Geplant |
| [80PERCENT_COVERAGE_STRATEGY.md](80PERCENT_COVERAGE_STRATEGY.md) | Community Edition Strategie | ✅ Aktuell |
| [PRICING_MODEL_v1.3.5.md](PRICING_MODEL_v1.3.5.md) | Pricing Model | ✅ Aktuell |

### Implementierungs-Reports

| Datei | Beschreibung | Status |
|-------|--------------|--------|
| [PORT_REFERENCE.md](PORT_REFERENCE.md) | Port-Mapping Referenz | ✅ Aktuell |
| [PORT_STANDARDIZATION.md](PORT_STANDARDIZATION.md) | Port-Standardisierung | ✅ Aktuell |

### Archivierte Dokumente

Historische Dokumentation (v1.3.0 Implementation Reports, konsolidierte Docker Docs) befindet sich in [archive/](archive/).

**Konsolidiert:** docker_build.md und docker_status.md wurden in DOCKER_DEPLOYMENT.md zusammengeführt.

---

## Verwandte Dokumentation

- [Guides: Deployment](../guides/guides_deployment.md) - Deployment Guide
- [Guides: Build Strategy](../guides/guides_build_strategy.md) - Build Toolchain
- [CI/CD](../build/README.md) - CI/CD Workflows
- [Architecture](../architecture/) - System Architecture Dokumentation
- [Performance](../performance/) - Performance Optimierungen

---

## 🎯 Zusammenfassung

Die Deployment-Dokumentation für ThemisDB v1.3.1 bietet:

✅ **Vollständige Library-Übersicht** mit 17 Kern-Dependencies + optionale Features  
✅ **61 CMake Build-Optionen** vollständig dokumentiert  
✅ **Offline-First vcpkg Strategie** für reproduzierbare Builds  
✅ **Multi-Plattform Support** (Windows, Linux, Docker, ARM, QNAP)  
✅ **Vendor-Dokumentation verlinkt** für alle Dependencies  
✅ **Best Practices** für Production, Development, Performance, IoT  
✅ **Konsolidierte Docker-Dokumentation** für einfachere Navigation  
✅ **Edition-spezifische Build-Strategien** (Community, Enterprise, Hyperscaler)  
✅ **Historische Dokumente archiviert** für saubere Struktur  

### Letzte Konsolidierung (v1.3.1)

**26. Dezember 2025:** Docker-Dokumentation konsolidiert
- `docker_build.md` → archiviert (Inhalte in DOCKER_DEPLOYMENT.md integriert)
- `docker_status.md` → archiviert (Historischer Status-Report)
- `DOCKER_DEPLOYMENT.md` → Erweitert mit Build-Strategien und Multi-Arch-Details

---

## 🌐 WordPress Plugin für Downloads

ThemisDB bietet ein WordPress Plugin, das automatisch die neuesten Releases von GitHub abruft und auf Ihrer Website als Download-Links mit SHA256-Checksums anzeigt.

**Features:**
- ✅ Automatisches Abrufen von GitHub Releases
- ✅ Download-Links für alle Plattformen (Windows, Linux, Docker, QNAP, ARM)
- ✅ SHA256-Checksums Anzeige
- ✅ Browser-basierte Download-Verifizierung
- ✅ Mehrere Anzeigestile (Standard, Kompakt, Tabelle)
- ✅ Cache-System zur Reduzierung von API-Aufrufen

**Dokumentation:** [wordpress-plugin/README.md](../../../wordpress-plugin/README.md)

**Installation:**
```bash
# Plugin-Verzeichnis in WordPress kopieren
cp -r wordpress-plugin/themisdb-downloads /var/www/html/wp-content/plugins/

# In WordPress aktivieren
WordPress Admin → Plugins → ThemisDB Downloads → Aktivieren
```

**Verwendung:**
```
[themisdb_downloads]              # Neueste Version anzeigen
[themisdb_downloads show="all"]   # Alle Releases anzeigen
[themisdb_verify]                 # Verifizierungs-Tool
```

Siehe [wordpress-plugin/themisdb-downloads/README.md](../../../wordpress-plugin/themisdb-downloads/README.md) für Details.

---

**Letzte Aktualisierung:** 7. Januar 2026  
**Version:** v1.3.1 + WordPress Plugin v1.0.0  
**Status:** Production-Ready ✅
