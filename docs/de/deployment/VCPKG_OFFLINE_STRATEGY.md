# vcpkg Offline Build Strategy

**Stand:** 6. April 2026  
**Version:** v1.3.1  
**Kategorie:** 🚀 Deployment  
**Plattformen:** Windows, Linux, Docker, ARM/Raspberry Pi

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Cache-Architektur](#-cache-architektur)
- [Setup-Prozess](#-setup-prozess)

## 🎯 Übersicht

ThemisDB nutzt vcpkg mit einer **Offline-First Strategie** für reproduzierbare, netzwerk-unabhängige Builds auf allen Plattformen.

### Vorteile

✅ **Offline-fähig:** Builds ohne Internet nach initialem Setup  
✅ **Reproduzierbar:** Identische Binaries durch versionierte Baseline  
✅ **Schnell:** ~50% schnellere Builds (keine Downloads)  
✅ **CI/CD-ready:** Cache zwischen Build-Agents teilbar  
✅ **Air-Gapped:** Enterprise/Government Compliance  
✅ **Cross-Platform:** Identischer Workflow für Windows/Linux/ARM  

---

## 📦 Cache-Architektur

### Verzeichnisstruktur

```
themis/
├── vcpkg/
│   ├── downloads/              ← ~2.5 GB - **SINGLE SOURCE OF TRUTH**
│   │   ├── openssl-3.6.0.tar.gz
│   │   ├── rocksdb-10.4.2.tar.gz
│   │   ├── boost_1_89_0.tar.gz
│   │   ├── simdjson-4.2.2.tar.gz
│   │   ├── tbb-2022.2.0.tar.gz
│   │   ├── arrow-21.0.0.tar.gz
│   │   ├── grpc-1.62.0.tar.gz
│   │   ├── protobuf-5.29.5.tar.gz
│   │   └── [130+ weitere Packages]
│   │
│   ├── buildtrees/            ← ~3 GB - Temp (nicht committet)
│   ├── packages/              ← ~10 GB - Installiert (nicht committet)
│   │
│   └── scripts/
│       └── buildsystems/
│           └── vcpkg.cmake    ← CMake Integration
│
├── vcpkg.json                 ← Dependency Manifest
├── vcpkg-configuration.json   ← Baseline & Registry
└── .gitignore                 ← downloads/ WIRD committet
```

### Was wird versioniert?

| Verzeichnis | Größe | Git | Zweck |
|-------------|-------|-----|-------|
| `vcpkg/downloads/` | ~2.5 GB | ✅ **JA** | Source-Archive für offline builds |
| `vcpkg/buildtrees/` | ~3 GB | ❌ NEIN | Temporäre Build-Artefakte |
| `vcpkg/packages/` | ~10 GB | ❌ NEIN | Installierte Libraries |
| `vcpkg/installed/` | ~5 GB | ❌ NEIN | Triplet-spezifische Installs |

**Wichtig:** `.gitignore` ausnahme für `downloads/`:
```gitignore
vcpkg/*
!vcpkg/downloads/
```

---

## 🔧 Setup-Prozess

### 1. Initiales Setup (Einmalig pro Entwicklungsumgebung)

#### Windows
```powershell
# vcpkg bootstrap
cd C:\VCC\themis\vcpkg
.\bootstrap-vcpkg.bat -disableMetrics

# Offline cache erstellen
cd ..
.\scripts\setup-vcpkg-offline.ps1
```

#### Linux/macOS
```bash
# vcpkg bootstrap
cd /path/to/themis/vcpkg
./bootstrap-vcpkg.sh -disableMetrics

# Offline cache erstellen
cd ..
./scripts/setup-vcpkg-offline.sh
```

#### ARM/Raspberry Pi
```bash
# Identisch wie Linux, aber mit arm64-linux triplet
export VCPKG_ROOT=/path/to/themis/vcpkg
./scripts/setup-vcpkg-offline.sh --triplet arm64-linux
```

### 2. Was macht `setup-vcpkg-offline.ps1`?

```powershell
# 1. vcpkg Repository update (für neue Packages)
git -C vcpkg pull

# 2. Manifest-Mode: Dependencies analysieren
vcpkg install --triplet x64-windows --dry-run

# 3. Source-Archive herunterladen (ohne Build)
vcpkg x-download openssl rocksdb boost-asio boost-beast simdjson tbb arrow hnswlib ...

# 4. Multi-Triplet Support
foreach ($triplet in @("x64-windows", "x64-linux", "arm64-linux")) {
    vcpkg x-download --triplet $triplet
}

# 5. Verifikation
Write-Host "✅ vcpkg offline cache bereit: $(Get-ChildItem vcpkg/downloads | Measure-Object).Count Packages"
```

**Ergebnis:**
- `vcpkg/downloads/` enthält ~130+ Source-Archive (~2.5 GB)
- Alle zukünftigen Builds sind offline-fähig
- Cache kann zwischen Maschinen geteilt werden (USB/NAS/Git LFS)

---

## 🏗️ Build-Workflows

### Workflow 1: Lokaler Entwickler-Build

```powershell
# Windows
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# vcpkg offline cache bereits im Repo (via Git LFS oder direkt committed)
# KEIN Internet nötig!

cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

### Workflow 2: CI/CD Pipeline

```yaml
# GitHub Actions / GitLab CI
- name: Restore vcpkg cache
  uses: actions/cache@v3
  with:
    path: vcpkg/downloads
    key: vcpkg-downloads-${{ hashFiles('vcpkg.json') }}

- name: Build
  run: |
    cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
    cmake --build build
```

**Vorteil:** Cache wird zwischen Runs wiederverwendet, ~10 Min Zeitersparnis.

### Workflow 3: Air-Gapped Build (Enterprise)

```powershell
# 1. Auf Maschine MIT Internet: Cache vorbereiten
.\scripts\setup-vcpkg-offline.ps1
Compress-Archive -Path vcpkg\downloads -DestinationPath vcpkg-cache.zip

# 2. Cache übertragen (USB/Intranet)
Copy-Item vcpkg-cache.zip \\air-gapped-server\share\

# 3. Auf Air-Gapped Maschine: Extrahieren & Bauen
Expand-Archive vcpkg-cache.zip -DestinationPath C:\VCC\themis\vcpkg\
cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build  # Komplett offline!
```

### Workflow 4: Docker Multi-Stage Build

```dockerfile
# Stage 1: vcpkg cache layer (cached, nur bei vcpkg.json Änderung rebuild)
FROM ubuntu:22.04 AS vcpkg-cache
WORKDIR /opt/themis
COPY vcpkg/ vcpkg/
COPY vcpkg.json vcpkg-configuration.json ./
RUN vcpkg/vcpkg install --triplet x64-linux

# Stage 2: Build (nutzt cached layer)
FROM ubuntu:22.04 AS builder
COPY --from=vcpkg-cache /opt/themis/vcpkg/installed /opt/themis/vcpkg/installed
COPY . /opt/themis
RUN cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake && \
    cmake --build build
```

**Vorteil:** vcpkg Layer wird nur bei Dependency-Änderungen neu gebaut.

---

## 🌍 Plattform-Spezifika

### Windows

**Triplets:** `x64-windows`, `x64-windows-static`

```powershell
# Static build (empfohlen wegen DLL export limit 65,535)
cmake -B build -DVCPKG_TARGET_TRIPLET=x64-windows-static \
  -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DTHEMIS_CORE_SHARED=OFF
```

**Besonderheiten:**
- MSVC Link-Time Code Generation (LTCG) deaktiviert für shared builds
- vcpkg packages ~15 GB installiert (daher offline cache wichtig)

### Linux (Ubuntu/Debian)

**Triplets:** `x64-linux`, `x64-linux-dynamic`

```bash
# Standard dynamic build
cmake -B build -DVCPKG_TARGET_TRIPLET=x64-linux \
  -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
```

**Besonderheiten:**
- System Dependencies: `build-essential`, `cmake`, `ninja-build`
- vcpkg packages ~8 GB installiert

### ARM64/Raspberry Pi

**Triplets:** `arm64-linux`, `armv7-linux`

```bash
# ARM64 (Raspberry Pi 4/5, 64-bit OS)
cmake -B build -DVCPKG_TARGET_TRIPLET=arm64-linux \
  -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DTHEMIS_QNAP_BUILD=ON  # Baseline x86-64, kein AVX

# ARMv7 (Raspberry Pi 2/3, 32-bit OS)
cmake -B build -DVCPKG_TARGET_TRIPLET=armv7-linux \
  -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
```

**Besonderheiten:**
- NEON SIMD automatisch aktiviert
- Build-Zeit ~2-3x länger als x86-64
- Empfohlen: 4+ GB RAM, SSD statt SD-Karte

### Docker

**Multi-Arch:** `linux/amd64`, `linux/arm64`

```dockerfile
# Nutzt vcpkg downloads/ als cache layer
COPY vcpkg/downloads vcpkg/downloads
RUN vcpkg install --triplet ${VCPKG_TRIPLET}
```

**Besonderheiten:**
- Nur `downloads/` in Image kopieren (~2.5 GB)
- NICHT `packages/` oder `buildtrees/` kopieren
- Multi-stage build spart ~8 GB Image-Größe

---

## 📊 Performance-Vergleich

### Build-Zeiten (Windows MSVC, Ryzen 9 5950X)

| Szenario | Mit Internet | Mit Cache | Ersparnis |
|----------|--------------|-----------|-----------|
| **Clean Build** | 42 Min | 18 Min | **57% schneller** |
| **Rebuild** | 38 Min | 15 Min | **61% schneller** |
| **CI Pipeline** | 55 Min | 22 Min | **60% schneller** |

### Download-Volumen

| Build-Typ | Ohne Cache | Mit Cache | Ersparnis |
|-----------|------------|-----------|-----------|
| **Minimal** | ~1.2 GB | 0 MB | **100%** |
| **LLM** | ~1.8 GB | 0 MB | **100%** |
| **Full** | ~2.5 GB | 0 MB | **100%** |

---

## 🔄 Cache-Wartung

### Cache-Update bei neuen Dependencies

```powershell
# Nach vcpkg.json Änderung: Neue Packages hinzufügen
.\scripts\update-vcpkg-cache.ps1

# Manuelle Verifikation
vcpkg x-download --dry-run
```

### Cache-Cleanup (optional)

```powershell
# Unbenutzte Archive entfernen (spart ~500 MB)
.\scripts\cleanup-vcpkg-cache.ps1

# Kompletter Rebuild (bei Problemen)
Remove-Item -Recurse -Force vcpkg\buildtrees, vcpkg\packages
.\scripts\setup-vcpkg-offline.ps1
```

### Binary Cache (Advanced)

```powershell
# Binary cache aktivieren (experimentell)
$env:VCPKG_BINARY_SOURCES="clear;files,$PWD\vcpkg-binary-cache,readwrite"

# Erster Build: Erstellt binaries
cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build

# Zweiter Build: Nutzt cached binaries (~5x schneller)
cmake -B build2 -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build2
```

**Resultat:** `vcpkg-binary-cache/` (~600 MB komprimiert) kann geteilt werden.

---

## 🚀 Best Practices

### ✅ DO

- **Committe `vcpkg/downloads/`** ins Repository (Git LFS empfohlen)
- **Nutze identische vcpkg baseline** für alle Entwickler
- **Teile Binary Cache** im Team (Netzwerk-Share/S3)
- **Aktiviere ccache** (Linux) für schnellere Rebuilds
- **Verwende Docker Multi-Stage** für kleinere Images

### ❌ DON'T

- **Committe NICHT `vcpkg/packages/`** oder `buildtrees/` (zu groß, platform-spezifisch)
- **Mixe NICHT verschiedene vcpkg-Versionen** im Team
- **Vergiss NICHT** `setup-vcpkg-offline.ps1` vor erstem Build
- **Nutze NICHT** `vcpkg install` direkt (nutze CMake Manifest-Mode)

---

## 📚 Referenzen

- **[CMakeLists.txt](../../CMakeLists.txt)** - vcpkg Integration
- **[vcpkg.json](../../vcpkg.json)** - Dependency Manifest
- **[Deployment Strategy](deployment_strategy.md)** - Übergeordnete Build-Strategie
- **[Docker Build](archive/docker_build.md)** - Docker-spezifische Details
- **[ARM Build](deployment_arm_build.md)** - Raspberry Pi / ARM Details

---

**Erstellt:** 18. Dezember 2025  
**Maintainer:** ThemisDB Build Team
