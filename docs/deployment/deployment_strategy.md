# ThemisDB Build & Deployment Strategy

**Version:** 3.0.0 (v1.3.0 LLM Integration)  
**Last Updated:** 17. Dezember 2025  
**Status:** Production-Ready  
**Architecture:** Unified Build System with Modular Features

---

## Overview

ThemisDB v1.3.0 verwendet ein **modulares, cache-gesteuertes Build-System** mit optionalen Features:

1. **Zentrale Build-Orchestrierung:** `.\scripts\build.ps1` (Cross-Platform Entry Point)
2. **Modulare Features:** LLM, RPC, GPU können unabhängig aktiviert werden
3. **Automatisches Cache-Management:** `.\scripts\update-vcpkg-cache.ps1` (präventiv vor jedem Build)
4. **Platform-spezifische Builds:** Windows (MSVC), Linux (GCC), Docker (multi-arch amd64/arm64)
5. **Offline-First Architektur:** vcpkg\downloads/ (~2GB) als Single Source of Truth
6. **Distribution:** Docker Hub, GitHub Releases, Debian/RPM Repositories

**NEU in v1.3.0:**
- ✅ LLM Integration mit llama.cpp (optional: +96 files)
- ✅ RPC Framework mit gRPC (optional: +26 files)
- ✅ GPU/CUDA Support für LLM (optional)
- ✅ Modular Build Flags für flexible Deployments

---

## Quick Start

### Option 1: Schnellbuild (Minimal)

```powershell
# Windows MSVC Release-Build mit automatischem Cache-Update
.\quick-build.ps1
```

### Option 2: Gezielte Builds

```powershell
# Nur Windows
.\scripts\build.ps1 -Target windows

# Nur Linux (GCC)
.\scripts\build.ps1 -Target linux

# Nur Docker (Multi-Arch amd64 + arm64)
.\scripts\build.ps1 -Target docker

# Alle Plattformen
.\scripts\build.ps1 -Target all

# Docker mit Push zu Registry
.\scripts\build.ps1 -Target docker -Push -Tag v1.3.0
```

### Option 3: v1.3.0 Feature Builds

```bash
# Minimal Build (Core nur, ~150 MB)
cmake -S . -B build \
  -DTHEMIS_ENABLE_LLM=OFF \
  -DTHEMIS_BUILD_RPC_FRAMEWORK=OFF \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# LLM Build mit GPU (Core + LLM + CUDA, ~250 MB)
cmake -S . -B build-llm \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-llm -j$(nproc)

# Full Build (Core + LLM + RPC + GPU, ~300 MB)
cmake -S . -B build-full \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DTHEMIS_BUILD_RPC_FRAMEWORK=ON \
  -DTHEMIS_ENABLE_GPU=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-full -j$(nproc)
```

### Option 4: Manueller Cache-Update (Optional)

```powershell
# Cache-Update ohne Build (z.B. für Offline-Szenarios vorbereiten)
.\scripts\update-vcpkg-cache.ps1 -Triplet x64-windows, x64-linux, arm64-linux
```

---

## Build-Plattformen (v1.3.0)

| Platform | Triplet | Compiler | Target | CMake Preset | Feature Flags |
|----------|---------|----------|--------|--------------|---------------|
| **Windows** | x64-windows | MSVC 2022 | Windows 10+ (x64) | default | LLM, RPC, GPU |
| **Linux (x64)** | x64-linux | GCC 11.4 | Ubuntu 22.04+ (x64) | linux-gcc | LLM, RPC, GPU, CUDA |
| **Linux (ARM64)** | arm64-linux | GCC 11.4 | Ubuntu 22.04+ (ARM64) | linux-gcc-arm | LLM, RPC |
| **Docker** | x64-linux / arm64-linux | GCC 11.4 | Docker Multi-Arch | docker-buildx | LLM, RPC, GPU, CUDA |
| **QNAP NAS** | x64-linux | GCC 11.4 | QNAP x86_64 | linux-gcc | Minimal |
| **macOS** | arm64-osx / x64-osx | Clang | macOS 11+ (x64/ARM) | macos | ⏳ Geplant |

