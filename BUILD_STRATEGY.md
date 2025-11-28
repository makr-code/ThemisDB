# ThemisDB Build & Deployment Strategy

## Ziel
Vereinheitlichte Build-Toolchain für alle Plattformen mit konsistentem Versioning und Packaging.

## Unterstützte Plattformen

| Plattform | Architektur | Build-Methode | Binary-Kompatibilität |
|-----------|-------------|---------------|----------------------|
| **Windows** | x64 | MSVC/ClangCL + vcpkg | Native .exe |
| **WSL/Linux** | x64 | GCC/Clang + vcpkg | GLIBC 2.38+ (Ubuntu 24.04) |
| **Docker (Standard)** | x64 | Ubuntu 24.04 | GLIBC 2.38+, GLIBCXX 3.4.32 |
| **Docker (QNAP)** | x64 | Ubuntu 20.04 | GLIBC 2.31, GLIBCXX 3.4.28 |
| **Raspberry Pi** | ARM64 | GCC + vcpkg | GLIBC 2.31+ (Debian Bullseye) |

## Versionierungs-Strategie

### Semantic Versioning
Format: `MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]`

**Beispiele:**
- `0.1.0` - Initial Public Release
- `0.2.0-beta.1` - Beta-Release
- `1.0.0` - Stable Release
- `1.0.1+qnap` - QNAP-spezifischer Build

### Version-Quellen
1. **CMakeLists.txt** - Single Source of Truth
   ```cmake
   project(ThemisDB VERSION 0.1.0)
   ```

2. **Git Tags** - Release-Tagging
   ```bash
   git tag -a v0.1.0 -m "Release 0.1.0"
   ```

3. **Docker Tags**
   - `latest` - Neueste stabile Version
   - `0.1.0` - Spezifische Version
   - `0.1.0-qnap` - Plattform-spezifisch
   - `dev` - Development builds

## Konsolidierte Build-Struktur

### 1. Native Builds (CMakePresets.json)
```bash
# Windows MSVC Release
cmake --preset windows-ninja-msvc-release
cmake --build --preset windows-ninja-msvc-release

# Linux/WSL Clang Release  
cmake --preset linux-ninja-clang-release
cmake --build --preset linux-ninja-clang-release

# Raspberry Pi ARM64
cmake --preset rpi-arm64-gcc-release
cmake --build --preset rpi-arm64-gcc-release
```

### 2. Docker Builds

#### Standard (Ubuntu 24.04)
```powershell
# Methode 1: Voller Build in Docker (langsam, vcpkg-Download-Probleme)
docker build -f Dockerfile -t themisdb:latest .

# Methode 2: Pre-built Binary (EMPFOHLEN)
.\build-docker-simple.ps1 -Tag themisdb:latest
```

#### QNAP (Ubuntu 20.04)
```powershell
# Aktuell: Nicht funktional (vcpkg-Downloads scheitern)
# TODO: Implementiere Cross-Compilation oder statisches Linking
.\build-docker-qnap.ps1 -Tag themisdb:qnap
```

### 3. Packaging

#### Debian/Ubuntu (.deb)
```bash
cd packaging/deb
./build-deb.sh
```

#### Red Hat/CentOS (.rpm)
```bash
cd packaging/rpm
./build-rpm.sh
```

#### Arch Linux (PKGBUILD)
```bash
makepkg -si
```

## Deployment-Strategie

### 1. GitHub Releases
**Automated via GitHub Actions:**
```yaml
# .github/workflows/release.yml
on:
  push:
    tags:
      - 'v*.*.*'
```

**Artifacts:**
- `themis_server-{version}-linux-x64.tar.gz`
- `themis_server-{version}-windows-x64.zip`
- `themis_server-{version}-arm64.tar.gz`
- `themis_server-{version}.deb`
- `themis_server-{version}.rpm`

### 2. Docker Hub
**Tags:**
```bash
themisdb/themisdb:latest
themisdb/themisdb:0.1.0
themisdb/themisdb:0.1.0-qnap
themisdb/themisdb:dev
```

**Automated Push:**
```powershell
# Nach erfolgreicher Build
docker tag themisdb:latest themisdb/themisdb:0.1.0
docker tag themisdb:latest themisdb/themisdb:latest
docker push themisdb/themisdb:0.1.0
docker push themisdb/themisdb:latest
```

### 3. GitHub Container Registry (ghcr.io)
```bash
docker tag themisdb:latest ghcr.io/makr-code/themisdb:latest
docker push ghcr.io/makr-code/themisdb:latest
```

## Vereinfachungs-Maßnahmen

### Zu Entfernen (Duplikate/Veraltet)
- [ ] `Dockerfile.old` - Veraltete Docker-Konfiguration
- [ ] `build-msvc/` - Veraltete MSVC-Build-Artefakte
- [ ] `build-wsl/` - Redundant (nutze CMakePresets)
- [ ] `build_final.txt`, `build_log.txt` - Veraltete Logs
- [ ] Diverse `tmp_*.json` - Temporäre Test-Dateien
- [ ] `server.pid`, `server.err`, `*.log` - Laufzeit-Artefakte (gitignore)

