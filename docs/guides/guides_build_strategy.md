# ThemisDB Build, Packaging & Deployment Strategy

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Guides

---


## Ziel
Konsistente Build-Toolchain für alle Plattformen, eindeutige Versionierung und abgestimmtes Packaging/CI-CD.

## Unterstützte Plattformen

| Plattform | Architektur | Build-Methode | Binary-Kompatibilität |
|-----------|-------------|---------------|----------------------|
| **Windows** | x64 | MSVC/ClangCL + vcpkg + boost | Native .exe |
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

#### Standard Builds
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

#### Enterprise Builds (mit zusätzlichen Features)

**Enterprise Features:**
- Token Bucket Rate Limiter (Priority-basiert)
- Per-Client Rate Limiter
- Adaptive Load Shedder (Multi-Metrik)
- HTTP Client Pool (Boost.Beast)

**Build-Befehle:**
```bash
# Windows Enterprise Build
cmake --preset windows-ninja-msvc-release
cmake --build --preset windows-ninja-msvc-release
# Enterprise Features sind automatisch aktiviert

# Linux Enterprise Build
cmake --preset linux-ninja-clang-release
cmake --build --preset linux-ninja-clang-release

# Tests ausführen (Enterprise Features)
./build-msvc-ninja-release/themis_tests --gtest_filter="*Enterprise*:TokenBucket*:PerClient*:LoadShed*"
```

**Enterprise Build Scripts:**
```powershell
# Windows
.\scripts\build_enterprise.cmd

# PowerShell Alternative
.\scripts\enable_enterprise_features.ps1

# Linux/WSL
./scripts/build_enterprise.sh
```

**Enterprise Dependencies (via vcpkg):**
- `boost-beast` - HTTP Client Pool
- `boost-asio` - Asynchrone Netzwerk-Operationen
- `openssl` - SSL/TLS für HTTPS
- Alle Standard-Dependencies

**CMake Optionen:**
```cmake
# Enterprise Features sind standardmäßig aktiviert
# Explizite Konfiguration in CMakeLists.txt:
# - THEMIS_ENABLE_RATE_LIMITING (immer ON)
# - THEMIS_ENABLE_LOAD_SHEDDING (immer ON)
# - THEMIS_ENABLE_HTTP_POOL (immer ON)
```

### 2. Docker Builds

#### Standard (Ubuntu 24.04)
```powershell
docker build -f Dockerfile -t themisdb/themisdb:1.0.1 -t themisdb/themisdb:latest --platform linux/amd64 .
```

#### QNAP (Ubuntu 20.04, SSE4.2 Basis)
```powershell
docker build -f Dockerfile.qnap -t themisdb/themisdb:1.0.1-qnap -t themisdb/themisdb:qnap --platform linux/amd64 .
```

Push:
```powershell
docker push themisdb/themisdb:1.0.1
docker push themisdb/themisdb:latest
docker push themisdb/themisdb:1.0.1-qnap
docker push themisdb/themisdb:qnap
```

### 3. Packaging

#### Portable Archives (Linux/QNAP/Windows)
```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package_releases.ps1
```
Erzeugt unter `dist/`: `themisdb-1.0.0-qnap-x64.tar.gz`, optional `themisdb-1.0.0-linux-x64.tar.gz` und `themisdb-1.0.0-windows-x64.zip` (wenn Binaries vorhanden). Enthält:
- `bin/` Binaries, `lib/` (gebündelte Shared Libs), `config/`, `openapi/`, `clients/`, `examples/`, `tools/`, `docs/`.

#### Debian/Ubuntu (.deb)
Build in Linux-Umgebung, siehe `themisdb.spec` Pendant und CI-Job (optional).

#### Red Hat/CentOS (.rpm)
```bash
rpmbuild -ba themisdb.spec
```

## CI/CD & Deployment

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
- `themis_server-{version}-linux-arm64.tar.gz`
- `themis_server-{version}-enterprise-linux-x64.tar.gz` *(mit Enterprise Features)*
- `themis_server-{version}-enterprise-windows-x64.zip` *(mit Enterprise Features)*
- `themis_server-{version}.deb`
- `themis_server-{version}.rpm`