**v1.3.0 Modular Build Matrix:**

| Configuration | ENABLE_LLM | BUILD_RPC | ENABLE_CUDA | ENABLE_GPU | Binary Size | Build Time |
|---------------|------------|-----------|-------------|------------|-------------|------------|
| **Minimal** | OFF | OFF | OFF | OFF | ~150 MB | 15-20 min |
| **LLM** | ON | OFF | OFF | OFF | ~250 MB | 25-30 min |
| **LLM+GPU** | ON | OFF | ON | ON | ~300 MB | 30-35 min |
| **LLM+RPC** | ON | ON | OFF | OFF | ~280 MB | 30-35 min |
| **Full** | ON | ON | ON | ON | ~350 MB | 35-40 min |

---

## Cache-Architektur (Offline-First, v1.3.0)

### Speicherstruktur

```
.\vcpkg\downloads\              (~2.5 GB, 135+ Source-Archive) ← SINGLE SOURCE OF TRUTH
  ├─ boost_1.89.0/
  ├─ rocksdb-8.x/
  ├─ simdjson-x/
  ├─ tbb-x/
  ├─ hnswlib-x/
  ├─ openssl-x/
  ├─ curl-x/
  ├─ spdlog-x/
  ├─ fmt-x/
  ├─ nlohmann-json-x/
  ├─ yaml-cpp-x/
  ├─ grpc-x/                    ← v1.3.0 (RPC)
  ├─ protobuf-x/                ← v1.3.0 (RPC)
  ├─ faiss-x/                   ← v1.3.0 (GPU)
  └─ [weitere Archive...]

.\vcpkg\packages\               (~10 GB, ephemär)
  └─ NICHT in Docker kopiert!

.\vcpkg\buildtrees\             (~3 GB, Temp. Build-Artifacts)
  └─ NICHT in Docker kopiert!

.\src\llm\                      ← v1.3.0 (llama.cpp integration, bundled)
  ├─ llamacpp_plugin.cpp
  ├─ gguf_loader.cpp
  ├─ paged_kv_cache.cpp
  └─ [96 files total]
```

### Cache-Update-Flow

```
┌─────────────────────────────────────────┐
│ .\scripts\update-vcpkg-cache.ps1        │
│ (Läuft VOR jedem Build automatisch)     │
└────────────┬────────────────────────────┘
             │
             ├─→ 1. Git Pull vcpkg/master
             │      (Aktualisiert Portfile-Versionen)
             │
             ├─→ 2. Registry-Baseline-Update
             │      (Neue Packages verfügbar)
             │
             ├─→ 3. Pre-Fetch für Triplet
             │      ├─ x64-windows
             │      ├─ x64-linux
             │      └─ arm64-linux
             │
             └─→ 4. .\vcpkg\downloads\ aktualisiert
                    (Alle Abhängigkeiten vorab)
```

**Automatische Integration:**
- Alle Build-Skripte rufen `update-vcpkg-cache.ps1` auf
- Optional deaktivierbar via `$SKIP_CACHE_UPDATE = $true`
- Docker-Builds kopieren nur `vcpkg/downloads/` in Image (nicht packages/buildtrees/)

---

## Build Process (Konsolidiert)

### Windows Build (MSVC 2022)

**Script:** `.\scripts\build-windows.ps1`

```powershell
# 1. Automatischer Cache-Update (x64-windows)
.\scripts\update-vcpkg-cache.ps1 -Triplet x64-windows

# 2. CMake Configuration (Release)
cmake -B build-msvc `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DCMAKE_BUILD_TYPE=Release `
   # Hinweis: Version wird automatisch aus der Datei `VERSION` gelesen

# 3. Compilation & Linking
cmake --build build-msvc --config Release -j 4

# 4. Binary Output
#    ✓ build-msvc\Release\themis_server.exe
#    ✓ build-msvc\Release\themis_cli.exe (falls vorhanden)

# 5. Time Estimate: 25-35 min (erste Build), 5-10 min (inkrementell)
```

