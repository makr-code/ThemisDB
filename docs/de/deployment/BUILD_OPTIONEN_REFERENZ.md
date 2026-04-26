# ThemisDB Build-Optionen Referenz

**Stand:** 6. April 2026  
**Version:** v1.3.1  
**Kategorie:** 🚀 Deployment  
**Status:** Production-Ready

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Standard Build-Optionen](#-standard-build-optionen)
- [Performance-Optimierungen](#-performance-optimierungen)
- [Netzwerk-Protokolle](#-netzwerk-protokolle)
- [Hardware-Beschleunigung](#-hardware-beschleunigung)
- [Edition & Features](#-edition--features)
- [Build-Konfiguration](#-build-konfiguration)

---

## 🎯 Übersicht

ThemisDB bietet **61 CMake Build-Optionen** für flexible Konfiguration:

- ✅ **Opt-In Design:** Alle Features standardmäßig deaktiviert (außer Kern-Features)
- 🎛️ **Fein-granular:** Jedes Feature einzeln steuerbar
- 📦 **Minimal Build:** ~150 MB Binärgröße (nur Kern-Features)
- 🚀 **Full Build:** ~350 MB Binärgröße (alle Features)

### Schnellübersicht

```bash
# Minimal Build (Standard)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Mit LLM Support
cmake -B build -DCMAKE_BUILD_TYPE=Release -DTHEMIS_ENABLE_LLM=ON

# Performance-Optimiert
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_MIMALLOC=ON \
  -DTHEMIS_ENABLE_HUGE_PAGES=ON \
  -DTHEMIS_ENABLE_RCU_INDEX=ON

# Full Build (alle Features)
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_GRPC=ON \
  -DTHEMIS_ENABLE_HTTP2=ON
```

---

## 🔧 Standard Build-Optionen

### THEMIS_BUILD_TESTS
- **Typ:** BOOL
- **Standard:** ON
- **Beschreibung:** Unit Tests bauen (Google Test)
- **Dependencies:** `gtest` (automatisch über vcpkg)

```bash
cmake -B build -DTHEMIS_BUILD_TESTS=OFF  # Deaktiviert Tests
```

### THEMIS_BUILD_BENCHMARKS
- **Typ:** BOOL
- **Standard:** ON
- **Beschreibung:** Performance Benchmarks bauen
- **Dependencies:** `benchmark` (Google Benchmark)

```bash
cmake -B build -DTHEMIS_BUILD_BENCHMARKS=OFF
```

### THEMIS_ENABLE_TRACING
- **Typ:** BOOL
- **Standard:** ON
- **Beschreibung:** OpenTelemetry Distributed Tracing
- **Dependencies:** `opentelemetry-cpp` mit `otlp-http`
- **Dokumentation:** [OpenTelemetry Docs](https://opentelemetry.io/docs/instrumentation/cpp/)

```bash
cmake -B build -DTHEMIS_ENABLE_TRACING=OFF  # Embedded/IoT Builds
```

### THEMIS_ENABLE_ASAN
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** AddressSanitizer für Memory-Bug-Detection
- **Plattform:** Linux/macOS (GCC/Clang)

```bash
cmake -B build -DTHEMIS_ENABLE_ASAN=ON
```

### THEMIS_STRICT_BUILD
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Warnings als Errors behandeln
- **Einsatz:** CI/CD, Pre-Release Testing

```bash
cmake -B build -DTHEMIS_STRICT_BUILD=ON
```

---

## 🚀 Performance-Optimierungen

### Phase 1: Quick Wins (Low Effort, High Impact)

#### THEMIS_ENABLE_MIMALLOC
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Microsoft mimalloc Allocator
- **Performance:** +10-20% bei Memory-intensiven Workloads
- **Dependencies:** `mimalloc` (vcpkg)
- **Dokumentation:** [mimalloc Docs](https://microsoft.github.io/mimalloc/)

```bash
cmake -B build -DTHEMIS_ENABLE_MIMALLOC=ON
```

**Empfohlen für:** Production Deployments, High-Throughput Workloads

#### THEMIS_ENABLE_HUGE_PAGES
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Huge Pages (2MB/1GB) Support
- **Performance:** +15-30% Memory-Performance
- **Plattform:** Linux/Windows (OS-Konfiguration erforderlich)

```bash
# Linux Setup
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

# Build
cmake -B build -DTHEMIS_ENABLE_HUGE_PAGES=ON
```

**Dokumentation:** [Linux Huge Pages](https://www.kernel.org/doc/Documentation/vm/hugetlbpage.txt)

#### THEMIS_ENABLE_RCU_INDEX
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Read-Copy-Update für Indexstrukturen
- **Performance:** +200-500% Read-Performance (read-heavy workloads)
- **Dependencies:** Keine (C++20 native)

```bash
cmake -B build -DTHEMIS_ENABLE_RCU_INDEX=ON
```

**Empfohlen für:** Read-Heavy Analytics, Reporting

#### THEMIS_ENABLE_LIRS_CACHE
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** LIRS Cache Replacement Policy
- **Performance:** +30-40% Cache Hit Rate
- **Dependencies:** Keine (C++20 native)

```bash
cmake -B build -DTHEMIS_ENABLE_LIRS_CACHE=ON
```

**Empfohlen für:** Scan-resistante Workloads, OLAP

### Phase 2: Medium-Term (Medium Effort, High Impact)

#### THEMIS_ENABLE_WISCKEY
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** WiscKey Key/Value Separation
- **Performance:** +40-60% Write-Throughput
- **Status:** Forschungs-Feature (Experimental)

```bash
cmake -B build -DTHEMIS_ENABLE_WISCKEY=ON
```

#### THEMIS_ENABLE_DOSTOEVSKY
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Dostoevsky Adaptive LSM
- **Performance:** +25-35% Mixed Workloads
- **Status:** Forschungs-Feature (Experimental)

#### THEMIS_ENABLE_CICADA
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Cicada Optimistic Concurrency Control
- **Performance:** +100-150% Transaction Throughput
- **Status:** Forschungs-Feature (Experimental)

#### THEMIS_ENABLE_LIGRA
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Ligra Graph Processing Framework
- **Performance:** +200-300% Graph Queries
- **Status:** Forschungs-Feature (Experimental)

#### THEMIS_ENABLE_RABITQ
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** RaBitQ Vector Quantization
- **Performance:** 16x Memory Reduction für Vektoren
- **Status:** Forschungs-Feature (Experimental)

### Phase 3: Long-Term (High Effort, Very High Impact)

#### THEMIS_ENABLE_DISKANN
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Microsoft DiskANN (Billion-Scale Vector Search)
- **Performance:** +300-400% Large-Scale Vector Search
- **Status:** Forschungs-Feature (Experimental)

#### THEMIS_ENABLE_BWTREE
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Bw-Tree Lock-Free Index
- **Performance:** +100-200% Concurrent Writes
- **Status:** Forschungs-Feature (Experimental)

#### THEMIS_ENABLE_SPLINTERDB
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** SplinterDB Concurrent Compaction
- **Performance:** -70% P99 Latency
- **Status:** Forschungs-Feature (Experimental)

#### THEMIS_ENABLE_GUNROCK
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** NVIDIA Gunrock GPU Graph Analytics
- **Performance:** +1000-3000% Graph Analytics (GPU)
- **Status:** Forschungs-Feature (Experimental)
- **Dependencies:** CUDA Toolkit

#### THEMIS_ENABLE_BAO
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Bao ML-Based Query Optimizer
- **Performance:** +30-70% Query Performance
- **Status:** Forschungs-Feature (Experimental)

**Hinweis:** Phase 2/3 Features sind experimentell. Siehe [Performance Research](../research/) für Details.

---

## 🌐 Netzwerk-Protokolle

Alle Protokolle sind **Opt-In** (Standard: OFF) außer HTTP/1.1 und gRPC.

### THEMIS_ENABLE_HTTP2
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** HTTP/2 Support mit Server Push
- **Dependencies:** `nghttp2` (vcpkg feature: `http2`)
- **Dokumentation:** [nghttp2 Docs](https://nghttp2.org/)

```bash
cmake -B build -DTHEMIS_ENABLE_HTTP2=ON
```

### THEMIS_ENABLE_HTTP3
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** HTTP/3 (QUIC) Support
- **Dependencies:** `nghttp3`, `ngtcp2` (vcpkg feature: `http3`)
- **Dokumentation:** [HTTP/3 Spec](https://www.rfc-editor.org/rfc/rfc9114.html)

```bash
cmake -B build -DTHEMIS_ENABLE_HTTP3=ON
```

### THEMIS_ENABLE_GRPC
- **Typ:** BOOL
- **Standard:** ON (v1.3.0+)
- **Beschreibung:** gRPC Support (Inter-Shard Communication)
- **Dependencies:** `grpc`, `protobuf` (vcpkg feature: `grpc`)
- **Dokumentation:** [gRPC C++ Docs](https://grpc.io/docs/languages/cpp/)

```bash
cmake -B build -DTHEMIS_ENABLE_GRPC=OFF  # Deaktivieren
```

### THEMIS_ENABLE_WEBSOCKET
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** WebSocket Support mit CDC
- **Dependencies:** Boost.Beast (bereits vorhanden)
- **Use-Case:** Real-Time Updates, Live Dashboards

```bash
cmake -B build -DTHEMIS_ENABLE_WEBSOCKET=ON
```

### THEMIS_ENABLE_GRAPHQL
- **Typ:** BOOL
- **Standard:** ON (v1.3.0+)
- **Beschreibung:** GraphQL API Support
- **Dependencies:** Eingebaut

### THEMIS_ENABLE_SSE
- **Typ:** BOOL
- **Standard:** ON (v1.3.0+)
- **Beschreibung:** Server-Sent Events Support
- **Dependencies:** Eingebaut
- **Use-Case:** Real-Time Notifications

### THEMIS_ENABLE_MCP
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Model Context Protocol (Anthropic)
- **Dependencies:** Keine
- **Use-Case:** LLM Tool Integration
- **Dokumentation:** [MCP Spec](https://modelcontextprotocol.io/)

```bash
cmake -B build -DTHEMIS_ENABLE_MCP=ON -DTHEMIS_ENABLE_LLM=ON
```

### THEMIS_ENABLE_MQTT
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** MQTT Protocol Support (IoT)
- **Dependencies:** `paho-mqttpp3` (vcpkg feature: `mqtt`)
- **Dokumentation:** [Eclipse Paho](https://eclipse.dev/paho/)

```bash
cmake -B build -DTHEMIS_ENABLE_MQTT=ON
```

### THEMIS_ENABLE_POSTGRES_WIRE
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** PostgreSQL Wire Protocol Support
- **Dependencies:** Eingebaut
- **Use-Case:** SQL Tool Compatibility (DBeaver, pgAdmin)

```bash
cmake -B build -DTHEMIS_ENABLE_POSTGRES_WIRE=ON
```

---

## 🎮 Hardware-Beschleunigung

### GPU Acceleration

#### THEMIS_ENABLE_GPU
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** GPU Acceleration für Vector Search
- **Dependencies:** `faiss` (vcpkg feature: `gpu`)

```bash
cmake -B build -DTHEMIS_ENABLE_GPU=ON
```

#### THEMIS_ENABLE_CUDA
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** NVIDIA CUDA Support
- **Dependencies:** CUDA Toolkit (extern)
- **Dokumentation:** [CUDA Docs](https://docs.nvidia.com/cuda/)

```bash
cmake -B build -DTHEMIS_ENABLE_CUDA=ON
```

#### THEMIS_ENABLE_HIP
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** AMD HIP Support
- **Dokumentation:** [HIP Docs](https://rocm.docs.amd.com/projects/HIP/)

#### THEMIS_ENABLE_ROCM
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** AMD ROCm Platform
- **Dokumentation:** [ROCm Docs](https://rocm.docs.amd.com/)

#### THEMIS_ENABLE_ZLUDA
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** CUDA Compatibility auf AMD GPUs
- **Dokumentation:** [ZLUDA GitHub](https://github.com/vosen/ZLUDA)

### Weitere Compute-Backends

#### THEMIS_ENABLE_VULKAN
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Vulkan Compute Shaders
- **Plattform:** Cross-Platform

#### THEMIS_ENABLE_DIRECTX
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** DirectX 12 Compute Shaders
- **Plattform:** Windows only

#### THEMIS_ENABLE_METAL
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Apple Metal Compute
- **Plattform:** macOS/iOS

#### THEMIS_ENABLE_OPENGL
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** OpenGL Compute Shaders

#### THEMIS_ENABLE_OPENCL
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** OpenCL Support

#### THEMIS_ENABLE_ONEAPI
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Intel OneAPI/SYCL
- **Dokumentation:** [OneAPI Docs](https://www.intel.com/content/www/us/en/developer/tools/oneapi/)

#### THEMIS_ENABLE_WEBGPU
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** WebGPU Support (Experimental)

---

## 🏢 Edition & Features

### THEMIS_EDITION
- **Typ:** STRING
- **Standard:** COMMUNITY
- **Werte:** `MINIMAL`, `COMMUNITY`, `ENTERPRISE`, `HYPERSCALER`
- **Beschreibung:** Compile-Time Edition Selection (v1.4.0+)
- **Lizenz:** Enterprise (Release)/Hyperscaler (alle) benötigen gültige Lizenz

```bash
# Minimal Edition (Embedded, 1 Node)
cmake -B build -DTHEMIS_EDITION=MINIMAL

# Community Edition (Standard, Open Source, 5 Nodes)
cmake -B build -DTHEMIS_EDITION=COMMUNITY

# Enterprise Edition (License erforderlich für Release)
cmake -B build -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_LICENSE_FILE=/path/to/license.json

# Hyperscaler Edition (License mandatory für alle Builds)
cmake -B build -DTHEMIS_EDITION=HYPERSCALER \
  -DTHEMIS_LICENSE_FILE=/path/to/license.json
```

**Edition-Limits (v1.4.0):**

Siehe [Edition Limits Matrix](EDITION_LIMITS_MATRIX.md) für vollständige Tabelle.

| Edition | GPU VRAM | Max Nodes | Lizenz |
|---------|----------|-----------|--------|
| **MINIMAL** | 0 GB | 1 | Optional |
| **COMMUNITY** | 24 GB | **5** ✅ | Optional (MIT Open Source) |
| **ENTERPRISE** | 256 GB | **100** ✅ | Required (Release), Optional (Debug) |
| **HYPERSCALER** | ∞ | ∞ | Mandatory (all builds) |

**Build Scripts:**
- `.github/workflows/04-release_build-binary-linux.yml` - Community Edition
- `.github/workflows/04-release_publish-enterprise.yml` - Enterprise Edition
- `.github/workflows/04-release_publish-hyperscaler.yml` - Hyperscaler Edition

**Dokumentation:** 
- [Edition Limits Matrix](EDITION_LIMITS_MATRIX.md) - **Single Source of Truth**
- [License Requirements](LICENSE_REQUIREMENTS.md) - Lizenz-Enforcement Details

### THEMIS_LICENSE_FILE (NEW v1.4.0)
- **Typ:** PATH
- **Standard:** `<empty>`
- **Beschreibung:** Pfad zur license.json Datei
- **Erforderlich für:**
  - ENTERPRISE Release Builds ⚠️
  - HYPERSCALER alle Builds ❌
- **Optional für:**
  - MINIMAL alle Builds ✅
  - COMMUNITY alle Builds ✅
  - ENTERPRISE Debug Builds ✅

```bash
# ENTERPRISE Release (License erforderlich)
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_LICENSE_FILE=/path/to/enterprise-license.json

# ENTERPRISE Debug (License optional)
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Debug
# ✅ Success - keine Lizenz erforderlich

# HYPERSCALER (License mandatory)
cmake -B build -S . \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DTHEMIS_LICENSE_FILE=/path/to/hyperscaler-license.json
```

**Siehe:** [License Requirements](LICENSE_REQUIREMENTS.md) für vollständige Details.

**Kontakt:** service@themisdb.org

### THEMIS_ENTERPRISE
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Enterprise Capabilities Bundle
- **Automatisch gesetzt:** Wenn `THEMIS_EDITION=ENTERPRISE`

### THEMIS_BUILD_ENTERPRISE_PLUGINS
- **Typ:** BOOL
- **Standard:** ON
- **Beschreibung:** Enterprise Plugins bauen
- **Enthält:** GPU Backends, Advanced Search, Compliance Audit

### Enterprise-spezifische Optionen

Die folgenden Optionen sind nur in **Enterprise** und **Hyperscaler** Editions verfügbar:

#### THEMIS_ENABLE_MULTI_MASTER
- **Typ:** BOOL
- **Standard:** OFF
- **Edition:** Enterprise, Hyperscaler
- **Beschreibung:** Multi-Master Replication für HA
- **Lizenz:** Enterprise License erforderlich

```bash
cmake -B build \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DTHEMIS_ENABLE_MULTI_MASTER=ON
```

#### THEMIS_ENABLE_FIELD_ENCRYPTION
- **Typ:** BOOL
- **Standard:** OFF
- **Edition:** Enterprise, Hyperscaler
- **Beschreibung:** Field-Level Encryption
- **Compliance:** GDPR, HIPAA, SOC2

```bash
cmake -B build \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DTHEMIS_ENABLE_FIELD_ENCRYPTION=ON
```

#### THEMIS_ENABLE_RBAC
- **Typ:** BOOL
- **Standard:** OFF
- **Edition:** Enterprise, Hyperscaler
- **Beschreibung:** Role-Based Access Control
- **Features:** Fine-grained Permissions, User Groups, Audit Logging

```bash
cmake -B build \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DTHEMIS_ENABLE_RBAC=ON
```

#### THEMIS_ENABLE_HSM
- **Typ:** BOOL
- **Standard:** OFF
- **Edition:** Enterprise, Hyperscaler
- **Beschreibung:** Hardware Security Module Integration
- **Standards:** PKCS#11, FIPS 140-2

```bash
cmake -B build \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DTHEMIS_ENABLE_HSM=ON
```

### Hyperscaler-spezifische Optionen

Die folgenden Optionen sind nur in **Hyperscaler** Edition verfügbar:

#### THEMIS_ENABLE_HYPERSCALER_OPTIMIZATION
- **Typ:** BOOL
- **Standard:** OFF
- **Edition:** Hyperscaler only
- **Beschreibung:** Massive-Scale Optimierungen
- **Features:** 1000+ Node Support, Cross-Region Replication

#### THEMIS_ENABLE_ADVANCED_CDC
- **Typ:** BOOL
- **Standard:** OFF
- **Edition:** Hyperscaler only
- **Beschreibung:** Advanced Change Data Capture
- **Features:** Real-time Sync, Auto-Rebalancing

#### THEMIS_ENABLE_GPU_CLUSTER
- **Typ:** BOOL
- **Standard:** OFF
- **Edition:** Hyperscaler only
- **Beschreibung:** Multi-GPU Cluster Support
- **Features:** Distributed GPU Processing, Unlimited VRAM

### THEMIS_ENABLE_LLM
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** LLM Support mit llama.cpp (v1.3.0+)
- **Dependencies:** llama.cpp (Git Submodule)

```bash
cmake -B build -DTHEMIS_ENABLE_LLM=ON
```

### Feature-Bundles

#### THEMIS_EMBEDDED
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Embedded/Lightweight Variant
- **Effekt:** Deaktiviert TBB, Arrow, OpenTelemetry, CUDA

```bash
cmake -B build -DTHEMIS_EMBEDDED=ON
```

#### THEMIS_ENABLE_OLAP_VARIANT
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** OLAP Workloads mit DuckDB
- **Status:** Geplant für v1.4.0

#### THEMIS_VLLM_COLOCATION
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** vLLM Co-Location Optimierung
- **Effekt:** Forciert CUDA ON

```bash
cmake -B build -DTHEMIS_VLLM_COLOCATION=ON
```

### Geo Features

#### THEMIS_GEO
- **Typ:** BOOL
- **Standard:** ON
- **Beschreibung:** Geo Core Feature Scaffolding

#### THEMIS_GEO_SIMD
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** SIMD Kernels für Geo (Enterprise Add-In)

#### THEMIS_GEO_GPU
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** GPU Backend für Geo (Enterprise Add-In)

#### THEMIS_GEO_H3
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Uber H3 Integration (Enterprise Add-In)
- **Dokumentation:** [H3 Docs](https://h3geo.org/)

#### THEMIS_GEO_GEOS_PLUGIN
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** GEOS Prepared Geometries Plugin
- **Dokumentation:** [GEOS Docs](https://libgeos.org/)

---

## ⚙️ Build-Konfiguration

### THEMIS_CORE_SHARED
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** themis_core als Shared Library (.so/.dll)
- **Hinweis:** Windows LNK1189 Export Limit → Static Build empfohlen

```bash
cmake -B build -DTHEMIS_CORE_SHARED=ON  # Linux: OK, Windows: Problematisch
```

### THEMIS_STATIC_BUILD
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Fully Static Binary
- **Use-Case:** QNAP NAS, Air-Gapped Deployments

```bash
cmake -B build -DTHEMIS_STATIC_BUILD=ON
```

### THEMIS_QNAP_BUILD
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** QNAP NAS Build (Baseline x86-64, kein AVX)
- **Effekt:** Forciert Static Build, deaktiviert AVX2

```bash
cmake -B build -DTHEMIS_QNAP_BUILD=ON
```

### THEMIS_ENABLE_AVX2
- **Typ:** BOOL
- **Standard:** ON
- **Beschreibung:** AVX2/FMA SIMD Kernels (MSVC)
- **Plattform:** x86-64 mit AVX2 Support

```bash
cmake -B build -DTHEMIS_ENABLE_AVX2=OFF  # Für ältere CPUs
```

### THEMIS_ENABLE_HSM_REAL
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Real PKCS#11 HSM Provider
- **Dependencies:** HSM PKCS#11 Library

### THEMIS_ENABLE_CONTENT_PROCESSORS
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** Content Processors (Audio/Image/Video/Geo/CAD)
- **Use-Case:** Media Asset Management

### THEMIS_BUILD_ADMIN_TOOLS
- **Typ:** BOOL
- **Standard:** OFF
- **Beschreibung:** .NET Admin Tools bauen
- **Dependencies:** .NET SDK 8.0+

---

## 📊 Zusammenfassung

| Kategorie | Anzahl Optionen | Standard ON | Standard OFF |
|-----------|-----------------|-------------|--------------|
| **Standard Build** | 5 | 2 | 3 |
| **Performance (Phase 1)** | 4 | 0 | 4 |
| **Performance (Phase 2/3)** | 10 | 0 | 10 |
| **Netzwerk-Protokolle** | 9 | 3 | 6 |
| **Hardware-Beschleunigung** | 12 | 0 | 12 |
| **Edition & Features** | 13 | 2 | 11 |
| **Build-Konfiguration** | 8 | 2 | 6 |
| **Gesamt** | **61** | **9** | **52** |

---

## 🔗 Verwandte Dokumentation

- [Library Overview](BIBLIOTHEKEN_UBERSICHT.md) - Alle Abhängigkeiten
- [Deployment Strategy](deployment_strategy.md) - Build & Deployment
- [Performance Research](../research/) - Optimierungs-Forschung
- [vcpkg Offline Strategy](VCPKG_OFFLINE_STRATEGY.md) - Dependency Management

---

## 💡 Best Practices

### Production Build

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_MIMALLOC=ON \
  -DTHEMIS_ENABLE_GRPC=ON \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_BUILD_BENCHMARKS=OFF
```

### Development Build

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_BUILD_BENCHMARKS=ON \
  -DTHEMIS_ENABLE_ASAN=ON
```

### High-Performance Build

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_MIMALLOC=ON \
  -DTHEMIS_ENABLE_HUGE_PAGES=ON \
  -DTHEMIS_ENABLE_RCU_INDEX=ON \
  -DTHEMIS_ENABLE_LIRS_CACHE=ON
```

### Embedded/IoT Build

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DTHEMIS_EMBEDDED=ON \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_BUILD_BENCHMARKS=OFF
```

---

## 📖 Siehe auch

- [Bibliotheken-Übersicht](BIBLIOTHEKEN_UBERSICHT.md) - Alle Dependencies mit Vendor-Links
- [Deployment Strategy](deployment_strategy.md) - Build & Deployment Prozesse
- [vcpkg Offline Strategy](VCPKG_OFFLINE_STRATEGY.md) - Dependency Management
- [Docker Deployment](DOCKER_DEPLOYMENT.md) - Container Deployment
- [README](README.md) - Deployment Dokumentations-Übersicht

---

## 📋 Option Index

Schnellzugriff auf häufig genutzte Optionen:

**Build-Typen:**
- `THEMIS_BUILD_TESTS` - Unit Tests
- `THEMIS_BUILD_BENCHMARKS` - Performance Tests
- `THEMIS_EMBEDDED` - Lightweight Build
- `THEMIS_STATIC_BUILD` - Static Linking
- `THEMIS_QNAP_BUILD` - QNAP NAS

**Performance:**
- `THEMIS_ENABLE_MIMALLOC` - Fast Allocator (+10-20%)
- `THEMIS_ENABLE_HUGE_PAGES` - Large Pages (+15-30%)
- `THEMIS_ENABLE_RCU_INDEX` - Lock-Free Reads (+200-500%)
- `THEMIS_ENABLE_LIRS_CACHE` - Better Cache (+30-40%)

**Features:**
- `THEMIS_ENABLE_LLM` - LLM Integration (llama.cpp)
- `THEMIS_ENABLE_GPU` - GPU Acceleration (FAISS)
- `THEMIS_ENABLE_CUDA` - NVIDIA CUDA
- `THEMIS_ENABLE_GRPC` - gRPC Protocol

**Protokolle:**
- `THEMIS_ENABLE_HTTP2` - HTTP/2
- `THEMIS_ENABLE_HTTP3` - HTTP/3 (QUIC)
- `THEMIS_ENABLE_WEBSOCKET` - WebSocket
- `THEMIS_ENABLE_MQTT` - MQTT (IoT)
- `THEMIS_ENABLE_POSTGRES_WIRE` - PostgreSQL Wire Protocol

**Editionen:**
- `THEMIS_EDITION` - COMMUNITY | ENTERPRISE | HYPERSCALER
- `THEMIS_ENTERPRISE` - Enterprise Bundle

---

**Letzte Aktualisierung:** 26. Dezember 2025  
**Version:** v1.3.1