### Docker Push
```powershell
docker push themisdb/themisdb:1.0.1
docker push themisdb/themisdb:latest
docker push themisdb/themisdb:1.0.1-qnap
docker push themisdb/themisdb:qnap
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
   - ✅ `scripts/build_enterprise.cmd` - Enterprise Build für Windows (NEU)
   - ✅ `scripts/enable_enterprise_features.ps1` - Enterprise Build PowerShell (NEU)
   - ❌ `build-docker-qnap.ps1` - Funktioniert nicht (ersetzen durch Cross-Compile)
   - ❌ `build-docker-qnap-simple.ps1` - Unvollständig (entfernen)
   - ❌ `build-tests-msvc.ps1` - Redundant (nutze CMakePresets)

2. **Docker-Dateien:**
   - ✅ `Dockerfile` - Multi-Stage Standard-Build
   - ✅ `Dockerfile.simple` - Pre-built Binary Deployment
  - ✅ `Dockerfile.qnap` - QNAP-kompatibel (SSE4.2; GLIBC 2.31)
  - ❌ `Dockerfile.runtime` - Redundant
   - ❌ `Dockerfile.old` - Veraltet

3. **Docker Compose:**
   - ✅ `docker-compose.yml` - Standard
   - ✅ `docker-compose-arm.yml` - ARM-spezifisch
   - ⚠️ `docker-compose.qnap.yml` - Behalten, aber fix required
   - ❌ `docker-compose.pull.qnap.yml` - Redundant

## QNAP-Deployment

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

#### Option 2: Ubuntu 20.04 Runtime-Image
Dockerfile.qnap nutzt 20.04 und SSE4.2 Basis.

#### Option 3: Cross-Compilation
```bash
# Auf Ubuntu 24.04 für Ubuntu 20.04 kompilieren
docker run -v $(pwd):/src ubuntu:20.04 bash -c "cd /src && ./build.sh"
```

## Automatisierung (CI/CD)

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
          # Standard Builds
          - os: ubuntu-24.04
            preset: linux-ninja-clang-release
            artifact: linux-x64
            variant: standard
          - os: windows-latest
            preset: windows-ninja-msvc-release
            artifact: windows-x64
            variant: standard
          - os: ubuntu-24.04
            preset: linux-arm64-gcc-release
            artifact: linux-arm64
            variant: standard
          
          # Enterprise Builds
          - os: ubuntu-24.04
            preset: linux-ninja-clang-release
            artifact: enterprise-linux-x64
            variant: enterprise
            extra_flags: "-DTHEMIS_BUILD_TESTS=ON"
          - os: windows-latest
            preset: windows-ninja-msvc-release
            artifact: enterprise-windows-x64
            variant: enterprise
            extra_flags: "-DTHEMIS_BUILD_TESTS=ON"
    
    steps:
      - name: Build
        run: |
          cmake --preset ${{ matrix.preset }} ${{ matrix.extra_flags }}
          cmake --build build
      
      - name: Test Enterprise Features
        if: matrix.variant == 'enterprise'
        run: |
          ./build/themis_tests --gtest_filter="*Enterprise*:TokenBucket*:PerClient*:LoadShed*"
      
      - name: Package Portable Archives
        run: |
          pwsh -File scripts/package_releases.ps1
          ls dist
```

### Vereinfachtes Release-Script
```powershell
# scripts/release.ps1
param(
    [string]$Version,
    [switch]$Enterprise
)

# 1. Tag erstellen
$tagSuffix = if ($Enterprise) { "-enterprise" } else { "" }
git tag -a "v$Version$tagSuffix" -m "Release $Version$tagSuffix"

# 2. Builds auslösen (GitHub Actions)
git push origin "v$Version$tagSuffix"

# 3. Docker Images
if ($Enterprise) {
    # Enterprise Build
    .\scripts\enable_enterprise_features.ps1
    .\build-docker-simple.ps1 -Tag "themisdb:enterprise-$Version"
    docker tag "themisdb:enterprise-$Version" "themisdb/themisdb:$Version-enterprise"
    docker tag "themisdb:enterprise-$Version" "themisdb/themisdb:enterprise-latest"
    docker push "themisdb/themisdb:$Version-enterprise"
    docker push "themisdb/themisdb:enterprise-latest"
} else {
    # Standard Build
    .\build-docker-simple.ps1 -Tag "themisdb:$Version"
    docker tag "themisdb:$Version" "themisdb/themisdb:$Version"
    docker tag "themisdb:$Version" "themisdb/themisdb:latest"
    docker push "themisdb/themisdb:$Version"
    docker push "themisdb/themisdb:latest"
}

# 4. Summary
Write-Host "✅ Released ThemisDB $Version$(if($Enterprise){' (Enterprise)'})" -ForegroundColor Green
```

**Verwendung:**
```powershell
# Standard Release
.\scripts\release.ps1 -Version "0.2.0"

# Enterprise Release
.\scripts\release.ps1 -Version "0.2.0" -Enterprise
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
5. **QNAP: Baseline SSE4.2** - Vermeidet FMA/AVX-Inkompatibilität
6. **Artefakt-Fluss** - Build → Artefakt → Packaging → Release/Docker

## Nächste Schritte

1. ✅ Konsolidiere Build-Scripts
2. ✅ Enterprise Build-Variante implementiert
3. ✅ Enterprise Tests erfolgreich (13/13 passed)
4. ⏸️ Implementiere statisches Linking für QNAP
5. ⏸️ Automatisiere Docker Hub Deployment (Standard + Enterprise)
6. ⏸️ Erweitere GitHub Actions für Multi-Platform (Standard + Enterprise)
7. ⏸️ Erstelle Release-Automation Script mit Enterprise-Support

## Enterprise Edition - Überblick

### Zusätzliche Features
- **Token Bucket Rate Limiter**: Priority-basierte Request-Limitierung (HIGH, NORMAL, LOW)
- **Per-Client Rate Limiter**: Unabhängige Quotas pro Client/API-Key
- **Adaptive Load Shedder**: Multi-Metrik Lastüberwachung (CPU 50%, Memory 30%, Queue 20%)
- **HTTP Client Pool**: Production-ready Boost.Beast Implementation mit SSL/TLS

### Code-Statistiken
- **Production Code**: 1.047 LOC (Rate Limiter: 407, Load Shedder: 116, HTTP Pool: 524)
- **Tests**: 478 LOC (13 Unit Tests, alle bestanden)
- **Dokumentation**: ~2.000 LOC (4 Markdown-Dateien)

### Build-Status
- ✅ Windows MSVC 19.44 - Build erfolgreich
- ✅ Unit Tests: 13/13 bestanden
- ✅ Integration: Bereit für HTTP Server Middleware
- 📋 Performance Tests: Ausstehend (k6 Load Testing)

### Deployment
Enterprise Features sind standardmäßig in allen Builds enthalten und können über Konfiguration aktiviert/deaktiviert werden:

```json
// config.json
{
  "rate_limiting": {
    "enabled": true,
    "capacity": 1000,
    "refill_rate": 100
  },
  "load_shedding": {
    "enabled": true,
    "cpu_threshold": 0.95,
    "memory_threshold": 0.90
  },
  "http_client_pool": {
    "max_connections": 100,
    "timeout_ms": 5000
  }
}
```