### Zu Konsolidieren
1. **Build-Scripts:**
   - ✅ `build.ps1` - Haupt-Windows-Build (behalten)
   - ✅ `build.sh` - Haupt-Linux-Build (behalten)
   - ✅ `build-docker-simple.ps1` - Vereinfachter Docker-Build (behalten)
   - ❌ `build-docker-qnap.ps1` - Funktioniert nicht (ersetzen durch Cross-Compile)
   - ❌ `build-docker-qnap-simple.ps1` - Unvollständig (entfernen)
   - ❌ `build-tests-msvc.ps1` - Redundant (nutze CMakePresets)

2. **Docker-Dateien:**
   - ✅ `Dockerfile` - Multi-Stage Standard-Build
   - ✅ `Dockerfile.simple` - Pre-built Binary Deployment
   - ❌ `Dockerfile.qnap` - Nicht funktional (vcpkg-Probleme)
   - ❌ `Dockerfile.qnap.simple` - Unvollständig
   - ❌ `Dockerfile.runtime` - Redundant zu Dockerfile.simple
   - ❌ `Dockerfile.old` - Veraltet

3. **Docker Compose:**
   - ✅ `docker-compose.yml` - Standard
   - ✅ `docker-compose-arm.yml` - ARM-spezifisch
   - ⚠️ `docker-compose.qnap.yml` - Behalten, aber fix required
   - ❌ `docker-compose.pull.qnap.yml` - Redundant

## QNAP-Deployment Lösung

### Problem
- WSL/Docker Ubuntu 24.04 → GLIBC 2.38, GLIBCXX 3.4.32
- QNAP benötigt → GLIBC 2.31, GLIBCXX 3.4.28

### Lösungsoptionen

#### Option 1: Static Linking (EMPFOHLEN)
```cmake
# CMakeLists.txt
option(THEMIS_STATIC_BUILD "Build fully static binary" OFF)

if(THEMIS_STATIC_BUILD)
  set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
  set(VCPKG_TARGET_TRIPLET "x64-linux-static")
endif()
```

Build:
```bash
cmake -DTHEMIS_STATIC_BUILD=ON ...
```

#### Option 2: Ubuntu 20.04 Build-Container
```dockerfile
# Dockerfile.qnap-static
FROM ubuntu:20.04 AS builder
# ... build with Ubuntu 20.04 toolchain
```

#### Option 3: Cross-Compilation
```bash
# Auf Ubuntu 24.04 für Ubuntu 20.04 kompilieren
docker run -v $(pwd):/src ubuntu:20.04 bash -c "cd /src && ./build.sh"
```

## Automatisierung

### GitHub Actions Workflow
```yaml
# .github/workflows/build-release.yml
name: Build & Release

on:
  push:
    tags: ['v*']

jobs:
  build-matrix:
    strategy:
      matrix:
        include:
          - os: ubuntu-24.04
            preset: linux-ninja-clang-release
            artifact: linux-x64
          - os: windows-latest
            preset: windows-ninja-msvc-release
            artifact: windows-x64
          - os: ubuntu-24.04
            preset: linux-arm64-gcc-release
            artifact: linux-arm64
```

### Vereinfachtes Release-Script
```powershell
# scripts/release.ps1
param([string]$Version)

# 1. Tag erstellen
git tag -a "v$Version" -m "Release $Version"

# 2. Builds auslösen (GitHub Actions)
git push origin "v$Version"

# 3. Docker Images
.\build-docker-simple.ps1 -Tag "themisdb:$Version"
docker tag "themisdb:$Version" "themisdb/themisdb:$Version"
docker tag "themisdb:$Version" "themisdb/themisdb:latest"
docker push "themisdb/themisdb:$Version"
docker push "themisdb/themisdb:latest"
```

## Migration-Plan

### Phase 1: Cleanup (Sofort)
1. Entferne veraltete Dateien
2. Aktualisiere .gitignore
3. Commit: "chore: Clean up build artifacts and obsolete files"

### Phase 2: QNAP-Fix (Priorität Hoch)
1. Implementiere statisches Linking
2. Teste QNAP-Deployment
3. Commit: "feat: Add static build option for QNAP compatibility"

### Phase 3: Automation (Kurzfristig)
1. Erweitere GitHub Actions
2. Automatisiere Docker Hub Push
3. Commit: "ci: Automate multi-platform builds and releases"

### Phase 4: Packaging (Mittelfristig)
1. Verbessere .deb/.rpm Packaging
2. Arch AUR Package
3. Homebrew Formula

## Verwendete Tools

| Tool | Zweck | Status |
|------|-------|--------|
| **CMake 3.28+** | Build-System | ✅ Produktiv |
| **vcpkg** | Dependency Management | ✅ Produktiv |
| **Ninja** | Build-Generator | ✅ Produktiv |
| **Docker** | Containerization | ✅ Produktiv |
| **GitHub Actions** | CI/CD | 🔧 Teilweise |
| **CPack** | Packaging | 📋 Geplant |

## Best Practices

1. **Immer CMakePresets verwenden** - Keine manuellen cmake-Aufrufe
2. **Version in CMakeLists.txt pflegen** - Single Source of Truth
3. **Git Tags für Releases** - Automatische CI/CD Trigger
4. **Docker: Pre-built Binary bevorzugen** - Schneller, stabiler
5. **QNAP: Statisches Linking** - Vermeidet GLIBC-Inkompatibilität

## Nächste Schritte

1. ✅ Konsolidiere Build-Scripts
2. ⏸️ Implementiere statisches Linking für QNAP
3. ⏸️ Automatisiere Docker Hub Deployment
4. ⏸️ Erweitere GitHub Actions für Multi-Platform
5. ⏸️ Erstelle Release-Automation Script