**Abhängigkeiten (vcpkg x64-windows):**
- rocksdb[lz4,zstd], simdjson, tbb, hnswlib
- boost-{system,asio,beast,optional}
- fmt, spdlog, nlohmann-json, curl, yaml-cpp

**Output:**
- `build-msvc\Release\themis_server.exe` (Hauptdatei)
- Statisch gelinkte Dependencies (eingebettet)

### Linux Build (GCC 11.4)

**Script:** `.\scripts\build-linux.sh` (oder WSL-Integration)

```bash
# 1. Cache-Update (x64-linux)
./scripts/update-vcpkg-cache.ps1 -Triplet x64-linux

# 2. CMake Configuration
cmake -B build-linux \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET=x64-linux \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_CXX_COMPILER=g++

# 3. Compilation
cmake --build build-linux --parallel 4

# 4. Output
#    ✓ build-linux/themis_server
#    ✓ build-linux/themis_cli (falls vorhanden)

# 5. Time Estimate: 30-40 min (erste Build), 8-12 min (inkrementell)
```

**Abhängigkeiten (vcpkg x64-linux):**
- Identisch zu Windows, aber für Linux kompiliert
- Systemlibraries: libssl-dev, libcurl4-openssl-dev, zlib1g-dev (optional)

**Output:**
- `build-linux/themis_server` (Hauptdatei, ELF x86_64)

### Linkage-Varianten (monolithisch vs. DLL/.so)

Sie können die Artefaktform per CMake steuern:

```powershell
# Monolithisch: statischer Core, exe beinhaltet Logik
cmake -B build-msvc `
   -G "Visual Studio 17 2022" -A x64 `
   -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" `
   -DCMAKE_BUILD_TYPE=Release `
   -DTHEMIS_CORE_SHARED=OFF

# Standard (dynamisch): exe + DLL/.so – geteilte Core-Library
cmake -B build-msvc `
   -G "Visual Studio 17 2022" -A x64 `
   -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" `
   -DCMAKE_BUILD_TYPE=Release `
   -DTHEMIS_CORE_SHARED=ON

# Maximale Portabilität (QNAP/alt): statisch linkende Runtime
cmake -B build-linux \
   -DCMAKE_BUILD_TYPE=Release \
   -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" \
   -DVCPKG_TARGET_TRIPLET=x64-linux \
   -DTHEMIS_STATIC_BUILD=ON
```

Hinweise:
- Standard ist dynamisch: `THEMIS_CORE_SHARED=ON` (Windows/Linux). Pakete enthalten DLL/.so zusätzlich zum Binary.
- QNAP/Static: `THEMIS_STATIC_BUILD=ON` oder `THEMIS_QNAP_BUILD=ON` erzwingen statischen Core.
- Unter Windows exportiert CMake für `THEMIS_CORE_SHARED=ON` automatisch Symbole (`WINDOWS_EXPORT_ALL_SYMBOLS`).

### Docker Multi-Arch Build (amd64 + arm64)

**Script:** `.\scripts\build-docker.ps1`

```powershell
# 1. Cache-Update (beide Triplets)
.\scripts\update-vcpkg-cache.ps1 -Triplet x64-linux, arm64-linux

# 2. Docker Buildx Configuration
docker buildx ls  # Stellt sicher, dass buildx verfügbar ist

# 3. Multi-Arch Build
docker buildx build \
  --platform linux/amd64,linux/arm64 \
   --tag themisdb/themisdb:v$((Get-Content VERSION).Trim()) \
  --tag themisdb/themisdb:latest \
  --build-arg VCPKG_ENABLE_ONLINE=OFF \
   --build-arg THEMIS_VERSION=$((Get-Content VERSION).Trim()) \
   # Version wird im Build aus `VERSION` bezogen (OCI Label: org.opencontainers.image.version)
  .

# 4. Optionaler Push zu Registry
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  --push \
   --tag themisdb/themisdb:v$((Get-Content VERSION).Trim()) \
  .

# 5. Time Estimate: 50-60 min (beide Architekturen)
```

