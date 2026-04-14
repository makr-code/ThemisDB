# ThemisDB Release & Distribution Strategie (Lokaler Build)

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Updates

---


## Übersicht

Diese Strategie beschreibt **manuelle Release-Prozesse** ohne GitHub Actions. Builds werden **lokal** erstellt, verpackt und als GitHub Releases hochgeladen. Kritische Bugfixes und Updates werden über strukturierte Channels verteilt.

## Release-Typen nach Semantic Versioning

| Release-Typ | Version Pattern | Use Case | Beispiel |
|-------------|----------------|----------|----------|
| **Major** | `X.0.0` | Breaking Changes, API-Inkompatibilität | `2.0.0` |
| **Minor** | `X.Y.0` | Neue Features (backward-compatible) | `1.3.0` |
| **Patch** | `X.Y.Z` | Bugfixes, Security Patches | `1.2.4` |
| **Hotfix** | `X.Y.Z` (schneller Cycle) | Critical Security/Data Loss Bugs | `1.2.5` |
| **Pre-Release** | `X.Y.Z-beta.N` | Testing vor Production | `2.0.0-beta.1` |

## Build-Architektur (Multi-Platform)

### Plattform-Matrix

| Plattform | Build-Umgebung | Output | Package Format |
|-----------|----------------|--------|----------------|
| **Windows x64** | Lokal (MSVC + Ninja) | `themis_server.exe` | `.zip` |
| **Linux x64** | Docker/WSL (gcc + vcpkg) | `themis_server` | `.tar.gz` |
| **Linux ARM64** | Docker BuildKit (cross-compile) | `themis_server` | `.tar.gz` |
| **Linux ARMv7** | Docker BuildKit (cross-compile) | `themis_server` | `.tar.gz` |
| **QNAP x64** | Docker (static build, Ubuntu 20.04) | `themis_server` | `.tar.gz` |
| **Docker Images** | Multi-Arch BuildKit | Container Image | Docker Hub |

### Build-Kommandos (Lokal)

#### Windows (Development Machine)
```powershell
# Release Build (MSVC + Ninja)
.\build.ps1 -BuildType Release -Generator Ninja -BuildDir build-msvc-ninja-release

# Output: build-msvc-ninja-release\themis_server.exe
```

#### Linux (via WSL oder Docker)
```powershell
# WSL Build (Ubuntu)
.\build-unified.ps1 -Platform linux -Config release

# Docker Build (reproduzierbar)
docker build -t themisdb-builder:latest -f Dockerfile --target build .
docker create --name themis-extract themisdb-builder:latest
docker cp themis-extract:/src/build/themis_server ./build-linux-gcc-release/
docker rm themis-extract

# Output: build-linux-gcc-release/themis_server
```

#### QNAP (Statischer Build)
```powershell
# Build-Skript nutzen
.\build-qnap.ps1

# Output: build-qnap/themis_server (statisch gelinkt)
```

#### ARM (Multi-Arch via Docker BuildKit)
```powershell
# ARM64 Build
docker buildx build --platform linux/arm64 \
    -t themisdb-arm64:latest \
    --output type=local,dest=./dist/arm64 \
    -f Dockerfile .

# ARMv7 Build (Raspberry Pi 3)
docker buildx build --platform linux/arm/v7 \
    -t themisdb-armv7:latest \
    --output type=local,dest=./dist/armv7 \
    -f Dockerfile .
```

## Release-Workflow (Step-by-Step)

### 1. Vorbereitung

```powershell
# Version festlegen (z.B. 1.3.0)
$VERSION = "1.3.0"

# Git-Status prüfen
git status
# → Sicherstellen: Working Directory clean oder nur VERSION-Änderungen

# Changelog aktualisieren
code CHANGELOG.md
# → Release-Notes manuell eintragen (Features, Bugfixes, Breaking Changes)
```

### 2. Version Bump

```powershell
# Alle Packaging-Files aktualisieren (CMakeLists.txt, vcpkg.json, etc.)
.github/workflows/04-release_create-release-archive.yml -Version $VERSION

# Änderungen überprüfen
git diff

# Commit erstellen
git add .
git commit -m "chore: Bump version to $VERSION"
```

### 3. Builds erstellen

