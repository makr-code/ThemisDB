# Build Scripts Reference

**Stand:** 23. April 2026  
**Version:** v1.4.0  
**Kategorie:** 🔨 Build Automation  
**Status:** Production-Ready

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Edition Build Scripts](#-edition-build-scripts)
- [Platform Build Scripts](#-platform-build-scripts)
- [Docker Build Scripts](#-docker-build-scripts)
- [Release & Packaging Scripts](#-release--packaging-scripts)
- [Development & Testing Scripts](#-development--testing-scripts)
- [Common Parameters](#-common-parameters)
- [Script Workflows](#-script-workflows)

---

## 🎯 Übersicht

ThemisDB bietet eine umfassende Sammlung von Build-Scripts für automatisierte Builds auf allen Plattformen:

### Script Categories

```
scripts/
├── Edition-spezifisch          # build-community-release.ps1, build-enterprise-release.ps1
├── Platform-spezifisch         # build-windows.ps1, build-linux.sh
├── Docker                      # build-docker.ps1, build-docker.sh
├── Release & Packaging         # prepare-release.ps1, create-release-archive.ps1
├── Development                 # build-clang.ps1, build-ninja.ps1
└── Testing                     # test scripts in test_phase2/
```

### Quick Start Examples

```powershell
# Community Edition (Windows)
.\scripts\build-community-release.ps1 -Platform all

# Enterprise Edition (Windows, License erforderlich)
.\scripts\build-enterprise-release.ps1 -Environment production

# Hyperscaler Edition (Windows, License erforderlich)
.\scripts\build-hyperscaler-release.ps1 -BuildType oem

# Docker Build (Multi-platform)
.\scripts\build-docker.ps1 -Tag themisdb:latest
```

---

## 🏢 Edition Build Scripts

### build-community-release.ps1

**Purpose:** Build COMMUNITY Edition (Open Source, 5 Nodes, 24GB GPU)

**Location:** `.github/workflows/04-release_build-binary-linux.yml`

**Usage:**

```powershell
.\scripts\build-community-release.ps1 `
  -Platform <windows|docker|all> `
  -Configuration <Release|Debug> `
  [-SkipTests] `
  [-SkipDocker]
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `Platform` | String | `all` | Build platform: `windows`, `docker`, `all` |
| `Configuration` | String | `Release` | Build configuration: `Release`, `Debug` |
| `SkipTests` | Switch | `false` | Skip unit tests |
| `SkipDocker` | Switch | `false` | Skip Docker build |

**Example - Full Build:**

```powershell
# Build for all platforms
.\scripts\build-community-release.ps1 -Platform all -Configuration Release

# Output:
# - build-msvc/Release/themis_server.exe (Windows)
# - Docker image: themisdb-community:1.4.0
```

**Example - Windows Only:**

```powershell
# Build only Windows binary
.\scripts\build-community-release.ps1 `
  -Platform windows `
  -Configuration Release `
  -SkipDocker

# Output:
# - build-msvc/Release/themis_server.exe
```

**Example - Debug Build:**

```powershell
# Debug build with tests
.\scripts\build-community-release.ps1 `
  -Platform windows `
  -Configuration Debug

# Output:
# - build-msvc/Debug/themis_server.exe
# - Test results in build-msvc/Testing/
```

**Features (Community Edition):**
- ✅ 5 Nodes maximum
- ✅ 24 GB GPU VRAM
- ✅ LLM Core (Embedding, Similarity, Inference)
- ✅ GPU Acceleration
- ❌ No Enterprise Plugins
- ❌ No Auto-Sharding
- ❌ No RBAC/Field Encryption

---

### build-enterprise-release.ps1

**Purpose:** Build ENTERPRISE Edition (100 Nodes, 256GB GPU, Requires License)

**Location:** `.github/workflows/04-release_publish-enterprise.yml`

**Usage:**

```powershell
.\scripts\build-enterprise-release.ps1 `
  -Environment <production|development|docker|all> `
  -Configuration <Release|Debug> `
  [-LicenseFile <path>] `
  [-SkipTests] `
  [-SkipDocker] `
  [-SignArtifacts]
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `Environment` | String | `all` | Build environment: `production`, `development`, `docker`, `all` |
| `Configuration` | String | `Release` | Build configuration |
| `LicenseFile` | String | `null` | Path to enterprise license.json (Required for Release) |
| `SkipTests` | Switch | `false` | Skip unit tests |
| `SkipDocker` | Switch | `false` | Skip Docker build |
| `SignArtifacts` | Switch | `false` | Code-sign binaries |

**⚠️ IMPORTANT:** ENTERPRISE Release builds require a valid license file!

**Example - Production Build (License Required):**

```powershell
# Production build with license
.\scripts\build-enterprise-release.ps1 `
  -Environment production `
  -Configuration Release `
  -LicenseFile "C:\licenses\enterprise-license.json" `
  -SignArtifacts

# ✅ Success - License validated
# Output:
# - build-msvc/Release/themis_server.exe (signed)
# - Docker image: themisdb-enterprise:1.4.0
```

**Example - Development Build (No License Required):**

```powershell
# Debug build without license (Development flexibility)
.\scripts\build-enterprise-release.ps1 `
  -Environment development `
  -Configuration Debug

# ✅ Success - License optional for Debug
# Output:
# - build-msvc/Debug/themis_server.exe
```

**Example - Release Build without License (Fails):**

```powershell
# ❌ ERROR: Release without license
.\scripts\build-enterprise-release.ps1 `
  -Environment production `
  -Configuration Release

# Error: ENTERPRISE Release builds require license!
# Please provide -LicenseFile parameter or use -Configuration Debug
```

**Features (Enterprise Edition):**
- ✅ 100 Nodes maximum
- ✅ 256 GB GPU VRAM
- ✅ Auto-Sharding (1-100 Nodes)
- ✅ Multi-Master Replication
- ✅ RBAC & User Management
- ✅ Field-Level Encryption
- ✅ HSM Integration
- ✅ Enterprise Plugins
- ⚠️ License Required (Release builds)

---

### build-hyperscaler-release.ps1

**Purpose:** Build HYPERSCALER Edition (Unlimited Scale, Mandatory License)

**Location:** `.github/workflows/04-release_publish-hyperscaler.yml`

**Usage:**

```powershell
.\scripts\build-hyperscaler-release.ps1 `
  -BuildType <oem|custom|docker|all> `
  -Configuration <Release|Debug> `
  -LicenseFile <path> `
  [-CustomerName <name>] `
  [-SkipTests] `
  [-SkipDocker] `
  [-SignArtifacts]
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `BuildType` | String | `all` | Build type: `oem`, `custom`, `docker`, `all` |
| `Configuration` | String | `Release` | Build configuration |
| `LicenseFile` | String | **Required** | Path to hyperscaler license.json |
| `CustomerName` | String | `OEM` | Customer/OEM name for branding |
| `SkipTests` | Switch | `false` | Skip unit tests |
| `SkipDocker` | Switch | `false` | Skip Docker build |
| `SignArtifacts` | Switch | `false` | Code-sign binaries |

**❌ CRITICAL:** HYPERSCALER builds **always** require license (no Debug exception)!

**Example - OEM Build:**

```powershell
# OEM build for cloud provider
.\scripts\build-hyperscaler-release.ps1 `
  -BuildType oem `
  -Configuration Release `
  -LicenseFile "C:\licenses\hyperscaler-aws.json" `
  -CustomerName "AWS Cloud Provider" `
  -SignArtifacts

# ✅ Success
# Output:
# - build-msvc/Release/themis_server.exe (AWS branded, signed)
# - Docker image: themisdb-hyperscaler:1.4.0-aws
```

**Example - Debug Build (License Still Required):**

```powershell
# ❌ ERROR: Even Debug needs license!
.\scripts\build-hyperscaler-release.ps1 `
  -BuildType custom `
  -Configuration Debug

# Error: HYPERSCALER Edition requires license for all builds!
# No Debug exception for HYPERSCALER.

# ✅ Correct:
.\scripts\build-hyperscaler-release.ps1 `
  -BuildType custom `
  -Configuration Debug `
  -LicenseFile "C:\licenses\hyperscaler.json"
```

**Features (Hyperscaler Edition):**
- ✅ Unlimited Nodes
- ✅ Unlimited GPU VRAM
- ✅ All Enterprise Features
- ✅ Custom Features
- ✅ OEM Branding
- ✅ Source Access (optional)
- ❌ License **Mandatory** (all builds)

---

## 🖥️ Platform Build Scripts

### build-windows.ps1

**Purpose:** Generic Windows build (any edition)

**Location:** `scripts/build-windows.ps1`

**Usage:**

```powershell
.\scripts\build-windows.ps1 `
  [-Configuration <Release|Debug>] `
  [-Compiler <MSVC|ClangCL>] `
  [-Edition <MINIMAL|COMMUNITY|ENTERPRISE|HYPERSCALER>] `
  [-LLM] `
  [-GPU] `
  [-Tests]
```

**Example:**

```powershell
# Community build with LLM + GPU
.\scripts\build-windows.ps1 `
  -Configuration Release `
  -Edition COMMUNITY `
  -LLM `
  -GPU
```

---

### build-linux.sh

**Purpose:** Generic Linux build (any edition)

**Location:** `scripts/build-linux.sh`

**Usage:**

```bash
./scripts/build-linux.sh \
  --config <release|debug> \
  --edition <MINIMAL|COMMUNITY|ENTERPRISE|HYPERSCALER> \
  [--llm] \
  [--gpu] \
  [--license <path>]
```

**Example:**

```bash
# Enterprise Release with license
./scripts/build-linux.sh \
  --config release \
  --edition ENTERPRISE \
  --license /path/to/license.json \
  --llm \
  --gpu
```

---

### build-minimal.sh

**Purpose:** Minimal Edition build (Embedded/IoT)

**Location:** `scripts/build-minimal.sh`

**Usage:**

```bash
./scripts/build-minimal.sh \
  [--target <native|armv7|arm64>] \
  [--static]
```

**Example - Raspberry Pi:**

```bash
# ARM64 minimal build for Raspberry Pi
./scripts/build-minimal.sh \
  --target arm64 \
  --static
```

---

## 🐳 Docker Build Scripts

### build-docker.ps1

**Purpose:** Build Docker images (Windows host)

**Location:** `scripts/build-docker.ps1`

**Usage:**

```powershell
.\scripts\build-docker.ps1 `
  -Tag <image:tag> `
  [-Edition <COMMUNITY|ENTERPRISE|HYPERSCALER>] `
  [-Platform <linux/amd64,linux/arm64>] `
  [-Push] `
  [-NoCache]
```

**Example - Multi-Arch:**

```powershell
# Build multi-arch Community image
.\scripts\build-docker.ps1 `
  -Tag themisdb:1.4.0 `
  -Edition COMMUNITY `
  -Platform "linux/amd64,linux/arm64" `
  -Push

# Builds:
# - linux/amd64 (x86_64)
# - linux/arm64 (ARM64)
# Pushes to Docker Hub
```

**Example - Single Platform:**

```powershell
# Build x86_64 only
.\scripts\build-docker.ps1 `
  -Tag themisdb:latest `
  -Edition COMMUNITY `
  -Platform linux/amd64
```

---

### build-docker.sh

**Purpose:** Build Docker images (Linux host)

**Location:** `scripts/build-docker.sh`

**Usage:**

```bash
./scripts/build-docker.sh \
  --tag <image:tag> \
  [--edition <COMMUNITY|ENTERPRISE|HYPERSCALER>] \
  [--platform <linux/amd64,linux/arm64>] \
  [--push] \
  [--no-cache]
```

**Example:**

```bash
# Enterprise Docker build
./scripts/build-docker.sh \
  --tag themisdb-enterprise:1.4.0 \
  --edition ENTERPRISE \
  --platform linux/amd64,linux/arm64 \
  --push
```

---

### build-hyperscaler-docker.sh

**Purpose:** Hyperscaler Docker build (OEM/Cloud)

**Location:** `scripts/build-hyperscaler-docker.sh`

**Usage:**

```bash
./scripts/build-hyperscaler-docker.sh \
  --tag <image:tag> \
  --license <path> \
  --customer <name> \
  [--platform <platforms>]
```

**Example:**

```bash
# AWS OEM Docker build
./scripts/build-hyperscaler-docker.sh \
  --tag aws-themisdb:1.4.0 \
  --license /licenses/hyperscaler-aws.json \
  --customer "AWS Cloud Provider" \
  --platform linux/amd64,linux/arm64
```

---

## 📦 Release & Packaging Scripts

### prepare-release.ps1

**Purpose:** Prepare release artifacts

**Location:** `.github/workflows/04-release_create-release-archive.yml`

**Usage:**

```powershell
.github/workflows/04-release_create-release-archive.yml `
  -Version <version> `
  [-Edition <COMMUNITY|ENTERPRISE|HYPERSCALER>] `
  [-Platforms <windows,linux,docker>]
```

**Example:**

```powershell
# Prepare v1.4.0 release
.github/workflows/04-release_create-release-archive.yml `
  -Version 1.4.0 `
  -Edition COMMUNITY `
  -Platforms windows,linux,docker

# Output:
# - releases/v1.4.0/themisdb-community-1.4.0-windows-x64.zip
# - releases/v1.4.0/themisdb-community-1.4.0-linux-x64.tar.gz
# - releases/v1.4.0/themisdb-community-1.4.0-docker-manifest.json
```

---

### create-release-archive.ps1

**Purpose:** Create release archives with checksums

**Location:** `.github/workflows/04-release_create-release-archive.yml`

**Usage:**

```powershell
.\scripts\create-release-archive.ps1 `
  -Version <version> `
  -Edition <edition> `
  [-IncludeSymbols] `
  [-Sign]
```

**Example:**

```powershell
# Create signed release archive
.\scripts\create-release-archive.ps1 `
  -Version 1.4.0 `
  -Edition ENTERPRISE `
  -IncludeSymbols `
  -Sign

# Output:
# - themisdb-enterprise-1.4.0.zip (signed)
# - themisdb-enterprise-1.4.0.zip.sha256
# - themisdb-enterprise-1.4.0-symbols.zip
```

---

### create-github-release.ps1

**Purpose:** Create GitHub release

**Location:** `.github/workflows/04-release_create-release-archive.yml`

**Usage:**

```powershell
.\scripts\create-github-release.ps1 `
  -Version <version> `
  -Token <github-token> `
  [-Prerelease]
```

**Example:**

```powershell
# Create GitHub release v1.4.0
.\scripts\create-github-release.ps1 `
  -Version 1.4.0 `
  -Token $env:GITHUB_TOKEN

# Creates:
# - GitHub Release v1.4.0
# - Uploads all artifacts from releases/v1.4.0/
```

---

### package-production-simple.ps1

**Purpose:** Create production deployment package

**Location:** `.github/workflows/04-release_build-binary-windows.yml`

**Usage:**

```powershell
.github/workflows/04-release_build-binary-windows.yml `
  -Edition <edition> `
  [-IncludeConfig] `
  [-IncludeDocs]
```

**Example:**

```powershell
# Production package with config
.github/workflows/04-release_build-binary-windows.yml `
  -Edition ENTERPRISE `
  -IncludeConfig `
  -IncludeDocs

# Output:
# - themisdb-enterprise-production.zip
#   ├── themis_server.exe
#   ├── config/
#   │   ├── production.json
#   │   └── docker-compose.yml
#   └── docs/
```

---

## 🧪 Development & Testing Scripts

### build-clang.ps1

**Purpose:** Build with Clang/LLVM

**Location:** `scripts/build-clang.ps1`

**Usage:**

```powershell
.\scripts\build-clang.ps1 `
  [-Configuration <Release|Debug>] `
  [-Sanitizer <address|thread|undefined>]
```

**Example:**

```powershell
# Debug build with AddressSanitizer
.\scripts\build-clang.ps1 `
  -Configuration Debug `
  -Sanitizer address
```

---

### build-ninja.ps1

**Purpose:** Fast build with Ninja

**Location:** `scripts/build-ninja.ps1`

**Usage:**

```powershell
.\scripts\build-ninja.ps1 `
  [-Configuration <Release|Debug>] `
  [-Parallel <jobs>]
```

**Example:**

```powershell
# Fast parallel build
.\scripts\build-ninja.ps1 `
  -Configuration Release `
  -Parallel 16
```

---

### build-llm.ps1

**Purpose:** Build with LLM support only

**Location:** `scripts/build-llm.ps1`

**Usage:**

```powershell
.\scripts\build-llm.ps1 `
  [-GPU] `
  [-CUDA]
```

**Example:**

```powershell
# LLM build with GPU + CUDA
.\scripts\build-llm.ps1 -GPU -CUDA
```

---

## ⚙️ Common Parameters

### Edition Parameter

```powershell
-Edition <MINIMAL|COMMUNITY|ENTERPRISE|HYPERSCALER>
```

**Values:**
- `MINIMAL` - 1 node, 0 GB GPU, Optional license
- `COMMUNITY` - 5 nodes, 24 GB GPU, Optional license
- `ENTERPRISE` - 100 nodes, 256 GB GPU, Required license (Release)
- `HYPERSCALER` - ∞ nodes, ∞ GPU, Mandatory license

### Configuration Parameter

```powershell
-Configuration <Release|Debug>
```

**Values:**
- `Release` - Optimized, no debug symbols
- `Debug` - Debug symbols, assertions, no optimization

### License Parameter

```powershell
-LicenseFile <path>
```

**Required for:**
- ENTERPRISE Release builds
- HYPERSCALER all builds

**Optional for:**
- MINIMAL all builds
- COMMUNITY all builds
- ENTERPRISE Debug builds

---

## 🔄 Script Workflows

### Workflow 1: Local Development (Community)

```powershell
# 1. Clone repo
git clone https://github.com/your-org/themisdb.git
cd themisdb

# 2. Build Community Edition
.\scripts\build-community-release.ps1 `
  -Platform windows `
  -Configuration Debug

# 3. Run tests
.\build-msvc\Debug\themis_server.exe --test

# 4. Build Release
.\scripts\build-community-release.ps1 `
  -Platform windows `
  -Configuration Release
```

### Workflow 2: Enterprise Production Build

```powershell
# 1. Obtain license
# Contact: service@themisdb.org

# 2. Save license
# C:\licenses\enterprise-license.json

# 3. Build Enterprise Release
.\scripts\build-enterprise-release.ps1 `
  -Environment production `
  -Configuration Release `
  -LicenseFile "C:\licenses\enterprise-license.json" `
  -SignArtifacts

# 4. Package for deployment
.github/workflows/04-release_build-binary-windows.yml `
  -Edition ENTERPRISE `
  -IncludeConfig

# 5. Deploy
# Copy themisdb-enterprise-production.zip to servers
```

### Workflow 3: Multi-Platform Release

```powershell
# 1. Prepare release
.github/workflows/04-release_create-release-archive.yml `
  -Version 1.4.0 `
  -Edition COMMUNITY `
  -Platforms windows,linux,docker

# 2. Build Windows
.\scripts\build-community-release.ps1 `
  -Platform windows `
  -Configuration Release

# 3. Build Linux (WSL)
wsl ./scripts/build-linux.sh --config release --edition COMMUNITY

# 4. Build Docker multi-arch
.\scripts\build-docker.ps1 `
  -Tag themisdb:1.4.0 `
  -Edition COMMUNITY `
  -Platform "linux/amd64,linux/arm64" `
  -Push

# 5. Create release archives
.\scripts\create-release-archive.ps1 `
  -Version 1.4.0 `
  -Edition COMMUNITY

# 6. Create GitHub release
.\scripts\create-github-release.ps1 `
  -Version 1.4.0 `
  -Token $env:GITHUB_TOKEN
```

### Workflow 4: Hyperscaler OEM Build

```powershell
# 1. Obtain OEM license
# Contact: service@themisdb.org

# 2. Build for customer
.\scripts\build-hyperscaler-release.ps1 `
  -BuildType oem `
  -Configuration Release `
  -LicenseFile "C:\licenses\hyperscaler-aws.json" `
  -CustomerName "AWS Cloud Provider" `
  -SignArtifacts

# 3. Build Docker
.\scripts\build-hyperscaler-docker.sh `
  --tag aws-themisdb:1.4.0 `
  --license /licenses/hyperscaler-aws.json `
  --customer "AWS Cloud Provider" `
  --platform linux/amd64,linux/arm64

# 4. Package OEM
.github/workflows/04-release_build-binary-windows.yml `
  -Edition HYPERSCALER `
  -IncludeConfig `
  -IncludeDocs

# 5. Deliver to customer
```

---

## 📊 Script Matrix

### Edition Build Scripts

| Script | Edition | License (Release) | License (Debug) | Platforms |
|--------|---------|------------------|----------------|-----------|
| `build-community-release.ps1` | COMMUNITY | ✅ Optional | ✅ Optional | Windows, Docker |
| `build-enterprise-release.ps1` | ENTERPRISE | ❌ Required | ✅ Optional | Windows, Docker |
| `build-hyperscaler-release.ps1` | HYPERSCALER | ❌ Mandatory | ❌ Mandatory | Windows, Docker |

### Platform Build Scripts

| Script | Platform | Edition Support | Multi-Arch |
|--------|----------|----------------|------------|
| `build-windows.ps1` | Windows | All | ❌ |
| `build-linux.sh` | Linux | All | ❌ |
| `build-minimal.sh` | Linux/ARM | MINIMAL | ✅ |
| `build-docker.ps1` | Docker | All | ✅ |
| `build-docker.sh` | Docker | All | ✅ |

### Development Scripts

| Script | Purpose | Use Case |
|--------|---------|----------|
| `build-clang.ps1` | Clang/LLVM build | Better diagnostics, sanitizers |
| `build-ninja.ps1` | Ninja build | Faster builds |
| `build-llm.ps1` | LLM-only build | LLM development |
| `build-wsl.sh` | WSL build | Windows → Linux |

---

## 🔗 Verwandte Dokumentation

### Build System
- [CMake Build System Overview](../deployment/CMAKE_BUILD_SYSTEM_OVERVIEW.md) - Architektur
- [Build Guide](guides_build.md) - Quick Start
- [Cross-Compile Guide](CROSS_COMPILE_COMPLETE.md) - Cross-Compilation

### Editions
- [Edition Limits Matrix](../deployment/EDITION_LIMITS_MATRIX.md) - Edition Vergleich
- [License Requirements](../deployment/LICENSE_REQUIREMENTS.md) - Lizenz-Info

### Deployment
- [Deployment Strategy](../deployment/deployment_strategy.md) - Strategie & Workflows
- [Docker Deployment](../deployment/DOCKER_DEPLOYMENT.md) - Container Deployment

---

## 💡 Tips & Best Practices

**1. Use Edition Scripts:**
```powershell
# ✅ Recommended
.\scripts\build-community-release.ps1

# ❌ Manual
cmake -B build -S . -DTHEMIS_EDITION=COMMUNITY ...
```

**2. Always Specify License for ENTERPRISE Release:**
```powershell
# ✅ Correct
.\scripts\build-enterprise-release.ps1 -LicenseFile "license.json"

# ❌ Fails
.\scripts\build-enterprise-release.ps1
```

**3. Use Multi-Arch Docker for Production:**
```powershell
# ✅ Better compatibility
.\scripts\build-docker.ps1 -Platform "linux/amd64,linux/arm64"

# ❌ Limited to one platform
.\scripts\build-docker.ps1 -Platform linux/amd64
```

**4. Sign Artifacts for Production:**
```powershell
# ✅ Production-ready
.\scripts\build-enterprise-release.ps1 -SignArtifacts

# ❌ Unsigned
.\scripts\build-enterprise-release.ps1
```

---

**Letzte Aktualisierung:** 23. April 2026  
**Version:** v1.4.0  
**Kategorie:** Build Scripts Reference
