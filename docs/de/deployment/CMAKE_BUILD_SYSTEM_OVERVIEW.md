# CMake Build System Architektur

**Stand:** 23. April 2026  
**Version:** v1.4.0  
**Kategorie:** 🏗️ Build System  
**Status:** Production-Ready

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Architekturdiagramm](#-architekturdiagramm)
- [Schnellnavigation](#-schnellnavigation)
- [Edition Selection Guide](#-edition-selection-guide)
- [3-Tier-System Details](#-3-tier-system-details)
- [Feature-System](#-feature-system)
- [Validierung & Constraints](#-validierung--constraints)
- [Build-Modi](#-build-modi)
- [Best Practices](#-best-practices)

---

## 🎯 Übersicht

ThemisDB nutzt ein modernes **3-Tier CMake Build System**, das Flexibilität mit Sicherheit kombiniert:

### Kernprinzipien

✅ **Platform Detection**: Automatische Erkennung von OS, CPU-Architektur, Cross-Compile  
✅ **Edition System**: Compile-Time Edition Selection (MINIMAL, COMMUNITY, ENTERPRISE, HYPERSCALER)  
✅ **Feature System**: Granulare Opt-In Features (60+ CMake-Optionen)  
✅ **Validation Layer**: Constraint-Checks für Konsistenz und Kompatibilität  
✅ **Cross-Compilation**: Volle Unterstützung für Windows, Linux, ARM, Docker

### Vorteile

- 🔒 **Type-Safe:** Compile-Time Edition Enforcement
- 🎛️ **Granular:** Jedes Feature einzeln steuerbar
- 📦 **Minimal:** Nur benötigte Features werden kompiliert
- 🚀 **Optimiert:** Edition-spezifische Optimierungen
- 🌍 **Cross-Platform:** Windows, Linux, macOS, ARM

---

## 🏗️ Architekturdiagramm

```
┌─────────────────────────────────────────────────────────────────┐
│                     CMake Build System v1.4.0                   │
└─────────────────────────────────────────────────────────────────┘

                            ┌──────────────┐
                            │   Platform   │
                            │  Detection   │
                            └──────┬───────┘
                                   │
                    ┌──────────────┴──────────────┐
                    │                             │
              ┌─────▼─────┐                 ┌─────▼─────┐
              │    OS     │                 │    CPU    │
              │ Windows   │                 │  x86_64   │
              │  Linux    │                 │  ARM64    │
              │  macOS    │                 │  ARMv7    │
              └─────┬─────┘                 └─────┬─────┘
                    │                             │
                    └──────────────┬──────────────┘
                                   │
                            ┌──────▼───────┐
                            │   Edition    │
                            │   System     │
                            └──────┬───────┘
                                   │
        ┌──────────────────────────┼──────────────────────────┐
        │                          │                          │
   ┌────▼─────┐            ┌───────▼────────┐        ┌───────▼──────┐
   │ MINIMAL  │            │   COMMUNITY    │        │  ENTERPRISE  │
   │ 1 node   │            │   5 nodes      │        │  100 nodes   │
   │ 0 GB GPU │            │   16 GB GPU (1× T4)    │        │  320 GB GPU (4× A100 80G)  │
   │ Optional │            │   Optional     │        │  Required*   │
   │ License  │            │   License      │        │  License     │
   └────┬─────┘            └───────┬────────┘        └───────┬──────┘
        │                          │                          │
        └──────────────────────────┼──────────────────────────┘
                                   │                   ┌───────▼──────┐
                                   │                   │ HYPERSCALER  │
                                   │                   │   ∞ nodes    │
                                   │                   │   ∞ GB GPU   │
                                   │                   │  Mandatory   │
                                   │                   │   License    │
                                   │                   └───────┬──────┘
                                   │                           │
                            ┌──────▼───────────────────────────┘
                            │   Feature System                 │
                            │   (60+ CMake Options)            │
                            └──────┬───────────────────────────┘
                                   │
        ┌──────────────────────────┼──────────────────────────┐
        │                          │                          │
   ┌────▼─────┐            ┌───────▼────────┐        ┌───────▼──────┐
   │ Network  │            │   Hardware     │        │  Performance │
   │ Protocols│            │  Acceleration  │        │  Features    │
   │          │            │                │        │              │
   │ - gRPC   │            │ - GPU (CUDA)   │        │ - mimalloc   │
   │ - HTTP/2 │            │ - Vulkan       │        │ - Huge Pages │
   │ - HTTP/3 │            │ - Metal        │        │ - RCU Index  │
   │ - MQTT   │            │ - OpenCL       │        │ - LIRS Cache │
   └────┬─────┘            └───────┬────────┘        └───────┬──────┘
        │                          │                          │
        └──────────────────────────┼──────────────────────────┘
                                   │
                            ┌──────▼───────┐
                            │  Validation  │
                            │    Layer     │
                            └──────┬───────┘
                                   │
        ┌──────────────────────────┼──────────────────────────┐
        │                          │                          │
   ┌────▼─────┐            ┌───────▼────────┐        ┌───────▼──────┐
   │ License  │            │  Constraints   │        │ Compatibility│
   │  Check   │            │     Check      │        │    Check     │
   │          │            │                │        │              │
   │ ENTER... │            │ GPU + Edition  │        │ OS + Feature │
   │ HYPERS...│            │ Nodes + Editon │        │ ARM + AVX2   │
   └────┬─────┘            └───────┬────────┘        └───────┬──────┘
        │                          │                          │
        └──────────────────────────┼──────────────────────────┘
                                   │
                            ┌──────▼───────┐
                            │    Build     │
                            │   themis_*   │
                            └──────────────┘

*ENTERPRISE: License required for Release builds only
```

---

## 🧭 Schnellnavigation

### Ich möchte...

**...einfach nur bauen?**  
→ [Build Guide](../guides/guides_build.md) - Quick Start für alle Plattformen

**...eine Edition auswählen?**  
→ [Edition Limits Matrix](EDITION_LIMITS_MATRIX.md) - Vollständiger Vergleich  
→ [Edition Selection Guide](#-edition-selection-guide) - Entscheidungshilfe

**...Lizenz-Fragen klären?**  
→ [License Requirements](LICENSE_REQUIREMENTS.md) - Wann ist Lizenz erforderlich?

**...Cross-Compilation?**  
→ [Cross-Compile Complete Guide](../guides/CROSS_COMPILE_COMPLETE.md) - Alle Szenarien

**...Build-Scripts nutzen?**  
→ [Build Scripts Reference](../guides/BUILD_SCRIPTS_REFERENCE.md) - Automatisierung

**...Features konfigurieren?**  
→ [Build Options Reference](BUILD_OPTIONEN_REFERENZ.md) - 60+ CMake-Optionen

**...Deployment planen?**  
→ [Deployment Strategy](deployment_strategy.md) - Strategie & Best Practices

---

## 🎯 Edition Selection Guide

Entscheidungsbaum für die richtige Edition:

```
                        ┌─────────────────┐
                        │  Use Case?      │
                        └────────┬────────┘
                                 │
           ┌─────────────────────┼─────────────────────┐
           │                     │                     │
    ┌──────▼──────┐       ┌──────▼──────┐      ┌──────▼──────┐
    │  Embedded   │       │ Development │      │ Production  │
    │  IoT, Edge  │       │ Testing     │      │ Clusters    │
    └──────┬──────┘       └──────┬──────┘      └──────┬──────┘
           │                     │                     │
           │                     │              ┌──────▼──────┐
           │                     │              │ How many    │
           │                     │              │   nodes?    │
           │                     │              └──────┬──────┘
           │                     │                     │
           │                     │        ┌────────────┼────────────┐
           │                     │        │            │            │
           │                     │   ┌────▼────┐  ┌────▼────┐  ┌────▼────┐
           │                     │   │  1-5    │  │  5-100  │  │  100+   │
           │                     │   │  nodes  │  │  nodes  │  │  nodes  │
           │                     │   └────┬────┘  └────┬────┘  └────┬────┘
           │                     │        │            │            │
      ┌────▼────┐           ┌────▼────┐  │       ┌────▼────┐  ┌────▼────┐
      │ MINIMAL │           │COMMUNITY│  │       │ENTERPRIS│  │HYPERSCA │
      │         │           │         │  │       │   E     │  │  LER    │
      │ 1 node  │           │ 5 nodes │◄─┘       │100 nodes│  │∞ nodes  │
      │ 0GB GPU │           │ 16GB GPU│          │320GB GPU│  │∞GB GPU  │
      │ License:│           │ License:│          │ License:│  │ License:│
      │Optional │           │Optional │          │Required*│  │Mandatory│
      └─────────┘           └─────────┘          └─────────┘  └─────────┘
```

*ENTERPRISE: License required for Release builds, Optional for Debug

### Use Cases pro Edition

#### MINIMAL Edition
**Ideal für:**
- 🔌 Embedded Systems (Raspberry Pi, IoT)
- 📱 Edge Computing
- 🏠 Single-Node Home Server
- 🔬 Research/Prototyping

**Limits:**
- Max 1 node
- Kein GPU Support
- Kein LLM Support
- Minimaler Footprint

#### COMMUNITY Edition (5 Nodes)
**Ideal für:**
- 💻 Entwicklung & Testing
- 🚀 Startups & kleine Teams
- 🎓 Universitäten & Forschung
- 🏢 Kleine Produktionsumgebungen (bis 5 Nodes)

**Limits:**
- Max **5 nodes**
- Max 16 GB GPU (1× T4) VRAM
- Keine Enterprise Plugins
- MIT Open Source Lizenz

#### ENTERPRISE Edition (100 Nodes)
**Ideal für:**
- 🏢 Mittlere bis große Unternehmen
- 🌐 Multi-Node Production Clusters
- 🔐 RBAC & Field-Encryption Anforderungen
- 💼 Commercial Support erforderlich

**Limits:**
- Max **100 nodes**
- Max 320 GB GPU (4× A100 80G) VRAM
- **License Required** (Release builds)
- License Optional (Debug builds)

#### HYPERSCALER Edition (Unlimited)
**Ideal für:**
- ☁️ Cloud Provider (AWS, Azure, GCP)
- 🏭 OEM Deployments
- 🌍 Global Scale (1000+ Nodes)
- 🎯 Custom Engineering Support

**Limits:**
- ✅ Unlimited nodes
- ✅ Unlimited GPU VRAM
- **License Mandatory** (all builds)

---

## 🏗️ 3-Tier-System Details

### Tier 1: Platform Detection

Automatische Erkennung zur Build-Zeit:

```cmake
# CMakeLists.txt
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(THEMIS_PLATFORM_WINDOWS TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(THEMIS_PLATFORM_LINUX TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(THEMIS_PLATFORM_MACOS TRUE)
endif()

# CPU Architecture Detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(THEMIS_ARCH_X64 TRUE)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(THEMIS_ARCH_ARM64 TRUE)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7")
    set(THEMIS_ARCH_ARMV7 TRUE)
endif()

# Cross-Compile Detection
if(CMAKE_CROSSCOMPILING)
    set(THEMIS_CROSS_COMPILE TRUE)
    message(STATUS "Cross-compiling: ${CMAKE_HOST_SYSTEM_NAME} → ${CMAKE_SYSTEM_NAME}")
endif()
```

**Unterstützte Plattformen:**
- Windows (x86_64, ARM64)
- Linux (x86_64, ARM64, ARMv7)
- macOS (x86_64, ARM64)
- Docker (multi-arch)

### Tier 2: Edition System

Edition-Auswahl via CMake Option:

```cmake
set(THEMIS_EDITION "COMMUNITY" CACHE STRING "ThemisDB Edition")
set_property(CACHE THEMIS_EDITION PROPERTY STRINGS 
    "MINIMAL" "COMMUNITY" "ENTERPRISE" "HYPERSCALER")

if(THEMIS_EDITION STREQUAL "MINIMAL")
    add_compile_definitions(THEMIS_EDITION_MINIMAL)
    set(THEMIS_MAX_NODES 1)
    set(THEMIS_MAX_GPU_VRAM 0)
elseif(THEMIS_EDITION STREQUAL "COMMUNITY")
    add_compile_definitions(THEMIS_EDITION_COMMUNITY)
    set(THEMIS_MAX_NODES 5)
    set(THEMIS_MAX_GPU_VRAM 24)  # GB
elseif(THEMIS_EDITION STREQUAL "ENTERPRISE")
    add_compile_definitions(THEMIS_EDITION_ENTERPRISE)
    set(THEMIS_MAX_NODES 100)
    set(THEMIS_MAX_GPU_VRAM 256)  # GB
elseif(THEMIS_EDITION STREQUAL "HYPERSCALER")
    add_compile_definitions(THEMIS_EDITION_HYPERSCALER)
    set(THEMIS_MAX_NODES 0)  # Unlimited
    set(THEMIS_MAX_GPU_VRAM 0)  # Unlimited
endif()
```

**Edition-Beispiele:**

```bash
# MINIMAL
cmake -B build -S . -DTHEMIS_EDITION=MINIMAL

# COMMUNITY (Standard)
cmake -B build -S . -DTHEMIS_EDITION=COMMUNITY

# ENTERPRISE (License required for Release)
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_LICENSE_FILE=/path/to/license.json

# HYPERSCALER (License mandatory)
cmake -B build -S . \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DTHEMIS_LICENSE_FILE=/path/to/hyperscaler-license.json
```

### Tier 3: Feature System

Granulare Feature-Optionen (60+ Options):

**Kategorien:**
1. **Network Protocols** (gRPC, HTTP/2, HTTP/3, MQTT, WebSocket)
2. **Hardware Acceleration** (GPU, CUDA, Vulkan, Metal, OpenCL)
3. **Performance** (mimalloc, Huge Pages, RCU, LIRS)
4. **Enterprise Features** (RBAC, HSM, Multi-Master, Field Encryption)
5. **Build Configuration** (Tests, Benchmarks, Static/Shared)

**Beispiel - Full Build:**

```bash
cmake -B build -S . \
  -DTHEMIS_EDITION=COMMUNITY \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_GRPC=ON \
  -DTHEMIS_ENABLE_HTTP2=ON \
  -DTHEMIS_ENABLE_MIMALLOC=ON \
  -DTHEMIS_ENABLE_HUGE_PAGES=ON
```

**Siehe:** [Build Options Reference](BUILD_OPTIONEN_REFERENZ.md)

---

## 🎛️ Feature-System

### Opt-In Design

Alle Features sind standardmäßig **deaktiviert** (außer Kern-Features):

```cmake
# Default: OFF
option(THEMIS_ENABLE_LLM "Enable LLM Support" OFF)
option(THEMIS_ENABLE_GPU "Enable GPU Acceleration" OFF)
option(THEMIS_ENABLE_HTTP2 "Enable HTTP/2" OFF)
option(THEMIS_ENABLE_MQTT "Enable MQTT Protocol" OFF)

# Default: ON (Kern-Features)
option(THEMIS_BUILD_TESTS "Build Unit Tests" ON)
option(THEMIS_ENABLE_TRACING "Enable OpenTelemetry" ON)
option(THEMIS_ENABLE_GRPC "Enable gRPC" ON)
```

### Feature-Bundles

Voreingestellte Kombinationen:

```bash
# Embedded/IoT Bundle
-DTHEMIS_EMBEDDED=ON
# Deaktiviert: TBB, Arrow, OpenTelemetry, GPU

# High-Performance Bundle
-DTHEMIS_ENABLE_MIMALLOC=ON
-DTHEMIS_ENABLE_HUGE_PAGES=ON
-DTHEMIS_ENABLE_RCU_INDEX=ON
-DTHEMIS_ENABLE_LIRS_CACHE=ON

# Full LLM Bundle
-DTHEMIS_ENABLE_LLM=ON
-DTHEMIS_ENABLE_GPU=ON
-DTHEMIS_ENABLE_CUDA=ON
```

---

## ✅ Validierung & Constraints

### License Validation

Automatische Prüfung zur Build-Zeit:

```cmake
# ENTERPRISE Release builds require license
if(THEMIS_EDITION STREQUAL "ENTERPRISE" AND CMAKE_BUILD_TYPE STREQUAL "Release")
    if(NOT DEFINED THEMIS_LICENSE_FILE)
        message(FATAL_ERROR 
            "ENTERPRISE Release builds require license!\n"
            "Usage: -DTHEMIS_LICENSE_FILE=/path/to/license.json\n"
            "Or build in Debug mode: -DCMAKE_BUILD_TYPE=Debug")
    endif()
endif()

# HYPERSCALER always requires license
if(THEMIS_EDITION STREQUAL "HYPERSCALER")
    if(NOT DEFINED THEMIS_LICENSE_FILE)
        message(FATAL_ERROR 
            "HYPERSCALER Edition requires license for all builds!\n"
            "Usage: -DTHEMIS_LICENSE_FILE=/path/to/license.json")
    endif()
endif()
```

### Compatibility Checks

Automatische Plattform-Feature-Validierung:

```cmake
# AVX2 requires x86_64
if(THEMIS_ENABLE_AVX2 AND NOT THEMIS_ARCH_X64)
    message(FATAL_ERROR "AVX2 requires x86_64 architecture")
endif()

# CUDA requires Linux/Windows
if(THEMIS_ENABLE_CUDA AND THEMIS_PLATFORM_MACOS)
    message(WARNING "CUDA not supported on macOS, using Metal instead")
    set(THEMIS_ENABLE_METAL ON)
endif()

# ARM + GPU constraints
if(THEMIS_ARCH_ARM64 AND THEMIS_ENABLE_GPU)
    message(STATUS "ARM GPU: Using Mali/Adreno backend")
endif()
```

### Constraint Checks

Edition-spezifische Limits:

```cmake
# COMMUNITY: Max 5 nodes
if(THEMIS_EDITION STREQUAL "COMMUNITY")
    if(THEMIS_SHARD_NODES GREATER 5)
        message(FATAL_ERROR 
            "COMMUNITY Edition: Max 5 nodes (requested: ${THEMIS_SHARD_NODES})\n"
            "Upgrade to ENTERPRISE Edition for 100 nodes")
    endif()
endif()

# MINIMAL: No GPU
if(THEMIS_EDITION STREQUAL "MINIMAL" AND THEMIS_ENABLE_GPU)
    message(FATAL_ERROR 
        "MINIMAL Edition does not support GPU\n"
        "Use COMMUNITY or higher")
endif()
```

---

## 🔧 Build-Modi

### Debug Build

Entwicklung & Testing:

```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Debug \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_ENABLE_ASAN=ON
```

**Features:**
- Debug-Symbole
- Assertions aktiv
- AddressSanitizer (optional)
- Keine Optimierung

### Release Build

Production Deployment:

```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_ENABLE_LLM=ON
```

**Features:**
- Optimierungen (O3)
- LTO (Link-Time Optimization)
- Keine Debug-Symbole
- Assertions deaktiviert

### RelWithDebInfo

Production mit Debug-Info:

```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

**Features:**
- Optimierungen (O2)
- Debug-Symbole
- Profiling möglich

### MinSizeRel

Embedded/IoT:

```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DTHEMIS_EMBEDDED=ON
```

**Features:**
- Minimale Binärgröße (Os)
- Keine Tests/Benchmarks
- Reduzierte Features

---

## 💡 Best Practices

### Production Build

```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DTHEMIS_LICENSE_FILE=/path/to/license.json \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_MIMALLOC=ON \
  -DTHEMIS_ENABLE_GRPC=ON \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_BUILD_BENCHMARKS=OFF

cmake --build build --parallel 8
```

### Development Build

```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Debug \
  -DTHEMIS_EDITION=COMMUNITY \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_BUILD_BENCHMARKS=ON \
  -DTHEMIS_ENABLE_ASAN=ON

cmake --build build --parallel 8
ctest --test-dir build --parallel 8
```

### High-Performance Build

```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_MIMALLOC=ON \
  -DTHEMIS_ENABLE_HUGE_PAGES=ON \
  -DTHEMIS_ENABLE_RCU_INDEX=ON \
  -DTHEMIS_ENABLE_LIRS_CACHE=ON

cmake --build build --parallel 8
```

### Embedded/IoT Build

```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DTHEMIS_EDITION=MINIMAL \
  -DTHEMIS_EMBEDDED=ON \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_BUILD_BENCHMARKS=OFF

cmake --build build --parallel 4
```

---

## 🔗 Verwandte Dokumentation

### Build & Deployment
- [Build Guide](../guides/guides_build.md) - Quick Start
- [Build Options Reference](BUILD_OPTIONEN_REFERENZ.md) - Alle CMake-Optionen
- [Deployment Strategy](deployment_strategy.md) - Strategie & Workflows
- [Build Scripts Reference](../guides/BUILD_SCRIPTS_REFERENCE.md) - Automatisierung

### Editions & Licensing
- [Edition Limits Matrix](EDITION_LIMITS_MATRIX.md) - Vergleichstabelle
- [License Requirements](LICENSE_REQUIREMENTS.md) - Lizenz-Enforcement
- [Edition Control Mechanisms](EDITION_CONTROL_MECHANISMS.md) - Technische Details

### Cross-Compilation
- [Cross-Compile Complete Guide](../guides/CROSS_COMPILE_COMPLETE.md) - Alle Szenarien
- [ARM Deployment](deployment_arm_build.md) - Raspberry Pi & IoT

### Performance & Features
- [LLM Complete Setup](../guides/LLM_COMPLETE_SETUP_GUIDE.md) - LLM Integration
- [GPU Support](../DOCKER_GPU_SUPPORT.md) - GPU Configuration

---

## 📖 Zusammenfassung

**3-Tier-System:**
1. **Platform Detection** → OS, CPU, Cross-Compile
2. **Edition System** → MINIMAL, COMMUNITY, ENTERPRISE, HYPERSCALER
3. **Feature System** → 60+ granulare CMake-Optionen

**Validation Layer:**
- License Checks (ENTERPRISE, HYPERSCALER)
- Compatibility Checks (Platform + Feature)
- Constraint Checks (Edition Limits)

**Build Modi:**
- Debug, Release, RelWithDebInfo, MinSizeRel

**Siehe auch:** [Edition Limits Matrix](EDITION_LIMITS_MATRIX.md) für vollständige Limits-Tabelle.

---

**Letzte Aktualisierung:** 23. April 2026  
**Version:** v1.4.0