**Dockerfile Highlights:**
- Multi-stage Build (Größe ~150MB statt 500MB+)
- Pre-cached Dependencies aus `.\vcpkg\downloads\`
- `VCPKG_ENABLE_ONLINE=OFF` (keine Online-Fetches während Build)
- Offline-First: Sämtliche Archives müssen vorab in Cache vorhanden sein

**Output:**
- `themisdb/themisdb:v1.0.1` (linux/amd64)
- `themisdb/themisdb:v1.0.1` (linux/arm64)
- Manifest-Liste für Multi-Arch Pull

---

## Automatische Cache-Updates (update-vcpkg-cache.ps1)

### Funktionsweise

```powershell
.\scripts\update-vcpkg-cache.ps1 `
  -Triplet @("x64-windows", "x64-linux", "arm64-linux") `
  -Force:$false
```

**Schritte:**

1. **Git Update**
   ```bash
   cd vcpkg && git pull origin master
   ```
   Holt neueste Portfile-Versionen von vcpkg-Repository

2. **Registry-Baseline aktualisieren**
   ```bash
   vcpkg x-update-baseline --add-initial-baseline
   ```
   Synchronisiert verfügbare Package-Versionen

3. **Abhängigkeits-Pre-Fetch pro Triplet**
   ```bash
   vcpkg fetch-all-archives --triplet x64-windows
   vcpkg fetch-all-archives --triplet x64-linux
   vcpkg fetch-all-archives --triplet arm64-linux
   ```
   Lädt alle Source-Archives für die Triplets herunter

4. **Cache-Validierung**
   - Überprüft, dass alle 119 Archives vorhanden sind
   - Berichtet Größe (~2GB) und Dateianzahl
   - Optional: Generiert Checksums

### Automatische Integration

Alle Build-Skripte rufen diese Funktion automatisch auf:

```powershell
# Beispiel: .\scripts\build.ps1
function Invoke-BuildTarget {
    # Automatic Cache Update (verhindert offline-Fehler)
    .\scripts\update-vcpkg-cache.ps1 -Triplet $tripletList
    
    # Platform-specific build follows...
}
```

**Kann deaktiviert werden via Umgebungsvariable:**
```powershell
$env:SKIP_VCPKG_UPDATE = "1"
.\scripts\build.ps1 -Target windows
```

---

## Offline-First Build (Szenario)

### Setup für Offline-Betrieb

1. **Cache einmalig synchronisieren (mit Internet):**
   ```powershell
   # Auf Build-Maschine mit Internet:
   .\scripts\update-vcpkg-cache.ps1 -Triplet x64-windows, x64-linux, arm64-linux
   
   # Dann .\vcpkg\downloads\ (~2GB) sichern
   Compress-Archive -Path ".\vcpkg\downloads" -DestinationPath "vcpkg-cache-2025-12-12.zip"
   ```

2. **Auf Offline-Maschine:**
   ```powershell
   # Cache hochladen
   Expand-Archive -Path "vcpkg-cache-2025-12-12.zip" -DestinationPath "."
   
   # Builds funktionieren ohne Netzwerk
   $env:SKIP_VCPKG_UPDATE = "1"
   .\scripts\build.ps1 -Target all
   ```

3. **Docker Offline-Build (mit lokalem Cache):**
   ```powershell
   docker build \
     --build-arg VCPKG_ENABLE_ONLINE=OFF \
     --build-arg VCPKG_ASSET_SOURCES="file:///opt/vcpkg/downloads" \
     -t themisdb/themisdb:v1.0.1-offline .
   ```

---

## Triplet-spezifische Abhängigkeiten

### Kernpaket-Liste (alle Triplets)

| Package | Version Range | Triplet Support | Größe Cache |
|---------|---------------|-----------------|-------------|
| rocksdb | [8.0,) | all | ~200 MB |
| simdjson | [3.0,) | all | ~20 MB |
| tbb | [2021.0,) | all | ~50 MB |
| hnswlib | [0.7,) | all | ~5 MB |
| boost-system | [1.89.0] | all | ~30 MB |
| boost-asio | [1.89.0] | all | ~25 MB |
| boost-beast | [1.89.0] | all | ~35 MB |
| boost-optional | [1.89.0] | all | ~5 MB |
| openssl | [3.0,) | all | ~80 MB |
| fmt | [10.0,) | all | ~10 MB |
| spdlog | [1.12,) | all | ~15 MB |
| nlohmann-json | [3.11,) | all | ~2 MB |
| curl | [8.0,) | all | ~30 MB |
| yaml-cpp | [0.8,) | all | ~15 MB |
| **Gesamt** | | | **~522 MB** |

**Hinweis:** Zusätzliche Abhängigkeiten (z.B. Zstandard, LZ4) sind Transitive Dependencies und werden automatisch mitgezogen.

### x64-windows (MSVC 2022)
```
cmake, ninja, vcpkg, git, powershell 5.1+
Visual Studio 2022 (Buildtools mind.)
```

### x64-linux & arm64-linux (GCC 11.4)
```
cmake 3.25+
ninja-build
g++ 11.4+
git
python3
```

---

## Distribution Channels

### 1. Docker Hub (Primär)

**Repository:** `themisdb/themisdb`

**Tags:**
- `latest` - Neueste stabile Release (prod)
- `v1.0.1` - Spezifische Version
- `v1.0` - Minor-Version
- `v1` - Major-Version
- `edge` - Nightly Builds (dev)
- `qnap` - QNAP-optimiert (latest)
- `v1.0.1-qnap` - QNAP-spezifische Version

**Build & Push:**
```powershell
# Automatisch via CI/CD
.\scripts\build.ps1 -Target docker -Push -Tag v1.0.1
```

**Abruf:**
```bash
# Multi-Arch Pull (auto selects amd64 or arm64)
docker pull themisdb/themisdb:v1.0.1