```powershell
# Package-Skript nutzt bestehende Build-Verzeichnisse
.\scripts\package_releases.ps1

# Output:
# dist/themis_server_windows_x64.zip
# dist/themis_server_linux_gcc_x64.tar.gz
# dist/themis_server_qnap_x64-linux.tar.gz
# dist/SHA256SUMS
```

**Manuelle ARM-Builds** (falls nicht automatisiert):
```powershell
# ARM64
docker buildx build --platform linux/arm64 `
    -f Dockerfile --target build `
    -o type=local,dest=./dist/arm64-stage .

# Packaging
cd dist/arm64-stage
tar -czf ../themis_server_linux_arm64.tar.gz themis_server
cd ../..

# Checksum
(Get-FileHash -Algorithm SHA256 dist/themis_server_linux_arm64.tar.gz).Hash.ToLower() `
    | Out-File -Append -Encoding ASCII dist/SHA256SUMS
```

### 4. Docker Images bauen & taggen

```powershell
# Multi-Arch Build (erfordert docker buildx setup)
docker buildx build --platform linux/amd64,linux/arm64,linux/arm/v7 `
    -t makrcode/themis:$VERSION `
    -t makrcode/themis:latest `
    -f Dockerfile .

# QNAP-spezifisches Image (statisch gelinkt)
docker buildx build --platform linux/amd64 `
    -t makrcode/themis-qnap:$VERSION `
    -t makrcode/themis-qnap:latest `
    -f Dockerfile.qnap .
```

**OHNE sofortigen Push** (erst nach Validierung):
```powershell
# Lokales Testing
docker run --rm makrcode/themis:$VERSION --version
docker run --rm -p 8080:8080 makrcode/themis:$VERSION
```

### 5. Git Tag erstellen

```powershell
# Annotated Tag mit Release-Notes
git tag -a "v$VERSION" -m "Release $VERSION

Features:
- [Feature 1]
- [Feature 2]

Bugfixes:
- [Fix 1]
- [Fix 2]

Breaking Changes:
- [BC 1] (nur bei Major-Releases)
"

# Tag verifizieren
git show v$VERSION

