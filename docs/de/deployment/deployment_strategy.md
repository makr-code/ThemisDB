# ThemisDB Build & Deployment Strategy

**Stand:** 6. April 2026  
**Version:** v1.3.1  
**Kategorie:** 🚀 Deployment  
**Status:** Production-Ready  
**Architecture:** Offline-First vcpkg Build System

---

## 📑 Inhaltsverzeichnis

- [Kernprinzip](#-kernprinzip-offline-first-vcpkg-strategy)
- [Quick Start](#-quick-start)
- [Plattform-Builds](#2-platform-spezifischer-build)
- [Edition-spezifische Build-Strategie](#-edition-spezifische-build-strategie)
- [Build-Dokumentation](#-build-dokumentation)
- [Verwandte Dokumentation](#-verwandte-dokumentation)

## 🎯 Kernprinzip: Offline-First vcpkg Strategy

ThemisDB nutzt eine **vcpkg Offline-First Architektur** für reproduzierbare, netzwerk-unabhängige Builds auf allen Plattformen:

### Vorteile
✅ **Offline-fähig:** Builds ohne Internetzugang nach initialem Download  
✅ **Reproduzierbar:** Identische Builds durch versionierte vcpkg baseline  
✅ **Schnell:** Keine wiederholten Downloads, ~50% schnellere Builds  
✅ **CI/CD-optimiert:** Cache kann zwischen Build-Agents geteilt werden  
✅ **Air-Gapped:** Perfekt für Enterprise/Government Deployments  

### Architektur

```
vcpkg/
├── downloads/           ← ~2.5 GB - SINGLE SOURCE OF TRUTH (alle Source-Archive)
│   ├── openssl-*.tar.gz
│   ├── rocksdb-*.tar.gz  
│   ├── boost_*.tar.gz
│   └── [135+ packages]
│
├── buildtrees/         ← ~3 GB - Temporäre Build-Artefakte (nicht versioniert)
├── packages/           ← ~10 GB - Installierte Packages (nicht versioniert)
└── scripts/
    └── buildsystems/
        └── vcpkg.cmake  ← CMake Integration
```

**NEU in v4.0.0:**
- ✅ Unified vcpkg offline cache für Windows, Linux, Docker, ARM
- ✅ Automatisches Cache-Management via `scripts/update-vcpkg-cache.ps1`
- ✅ CI/CD-ready vcpkg binary cache (~600 MB komprimiert)
- ✅ Docker multi-stage builds mit vcpkg cache layer
- ✅ Raspberry Pi / ARM offline build support

---

## 🚀 Quick Start

### 1. vcpkg Cache Setup (Einmalig)

**Wichtig:** Zuerst vcpkg cache initialisieren für offline builds:

```powershell
# Windows
.\scripts\setup-vcpkg-offline.ps1

# Linux/macOS
./scripts/setup-vcpkg-offline.sh
```

Dies lädt ~2.5 GB an Source-Archiven herunter in `vcpkg/downloads/`.  
**Einmalig erforderlich**, danach sind alle Builds offline-fähig.

### 2. Platform-Spezifischer Build

#### Windows (MSVC 2022) - **EMPFOHLEN**
```powershell
# Option 1: Schnellbuild mit allen Features (empfohlen für Deployment)
.\quick-build.ps1

# Option 2: Manuell mit VS 2022 Generator Preset (neu v4.0.0+)
$env:VCPKG_ROOT = "C:\VCC\themis\vcpkg"
cmake --preset windows-vs2022-release
cmake --build --preset windows-vs2022-release --parallel 8

# Option 3: Manuell mit CMake (Legacy)
cmake -B build-msvc -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DTHEMIS_CORE_SHARED=OFF `
  -DTHEMIS_ENABLE_LLM=OFF
cmake --build build-msvc --config Release --parallel 8
```

**Output:** `build-msvc/Release/themis_server.exe`  
**Zeit:** 25-35 min (erste Build), 5-10 min (inkrementell)  
**CMake Preset:** `windows-vs2022-release` (✨ neu, automatisch konfiguriert)

#### Linux (GCC/Clang)
```bash
# Ubuntu/Debian
cmake -B build-linux -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" \
  -DTHEMIS_ENABLE_LLM=ON
cmake --build build-linux -j$(nproc)
```

#### Raspberry Pi / ARM64
```bash
# Nach vcpkg offline setup
cmake -B build-arm -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" \
  -DTHEMIS_QNAP_BUILD=ON \  # Baseline x86-64, kein AVX
  -DTHEMIS_ENABLE_LLM=OFF   # Optional: LLM auf ARM
cmake --build build-arm -j4
```

#### Docker (Multi-Arch)
```bash
# Nutzt vcpkg cache layer automatisch
docker build -t themisdb:latest .

# Oder mit buildx für multi-arch
docker buildx build --platform linux/amd64,linux/arm64 \
  -t themisdb:latest --push .
```

### 3. Optional: Feature-Builds

```bash
# Minimal Build (Core nur, ~150 MB)
cmake -B build -DTHEMIS_ENABLE_LLM=OFF -DTHEMIS_BUILD_RPC_FRAMEWORK=OFF

# LLM Build (Core + llama.cpp, ~250 MB)
cmake -B build -DTHEMIS_ENABLE_LLM=ON

# Full Build (Core + LLM + RPC + GPU, ~350 MB)
cmake -B build -DTHEMIS_ENABLE_LLM=ON -DTHEMIS_BUILD_RPC_FRAMEWORK=ON -DTHEMIS_ENABLE_GPU=ON
```

---

## Build-Plattformen (v1.3.1) - **Stand: 26. Dezember 2025**

| Platform | Triplet | Compiler | Target | Binary Size | Package Size | Status |
|----------|---------|----------|--------|-------------|--------------|--------|
| **Windows** | x64-windows | MSVC 2022 (17.14) | Windows 10+ (x64) | 32 MB | 23 MB | ✅ Produktiv |
| **Linux (x64)** | x64-linux | GCC 13.3 | Ubuntu 22.04+ (x64) | 32 MB | 29 MB | ✅ Produktiv |
| **Linux (ARM64)** | arm64-linux | GCC 11.4 | Ubuntu 22.04+ (ARM64) | ~35 MB | ~30 MB | 🧪 Beta |
| **Docker** | x64-linux / arm64-linux | GCC 11.4 | Docker Multi-Arch | Varies | ~150 MB | ✅ Produktiv |
| **QNAP NAS** | x64-linux | GCC 11.4 | QNAP x86_64 | ~30 MB | ~28 MB | 🧪 Beta |
| **macOS** | arm64-osx / x64-osx | Clang | macOS 11+ (x64/ARM) | TBD | TBD | ⏳ Geplant |

**v1.3.1 Modular Build Matrix:**

| Configuration | ENABLE_LLM | BUILD_RPC | ENABLE_CUDA | ENABLE_GPU | Binary Size | Build Time |
|---------------|------------|-----------|-------------|------------|-------------|------------|
| **Minimal** | OFF | OFF | OFF | OFF | ~150 MB | 15-20 min |
| **LLM** | ON | OFF | OFF | OFF | ~250 MB | 25-30 min |
| **LLM+GPU** | ON | OFF | ON | ON | ~300 MB | 30-35 min |
| **LLM+RPC** | ON | ON | OFF | OFF | ~280 MB | 30-35 min |
| **Full** | ON | ON | ON | ON | ~350 MB | 35-40 min |

---

## 🏢 Edition-spezifische Build-Strategie

ThemisDB bietet **drei Editions-Modelle** mit unterschiedlichen Features, Lizenzmodellen und Build-Konfigurationen:

### Edition-Übersicht

| Edition | Lizenz | GPU VRAM Limit | Max. Nodes | Plugins | LLM Features | Build Script |
|---------|--------|----------------|------------|---------|--------------|--------------|
| **Community** | MIT (Open Source) | 24 GB | **5** ✅ | Core Only | Embedding, Similarity, Inference | `build-community-release.ps1` |
| **Enterprise** | Commercial Subscription | 256 GB | **100** ✅ (Sharding) | Enterprise Add-Ons | + Fine-Tuning, Model Management | `build-enterprise-release.ps1` |
| **Hyperscaler** | Custom OEM/Cloud | Unlimited | Unlimited (10000+) | All + Custom | Full Advanced Features | `build-hyperscaler-release.ps1` |

### Community Edition Build

**Zielgruppe:** Open Source Community, Startups, Entwicklung

```powershell
# Windows - Community Edition
.\scripts\build-community-release.ps1 -Platform all -Configuration Release

# Oder manuell mit CMake
cmake -B build-community -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DTHEMIS_EDITION=COMMUNITY `
  -DCMAKE_BUILD_TYPE=Release `
  -DTHEMIS_BUILD_TESTS=ON `
  -DTHEMIS_BUILD_BENCHMARKS=ON `
  -DTHEMIS_ENABLE_GPU=ON `
  -DTHEMIS_ENABLE_TRACING=ON

cmake --build build-community --config Release --parallel 8
```

**Linux - Community Edition:**
```bash
cmake -B build-community \
  -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" \
  -DTHEMIS_EDITION=COMMUNITY \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_LLM=ON

cmake --build build-community -j$(nproc)
```

**Features (Community Edition):**
- ✅ Kern-Datenbankfunktionalität (JSON, Graph, Vector, Time-Series)
- ✅ LLM Core Features (Embedding, Similarity Search, Inference)
- ✅ GPU Acceleration (bis 24 GB VRAM)
- ✅ Multi-Node Support (bis **5 Nodes** ✅)
- ✅ Basic Monitoring & Metrics (Prometheus/Grafana)
- ❌ Keine Enterprise Plugins
- ❌ Kein Auto-Sharding (manuelle Konfiguration möglich)
- ❌ Kein RBAC/Field-Encryption

### Enterprise Edition Build

**Zielgruppe:** Mittlere bis große Unternehmen, Production Deployments

**⚠️ LIZENZ ERFORDERLICH:** Enterprise Edition benötigt gültige Lizenz-Datei für **Release Builds** (Debug optional)

**Kontakt:** service@themisdb.org

```powershell
# Windows - Enterprise Edition
.\scripts\build-enterprise-release.ps1 -Environment production -Configuration Release

# Oder manuell mit CMake
cmake -B build-enterprise -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DTHEMIS_EDITION=ENTERPRISE `
  -DCMAKE_BUILD_TYPE=Release `
  -DTHEMIS_BUILD_TESTS=ON `
  -DTHEMIS_BUILD_BENCHMARKS=ON `
  -DTHEMIS_ENABLE_GPU=ON `
  -DTHEMIS_ENABLE_TRACING=ON `
  -DTHEMIS_ENABLE_LLM=ON `
  -DTHEMIS_ENABLE_ENTERPRISE_PLUGINS=ON `
  -DTHEMIS_ENABLE_MULTI_MASTER=ON `
  -DTHEMIS_ENABLE_FIELD_ENCRYPTION=ON `
  -DTHEMIS_ENABLE_RBAC=ON `
  -DTHEMIS_ENABLE_HSM=ON

cmake --build build-enterprise --config Release --parallel 8
```

**Linux - Enterprise Edition:**
```bash
cmake -B build-enterprise \
  -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_ENTERPRISE_PLUGINS=ON \
  -DTHEMIS_ENABLE_MULTI_MASTER=ON \
  -DTHEMIS_ENABLE_FIELD_ENCRYPTION=ON \
  -DTHEMIS_ENABLE_RBAC=ON \
  -DTHEMIS_ENABLE_HSM=ON

cmake --build build-enterprise -j$(nproc)
```

**Features (Enterprise Edition):**
- ✅ Alle Community Features
- ✅ Sharding & Horizontal Skalierung (bis 100 Nodes)
- ✅ Multi-Master Replication
- ✅ RBAC (Role-Based Access Control)
- ✅ Field-Level Encryption
- ✅ HSM Integration (Hardware Security Module)
- ✅ Enterprise Plugins (GPU Backends, Advanced Search)
- ✅ Advanced LLM Features (Fine-Tuning, Model Management)
- ✅ Advanced Monitoring & Metrics (Custom Dashboards, Alerting)
- ✅ Premium Support (SLA, 24/5)

**Lizenz-Konfiguration:**
```bash
# Lizenz-Datei bereitstellen
export THEMIS_LICENSE_FILE=/path/to/enterprise-license.lic

# Oder in config.yaml
license:
  file: /etc/themis/enterprise-license.lic
  type: enterprise
  validation: strict
```

### Hyperscaler Edition Build

**Zielgruppe:** Cloud Provider (AWS, Azure, GCP), Massive Scale Deployments

**⚠️ CUSTOM LIZENZ:** Hyperscaler Edition nur über OEM/Cloud-Partnerschaften

```powershell
# Windows - Hyperscaler Edition
.\scripts\build-hyperscaler-release.ps1 -Environment production -Configuration Release

# Oder manuell mit CMake
cmake -B build-hyperscaler -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DTHEMIS_EDITION=HYPERSCALER `
  -DCMAKE_BUILD_TYPE=Release `
  -DTHEMIS_BUILD_TESTS=ON `
  -DTHEMIS_BUILD_BENCHMARKS=ON `
  -DTHEMIS_ENABLE_GPU=ON `
  -DTHEMIS_ENABLE_TRACING=ON `
  -DTHEMIS_ENABLE_LLM=ON `
  -DTHEMIS_ENABLE_ENTERPRISE_PLUGINS=ON `
  -DTHEMIS_ENABLE_MULTI_MASTER=ON `
  -DTHEMIS_ENABLE_FIELD_ENCRYPTION=ON `
  -DTHEMIS_ENABLE_RBAC=ON `
  -DTHEMIS_ENABLE_HSM=ON `
  -DTHEMIS_ENABLE_HYPERSCALER_OPTIMIZATION=ON `
  -DTHEMIS_ENABLE_ADVANCED_CDC=ON `
  -DTHEMIS_ENABLE_GPU_CLUSTER=ON

cmake --build build-hyperscaler --config Release --parallel 8
```

**Features (Hyperscaler Edition):**
- ✅ Alle Enterprise Features
- ✅ Unbegrenzte GPU VRAM (Multi-GPU Cluster)
- ✅ Unbegrenzte Nodes (1000+ Nodes)
- ✅ GPU-optimierte Queries (Distributed GPU Processing)
- ✅ Advanced CDC (Real-time Sync, Auto-Rebalancing)
- ✅ Hyperscaler-spezifische Optimierungen
- ✅ Custom Plugin System
- ✅ White-Label Support
- ✅ Dedicated Support (SLA, 24/7)

### Edition Metriken & Monitoring

Alle Editions unterstützen **Prometheus-Metriken**, aber mit unterschiedlichem Umfang:

#### Community Edition Metriken
```yaml
# Basis-Metriken (Community)
metrics:
  enabled: true
  exporter: prometheus
  port: 9090
  
  # Verfügbare Metriken
  basic_metrics:
    - http_requests_total
    - http_request_duration_seconds
    - database_queries_total
    - database_query_duration_seconds
    - memory_usage_bytes
    - cpu_usage_percent
```

#### Enterprise Edition Metriken
```yaml
# Erweiterte Metriken (Enterprise)
metrics:
  enabled: true
  exporter: prometheus
  port: 9090
  
  # Zusätzliche Enterprise-Metriken
  enterprise_metrics:
    - sharding_node_health
    - replication_lag_seconds
    - rbac_authorization_checks_total
    - field_encryption_operations_total
    - hsm_operations_total
    - cache_hit_ratio
    - query_plan_optimization_duration
    
  # Custom Dashboards
  dashboards:
    - grafana_enterprise_overview
    - grafana_security_audit
    - grafana_performance_deep_dive
```

#### Hyperscaler Edition Metriken
```yaml
# Vollständige Metriken (Hyperscaler)
metrics:
  enabled: true
  exporter: prometheus
  port: 9090
  
  # Alle Metriken + Hyperscaler-spezifisch
  hyperscaler_metrics:
    - gpu_cluster_utilization
    - distributed_query_coordination
    - cdc_replication_throughput
    - auto_rebalancing_events
    - cross_region_latency
    - multi_tenant_isolation_score
    
  # Advanced Monitoring
  telemetry:
    opentelemetry: true
    distributed_tracing: true
    custom_exporters: ["datadog", "newrelic", "splunk"]
```

### Lizenz-Validierung zur Build-Zeit

```cmake
# CMakeLists.txt - Edition Validation
if(THEMIS_EDITION STREQUAL "ENTERPRISE")
    # Prüfe Enterprise Lizenz zur Build-Zeit (optional)
    if(DEFINED ENV{THEMIS_BUILD_LICENSE_KEY})
        message(STATUS "Enterprise Build License: Verified")
    else()
        message(WARNING "No Enterprise Build License found. Runtime license required.")
    endif()
    
    add_compile_definitions(
        THEMIS_EDITION_ENTERPRISE=1
        THEMIS_GPU_MAX_VRAM_GB=256
        THEMIS_SHARDING_MAX_NODES=100
    )
    
elseif(THEMIS_EDITION STREQUAL "HYPERSCALER")
    # Hyperscaler benötigt spezielle Build-Lizenz
    if(NOT DEFINED ENV{THEMIS_HYPERSCALER_BUILD_KEY})
        message(FATAL_ERROR "Hyperscaler Edition requires THEMIS_HYPERSCALER_BUILD_KEY")
    endif()
    
    add_compile_definitions(
        THEMIS_EDITION_HYPERSCALER=1
        THEMIS_GPU_MAX_VRAM_GB=0  # Unlimited
        THEMIS_SHARDING_MAX_NODES=0  # Unlimited
    )
endif()
```

### Runtime Lizenz-Prüfung

```cpp
// src/license/license_validator.cpp
/**
 * LicenseValidator - Validates ThemisDB edition licenses at runtime
 * 
 * Reads and validates license files to ensure the binary edition matches
 * the license edition and checks signature, expiry, and feature flags.
 * 
 * @throws LicenseException if validation fails (invalid signature, expired, edition mismatch)
 */
class LicenseValidator {
public:
    /**
     * Validates a license file and returns the licensed edition
     * 
     * @param licensePath Full path to the license file
     * @return EditionType The edition specified in the valid license
     * @throws LicenseException on validation failure
     */
    static EditionType validateLicense(const std::string& licensePath) {
        // Lese und validiere Lizenz-Datei
        auto license = parseLicenseFile(licensePath);
        
        // Prüfe Signatur und Gültigkeit
        if (!verifySignature(license)) {
            throw LicenseException("Invalid license signature");
        }
        
        if (license.expiryDate < getCurrentDate()) {
            throw LicenseException("License expired");
        }
        
        // Prüfe Edition-Match
        #ifdef THEMIS_EDITION_ENTERPRISE
            if (license.edition != Edition::ENTERPRISE) {
                throw LicenseException("Enterprise binary requires Enterprise license");
            }
        #endif
        
        return license.edition;
    }
};
```

**Siehe auch:**
- [EDITION_DEPLOYMENT_STRATEGY.md](EDITION_DEPLOYMENT_STRATEGY.md) - Vollständige Edition-Architektur
- [EDITION_CONTROL_MECHANISMS.md](EDITION_CONTROL_MECHANISMS.md) - Technische Implementierung
- [PRICING_MODEL_v1.3.5.md](PRICING_MODEL_v1.3.5.md) - Lizenzmodelle & Preise

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
  ├─ llama_wrapper.cpp
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

### Windows Build (MSVC 2022) - **✅ PRODUKTIONSREIF**

**Scripts:** 
- `.\quick-build.ps1` (automatisch)
- `.\scripts\build-windows.ps1` (manuell)

**Methode 1: Manuell mit VS 2022 Generator (EMPFOHLEN)**

```powershell
# 1. VCPKG_ROOT setzen
$env:VCPKG_ROOT = "C:\VCC\themis\vcpkg"

# 2. CMake Konfiguration
cmake -B build-msvc -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DTHEMIS_BUILD_TESTS=OFF `
  -DTHEMIS_BUILD_BENCHMARKS=OFF `
  -DTHEMIS_ENABLE_TRACING=OFF

# 3. Kompilierung (Release)
cmake --build build-msvc --config Release --parallel 8

# 4. Packaging
.\scripts\package-release-v1.3.0.ps1
```

**Bekannte Probleme & Lösungen:**
- ⚠️ **Thrift Build-Fehler** (Zugriff verweigert): Arrow-Dependency, wird durch vcpkg binary cache gelöst
- ⚠️ **HybridLogicalClock nicht gefunden**: Include `replication/multi_master_replication.h` in `replication_manager.cpp` (bereits gefixt)

**Output:**
- `build-msvc\Release\themis_server.exe` (~32 MB Binary)
- `release\themisdb-v1.3.0-windows-x64.zip` (~23 MB Package mit DLLs)
- Statisch gelinkte Dependencies + vcpkg DLLs

**Time Estimate:** 
- CMake Konfiguration: ~13 Minuten (vcpkg install)
- Kompilierung: ~2-5 Minuten
- Packaging: ~30 Sekunden (eingebettet)

### Linux Build (GCC 13.3) - **✅ PRODUKTIONSREIF**

**Script:** `.\scripts\build-linux.sh` (oder WSL-Integration)

**Via WSL (empfohlen für Windows-Entwickler):**

```powershell
# 1. CMake Konfiguration (in WSL)
wsl bash -c "cd /mnt/c/VCC/themis && \
  export VCPKG_ROOT=/mnt/c/VCC/themis/vcpkg && \
  cmake -B build-wsl -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/mnt/c/VCC/themis/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DTHEMIS_BUILD_TESTS=OFF \
    -DTHEMIS_BUILD_BENCHMARKS=OFF \
    -DTHEMIS_ENABLE_TRACING=OFF"

# 2. Kompilierung
wsl bash -c "cd /mnt/c/VCC/themis/build-wsl && cmake --build . --config Release -j8"

# 3. Packaging
wsl bash -c "cd /mnt/c/VCC/themis && \
  tar -czf release/themisdb-v1.3.0-linux-x64.tar.gz \
    -C build-wsl themis_server themis_demo themis_demo_encryption \
    --transform 's|^|themisdb-1.3.0/bin/|'"
```

**Native Linux:**

```bash
# Identisch, aber ohne wsl prefix und /mnt/c Pfade
cmake -B build-linux -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build-linux -j$(nproc)
```

**Output:**
- `build-wsl/themis_server` (32 MB Binary, ELF x86_64)
- `release/themisdb-v1.3.0-linux-x64.tar.gz` (29 MB Package)

**Time Estimate:** 
- CMake Konfiguration: ~38 Minuten (vcpkg install)
- Kompilierung: ~5-10 Minuten
- Packaging: ~10 Sekunden

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

- [Bibliotheken-Übersicht](BIBLIOTHEKEN_UBERSICHT.md) - Alle verwendeten Libraries mit Vendor-Dokumentation
- [Build-Optionen Referenz](BUILD_OPTIONEN_REFERENZ.md) - Alle CMake Schalter (61 Optionen)
- [vcpkg Offline Strategy](VCPKG_OFFLINE_STRATEGY.md) - Offline Build System Details
- [Docker Deployment](DOCKER_DEPLOYMENT.md) - Container Deployment Guide
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

**Letzte Aktualisierung:** 26. Dezember 2025  
**Autor:** Build System v2.0 Implementation Team  
**Status:** Production-Ready ✓