# Explizit amd64
docker pull themisdb/themisdb:v1.0.1 --platform linux/amd64

# Explizit arm64
docker pull themisdb/themisdb:v1.0.1 --platform linux/arm64
```

### 2. GitHub Releases (Binäre Pakete)

**URL:** https://github.com/makr-code/ThemisDB/releases

**Assets pro Release:**
- Quellcode (ZIP + tar.gz) - Auto-generiert
- Binary-Pakete:
  - `themisdb-v1.0.1-linux-x64.zip`
  - `themis-v1.0.1-windows-x64.zip`
  - `themisdb_1.0.1_amd64.deb`
  - `themisdb-1.0.1-1.x86_64.rpm`
  - `themisdb-v1.0.1-qnap-x64.zip`
- Checksums: `SHA256SUMS.txt`
- Release Notes: `RELEASE_NOTES_v1.0.1.md`

**Build & Upload:**
```powershell
# Lokal alle Binäre erzeugen
.\scripts\build.ps1 -Target all

# Upload zu GitHub Release (via CI/CD oder Manual)
# Assets landen in release/ Verzeichnis
```

### 3. Linux Package Repositories

#### Debian/Ubuntu Repository (Geplant)

```bash
# Repository hinzufügen
echo "deb https://repo.themisdb.org/debian stable main" | \
  sudo tee /etc/apt/sources.list.d/themisdb.list

# Installieren
sudo apt update
sudo apt install themisdb
```

#### RPM Repository (RHEL/CentOS/Fedora) (Geplant)

```bash
# Repository hinzufügen
sudo tee /etc/yum.repos.d/themisdb.repo <<EOF
[themisdb]
name=ThemisDB Repository
baseurl=https://repo.themisdb.org/rpm/el\$releasever/\$basearch
enabled=1
gpgcheck=1
gpgkey=https://repo.themisdb.org/rpm/RPM-GPG-KEY-themisdb
EOF