# Tag zu Remote pushen
git push origin main
git push origin v$VERSION
```

### 6. GitHub Release erstellen

**Manuell über GitHub Web UI:**

1. **Navigieren:** https://github.com/makr-code/ThemisDB/releases/new
2. **Tag auswählen:** `v$VERSION` (bereits gepusht)
3. **Release Title:** `ThemisDB v$VERSION`
4. **Description:** Aus `CHANGELOG.md` kopieren + zusätzliche Infos:
   ```markdown
   ## What's Changed
   
   ### Features
   - Feature 1 (#123)
   - Feature 2 (#124)
   
   ### Bugfixes
   - Fixed critical issue (#125)
   
   ### Security
   - Patched CVE-2025-XXXX (#126)
   
   ## Supported Platforms
   
   - Windows x64 (MSVC)
   - Linux x64 (glibc 2.31+)
   - Linux ARM64 (Raspberry Pi 4/5, AWS Graviton)
   - Linux ARMv7 (Raspberry Pi 3)
   - QNAP NAS (static build)
   
   ## Docker Images
   
   ```bash
   docker pull makrcode/themis:1.3.0        # Multi-arch (amd64/arm64/armv7)
   docker pull makrcode/themis-qnap:1.3.0   # QNAP-optimized (static)
   ```
   
   ## Installation
   
   ### Binary Download
   Download the appropriate binary for your platform and verify checksum:
   
   ```bash
   # Linux
   wget https://github.com/makr-code/ThemisDB/releases/download/v1.3.0/themis_server_linux_gcc_x64.tar.gz
   sha256sum -c SHA256SUMS --ignore-missing
   tar -xzf themis_server_linux_gcc_x64.tar.gz
   ./themis_server --version
   
   # Windows
   Invoke-WebRequest https://github.com/makr-code/ThemisDB/releases/download/v1.3.0/themis_server_windows_x64.zip -OutFile themis.zip
   Expand-Archive themis.zip
   .\themis\themis_server.exe --version
   ```
   
   ## Full Changelog
   
   https://github.com/makr-code/ThemisDB/compare/v1.2.0...v1.3.0
   ```

5. **Assets hochladen:**
   - Drag & Drop aus `dist/` Ordner:
     - `themis_server_windows_x64.zip`
     - `themis_server_linux_gcc_x64.tar.gz`
     - `themis_server_linux_arm64.tar.gz`
     - `themis_server_linux_armv7.tar.gz`
     - `themis_server_qnap_x64-linux.tar.gz`
     - `SHA256SUMS`

6. **Pre-Release markieren** (falls Beta):
   - ☑ "This is a pre-release" für Versionen wie `2.0.0-beta.1`

7. **Publish Release** klicken

### 7. Docker Images zu Docker Hub pushen

```powershell
# Login (einmalig)
docker login -u makrcode

# Multi-Arch Images pushen
docker buildx build --platform linux/amd64,linux/arm64,linux/arm/v7 `
    -t makrcode/themis:$VERSION `
    -t makrcode/themis:latest `
    --push `
    -f Dockerfile .

# QNAP Image pushen
docker buildx build --platform linux/amd64 `
    -t makrcode/themis-qnap:$VERSION `
    -t makrcode/themis-qnap:latest `
    --push `
    -f Dockerfile.qnap .

# Verify
docker buildx imagetools inspect makrcode/themis:$VERSION
```

## Hotfix-Workflow (Critical Bugfix)

### Szenario: Critical Security Vulnerability in v1.3.0

```powershell
# 1. Hotfix-Branch erstellen (vom betroffenen Tag)
git checkout -b hotfix/1.3.1 v1.3.0

# 2. Bugfix committen
# ... Code-Änderungen ...
git add .
git commit -m "fix: Critical security patch for CVE-2025-XXXX"

# 3. Version bumpen
.github/workflows/04-release_create-release-archive.yml -Version 1.3.1

# 4. CHANGELOG aktualisieren
code CHANGELOG.md
# → Eintrag für 1.3.1 mit Security-Fix

git add .
git commit -m "chore: Bump version to 1.3.1 (hotfix)"

# 5. Builds erstellen (NUR betroffene Plattformen)
.\build.ps1 -BuildType Release -Generator Ninja -BuildDir build-msvc-ninja-release
.\scripts\package_releases.ps1

# 6. Git Tag & Push
git tag -a "v1.3.1" -m "Hotfix Release 1.3.1

Security:
- Fixed CVE-2025-XXXX (High Severity)
"
git push origin hotfix/1.3.1
git push origin v1.3.1

# 7. Merge zurück zu main
git checkout main
git merge --no-ff hotfix/1.3.1 -m "Merge hotfix 1.3.1"
git push origin main

# 8. GitHub Release erstellen (wie oben)
# → **"This is a security update"** deutlich kennzeichnen

# 9. Docker Images aktualisieren
docker buildx build --platform linux/amd64,linux/arm64,linux/arm/v7 `
    -t makrcode/themis:1.3.1 `
    -t makrcode/themis:latest `  # ← latest überschreiben!
    --push -f Dockerfile .
```

### Hotfix-Kommunikation

**Security Advisory erstellen:**
1. GitHub → Security → Advisories → "New draft security advisory"
2. **Severity:** High/Critical
3. **CVE ID:** Anfordern oder selbst assignen
4. **Affected Versions:** `< 1.3.1`
5. **Patched Versions:** `>= 1.3.1`
6. **Description:**
   ```markdown
   ## Impact
   Vulnerability allows [attack vector] leading to [consequence].
   
   ## Patches
   Fixed in version 1.3.1. Users should upgrade immediately.
   
   ## Workarounds
   [Temporary mitigation if available, otherwise "None"]
   
   ## References
   - Commit: <commit-hash>
   - Release: https://github.com/makr-code/ThemisDB/releases/tag/v1.3.1
   ```
7. **Publish Advisory**

**User-Notification:**
```markdown
# In GitHub Release Description (v1.3.1)

⚠️ **SECURITY UPDATE - IMMEDIATE ACTION REQUIRED** ⚠️

This release fixes a critical security vulnerability (CVE-2025-XXXX).
All users running versions < 1.3.1 should upgrade immediately.

## Upgrade Instructions

### Docker Users
```bash
docker pull makrcode/themis:latest  # Auto-updates to 1.3.1
docker-compose down && docker-compose up -d
```

### Binary Users
Download the updated binary from the Assets below and replace your existing installation.
```

## Update-Channels & Distribution

### 1. GitHub Releases (Primary)
- **URL:** https://github.com/makr-code/ThemisDB/releases
- **Audience:** Alle User (Binary Downloads, Changelog)
- **Notification:** GitHub Watch + Email (für Starred Repos)

### 2. Docker Hub (Container Users)
- **URL:** https://hub.docker.com/r/makrcode/themis
- **Tags:**
  - `latest` → Neueste stabile Version (auto-update bei Pull)
  - `1.3.0`, `1.3.1` → Pinned Versions
  - `1.3` → Minor-Version Track (Bugfixes ohne Breaking Changes)
  - `1` → Major-Version Track (alle 1.x.x Updates)
- **Webhook:** Docker Hub kann Webhooks zu externen Services senden (optional)

### 3. Package Managers (Zukünftig)

#### Chocolatey (Windows)
```powershell
# Setup (einmalig)
choco apikey --key YOUR_API_KEY --source https://push.chocolatey.org/

# Package & Push
cd packaging/chocolatey
choco pack themisdb.nuspec
choco push themisdb.1.3.0.nupkg --source https://push.chocolatey.org/
```

**User-Installation:**
```powershell
choco install themisdb
choco upgrade themisdb  # Auto-checks Chocolatey repo
```

#### Homebrew (macOS/Linux)
```bash
# Tap-Repository erstellen (einmalig)
# https://github.com/makr-code/homebrew-themis

# Formula aktualisieren
cd homebrew-themis
# ... themisdb.rb bearbeiten (URL + SHA256)
git commit -am "Update ThemisDB to 1.3.0"
git push
```

**User-Installation:**
```bash
brew tap makr-code/themis
brew install themisdb
brew upgrade themisdb  # Auto-checks tap
```

#### APT Repository (Debian/Ubuntu) - Fortgeschritten
```bash
# Reprepro Setup für selbst-gehostetes APT-Repo
# Erfordert: Web-Server + GPG-Keys + Debian-Package-Building

# Alternativ: PPA (Ubuntu Personal Package Archive)
# https://launchpad.net/~makr-code/+archive/ubuntu/themis
```

### 4. Update-Checker (In-App)

**Implementierung in ThemisDB:**

```cpp
// src/update_checker.cpp (Beispiel)
#include <nlohmann/json.hpp>
#include <curl/curl.h>

class UpdateChecker {
public:
    struct VersionInfo {
        std::string latest_version;
        std::string release_url;
        bool is_security_update;
    };
    
    static VersionInfo checkForUpdates(const std::string& current_version) {
        // GitHub Releases API abfragen
        const std::string api_url = 
            "https://api.github.com/repos/makr-code/ThemisDB/releases/latest";
        
        // CURL Request (vereinfacht)
        auto response = httpGet(api_url);
        auto json = nlohmann::json::parse(response);
        
        VersionInfo info;
        info.latest_version = json["tag_name"].get<std::string>();
        info.release_url = json["html_url"].get<std::string>();
        
        // Check Release Body für "SECURITY UPDATE" Keyword
        std::string body = json["body"].get<std::string>();
        info.is_security_update = 
            (body.find("SECURITY UPDATE") != std::string::npos);
        
        return info;
    }
};
```

**CLI-Integration:**
```bash
# Bei Server-Start (optional, deaktivierbar)
themis_server --check-updates

# Output:
# ℹ New version available: v1.3.1 (current: v1.3.0)
# ⚠ Security update available - please upgrade immediately!
# Release notes: https://github.com/makr-code/ThemisDB/releases/tag/v1.3.1
```

**Konfiguration:**
```json
// config.json
{
  "update_checker": {
    "enabled": true,
    "check_interval_hours": 24,
    "notify_on_security_only": false
  }
}
```

## Best Practices

### 1. Reproducible Builds
```powershell
# Dockerfile mit pinned Dependencies (vcpkg baseline)
# → Gleicher Source-Code = identisches Binary

# Verification
docker build -t themis:verify-1.3.0 --build-arg VERSION=1.3.0 .
docker run --rm themis:verify-1.3.0 sha256sum /usr/local/bin/themis_server
# → Checksum vergleichen mit offizieller Release
```

### 2. Checksum-Verifikation
```bash
# SHA256SUMS enthält alle Binaries
# User können Download verifizieren:

# Linux
sha256sum -c SHA256SUMS --ignore-missing

# Windows
Get-FileHash themis_server_windows_x64.zip -Algorithm SHA256
# → Manuell mit SHA256SUMS vergleichen
```

### 3. Code Signing (Empfohlen für Production)

**Windows:**
```powershell
# Authenticode Signing (benötigt Code Signing Certificate)
signtool sign /f certificate.pfx /p PASSWORD /tr http://timestamp.digicert.com `
    build-msvc-ninja-release/themis_server.exe

# Verify
signtool verify /pa build-msvc-ninja-release/themis_server.exe
```

**macOS:**
```bash
# Apple Developer ID (benötigt Apple Developer Account)
codesign --sign "Developer ID Application: Your Name" themis_server
codesign --verify --verbose themis_server
```

### 4. Release-Checkliste

- [ ] **Version bumped** in allen Files (CMakeLists.txt, vcpkg.json, etc.)
- [ ] **CHANGELOG.md** aktualisiert mit Release-Notes
- [ ] **Alle Plattformen gebaut** (Windows, Linux x64, ARM64, ARMv7, QNAP)
- [ ] **Tests durchgeführt** (`.\test.ps1` auf allen Plattformen)
- [ ] **Docker Images getestet** (lokaler Run + Healthcheck)
- [ ] **SHA256SUMS generiert** und verifiziert
- [ ] **Git Tag erstellt** und gepusht
- [ ] **GitHub Release erstellt** mit vollständigen Release-Notes
- [ ] **Assets hochgeladen** (alle Binaries + Checksums)
- [ ] **Docker Images gepusht** (makrcode/themis + makrcode/themis-qnap)
- [ ] **Security Advisory** (bei Hotfixes) veröffentlicht
- [ ] **Package Manager Updates** (Chocolatey, Homebrew) eingereicht

## Automatisierung (Zukünftig)

**Optionale GitHub Actions für Auto-Build** (trotz "ohne Workflow"):

```yaml
# .github/workflows/release-assist.yml
# Nur für Build-Automation, Upload bleibt manuell

name: Release Build Assist

on:
  push:
    tags:
      - 'v*'

jobs:
  build-matrix:
    strategy:
      matrix:
        include:
          - os: windows-latest
            platform: windows
          - os: ubuntu-latest
            platform: linux
          - os: ubuntu-latest
            platform: arm64
    
    runs-on: ${{ matrix.os }}
    
    steps:
      - uses: actions/checkout@v3
      
      - name: Build
        run: |
          # Platform-spezifischer Build
          # Output als Artifact speichern
      
      - name: Upload Artifact
        uses: actions/upload-artifact@v3
        with:
          name: themis-${{ matrix.platform }}
          path: dist/*

  # GitHub Runner produziert Binaries
  # Developer lädt Artifacts herunter und uploaded zu Release manuell
```

**Vorteil:** Konsistente Build-Umgebung, aber volle Kontrolle über Release-Prozess.

## Support & Wartung

### Supported Versions

| Version | Support Status | End of Life |
|---------|---------------|-------------|
| 1.3.x | ✅ Active | TBD |
| 1.2.x | ⚠️ Security Only | 2025-12-31 |
| 1.1.x | ❌ Unsupported | 2025-06-30 |
| 1.0.x | ❌ Unsupported | 2025-03-31 |

**Policy:**
- **Latest Minor:** Full support (Features + Bugfixes)
- **Previous Minor:** Security patches only (6 Monate)
- **Older Versions:** End of Life (keine Updates)

### Rollback-Strategie

**Docker:**
```bash
# Rollback zu vorheriger Version
docker pull makrcode/themis:1.2.5
docker-compose down
# docker-compose.yml anpassen: image: makrcode/themis:1.2.5
docker-compose up -d
```

**Binary:**
```bash
# Alte Version von GitHub Releases herunterladen
wget https://github.com/makr-code/ThemisDB/releases/download/v1.2.5/themis_server_linux_gcc_x64.tar.gz
tar -xzf themis_server_linux_gcc_x64.tar.gz
./themis_server --version
```

## Referenzen

- [Semantic Versioning](https://semver.org/)
- [GitHub Releases Best Practices](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases)
- [Docker Multi-Arch Images](https://docs.docker.com/build/building/multi-platform/)
- [Reproducible Builds](https://reproducible-builds.org/)
- [Code Signing Guide](https://docs.microsoft.com/en-us/windows/win32/seccrypto/using-signtool-to-sign-a-file)
