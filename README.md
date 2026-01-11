<div align="center">
  <h1>🗄️ ThemisDB</h1>
  <p><strong>High-Performance Multi-Model Database with Native AI/LLM Integration</strong></p>
  
  > *"ThemisDB keeps its own llamas."* – Run LLaMA, Mistral, Phi-3 directly in your database, no API calls needed.
  
  [![CI](https://github.com/makr-code/ThemisDB/actions/workflows/ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/ci.yml)
  [![Code Quality](https://github.com/makr-code/ThemisDB/actions/workflows/code-quality.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/code-quality.yml)
  [![Coverage](https://img.shields.io/badge/coverage-view%20report-brightgreen)](https://makr-code.github.io/ThemisDB/coverage/)
  [![Version](https://img.shields.io/badge/version-1.4.0--alpha-blue)](https://github.com/makr-code/ThemisDB/releases/tag/v1.4.0-alpha)
  [![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
</div>

---

## 🎉 What's New

### 🚀 v1.4.0-alpha - Advanced LLM Features (2026-01-05)

#### 🧠 Advanced LLM Capabilities
- 📝 **Grammar-Constrained Generation** - EBNF/GBNF support for guaranteed valid JSON/XML/CSV outputs (95-99% reliability vs 60-70%)
  - Built-in grammars: JSON, XML, CSV, ReAct Agent
  - Thread-safe grammar cache with LRU eviction
  - Zero post-processing required
- 🔭 **RoPE Scaling** - Extended context window from 4K → 32K tokens (8x increase)
  - Linear, NTK-aware, YaRN scaling methods
  - Process entire research papers and codebases
- 🖼️ **Vision Support** - Multi-modal LLMs with CLIP-based image encoding
  - LLaVA integration for image analysis
  - Single and multiple image support
  - Thread-safe VisionEncoder class
- ⚡ **Flash Attention** - CUDA kernels for 15-25% speedup, 30% memory reduction
  - Optimized attention mechanism
  - Backward pass for training support
  - Multi-compute capability support
- 🎯 **Speculative Decoding** - 2-3x faster inference with draft+target models
- 🔄 **Continuous Batching** - 2x+ throughput with dynamic request batching

#### 🏢 Enterprise Enhancements
- 🔥 **Hot Spare Management** - Automatic failover with health monitoring
- 📊 **Enhanced Prometheus Metrics** - LLM inference, cache performance, response tracking
- 🔄 **WAL Replication via gRPC** - Distributed inter-shard replication
- 🎮 **Multi-GPU LoRA Support** - Distributed LoRA adapters across GPUs
- 🐘 **PostgreSQL Protocol** - COPY, prepared statements, transaction support

#### 📊 Quality & Testing
- 31 new test suites with comprehensive coverage
- 11 new performance benchmarks
- 17 new documentation guides
- 938 files changed (+113,762 lines, -45,154 lines)

**📚 Quick Links:**
- [Grammar-Constrained Generation](docs/en/llm/GRAMMAR_CONSTRAINED_GENERATION.md)
- [RoPE Scaling Guide](docs/en/llm/ROPE_SCALING_IMPLEMENTATION.md)
- [Vision Support Quick Start](docs/en/llm/VISION_SUPPORT_QUICK_START.md)
- [Flash Attention Implementation](docs/en/llm/FLASH_ATTENTION_IMPLEMENTATION.md)
- [Complete Changelog](CHANGELOG.md#v140-alpha)

---

### �️ RAID Sharding Deadlock Hotfix (2026-01-04)
- 🔧 **Critical Fix** - Resolved server hang in RAID cluster mode at "Adaptive Index Manager initialized"
- 🎯 **Root Cause** - AdaptiveIndexManager MVCC coordination across 2 CFs before Sharding Manager initialization
- ✅ **Solution** - Conditional Column Family opening when `THEMIS_ENABLE_SHARDING=true` detected
- 🔧 **Port Mapping Fix** - Corrected docker-compose HTTP mappings from `808X:8080` to `808X:8765`
- 🧪 **RAID Testing** - 2-hour endurance test suite with monitoring dashboard
- 📊 **Verification** - All 9 RAID shards (RAID 0/1/5) operational with 0% error rate
- 📦 **Build Optimization** - Docker build context reduced from 3GB to 85MB (97% reduction)

**Files Modified:** `src/storage/rocksdb_wrapper.cpp`, `src/server/http_server.cpp`, `docker/compose/docker-compose-sharding.yml`, `Dockerfile.themis-server`, `.dockerignore`  
**Tools Added:** `scripts/raid_endurance_test.py`, `scripts/monitor_raid_test.ps1`

### 🔒 Security Improvements Summary (v1.3.0 - v1.3.4)
- 🛡️ **Critical Security Fixes** - Addressed 7 critical and 8 medium severity issues in RocksDB wrapper (100% segfault risk elimination)
- 🐳 **Docker Security** - Upgraded to Ubuntu 24.04 LTS with 80%+ CVE reduction
- 🔄 **Update Checker Security** - Secure token handling, HTTPS-only, thread-safe implementation
- 🔐 **Binary Authenticity** - Cryptographic manifest signing architecture (RSA-4096, SHA-256)
- 🔧 **Memory Safety** - Fixed use-after-free vulnerabilities, memory leaks, and resource management
- ✅ **Verification** - CodeQL passed, comprehensive audit reports, all critical issues resolved

**Quick Links:** [Security Summary (EN)](docs/SECURITY_WORK_SUMMARY_v1.3.4_EN.md) | [Sicherheitszusammenfassung (DE)](docs/de/releases/SECURITY_WORK_SUMMARY_V1.3.4.md) | [Security Audit](docs/ROCKSDB_WRAPPER_AUDIT_REPORT.md) | [Security Policy](SECURITY.md)

### 🎙️ Voice Assistant Integration (2025-12-30) - Enterprise Feature
- 🗣️ **Natural Language Voice Interaction** - Similar to Alexa/Siri, powered by Whisper.cpp + Piper TTS + llama.cpp
- 📞 **Phone Call Recording** - Automatic transcription and secure storage with revision control
- 📝 **Meeting Protocol Generation** - AI-powered meeting minutes, key points, and action items
- 🎯 **Speaker Diarization** - Identify and label different speakers in recordings
- 🔒 **Revision-Safe Storage** - All recordings stored securely in ThemisDB with full audit trails
- 🌐 **Multi-Language Support** - 100+ languages for STT, multiple voices for TTS

**Quick Links:** [Voice Assistant Guide (EN)](docs/en/features/voice_assistant_guide.md) | [Sprachassistent (DE)](docs/de/features/sprachassistent_anleitung.md)

### 🌿 Git Flow Branching Strategy (2025-12-30)
- 🎯 **Git Flow Implementation** - `main` as production release branch, `develop` as integration branch
- 📚 **Comprehensive Documentation** - Bilingual guides (DE/EN), visual workflows, quick reference cards
- 🚀 **Branch-Based CI/CD** - Fast builds on `develop` (~5-10 min), production builds on `main` (~30-40 min)
- 🔄 **Migration Guide** - Step-by-step transition for existing contributors
- 🛡️ **Branch Protection Setup** - GitHub configuration with automated scripts

**Quick Links:** [Branching Strategy (DE)](BRANCHING_STRATEGY.md) | [Branching Strategy (EN)](BRANCHING_STRATEGY_EN.md) | [Quick Reference](BRANCHING_QUICK_REF.md) | [Documentation Hub](BRANCHING_DOCS_INDEX.md)

### v1.3.3 - Network Protocol Enhancements (2025-12-21)
- 🌐 **HTTP/2 with Server Push** for CDC/Changefeed with ~0ms latency
- 📡 **WebSocket support** with CDC streaming for real-time communication
- 📬 **MQTT broker** with WebSocket transport and monitoring
- ⚡ **HTTP/3 base implementation** with QUIC protocol
- 🐘 **PostgreSQL Wire Protocol** for BI tool compatibility
- 🔌 **MCP Server** (Model Context Protocol) support

### v1.3.2 - Image Analysis AI Plugin (2025-12-21)
- 🖼️ **Image Analysis AI Plugin Architecture** running parallel with LLM
- 🔧 **Multi-backend support:** llama.cpp Vision, ONNX Runtime, OpenCV DNN, OpenVINO, ncnn
- 🛠️ Plugin interfaces: `IImageAnalysisBackend`, `ImageAnalysisManager`
- 🧪 Comprehensive unit tests (15+ test cases) and benchmarks

### v1.3.1 - Third-Party Attribution (2025-12-20)
- 📄 Added `ATTRIBUTIONS.md` documenting 15+ core dependencies
- 🏆 Documented ThemisDB's **12 unique innovations**
- ✅ Clear attribution for all major dependencies

### v1.3.0 - LLM Integration (2025-12-17)

<details>
<summary><b>🧠 Native LLM Integration with llama.cpp (Optional)</b></summary>

**"ThemisDB keeps its own llamas."** – Run AI/LLM workloads directly in your database - no external API costs!

> [!NOTE]
> LLM integration is an **optional feature** that requires:
> -# LLM Features (When Enabled)

| Feature | Description | Status |
|---------|-------------|--------|
| 🧠 **Embedded LLM Engine** | llama.cpp integration for LLaMA/Mistral/Phi-3 (1B-70B params) | ✅ |
| 📝 **Grammar Constraints** | EBNF/GBNF for guaranteed valid JSON/XML/CSV outputs | ✅ v1.4.0-alpha |
| 🔭 **RoPE Scaling** | Extended context window 4K → 32K tokens (8x increase) | ✅ v1.4.0-alpha |
| 🖼️ **Vision Support** | Multi-modal LLMs with CLIP image encoding (LLaVA) | ✅ v1.4.0-alpha |
| ⚡ **Flash Attention** | CUDA kernels: 15-25% speedup, 30% memory reduction | ✅ v1.4.0-alpha |
| 🎯 **Speculative Decoding** | 2-3x faster inference with draft+target models | ✅ v1.4.0-alpha |
| 🔄 **Continuous Batching** | 2x+ throughput with dynamic request batching | ✅ v1.4.0-alpha |
| 🎙️ **Voice Assistant** | STT/TTS/LLM for phone calls, meetings, voice commands (Enterprise) | ✅ |
| 🖼️ **Image Analysis AI** | Multi-backend plugins (llama.cpp Vision, ONNX CLIP, OpenCV DNN) | ✅ |
| ⚡ **GPU Acceleration** | NVIDIA CUDA support with significant speedup | ✅ |
| 💾 **PagedAttention** | Advanced memory management | ✅ |
| 🔧 **Quantization** | Q4_K_M, Q5_K_M, Q8_0 for efficient memory usage | ✅ |
| 📊 **Monitoring** | Grafana dashboards with metrics and alerts | ✅ |
| 🔌 **Plugin Architecture** | Extensible LLM and image analysis backends | ✅ |
| 🌐 **Distributed RPC** | Inter-shard communication for distributed LLM ops | ✅ |

#### Performance Highlights

> [!TIP]
> GPU acceleration provides significant speedup over CPU with PagedAttention memory savings.

- ⚡ **Significant speedup** with GPU acceleration vs CPU
- 💾 **Memory savings** with PagedAttention and prefix caching
- 🚀 **Kernel fusion** for additional performance gains
- ✅ **Comprehensive test coverage** with unit tests

**📚 Documentation:**
- [🧠 LLM Complete Setup Guide (DE)](docs/de/guides/LLM_COMPLETE_SETUP_GUIDE.md) - Vollständiger Guide für LLM-Setup und Inferencing
- [🎯 Overview](docs/architecture/ARCHITECTURE_OVERVIEW.md) - System architecture and design

ThemisDB is a **production-ready multi-model database** that combines relational, graph, vector, and document models in a single system with full ACID transaction support. Built on RocksDB with advanced security and compliance features.

### 📦 Editions

<details open>
<summary><b>Available Editions</b></summary>

| Edition | License | Features | Use Case |
|---------|---------|----------|----------|
| 🔹 **Minimal** | Open Source (MIT) | Core database only - no LLM, GPU, sharding, advanced protocols | Embedded systems, IoT, edge devices, fast builds |
| 🆓 **Community** | Open Source (MIT) | Full-featured single-node database with all core capabilities | Development, startups, single-server deployments |
| 🔒 **Enterprise** | Commercial | + Horizontal scaling, advanced analytics, HA/replication, and more | Large-scale production deployments |

**[→ See Minimal Edition Details](docs/MINIMAL_EDITION.md)** | **[→ See Enterprise Edition Details](ENTERPRISE.md)**

</details>

---

## ✨ Features

### 🔑 Core Features

<details open>
<summary><b>Database Capabilities</b></summary>

| Feature | Description | Community | Enterprise |
|---------|-------------|:---------:|:----------:|
| ## 🚀 Quick Start

### 🐳 Docker (Recommended)

**Option 1: Standard Docker**

```bash
# Pull and run the latest version
docker pull themisdb/themisdb:latest

# Run with Docker
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 18765:18765 \
  -p 4318:4318 \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Or use Docker Compose
docker compose up -d

# Verify installation
curl http://localhost:8080/health
```

**Option 2: LoRA Framework Development Environment** ⭐ **New!**

Complete development environment with ThemisDB, Prometheus, and Grafana in < 5 minutes:

```bash
# Navigate to docker directory
cd docker

# Start all services
./scripts/start.sh

# Access:
# - ThemisDB:   http://localhost:8529
# - Prometheus: http://localhost:9091
# - Grafana:    http://localhost:3000 (admin/admin)
```

**Full documentation**: See [docker/README.md](docker/README.md) | [LoRA Build Guide](LORA_BUILD_GUIDE.md)

> [!TIP]
> Use Docker Compose for production deployments with proper configuration.

### 📡 Default Ports

| Port | Protocol | Description |
|------|----------|-------------|
| `8080` | HTTP/1.1 | REST API, GraphQL |
| `18765` | Binary | Wire Protocol, gRPC |
| `4318` | HTTP | OpenTelemetry/Prometheus |

> [!NOTE]
> **Complete Port Reference:** See [v1.3.0+)
- ✅ **Image Analysis** - Multi-backend AI plugins (v1.3.0+)
- ✅ **GNN Embeddings** - Graph Neural Network support

</details>

<details>
<summary><b>🌐 Modern Protocols</b></summary>

| Protocol | Status | Description |
|----------|--------|-------------|
| HTTP/1.1 | ✅ | REST API, GraphQL |
| HTTP/2 | ✅ | Server Push for CDC |
| HTTP/3 | 🚧 | QUIC (experimental) |
| WebSocket | ✅ | Bidirectional streaming |
| gRPC | ✅ | Binary RPC |
| MQTT | ✅ | IoT messaging |
| PostgreSQL Wire | ✅ | BI tool compatibility |
| MCP | ✅ | Model Context Protocol |
| SSE | ✅ | Server-Sent Events |

</details>

<details>
<summary><b>📚 Transparency & Attribution</b></summary>

ThemisDB is built on proven open-source foundations with clear attribution:

- ✅ **Transparent Attribution** - Clear documentation of all dependencies
- ✅ **Innovation Documentation** - ThemisDB's unique contributions vs third-party features
- ✅ **License Compliance** - Full license information for all components

**[→ See Complete Attribution](ATTRIBUTIONS.md)**

</details>
**Key Features:**

- 🔒 **ACID Transactions** - Full snapshot isolation with MVCC
- 🔍 **Multi-Model** - Relational, Graph, Vector, Document in one database
- 🚀 **High Performance** - 45K writes/s, 120K reads/s, GPU-accelerated vector search
- 🛡️ **Security** - TLS 1.3, RBAC, field-level encryption, audit logging (Enterprise: HSM integration)
- 📊 **Analytics** - Time-series, aggregations (Enterprise: OLAP, CEP, materialized views)
- 🌐 **Distribution** - Single-node optimized (Enterprise: horizontal sharding, replication, Kubernetes)
- 🧠 **AI-Ready** - Hybrid search (RAG), embedding cache, FAISS integration, **optional LLM engine with llama.cpp** (v1.3.0+), **image analysis AI plugins** (v1.3.0+), **voice assistant with STT/TTS** (v1.4.0+, Enterprise)
- 🌐 **Modern Protocols** - HTTP/1.1, GraphQL, SSE, gRPC (v1.3.0), **HTTP/2 with Server Push** ✅, **WebSocket** ✅, **MQTT** ✅, **HTTP/3** 🚧, **PostgreSQL Wire** ✅, **MCP** ✅
- 📚 **Transparent Attribution** - Clear documentation of third-party dependencies vs ThemisDB innovations (see [ATTRIBUTIONS.md](ATTRIBUTIONS.md))
- 🖼️ **Image Analysis** - Multi-backend AI plugin architecture (llama.cpp Vision, ONNX CLIP, OpenCV DNN)

---

## Quick Start

### Docker (Recommended)

```bash
# Pull and run the latest stable version
docker pull themisdb/themisdb:latest
docker run -d \
  -p 8080:8080 \
  -p 18765:18765 \
  -p 4318:4318 \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Or use nightly builds (latest development version)
docker pull themisdb/server:nightly
docker run -d -p 8080:8080 -p 18765:18765 -v themis_data:/data themisdb/server:nightly

# Or use Docker Compose
docker compose up -d

# Check health
curl http://localhost:8080/health
```

**Default Ports:**
- `8080` - HTTP/REST API, GraphQL
- `18765` - Binary Wire Protocol, gRPC
- `4318` - OpenTelemetry/Prometheus metrics

**📖 Complete Port Reference:** See [docs/deployment/PORT_REFERENCE.md](docs/deployment/PORT_REFERENCE.md) for all ports including optional protocols (MQTT, PostgreSQL Wire, MCP).

> **Release Builds**: Automated release builds with edition support (community/enterprise/hyperscaler) are available on DockerHub. Each release includes automatically generated changelogs and enterprise security artifacts (SBOM, signatures, SLSA provenance). See [Release Build Documentation](docs/de/deployment/deployment_nightly_builds.md) and [Release Changelogs](release-changelogs/) for details.

### From Source

```bash
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Setup and build (Linux/macOS)
./scripts/setup.sh
./scripts/build.sh

# Setup and build (Windows)
.\scripts\setup.ps1
.\scripts\build.ps1

# Start server
./build/themis_server --config config.yaml
```

> [!TIP]
> **Configuration:** See [`config/config.yaml`](config/config.yaml) for complete configuration reference with all available options, or check the [Configuration Guide](docs/en/guides/guides_configuration.md) | [Tuning Guide (DE)](docs/de/guides/CONFIGURATION_TUNING_GUIDE.md) for detailed tuning recommendations.

**Optional Protocol Support (Security: Opt-In by Default):**

```bash
# Enable HTTP/2 with Server Push (explicit opt-in for security)
cmake -B build -S . -DTHEMIS_ENABLE_HTTP2=ON

# Enable WebSocket with CDC (explicit opt-in for security)
cmake -B build -S . -DTHEMIS_ENABLE_WEBSOCKET=ON

# Enable MQTT broker (explicit opt-in for security)
cmake -B build -S . -DTHEMIS_ENABLE_MQTT=ON

# Enable PostgreSQL Wire Protocol (explicit opt-in for security)
cmake -B build -S . -DTHEMIS_ENABLE_POSTGRES_WIRE=ON

# Enable MCP for LLM integration (explicit opt-in for security)
cmake -B build -S . -DTHEMIS_ENABLE_MCP=ON

# Enable HTTP/3 (explicit opt-in for security)
cmake -B build -S . -DTHEMIS_ENABLE_HTTP3=ON

# Default build only includes HTTP/1.1, GraphQL, SSE, gRPC (minimal attack surface)
```

See [Protocol Documentation](docs/apis/) for details.

### Windows: Build mit LLM (llama.cpp) - Optional

```powershell
# OPTIONAL: Für LLM-Unterstützung - lokaler Clone von llama.cpp erforderlich
if (!(Test-Path "llama.cpp")) {
  git clone https://github.com/ggerganov/llama.cpp.git llama.cpp
}

# MSVC Release-Build mit LLM-Unterstützung
powershell -File scripts/build-themis-server-llm.ps1

# Sanity-Check
./build-msvc/bin/themis_server.exe --help
```

Hinweise:
- LLM-Unterstützung ist **optional** und erfordert `-DTHEMIS_ENABLE_LLM=ON` beim Build
- `llama.cpp/` liegt als lokaler Clone im Projekt-Root und ist per `.gitignore` und `.dockerignore` ausgeschlossen (wird nicht committed oder in Docker kopiert)
- Der Build-Skript setzt Visual Studio 2022 (`-G "Visual Studio 17 2022"`) und `-A x64`, bindet die vcpkg-Toolchain ein und behebt MSVC‑spezifische `char8_t`‑Fehler am `llama`‑Target

**[→ Comprehensive Build Documentation](docs/guides/guides_build_strategy.md)** | Build-Varianten, Plattformen, Troubleshooting

### Package Managers

**Linux (Debian/Ubuntu):**
```bash
wget https://github.com/makr-code/ThemisDB/releases/latest/download/themisdb_1.3.0-1_amd64.deb
sudo apt install ./themisdb_1.3.0-1_amd64.deb
sudo systemctl start themisdb
```

**macOS (Homebrew):**
```bash
brew install themisdb
brew services start themisdb
```

**Windows (Chocolatey):**
```powershell
choco install themisdb
```

---

## **Release & Publication Policy**

- **Public Editions:** Minimal, Community. Enterprise bleibt kommerziell und wird nicht im öffentlichen Repo geführt.
- **Nicht veröffentlichte Inhalte:** Werden über `.gitignore` und Release-Prozesse ausgeschlossen.
- **Branch-Scope:** `main` enthält nur Minimal/Community; `develop` enthält alle Komponenten (inkl. Enterprise, Benchmarks, Marketing-Assets).
- **Merge-Schutz:** `.gitattributes` erzwingt für sensible Pfade `merge=ours`, damit `develop → main` keine nicht-öffentlichen Inhalte überträgt.
- **Begründung:** Vermeidung von versehentlichen Veröffentlichungen vertraulicher oder nicht betriebsrelevanter Komponenten.

**Explizit ausgeschlossen (nicht Teil der öffentlichen Veröffentlichung):**
- **`llama.cpp/`**: Lokaler Clone für optionale LLM-Builds; nicht committen.
- **`internal/`**: Interne vertrauliche Dokumente und Artefakte.
- **Enterprise-Quellcode/Dokumente**: `src/enterprise/`, `include/enterprise/`, `plugins/enterprise/`, `docs/enterprise/*` (nur feature Docs bleiben öffentlich). 
- **WordPress-Assets**: `wordpress-plugin/`, `wordpress-theme/` (Marketing/Website-Integration, nicht server-kritisch).
- **Ephemere/experimentelle Apps**: `epServer/` (nicht für regulären Betrieb vorgesehen).
- **Build-/Artefakt-Verzeichnisse**: `build*/`, `out/`, `bin/`, `lib/`, `CMakeFiles/`, `site/`, Logs und temporäre Dateien.
- **Lokale Toolchains/Vendor-Artefakte**: `vcpkg/`, `vcpkg_installed/`, `packages/`, `downloads/`, `buildtrees/`.
- **Model-/Testdaten**: `models/*` (nur Platzhalter erlaubt), `data/`, RocksDB-Dateien (`*.db`, `*.sst`).

Hinweise zur Editionsgrenze:
- Community/Minimal enthalten nur komponentenrelevanten Code für den Betrieb (Server, Kernmodule, öffentlich dokumentierte Plugins, Tests, APIs, Docker/Helm für Deployment).
- Enterprise-spezifische Benchmarks/Analysen und proprietäre Plugins bleiben ausgeschlossen.

> Pflegehinweis: Siehe `.gitignore` im Projekt-Root für die vollständige, bindende Liste der Ausschlüsse. Änderungen an der Veröffentlichungsstrategie werden dort und im CHANGELOG dokumentiert.

---

## **Merge-Vorbereitung: develop → main**

- **Dokumentation aktualisiert:** README (Release-/Exclusions-Abschnitt), `.gitignore` erweitert, CHANGELOG ergänzt.
- **Ziel:** Sauberer Stand für Veröffentlichung der öffentlichen Editionen ohne vertrauliche Inhalte.
- **Validierung:** CI/CTest auf `build-msvc`/Release; optional Docker Nightly gegen Community-Edition.

> Nächste Schritte nach Merge: Release-Tag setzen, Artefakte bauen (Docker/DEB/RPM), Changelog veröffentlichen, Coverage/Docs aktualisieren.

---

## 5-Minute Tutorial

```bash
# 1. Check server health
curl http://localhost:8765/health

# 2. Create an entity
curl -X PUT http://localhost:8765/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Alice\",\"age\":30,\"city\":\"Berlin\"}"}'

# 3. Create an index
curl -X POST http://localhost:8765/index/create \
  -H "Content-Type: application/json" \
  -d '{"table":"users","column":"city"}'

# 4. Query by index
curl -X POST http://localhost:8765/query \
  -H "Content-Type: application/json" \
  -d '{"table":"users","predicates":[{"column":"city","value":"Berlin"}],"return":"entities"}'

# 5. View metrics
curl http://localhost:8765/metrics
```

---

## Architecture

ThemisDB uses a unified storage architecture with specialized projection layers:

```
┌─────────────────────────────────────────────────────────┐
│                   Query Layer (AQL)                     │
│  SQL-like • Graph Traversals • Vector Search • Analytics│
├─────────────────────────────────────────────────────────┤
│                 Projection Layers                        │
│  Secondary Indices • Graph Adjacency • HNSW Vector      │
├─────────────────────────────────────────────────────────┤
│              Canonical Storage (Base Entity)             │
│         RocksDB LSM-Tree • MVCC Transactions            │
└─────────────────────────────────────────────────────────┘
```

**Core Components:**
- **Storage Engine**: RocksDB TransactionDB with LSM-Tree
- **Transaction Manager**: MVCC with snapshot isolation
- **Query Engine**: Advanced Query Language (AQL) with graph/vector support
- **Index Manager**: Automatic maintenance of secondary, graph, and vector indexes
- **Security**: TLS 1.3, RBAC, field encryption, audit logging
- **Observability**: Prometheus metrics, OpenTelemetry tracing

**[→ Full Architecture Documentation](docs/architecture/ARCHITECTURE_OVERVIEW.md)**

---

## Core Features

### Multi-Model Database
- **Relational**: SQL-like queries with secondary indexes
- **Graph**: BFS, Dijkstra, A* traversals with path constraints
- **Vector**: HNSW and FAISS for similarity search (GPU-accelerated)
- **Document**: JSON storage with flexible schema
- **Time-Series**: Gorilla compression, continuous aggregates

### Transaction Support
- Full ACID guarantees with snapshot isolation
- Write-write conflict detection
- Atomic updates across all index types
- Session-based and direct API

### Advanced Analytics
- **CEP Engine**: Complex Event Processing with pattern matching
- **OLAP**: CUBE, ROLLUP, window functions
- **Time-Series**: Compression, retention policies, aggregates
- **Hybrid Search**: BM25 + vector for RAG workflows

### Enterprise Security
- TLS 1.3 with mTLS support
- Role-Based Access Control (RBAC)
- Field-level encryption
- Audit logging with SIEM integration
- Certificate pinning for HSM/TSA
- Secrets management (HashiCorp Vault)

### Distributed Capabilities
- Horizontal sharding with consistent hashing
- Leader-follower and multi-master replication
- RAID-like redundancy (MIRROR, STRIPE, PARITY)
- Kubernetes operator with CRDs
- Auto-rebalancing and cloud deployment

### GPU Acceleration (Optional)
- 10 backend options: CUDA, Vulkan, HIP, OpenCL, DirectX, OneAPI, ZLUDA
- 10-50x speedup for vector search
- Automatic platform detection and fallback

---

## Documentation

**Getting Started:**

**Core Concepts:**

**Features:**

**Operations:**

**Development:**

**Full Documentation:** [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
---

## 📚 Documentation

<details>
<summary><b>Getting Started</b></summary>

- 🚀 [Quick Start](#quick-start)
- 📖 [5-Minute Tutorial](#5-minute-tutorial)
- 🐳 [Docker Deployment](docs/deployment/DOCKER_DEPLOYMENT.md)
- 🔧 [Building from Source](docs/guides/guides_build_strategy.md)

</details>

<details>
<summary><b>Core Concepts</b></summary>

- 🏗️ [Architecture Overview](docs/architecture/ARCHITECTURE_OVERVIEW.md)
- 💾 [Multi-Model Design](docs/architecture/architecture_base_entity.md)
- 🔄 [Transaction Management](docs/features/features_transactions.md)
- 🔍 [AQL Query Language](docs/aql/aql_syntax.md)

</details>

<details>
<summary><b>Features</b></summary>

- 🎯 [Vector Search](docs/features/features_vector_ops.md)
- 🕸️ [Graph Operations](docs/features/features_graph.md)
- 📈 [Time-Series Engine](docs/features/features_time_series.md)
- 🔐 [Security & Compliance](docs/security/security_implementation.md)
- ⚡ [Feature Overview](docs/features/features_overview.md)

</details>

<details>
<summary><b>Operations</b></summary>

- ⚙️ [Configuration Guide](docs/en/guides/guides_configuration.md) | [Tuning Guide (DE)](docs/de/guides/CONFIGURATION_TUNING_GUIDE.md)
- 📊 [Monitoring & Metrics](docs/observability/observability_prometheus.md)
- 💾 [Backup & Recovery](docs/guides/guides_deployment.md#backup--recovery)
- ⚡ [Performance Tuning](docs/performance/performance_memory.md)

</details>

<details>
<summary><b>Development</b></summary>

- 🔨 [Build Guide](docs/guides/guides_build_strategy.md)
- 🌿 [Branching Strategy](BRANCHING_STRATEGY.md) | [EN](BRANCHING_STRATEGY_EN.md)
- 🤝 [Contributing](CONTRIBUTING.md)
- 📖 [API Reference](docs/api/api_reference.md)
- 📦 [Client SDKs](clients/README.md)

</details>

<details>
<summary><b>Enterprise & Strategy</b></summary>

- 🏛️ [CMS Strategy Paper (DE)](docs/de/governance/CMS_STRATEGY_PAPER.md) - ThemisDB für Content Management in Government und Enterprise
- 💼 [Enterprise Edition](ENTERPRISE.md) - Enterprise features and licensing
- 📊 [Governance](docs/de/governance/README.md) - Data governance and policies

</details>

> [!NOTE]
> **Full Documentation:** [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)

---

## 🗺️ Roadmap

### ✅ Completed (v1.0 - v1.3)

<details>
<summary><b>Production-Ready Features</b></summary>

- ✅ ACID transactions with MVCC
- ✅ Multi-model support (relational, graph, vector, document)
- ✅ Horizontal sharding and replication
- ✅ GPU acceleration (10 backends)
- ✅ Enterprise security features
- ✅ Client SDKs (7 languages)
- ✅ Kubernetes operator
- ✅ Native LLM integration (optional)
- ✅ Modern protocol support (HTTP/2, WebSocket, gRPC, MQTT, PostgreSQL Wire, MCP)

</details>

### 🚧 In Progress (v1.4 - Q1 2026)

- 🚧 **Query Optimizer** - Advanced query optimization and execution plans
- 🚧 **Multi-Datacenter** - Cross-region deployment support
- 🚧 **Advanced ML/GNN** - Enhanced machine learning features
- 🚧 **Production Hardening** - Additional stability and performance improvements

### 📋 Planned (v1.5+ - 2026)

- 📋 **Modular Architecture** - Split monolithic core into 11 focused libraries
- 📋 **Real-Time Views** - Materialized views with automatic updates
- 📋 **Cross-Region Replication** - Global data distribution
- 📋 **Advanced Compliance** - SOC 2, HIPAA certification
- 📋 **Cloud-Native Optimizations** - Enhanced cloud provider integrations

**📚 Detailed Planning:**
- [Complete Roadmap](docs/roadmap/ROADMAP.md)
- [Modularization Plan](docs/architecture/MODULARIZATION_PLAN.md)

---

## ⚡ Performance

### Benchmark Results

> **Test Environment:** Release build, Windows x64, 20 cores @ 3696 MHz

| Operation | Throughput | Latency (avg) | Notes |
|-----------|:----------:|:-------------:|-------|
| 📝 Entity PUT | 45,000 ops/s | 0.02 ms | Write throughput |
| 📖 Entity GET | 120,000 ops/s | 0.008 ms | Read throughput |
| 🔍 Indexed Query | 3.4M queries/s | 0.29 μs | AQL WHERE clause |
| 🕸️ Graph Traverse | 9.56M ops/s | 0.105 μs | BFS (depth=3) |
| 🎯 Vector Search (RGB) | 59.7M queries/s | 0.017 μs | Simple 3D vectors |
| 📊 Vector Insert (384D) | 411k vectors/s | 2.44 μs | Typical embeddings |
| 🧠 RAG Search (Top-50) | 7.17M queries/s | 0.14 μs | LLM retrieval |

> [!IMPORTANT]
> **Performance Disclaimer:** Benchmarks represent optimal conditions. Actual performance varies based on:
> - Hardware configuration (CPU, RAM, storage)
> - Data size and complexity
> - Concurrent workload patterns
> - Build configuration and optimizations

**📊 Detailed Analysis:**
- [Complete Benchmark Results](benchmarks/BENCHMARK_DETAILED_RESULTS.md)
- [🤝 Community & Support

| Resource | Description | Link |
|----------|-------------|------|
| 📚 **Documentation** | Complete guides and API reference | [Docs Site](https://makr-code.github.io/ThemisDB/) |
| 🐛 **Issues** | Report bugs or request features | [GitHub Issues](https://github.com/makr-code/ThemisDB/issues) |
| 💬 **Discussions** | Community Q&A and discussions | [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions) |
| 🤝 **Contributing** | How to contribute to ThemisDB | [Contributing Guide](CONTRIBUTING.md) |
| 🌿 **Branching Strategy** | Git Flow workflow (main = release, develop = dev) | [Branching Strategy](BRANCHING_STRATEGY.md) |
| 🔒 **Security** | Responsible disclosure policy | [Security Policy](SECURITY.md) |

---

## 📄 License

<details open>
<summary><b>License Information</b></summary>

### Community Edition
**ThemisDB Community Edition** is released under the [MIT License](LICENSE).
- ✅ Free to use, modify, and distribute
- ✅ Commercial use allowed
- ✅ Full feature set for single-node deployments

### Enterprise Edition
**ThemisDB Enterprise Edition** features (horizontal sharding, advanced analytics, HA/replication, etc.) are available under a commercial license.

**Enterprise Inquiries:** sales@themisdb.com

**[→ See Enterprise Features](ENTERPRISE.md)**

</details>

---

## 🙏 Acknowledgments

ThemisDB builds upon and is inspired by these excellent projects:

<details>
<summary><b>Inspirations & Foundations</b></summary>

| Project | Influence | Area |
|---------|-----------|------|
| **ArangoDB** | Multi-model architecture | Design Philosophy |
| **CozoDB** | Hybrid relational-graph-vector | Data Models |
| **Azure Cosmos DB** | Multi-model with unified API | API Design |
| **RocksDB** | High-performance LSM-Tree storage | Storage Engine |
| **FAISS** | Efficient similarity search | Vector Search |

</details>

> [!NOTE]
> **For a complete list of third-party libraries and detailed feature attributions, see [ATTRIBUTIONS.md](ATTRIBUTIONS.md).**

---

<div align="center">
  
**Built with ❤️ for the database community**

[⭐ Star us on GitHub](https://github.com/makr-code/ThemisDB) · [📖 Read the Docs](https://makr-code.github.io/ThemisDB/) · [🤝 Contribute](CONTRIBUTING.md)

</div>es and feature attributions, see [ATTRIBUTIONS.md](ATTRIBUTIONS.md).**

---

**Built with ❤️ for the database community**