# Installieren
sudo yum install themisdb
```

#### Homebrew (macOS) (Geplant)

```bash
brew install themisdb
```

---

## Release Checklist

### Pre-Release

 - [ ] `VERSION` aktualisieren (Single-Source-of-Truth)
- [ ] Update `CHANGELOG.md` mit Release-Notes
- [ ] Update `docs/VERSION.json` Struktur/Datei
- [ ] Alle Unit-Tests lokal laufen (pass)
- [ ] Build all platforms lokal testen
- [ ] Performance-Benchmarks (optional)

### Release-Prozess

1. **Git Tag erstellen:**
   ```bash
   $ver = (Get-Content VERSION).Trim()
   git tag -a v$ver -m "Release v$ver: [Release Description]"
   git push origin v$ver
   ```

2. **Automatische CI/CD (GitHub Actions):**
   - Trigger auf Tag-Push
   - Alle Builds parallel (Windows/Linux/Docker)
   - Binary-Extraktion & Packaging
   - Debian/RPM Package-Builds
   - GitHub Release erstellen
   - Assets hochladen
   - Docker Images pushen
   - `docs/VERSION.json` updaten

3. **Manuelle Schritte** (bis vollständig automatisiert):
   ```powershell
   # Local Test-Build all platforms
   .\scripts\build.ps1 -Target all
   
   # Docker Push (falls nicht via CI/CD)
   .\scripts\build.ps1 -Target docker -Push -Tag (Get-Content VERSION).Trim()
   ```

### Post-Release

- [ ] Docker Hub Images verifizieren
- [ ] Installation aus allen Package-Formaten testen
- [ ] Dokumentations-Website aktualisieren
- [ ] Release ankündigen (Blog, Social Media)
- [ ] Probleme monitoren & Hotfixes vorbereiten

---

## Versioning Strategy

### Semantic Versioning (SemVer)

```
MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]

