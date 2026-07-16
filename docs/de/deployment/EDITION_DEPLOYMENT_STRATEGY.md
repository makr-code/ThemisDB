# ThemisDB Edition Deployment Strategy

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🚀 Deployment  
**Status:** Under Development  
**Scope:** Community, Enterprise, Hyperscaler Editions

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Build-Architektur](#️-build-architektur)
- [CMake Edition](#cmake-edition-configuration)

---

## 📋 Übersicht

ThemisDB bietet ab v1.3.0 **drei Editions-Modelle** mit unterschiedlichen Features und Lizenzmodellen:

| Edition | Kosten | Zielgruppe | Build-Typ | Support | Features |
|---------|--------|-----------|-----------|---------|----------|
| **Community** | Kostenlos | Open Source / Startups | Open Source | Community | Basis-DB Features |
| **Enterprise** | Abo | Mittlere bis große Unternehmen | Closed Source | Premium | Advanced + Security |
| **Hyperscaler** | Abo | Cloud-Provider / Massive Scale | Closed Source | 24/7 | All Features + Optimization |

---

## 🏗️ Build-Architektur

### Source-Code Struktur

```
themis/
├── src/
│   ├── core/                    ← Alle Editions (Community/Enterprise/Hyperscaler)
│   │   ├── database/
│   │   ├── query/
│   │   └── index/
│   │
│   ├── enterprise/              ← Enterprise + Hyperscaler ONLY
│   │   ├── sharding/            (Horizontale Skalierung)
│   │   ├── replication/         (Multi-Master, Geo-Replication)
│   │   ├── security/            (RBAC, HSM, Field-Encryption)
│   │   ├── management/          (Multi-Tenancy, Rate-Limiting)
│   │   └── analytics/           (OLAP, CEP Streaming)
│   │
│   └── hyperscaler/             ← Hyperscaler ONLY
│       ├── gpu_optimization/    (GPU-accelerated Queries)
│       ├── distributed/         (Massive Cluster, 1000+ nodes)
│       └── advanced_cdc/        (Real-time Sync, Rebalancing)
│
├── include/
│   ├── core/                    ← Alle Editions
│   ├── enterprise/              ← Enterprise + Hyperscaler
│   └── hyperscaler/             ← Hyperscaler
│
├── plugins/
│   ├── core/                    ← Community
│   │   ├── image_analysis/
│   │   ├── audio_processor/
│   │   └── geo/
│   │
│   ├── enterprise/              ← Enterprise + Hyperscaler
│   │   ├── gpu_backends/        (CUDA, Vulkan, HIP)
│   │   ├── advanced_search/
│   │   └── compliance_audit/
│   │
│   └── hyperscaler/             ← Hyperscaler
│       └── distributed_gpu/
│
└── CMakeLists.txt               ← Edition-Switch via -DTHEMIS_EDITION=...
```

### CMake Edition Configuration

```cmake
# CMakeLists.txt

option(THEMIS_EDITION "Edition: COMMUNITY, ENTERPRISE, HYPERSCALER" "COMMUNITY")

if(THEMIS_EDITION STREQUAL "COMMUNITY")
    message(STATUS "Building ThemisDB Community Edition")
    target_compile_definitions(themis_core PUBLIC 
        THEMIS_EDITION_COMMUNITY=1
        THEMIS_COMMUNITY_ONLY=1
    )
    
elseif(THEMIS_EDITION STREQUAL "ENTERPRISE")
    message(STATUS "Building ThemisDB Enterprise Edition")
    target_compile_definitions(themis_core PUBLIC 
        THEMIS_EDITION_ENTERPRISE=1
        THEMIS_ENTERPRISE_ENABLED=1
    )
    add_subdirectory(src/enterprise)
    add_subdirectory(plugins/enterprise)
    
elseif(THEMIS_EDITION STREQUAL "HYPERSCALER")
    message(STATUS "Building ThemisDB Hyperscaler Edition")
    target_compile_definitions(themis_core PUBLIC 
        THEMIS_EDITION_HYPERSCALER=1
        THEMIS_ENTERPRISE_ENABLED=1
        THEMIS_HYPERSCALER_ENABLED=1
    )
    add_subdirectory(src/enterprise)
    add_subdirectory(src/hyperscaler)
    add_subdirectory(plugins/enterprise)
    add_subdirectory(plugins/hyperscaler)
else()
    message(FATAL_ERROR "Unknown edition: ${THEMIS_EDITION}")
endif()
```

---

## 📦 Release & Package Structure

### Directory Layout

```
release/
├── v1.3.0/
│   │
│   ├── community/               ← Open Source Edition (GitHub)
│   │   ├── windows/
│   │   │   ├── themisdb-1.3.0-windows-x64-community.zip
│   │   │   ├── SHA256SUMS
│   │   │   ├── RELEASE_NOTES.md
│   │   │   └── CHANGELOG.md
│   │   │
│   │   ├── linux/
│   │   │   ├── themisdb-1.3.0-linux-x64-community.tar.gz
│   │   │   ├── themisdb-1.3.0-arm64-community.tar.gz
│   │   │   └── SHA256SUMS
│   │   │
│   │   ├── docker/
│   │   │   ├── themisdb:1.3.0-community (Docker Hub: Public)
│   │   │   ├── themisdb:1.3.0-community-dev
│   │   │   └── Dockerfile.community
│   │   │
│   │   ├── qnap/
│   │   │   ├── themisdb-1.3.0-qnap-x64-community.zip
│   │   │   └── SHA256SUMS
│   │   │
│   │   └── SOURCE.tar.gz        ← Full source code
│   │
│   ├── enterprise/              ← Licensed Edition (GitHub Releases + Private Repo)
│   │   ├── windows/
│   │   │   ├── themisdb-1.3.0-windows-x64-enterprise-prod.zip
│   │   │   ├── themisdb-1.3.0-windows-x64-enterprise-dev.zip
│   │   │   ├── LICENSE_KEY_REQUIRED.txt
│   │   │   └── SHA256SUMS
│   │   │
│   │   ├── linux/
│   │   │   ├── themisdb-1.3.0-linux-x64-enterprise-prod.tar.gz
│   │   │   ├── themisdb-1.3.0-arm64-enterprise-prod.tar.gz
│   │   │   ├── themisdb-1.3.0-linux-x64-enterprise-dev.tar.gz
│   │   │   └── SHA256SUMS
│   │   │
│   │   ├── docker/
│   │   │   ├── themisdb:1.3.0-enterprise (Docker Hub: Private)
│   │   │   ├── themisdb:1.3.0-enterprise-dev
│   │   │   └── Dockerfile.enterprise
│   │   │
│   │   ├── qnap/
│   │   │   ├── themisdb-1.3.0-qnap-x64-enterprise-prod.zip
│   │   │   ├── themisdb-1.3.0-qnap-x64-enterprise-dev.zip
│   │   │   └── SHA256SUMS
│   │   │
│   │   └── LICENSE_KEY_VALIDATOR.exe  ← License key validation tool
│   │
│   └── hyperscaler/             ← OEM Edition (Direct Distribution Only)
│       ├── windows/
│       │   ├── themisdb-1.3.0-windows-x64-hyperscaler-prod.zip
│       │   ├── themisdb-1.3.0-windows-x64-hyperscaler-dev.zip
│       │   └── SHA256SUMS
│       │
│       ├── linux/
│       │   ├── themisdb-1.3.0-linux-x64-hyperscaler-prod.tar.gz
│       │   ├── themisdb-1.3.0-arm64-hyperscaler-prod.tar.gz
│       │   ├── themisdb-1.3.0-linux-x64-hyperscaler-dev.tar.gz
│       │   └── SHA256SUMS
│       │
│       ├── docker/
│       │   ├── themisdb:1.3.0-hyperscaler (Private Registry Only)
│       │   ├── themisdb:1.3.0-hyperscaler-dev
│       │   └── Dockerfile.hyperscaler
│       │
│       ├── qnap/
│       │   ├── themisdb-1.3.0-qnap-x64-hyperscaler-prod.zip
│       │   └── SHA256SUMS
│       │
│       ├── LICENSE_KEY_VALIDATOR.exe
│       └── CUSTOM_OPTIMIZATION_GUIDE.md

│
└── EDITION_COMPATIBILITY.md     ← Feature Matrix
```

---

## 🔨 Build Process per Edition

### Community Edition (Open Source)

**Build Command:**
```powershell
# Windows
cmake -B build-community -DTHEMIS_EDITION=COMMUNITY -DCMAKE_BUILD_TYPE=Release
cmake --build build-community --config Release --parallel 8

# Linux
cmake -B build-community -DTHEMIS_EDITION=COMMUNITY -DCMAKE_BUILD_TYPE=Release
cmake --build build-community -j$(nproc)

# Docker
docker build -f Dockerfile.community -t themisdb:1.3.0-community .
```

**Output Artifacts:**
- `themisdb-1.3.0-windows-x64-community.zip` (~25 MB, mit GPU-Support)
- `themisdb-1.3.0-linux-x64-community.tar.gz` (~31 MB, mit GPU-Support)
- `themisdb:1.3.0-community` (Docker Hub public, GPU optional)
- Source Code (full repository)

**License:** MIT / Open Source  
**Distribution:** GitHub Releases (public)

**Community Features:**
```
✅ GPU Acceleration (CUDA, Vulkan, HIP - optional)
✅ Core Database Engine
✅ Vector Search (HNSW)
✅ Graph Queries
✅ Geospatial Queries
✅ Full-Text Search
✅ Time-Series Support
✅ JSON/Blob Storage
✅ Content Processing
✅ LLM Integration (llama.cpp - Embedding, Similarity, Inference)
```

---

### Enterprise Edition (Licensed)

**Build Command:**
```powershell
# Windows - Production
cmake -B build-enterprise -DTHEMIS_EDITION=ENTERPRISE -DCMAKE_BUILD_TYPE=Release
cmake --build build-enterprise --config Release --parallel 8

# Windows - Development (Debug symbols)
cmake -B build-enterprise-dev -DTHEMIS_EDITION=ENTERPRISE -DCMAKE_BUILD_TYPE=Debug
cmake --build build-enterprise-dev --config Debug --parallel 8

# Linux
cmake -B build-enterprise -DTHEMIS_EDITION=ENTERPRISE -DCMAKE_BUILD_TYPE=Release
cmake --build build-enterprise -j$(nproc)

# Docker
docker build -f Dockerfile.enterprise -t themisdb:1.3.0-enterprise .
```

**Output Artifacts:**
- `themisdb-1.3.0-windows-x64-enterprise-prod.zip` (~35 MB, optimiert)
- `themisdb-1.3.0-windows-x64-enterprise-dev.zip` (~65 MB, Debug-Symbole)
- `themisdb-1.3.0-linux-x64-enterprise-prod.tar.gz` (~38 MB)
- `themisdb-1.3.0-linux-x64-enterprise-dev.tar.gz` (~72 MB)
- `themisdb:1.3.0-enterprise` (Docker Hub private)
- `LICENSE_KEY_VALIDATOR.exe` (License key verification)

**License:** Commercial / Subscription  
**Distribution:** GitHub Releases (private) + Customer Portals

**Features Added (beyond Community):**
```
✅ Horizontal Sharding       (Skalierung auf 100+ nodes)
✅ Multi-Master Replication (Active-Active)
✅ Geo-Replication          (Cross-region)
✅ RBAC + HSM Integration   (Enterprise Security)
✅ Field-Level Encryption
✅ Advanced Audit Logging
✅ Multi-Tenancy
✅ OLAP Analytics
✅ Advanced Change Data Capture
✅ 24/7 Premium Support
```

---

### Hyperscaler Edition (OEM)

**Build Command:**
```powershell
# Windows - Production
cmake -B build-hyperscaler -DTHEMIS_EDITION=HYPERSCALER -DCMAKE_BUILD_TYPE=Release
cmake --build build-hyperscaler --config Release --parallel 8

# With GPU Optimization (CUDA/Vulkan)
cmake -B build-hyperscaler-gpu -DTHEMIS_EDITION=HYPERSCALER -DCMAKE_BUILD_TYPE=Release `
  -DTHEMIS_ENABLE_GPU=ON -DCUDA_COMPUTE_CAPABILITY=8.0
cmake --build build-hyperscaler-gpu --config Release --parallel 8

# Docker Multi-Arch with GPU
docker buildx build -f Dockerfile.hyperscaler --platform linux/amd64,linux/arm64 `
  -t themisdb:1.3.0-hyperscaler .
```

**Output Artifacts:**
- `themisdb-1.3.0-windows-x64-hyperscaler-prod.zip` (~42 MB)
- `themisdb-1.3.0-linux-x64-hyperscaler-prod.tar.gz` (~45 MB)
- `themisdb-1.3.0-arm64-hyperscaler-prod.tar.gz` (~48 MB)
- `themisdb:1.3.0-hyperscaler` (Private registry only, e.g., ECR, ACR, GCR)
- `CUSTOM_OPTIMIZATION_GUIDE.md`

**License:** OEM / Hyperscaler Contract  
**Distribution:** Direct delivery (no GitHub public release)

**Features Added (beyond Enterprise):**
```
✅ Massive Cluster Support (1000+ nodes)
✅ Advanced Distributed Consensus
✅ Real-time Rebalancing
✅ Custom Performance Tuning for Scale
✅ Advanced GPU Optimization (DirectX, Advanced HIP)
✅ Dedicated SLA Support (24/7/365)
✅ Custom Source Code Audit Rights
```

---

## 📦 Packaging & Release Scripts

### New Build Scripts

```powershell
# ============ Community Edition ============
.\scripts\build-community-release.ps1
  ├─ Builds Community Edition (all platforms)
  ├─ Generates SHA256SUMS
  ├─ Output: release/v1.3.0/community/
  └─ Action: Push to GitHub public release

# ============ Enterprise Edition ============
.\scripts\build-enterprise-release.ps1 -Environment production
  ├─ Builds Enterprise Production (all platforms)
  ├─ Generates LICENSE_KEY_VALIDATOR
  ├─ Output: release/v1.3.0/enterprise/
  └─ Action: Push to GitHub private + Customer Portal

.\scripts\build-enterprise-release.ps1 -Environment development
  ├─ Builds Enterprise Development (Debug Symbols)
  ├─ Output: release/v1.3.0/enterprise/dev/
  └─ Action: Internal use only

# ============ Hyperscaler Edition ============
.\scripts\build-hyperscaler-release.ps1 -GPU $true
  ├─ Builds Hyperscaler Edition (all platforms, GPU-optimized)
  ├─ Generates OEM packaging
  ├─ Output: release/v1.3.0/hyperscaler/
  └─ Action: Direct delivery (no public release)

# ============ Docker Multi-Edition ============
.\scripts\build-docker-editions.ps1
  ├─ docker build -f Dockerfile.community -t themisdb:1.3.0-community
  ├─ docker build -f Dockerfile.enterprise -t themisdb:1.3.0-enterprise
  ├─ docker build -f Dockerfile.hyperscaler -t themisdb:1.3.0-hyperscaler
  └─ Push to: Public Hub / Private Hub / Private Registry
```

---

## 🔐 License & Feature Gating

### Runtime Edition Detection

```cpp
// src/main_server.cpp

#if defined(THEMIS_COMMUNITY_ONLY)
    // Community features only
    if (config.enable_sharding) {
        throw std::runtime_error(
            "Sharding requires Enterprise Edition. "
            "Download at: https://themisdb.io/enterprise"
        );
    }
#elif defined(THEMIS_ENTERPRISE_ENABLED)
    // Enterprise + Community features
    if (config.enable_hyperscaler_mode && !validate_license_key(config.license_key)) {
        throw std::runtime_error("Invalid license key for Enterprise Edition");
    }
#elif defined(THEMIS_HYPERSCALER_ENABLED)
    // All features including Hyperscaler
    if (!validate_hyperscaler_license(config.license_key)) {
        throw std::runtime_error("Invalid hyperscaler license");
    }
#endif

// Log detected edition
THEMIS_INFO("=== ThemisDB Edition Information ===");
#if defined(THEMIS_COMMUNITY_ONLY)
THEMIS_INFO("Edition: Community (Open Source)");
THEMIS_INFO("Features: Core Database, Vector Search, Graph Queries");
#elif defined(THEMIS_ENTERPRISE_ENABLED)
THEMIS_INFO("Edition: Enterprise (Licensed)");
THEMIS_INFO("Features: Core + Sharding + Replication + Security");
#elif defined(THEMIS_HYPERSCALER_ENABLED)
THEMIS_INFO("Edition: Hyperscaler (OEM)");
THEMIS_INFO("Features: All Enterprise + GPU + Massive Scale");
#endif
```

### License Key Validator

```powershell
# LICENSE_KEY_VALIDATOR.exe

& "$PSScriptRoot\LICENSE_KEY_VALIDATOR.exe" `
    -EditionType "ENTERPRISE" `
    -LicenseKey $config.license_key `
    -CheckExpiration $true `
    -ValidateSignature $true
```

---

## 🐳 Docker Registry Strategy

### Public Docker Hub (Community)
```bash
docker.io/themisdb/themisdb:1.3.0-community       # Latest community
docker.io/themisdb/themisdb:1.3.0-community-dev   # Community dev
```

### Private Docker Hub (Enterprise)
```bash
docker.io/themisdb-enterprise/themisdb:1.3.0-enterprise
docker.io/themisdb-enterprise/themisdb:1.3.0-enterprise-dev
# Access: Username + API Token (provided to licensed customers)
```

### Private Registries (Hyperscaler)
```bash
# AWS ECR
<account>.dkr.ecr.<region>.amazonaws.com/themisdb:1.3.0-hyperscaler

# Azure ACR
<registry>.azurecr.io/themisdb:1.3.0-hyperscaler

# Google GCR
gcr.io/<project>/themisdb:1.3.0-hyperscaler
```

---

## 📋 Feature Matrix by Edition

| Feature | Community | Enterprise | Hyperscaler |
|---------|-----------|-----------|-------------|
| **Core Database** | ✅ | ✅ | ✅ |
| **Vector Search (HNSW)** | ✅ | ✅ | ✅ |
| **Graph Queries** | ✅ | ✅ | ✅ |
| **Geospatial** | ✅ | ✅ | ✅ |
| **Full-Text Search** | ✅ | ✅ | ✅ |
| **Time-Series** | ✅ | ✅ | ✅ |
| **JSON/Blob Storage** | ✅ | ✅ | ✅ |
| **GPU Acceleration (CUDA, Vulkan, HIP)** | ✅ | ✅ | ✅ |
| **Horizontal Sharding** | ❌ | ✅ | ✅ |
| **Multi-Master Replication** | ❌ | ✅ | ✅ |
| **Geo-Replication** | ❌ | ✅ | ✅ |
| **RBAC + HSM** | ❌ | ✅ | ✅ |
| **Field-Level Encryption** | ❌ | ✅ | ✅ |
| **Multi-Tenancy** | ❌ | ✅ | ✅ |
| **OLAP/CUBE/ROLLUP** | ❌ | ✅ | ✅ |
| **CEP Streaming** | ❌ | ✅ | ✅ |
| **Massive Clustering (1000+ nodes)** | ❌ | ❌ | ✅ |
| **Advanced CDC** | ❌ | ✅ | ✅ |
| **Custom Optimization** | ❌ | ❌ | ✅ |
| **Support Level** | Community | 24/7 Premium | 24/7/365 Dedicated |

---

## 🚀 CI/CD Pipeline

### GitHub Actions Workflow

```yaml
# .github/workflows/multi-edition-release.yml

name: Multi-Edition Release Build

on:
  push:
    tags:
      - 'v*.*.*'

jobs:
  build-community:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build Community Edition
        run: ./.github/workflows/04-release_build-binary-linux.yml
      - name: Upload to GitHub Releases (Public)
        uses: actions/upload-release-asset@v1

  build-enterprise:
    runs-on: ubuntu-latest
    if: secrets.ENTERPRISE_BUILD_ENABLED == 'true'
    steps:
      - uses: actions/checkout@v3
      - name: Build Enterprise Edition
        run: ./.github/workflows/04-release_publish-enterprise.yml -Environment production
      - name: Upload to Private Release Portal
        run: |
          $files = Get-ChildItem release/v*/enterprise/ -Recurse
          foreach ($file in $files) {
            & ./scripts/upload-to-portal.ps1 -File $file
          }

  build-hyperscaler:
    runs-on: ubuntu-latest
    if: secrets.HYPERSCALER_BUILD_ENABLED == 'true'
    steps:
      - uses: actions/checkout@v3
      - name: Build Hyperscaler Edition (GPU)
        run: ./.github/workflows/04-release_publish-hyperscaler.yml -GPU $true
      - name: Push to Private Registries
        run: |
          docker login -u ${{ secrets.ECR_USERNAME }} ...
          docker push <account>.dkr.ecr.us-east-1.amazonaws.com/themisdb:1.3.0-hyperscaler
```

---

## 📊 Version & Build Matrix Summary

**v1.3.0 Release Plan:**

```
┌─────────────┬──────────────┬─────────────┬──────────────────┐
│   Edition   │ Release Date │ Platforms   │ Docker Registries│
├─────────────┼──────────────┼─────────────┼──────────────────┤
│ Community   │ 21.12.2025   │ Win/Linux   │ Public DockerHub  │
│             │              │ Docker/QNAP │                  │
├─────────────┼──────────────┼─────────────┼──────────────────┤
│ Enterprise  │ 21.12.2025   │ Win/Linux   │ Private DockerHub │
│             │              │ Docker/QNAP │ (Subscription)   │
├─────────────┼──────────────┼─────────────┼──────────────────┤
│ Hyperscaler │ 21.12.2025   │ Win/Linux   │ Private Registries│
│             │              │ Docker/QNAP │ (OEM only)       │
└─────────────┴──────────────┴─────────────┴──────────────────┘
```

---

## ✅ Next Steps

1. **Update CMakeLists.txt** - Add `THEMIS_EDITION` option
2. **Create Edition-specific Dockerfiles** - Dockerfile.community, .enterprise, .hyperscaler
3. **Implement License Key Validator** - Runtime feature gating
4. **Build Release Scripts** - Automation für alle Editions
5. **Update Documentation** - Deployment guides per Edition
6. **Setup Private Registries** - ECR, ACR, GCR für Hyperscaler
7. **Customer Portal** - Distribution für Enterprise Packages