Beispiele:
- 1.0.0          # Stabile Release
- 1.1.0          # Neue Features (rückwärts kompatibel)
- 1.1.1          # Bugfixes
- 2.0.0          # Breaking Changes
- 1.2.0-beta.1   # Beta-Release
- 1.2.0-rc.1     # Release Candidate
```

### Version-Bump-Regeln

- **MAJOR:** Breaking API Changes, Major Architecture-Changes
- **MINOR:** Neue Features, rückwärts-kompatible Änderungen
- **PATCH:** Bugfixes, Security Patches, Performance Improvements

### Pre-Release Tags

- `alpha` - Frühe Entwicklung, instabil
- `beta` - Feature-complete, Testing-Phase
- `rc` (Release Candidate) - Final Testing

---

## Security & Quality Assurance

### Package-Signing (Geplant)

```bash
# GPG-Signing (Zukunft)
gpg --detach-sign --armor themisdb_1.0.1_amd64.deb
gpg --detach-sign --armor themisdb-1.0.1-1.x86_64.rpm
```

### Checksums & Integrität

- SHA256 Checksums für alle Packages
- Veröffentlicht in `SHA256SUMS.txt`
- Automatische Verifikation in Install-Skripten

```bash
# Manual SHA256-Verifikation
sha256sum -c SHA256SUMS.txt
```

### Docker Image Security

- Multi-stage Builds (minimal attack surface)
- Non-root User-Execution
- Regular Base-Image Updates (ubuntu:22.04)
- Vulnerability Scanning (Trivy)

### Build-Audits

- Deterministische Builds (reproducible artifacts)
- Signed Git Tags
- Build-Logs archivieren
- Dependency Lock-File (vcpkg baseline)

---

## Troubleshooting

### Build-Fehler: "Subprocess aborted" (Network Timeout)

**Symptom:** Docker-Build schlägt fehl beim Download von Boost/Arrow

**Lösung:**
1. Cache manuell aktualisieren:
   ```powershell
   $env:SKIP_VCPKG_UPDATE = ""  # Aktiviere Cache-Updates
   .\scripts\update-vcpkg-cache.ps1 -Triplet x64-linux, arm64-linux
   ```

2. Retry Docker-Build:
   ```powershell
   .\scripts\build-docker.ps1
   ```

3. Offline-Mode testen:
   ```powershell
   $env:VCPKG_ENABLE_ONLINE = "OFF"
   .\scripts\build.ps1 -Target docker
   ```

### Build-Fehler: "CMake Error: Toolchain not found"

**Symptom:** `CMAKE_TOOLCHAIN_FILE` nicht vorhanden

**Lösung:**
1. Stelle sicher, dass vcpkg initialisiert ist:
   ```powershell
   cd vcpkg && git pull origin master
   ```

2. Lösche Build-Cache:
   ```powershell
   Remove-Item -Path build-msvc, build-linux -Recurse -Force
   ```

3. Neuer Build:
   ```powershell
   .\scripts\build.ps1 -Target windows
   ```

### Docker-Image: "Not found" bei Multi-Arch Pull

**Symptom:** `docker pull` schlägt auf ARM-Maschine fehl

**Lösung:**
1. Verifiziere, dass beide Architekturen gepushed wurden:
   ```bash
   docker buildx ls
   docker manifest inspect themisdb/themisdb:v1.0.1
   ```

2. Expliziter Platform-Pull:
   ```bash
   docker pull themisdb/themisdb:v1.0.1 --platform linux/arm64
   ```

---

## Performance & Build-Zeiten

### Benchmark (Intel i7, 8 Cores, 16 GB RAM, SSD)

| Platform | Cache-Status | Time | Notes |
|----------|--------------|------|-------|
| Windows (MSVC) | ✓ Warm | 25-35 min | First build, full compilation |
| Windows (MSVC) | ✓ Incremental | 5-10 min | Recompile only changed |
| Linux (GCC) | ✓ Warm | 30-40 min | First build, all deps |
| Linux (GCC) | ✓ Incremental | 8-12 min | Partial rebuild |
| Docker amd64 | ✓ Warm | 35-45 min | Single-arch |
| Docker arm64 | ✓ Warm | 40-50 min | Emulated (QEMU) |
| Docker Multi | ✓ Warm | 50-60 min | Both architectures |
| **Cache-Update** | — | 8-15 min | Pre-fetch all archives |

### Optimierungen

1. **Parallel Compilation:** `-j 4` (anpassbar via Cores)
2. **Ninja statt Make:** ~30% schneller
3. **Pre-fetched Cache:** Verhindert Runtime-Downloads
4. **Incremental Builds:** Header-Only Changes minimal

---

## Monitoring & Updates

### Build-Status Monitoring

- GitHub Actions Dashboard: Actions Tab
- Build-Logs: Actions → Workflow Run → Logs
- Failure Notifications: Email, Slack (optional)

### Update-Checker (Runtime)

ThemisDB enthält Subsystem zur Versions-Überprüfung:
- Periodisches Polling gegen `docs/VERSION.json`
- Notifications für neue Releases
- Admin-Benachrichtigungen

### Related Documentation

- [Build Guide](../build/BUILDGUIDE.md) - Detaillierte Build-Anleitung
- [Build System](../build/BUILD-SYSTEM.md) - Architektur-Übersicht
- [Implementation Summary](../development/IMPLEMENTATION-SUMMARY.md) - Was/Warum/Wie
- [Docker Guide](../guides/guides_docker.md) - Docker Deployment
- [Packaging Guide](../guides/guides_packaging.md) - Package-Erstellung

---

## Appendix: Environment Variables

| Variable | Default | Platform | Purpose |
|----------|---------|----------|---------|
| SKIP_VCPKG_UPDATE | `false` | All | Cache-Update überspringen |
| VCPKG_ENABLE_ONLINE | `ON` | All | Online-Fetches erlauben (OFF für Offline) |
| VCPKG_ASSET_SOURCES | (default) | All | Alternative Asset-Quelle |
| VCPKG_BINARY_SOURCES | (default) | All | Alternative Binary-Cache |
| VERSION (Datei) | n/a | All | Single-Source-of-Truth für Release-Version |
| DOCKER_BUILDKIT | `1` | Docker | BuildKit Engine aktivieren |
| DOCKER_BUILDKIT_PROGRESS | `plain` | Docker | Build-Ausgabe-Format |

---

**Letzte Aktualisierung:** 12. Dezember 2025  
**Autor:** Build System v2.0 Implementation Team  
**Status:** Production-Ready ✓
