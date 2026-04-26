# ThemisDB - Complete Documentation
**Version:** 1.3.0
**Generated:** 2025-12-21

---

## File: README.md

# ThemisDB

**A high-performance multi-model database with ACID guarantees + Native AI/LLM Integration**

> *"ThemisDB keeps its own llamas."* – Run LLaMA, Mistral, Phi-3 directly in your database, no API calls needed.

[![CI](https://github.com/makr-code/ThemisDB/actions/workflows/ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/ci.yml)
[![Code Quality](https://github.com/makr-code/ThemisDB/actions/workflows/code-quality.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/code-quality.yml)
[![Coverage](https://img.shields.io/badge/coverage-view%20report-brightgreen)](https://makr-code.github.io/ThemisDB/coverage/)
[![Version](https://img.shields.io/badge/version-1.3.0-blue)](https://github.com/makr-code/ThemisDB/releases/tag/v1.3.0)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)

---

## 🚀 NEW in v1.3.0: Native LLM Integration with llama.cpp (Optional)

**"ThemisDB keeps its own llamas."** – Run AI/LLM workloads directly in your database - no external API costs!

> **Note**: LLM integration is an **optional feature** that requires:
> - Build flag: `-DTHEMIS_ENABLE_LLM=ON`
> - External dependency: llama.cpp (clone separately)
> - See [Build Guide](../guides/guides_build_strategy.md) for setup instructions

### Key Features (When LLM Support Enabled)

- 🧠 **Embedded LLM Engine** - llama.cpp integration for LLaMA/Mistral/Phi-3 (1B-70B params)
- 🖼️ **Image Analysis AI Plugins** - llama.cpp Vision (primary), ONNX CLIP, OpenCV DNN for image embeddings, captioning, detection
- ⚡ **GPU Acceleration** - NVIDIA CUDA support with significant speedup vs CPU
- 💾 **PagedAttention** - Advanced memory management with memory savings
- 🎯 **Continuous Batching** - Handle concurrent inference requests
- 🔧 **Quantization Support** - Q4_K_M, Q5_K_M, Q8_0 for efficient memory usage
- 📊 **Production Monitoring** - Grafana dashboards with metrics and alert rules
- 🔌 **Plugin Architecture** - Extensible LLM and image analysis backend system
- 🌐 **Distributed RPC Framework** - Inter-shard communication for distributed LLM operations

### Performance Highlights (GPU Acceleration)

- **Significant speedup** with GPU acceleration vs CPU
- **Memory savings** with PagedAttention and prefix caching
- **Kernel fusion** for additional performance gains
- **Comprehensive test coverage** with unit tests

**[→ See LLM Integration Guide](../llm/LLAMA_CPP_INTEGRATION.md)**  
**[→ See Complete LLM Documentation](../README.md)**  
**[→ See Image Analysis Plugin Documentation](docs/plugins/)**

---

## Overview

ThemisDB is a production-ready multi-model database that combines relational, graph, vector, and document models in a single system with full ACID transaction support. Built on RocksDB with advanced security and compliance features.

**Available in two editions:**
- **Community Edition** (Free, Open Source): Full-featured single-node database with all core capabilities
- **Enterprise Edition** (Commercial License): Adds horizontal scaling, advanced analytics, HA/replication, and more

**[→ See Enterprise Edition Details](ENTERPRISE.md)**

**Key Features:**

- 🔒 **ACID Transactions** - Full snapshot isolation with MVCC
- 🔍 **Multi-Model** - Relational, Graph, Vector, Document in one database
- 🚀 **High Performance** - 45K writes/s, 120K reads/s, GPU-accelerated vector search
- 🛡️ **Security** - TLS 1.3, RBAC, field-level encryption, audit logging (Enterprise: HSM integration)
- 📊 **Analytics** - Time-series, aggregations (Enterprise: OLAP, CEP, materialized views)
- 🌐 **Distribution** - Single-node optimized (Enterprise: horizontal sharding, replication, Kubernetes)
- 🧠 **AI-Ready** - Hybrid search (RAG), embedding cache, FAISS integration, **optional LLM engine with llama.cpp** (v1.3.0+), **image analysis AI plugins** (v1.3.0+)
- 🌐 **Modern Protocols** - HTTP/1.1, GraphQL, SSE, gRPC (v1.3.0), **HTTP/2 with Server Push** ✅, **WebSocket** ✅, **MQTT** ✅, **HTTP/3** 🚧, **PostgreSQL Wire** ✅, **MCP** ✅
- 📚 **Transparent Attribution** - Clear documentation of third-party dependencies vs ThemisDB innovations (see [ATTRIBUTIONS.md](../legal/ATTRIBUTIONS.md))
- 🖼️ **Image Analysis** - Multi-backend AI plugin architecture (llama.cpp Vision, ONNX CLIP, OpenCV DNN)

---

## Quick Start

### Docker (Recommended)

```bash
# Pull and run the latest version
docker pull themisdb/themisdb:latest
docker run -d -p 8080:8080 -p 18765:18765 -v themis_data:/data themisdb/themisdb:latest

# Or use Docker Compose
docker compose up -d
```

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

**[→ Comprehensive Build Documentation](../guides/guides_build_strategy.md)** | Build-Varianten, Plattformen, Troubleshooting

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

**[→ Full Architecture Documentation](../architecture/ARCHITECTURE_OVERVIEW.md)**

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
- [Installation Guide](../guides/guides_deployment.md)
- [Docker Deployment](../deployment/DOCKER_DEPLOYMENT.md)
- [Quick Start Tutorial](docs/guides/quick_start.md)

**Core Concepts:**
- [Architecture Overview](../architecture/ARCHITECTURE_OVERVIEW.md)
- [Multi-Model Design](../architecture/architecture_base_entity.md)
- [Transaction Management](../features/features_transactions.md)
- [AQL Query Language](../aql/aql_syntax.md)

**Features:**
- [Vector Search](../features/features_vector_ops.md)
- [Graph Operations](docs/features/features_graph.md)
- [Time-Series Engine](../features/features_time_series.md)
- [Security & Compliance](../security/security_implementation.md)
- [Feature Overview](../features/features_overview.md)

**Operations:**
- [Configuration Guide](docs/guides/guides_configuration.md)
- [Monitoring & Metrics](../observability/observability_prometheus.md)
- [Backup & Recovery](../guides/guides_deployment.md#backup--recovery)
- [Performance Tuning](../performance/performance_memory.md)

**Development:**
- [Build Guide](../guides/guides_build_strategy.md)
- [Contributing](CONTRIBUTING.md)
- [API Reference](docs/apis/api_reference.md)
- [Client SDKs](../README.md)

**Full Documentation:** [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)

---

## Roadmap

**Completed (v1.0 - v1.2):**
- ✅ ACID transactions with MVCC
- ✅ Multi-model support (relational, graph, vector, document)
- ✅ Horizontal sharding and replication
- ✅ GPU acceleration (10 backends)
- ✅ Enterprise security features
- ✅ Client SDKs (7 languages)
- ✅ Kubernetes operator

**In Progress (v1.3 - Q1 2026):**
- 🚧 Query optimizer enhancements
- 🚧 Multi-datacenter deployment
- 🚧 Advanced ML/GNN features
- 🚧 Production hardening

**Planned (v1.4+ - 2026):**
- 📋 **Modular Architecture** - Split monolithic core into 11 focused libraries (post-v1.3.0)
- 📋 Real-time materialized views
- 📋 Cross-region replication
- 📋 Advanced security compliance (SOC 2, HIPAA)
- 📋 Cloud-native optimizations

**[→ Detailed Roadmap](../roadmap/ROADMAP.md)**  
**[→ Modularization Plan](../architecture/MODULARIZATION_PLAN.md)** (post-v1.3.0)

---

## Performance

**Benchmark Results** (Release build, Windows x64, 20 cores @ 3696 MHz):

| Operation | Throughput | Latency (avg) | Notes |
|-----------|------------|---------------|-------|
| Entity PUT | 45,000 ops/s | 0.02 ms | Write throughput |
| Entity GET | 120,000 ops/s | 0.008 ms | Read throughput |
| Indexed Query | 3.4M queries/s | 0.29 μs | AQL WHERE clause |
| Graph Traverse (depth=3) | 9.56M ops/s | 0.105 μs | BFS traversal |
| Vector Search (RGB) | 59.7M queries/s | 0.017 μs | Simple 3D vectors |
| Vector Insert (384D) | 411k vectors/s | 2.44 μs | Typical embeddings |
| RAG Search (Top-50) | 7.17M queries/s | 0.14 μs | LLM retrieval |

> **Note**: These benchmarks represent optimal conditions. Actual performance varies based on:
> - Hardware configuration (CPU, RAM, storage)
> - Data size and complexity
> - Concurrent workload patterns
> - Build configuration and optimizations

**[→ Detailed Benchmarks](benchmarks/BENCHMARK_DETAILED_RESULTS.md)**  
**[→ Benchmark Suite Documentation](benchmarks/COMPREHENSIVE_BENCHMARK_GUIDE.md)**

---

## Community & Support

- **Documentation**: [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
- **GitHub Issues**: [Report bugs or request features](https://github.com/makr-code/ThemisDB/issues)
- **Discussions**: [Community discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Contributing**: [Contributing guidelines](CONTRIBUTING.md)
- **Security**: [Security policy](SECURITY.md)

---

## License

**Community Edition**: ThemisDB Community Edition is released under the [MIT License](LICENSE).

**Enterprise Edition**: Enterprise features (horizontal sharding, advanced analytics, HA/replication, etc.) are available under a commercial license. See [ENTERPRISE.md](ENTERPRISE.md) for details.

For enterprise licensing inquiries, contact sales@themisdb.com

---

## Acknowledgments

ThemisDB is inspired by and builds upon the ideas from:
- **ArangoDB** - Multi-model architecture
- **CozoDB** - Hybrid relational-graph-vector design
- **Azure Cosmos DB** - Multi-model with unified API
- **RocksDB** - High-performance LSM-Tree storage
- **FAISS** - Efficient similarity search

**For a complete list of third-party libraries and feature attributions, see [ATTRIBUTIONS.md](../legal/ATTRIBUTIONS.md).**

---

**Built with ❤️ for the database community**


---

## File: CHANGELOG.md

# Changelog

All notable changes to ThemisDB will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **Network Protocol Enhancements** (PR #111 - 2025-12-21)
  - HTTP/2 with Server Push for CDC/Changefeed (proactive event delivery, ~0ms latency)
  - WebSocket support with CDC streaming (bidirectional real-time communication)
  - MQTT broker with WebSocket transport, rate limiting, and monitoring metrics
  - HTTP/3 base implementation with QUIC (ngtcp2 + nghttp3)
  - PostgreSQL Wire Protocol with SQL-to-Cypher translation for BI tool compatibility
  - MCP Server (Model Context Protocol) with cross-platform stdio/SSE/WebSocket transports
  - Production-ready security: explicit opt-in build switches, TLS/mTLS support
  - Comprehensive testing with Google Test framework (HTTP/2, WebSocket CDC, MQTT, PostgreSQL Wire)

- **Third-Party Attribution Documentation** (PR #119 - 2025-12-20)
  - Added `ATTRIBUTIONS.md` documenting 15+ core dependencies
  - Documented ThemisDB's 12 unique innovations vs third-party features
  - Clear attribution for RocksDB, FAISS, hnswlib, simdjson, Arrow, TBB, Boost, OpenSSL, etc.
  - Documented exact usage and ThemisDB extensions for each library
  - License information and repository links for all dependencies

- **Image Analysis AI Plugin Architecture** (PR #118 - 2025-12-21)
  - Plugin architecture for image analysis AI (Stable Diffusion/CLIP) running parallel with LLM
  - Multi-backend support: llama.cpp Vision (primary), ONNX Runtime, OpenCV DNN, OpenVINO, ncnn
  - Complete license compatibility analysis (all MIT/Apache 2.0/BSD compatible)
  - Plugin interfaces: `IImageAnalysisBackend`, `ImageAnalysisManager`
  - Comprehensive documentation: 7 C++ libraries evaluated, benchmarks, optimization guide
  - Example ONNX CLIP plugin implementation
  - Configuration templates for plugin management
  - Comprehensive unit tests (15+ test cases) and benchmarks (11+ categories)

### Changed
### Deprecated
### Removed
### Fixed
### Security

---

## [1.3.0] - 2025-12-17

### Added - LLM Integration (PRIMARY FEATURE - OPTIONAL)

- **llama.cpp Integration** - Optional native LLM inference engine (requires `-DTHEMIS_ENABLE_LLM=ON`)
  - Complete plugin-based architecture (ILLMPlugin, LLMPluginManager)
  - GGUF model loader with Blob Store integration
  - Asynchronous inference engine for non-blocking operations
  - Support for LLaMA, Mistral, Phi-3 models (1B-70B parameters)
  - Quantization support: Q4_K_M, Q5_K_M, Q8_0
  - Requires external llama.cpp clone (not included in repository)

- **GPU Acceleration & Performance** (when LLM enabled)
  - NVIDIA CUDA support with automatic detection and graceful CPU fallback
  - Significant speedup vs CPU-only inference (hardware dependent)
  - PagedAttention with BlockTable and Copy-on-Write prefix sharing
  - Continuous Batch Scheduler supporting concurrent requests
  - Kernel Fusion with fused CUDA kernels for additional performance
  - Multi-compute capability support (Pascal to Ada architectures)

- **Advanced LLM Features** (when LLM enabled)
  - Ollama-style lazy model loading
  - vLLM-style multi-LoRA management
  - Prefix caching for repeated prompts
  - Response caching for common queries
  - Model metadata caching

- **Monitoring & Observability**
  - Grafana/Prometheus integration with metrics
  - Dashboard panels for LLM performance monitoring
  - Alert rules for production deployment
  - Docker Compose deployment stack

- **Testing & Quality**
  - Comprehensive unit tests for LLM functionality
  - Integration tests
  - End-to-end scenarios
  - Benchmark scenarios
  - High test coverage

- **LLM Documentation** (33 guides)
  - GPU integration guide
  - Quantization guide
  - Performance benchmarks
  - Deployment guide
  - Complete API documentation (HTTP, gRPC, AQL extensions)

### Added - RPC Framework (SUPPORTING INFRASTRUCTURE)

- **Distributed Communication**
  - Protocol-agnostic IRPCPlugin and IRPCServer interfaces
  - gRPC plugin implementation (258 LOC, security hardened)
  - TLS/mTLS support with X.509 certificates
  - 15 RPC methods for CRUD, query, transactions, authentication

- **Inter-Shard Data Transfer**
  - RocksDB snapshot transfer (10-20x faster bulk migration)
  - Blob transfer for LoRA adapters (100 MB - 10 GB files)
  - Differential update mode (90-98% bandwidth savings)
  - Chunking, compression (ZSTD, LZ4), and checksums

- **Security Hardening**
  - Path traversal protection with robust validation
  - Input validation and memory limits
  - Secure temporary file handling
  - TLS fail-closed design

- **C++ Handler Implementations**
  - SnapshotTransferHandler (664 LOC)
  - BlobTransferHandler (526 LOC)
  - DifferentialUpdateEngine (381 LOC) with CDC and bsdiff

- **Temporal Consistency**
  - MVCC-aware snapshots
  - Point-in-time consistency guarantees
  - Version catalog and WAL replay

### Added - Foundation Improvements

- **Infrastructure Gaps Closed**
  - Embedding Cache: Configurable cache_dir
  - CTE Support: Full entity JSON serialization via BaseEntity::toJson()

- **Advanced Features**
  - Process Mining (200+ lines): Graph-based event extraction, conformance checking
  - Stream Protocol (100+ lines): Chunking, compression, checksums

- **Production Confirmations**
  - Hybrid Search (BM25+Vector)
  - Video Processor (LibAVFormat)
  - OLAP Analytics (30+ Google Test cases)
  - Distributed Transactions

### Changed

- Updated version from 1.2.0 to 1.3.0
- Enhanced README with v1.3.0 LLM capabilities
- Added THEMIS_BUILD_RPC_FRAMEWORK option to build system

### Security

- Enhanced JWT validation with security warnings (development placeholder)
- Secure temporary file creation with random suffixes
- Strengthened path traversal protection in RPC handlers
- Platform-specific temporary directory usage (not hardcoded /tmp)

### Files Changed

- 153 files changed
- 48,371 insertions
- 655 deletions

---

## [1.2.0] - 2025-12-15

### Added

- **Hypertables** - TimescaleDB-compatible time-series storage
  - Automatic time-based partitioning using RocksDB Column Families
  - TTL-based retention policies (leveraging v1.1.0 TTL feature)
  - ZSTD compression for historical data
  - Efficient time-range queries
  - GDPR-compliant data lifecycle management
  - Performance: 100K inserts/s, 5ms query latency (1-day range)
  
- **Hybrid Search** - RAG-optimized search engine
  - Reciprocal Rank Fusion (RRF) algorithm
  - Combines BM25 full-text + vector semantic search
  - Configurable weights for BM25 and vector components
  - 85% recall@10 (vs 60% BM25-only, 70% vector-only)
  - 88% precision@10
  - Optimized for Retrieval-Augmented Generation workflows
  
- **FAISS Advanced** - Production-scale vector search
  - IVF (Inverted File Index) + PQ (Product Quantization)
  - 10-100x memory reduction (1536D: 6KB → 60B per vector)
  - Multiple index types: IVF_PQ, IVF_FLAT, HNSW_FLAT, IVF_HNSW_PQ
  - GPU acceleration via CUDA
  - Persistent index save/load
  - 2-10x faster on large datasets (>1M vectors)
  - 95-99% recall with proper nprobe tuning
  
- **Embedding Cache** - Semantic caching for cost reduction
  - Fuzzy matching via vector similarity
  - Cost tracking and API savings estimation
  - TTL-based automatic cleanup
  - 70-90% cost reduction for LLM applications
  - 100-1000x faster (1ms cache hit vs 100-1000ms API call)
  - ROI: Pays for itself in days for high-volume workloads
  
- **Time-Series Aggregates** - SIMD-accelerated analytics
  - Functions: SUM, AVG, MIN, MAX, COUNT, STDDEV, VARIANCE, FIRST, LAST, P50, P95, P99
  - Resample operations (1-second → 1-minute aggregates)
  - Rolling windows (5-minute moving average)
  - Time bucketing (hourly/daily aggregates)
  - 5-10x faster than naive loops (AVX2/AVX512 SIMD)
  - Zero-copy batch processing

### Performance

- Hypertables: 5x storage compression (100GB → 20GB for 30 days)
- Hybrid Search: 12ms latency (combined BM25 + vector)
- FAISS: 2-10x speed improvement on >1M vectors
- Embedding Cache: ~$100-500/month savings (1M OpenAI ada-002 calls)
- Time-Series: 5-10x aggregation speedup via SIMD

### Known Issues

- Hypertables: Column Family listing not exposed, chunk statistics are placeholders
- Hybrid Search: Stub implementation requires full SecondaryIndexManager/VectorIndexManager integration

### Notes

- No new dependencies (uses existing libraries from v1.1.0)
- Fully backward compatible with v1.1.0
- All features are opt-in via CMake flags and explicit API usage
- Full documentation: `docs/releases/v1.2.0.md`

---

## [1.1.0] - 2025-12-15

### Added

- **RocksDB TTL Support**
  - Column family level TTL configuration
  - Automatic data expiration and retention policies
  - GDPR/compliance-ready data lifecycle management
  
- **RocksDB Incremental Backup**
  - Space-efficient backups using `BackupEngine`
  - Share table files between backups
  - Backup restore functionality
  - Production-ready backup strategies
  
- **RocksDB Statistics Export**
  - Real-time performance metrics
  - Monitoring integration (Prometheus, OpenTelemetry)
  - Performance insights (compaction, memtable, cache)
  
- **TBB Parallel Sort**
  - Replaced 23 instances of `std::sort` with `tbb::parallel_sort`
  - 2-4x speedup for large datasets
  - Applied in query engine and analytics paths
  
- **TBB Concurrent Hash Maps**
  - Lock-free concurrent_hash_map implementation
  - 2-3x throughput improvement
  - Thread-safe concurrent operations
  - Applied in LLM prompt manager
  
- **Apache Arrow Parquet Export**
  - Type inference for schema generation
  - Multiple compression codecs (SNAPPY, GZIP, ZSTD, LZ4)
  - 90% storage compression for analytical exports
  - Conditional compilation with `ARROW_ENABLED` flag
  
- **vLLM Co-Location Resource Manager**
  - GPU resource coordination between ThemisDB and vLLM
  - NVML integration for GPU monitoring
  - Low-priority CUDA streams for background operations
  - 80% GPU usage threshold for adaptive allocation
  - Automatic platform detection (NVIDIA, AMD, Intel)
  - Docker Compose configuration: `docker-compose-vllm.yml`
  
- **mimalloc Memory Allocator**
  - Drop-in replacement for system allocator
  - 20-40% memory performance improvement
  - Reduced memory fragmentation
  - Zero-change integration (automatic override)
  - Configurable via `THEMIS_USE_MIMALLOC=ON`

### Changed

- Build system now supports 4 variants: Standard (OLTP), OLAP, Embedded, vLLM Co-Location
- CUDA backend enhanced with low-priority stream support
- Query engine optimized with parallel sorting

### Performance

- Overall: 3-10x performance improvement across workloads
- Parallel sort: 2-4x speedup for large datasets
- Concurrent hash maps: 2-3x throughput increase
- Parquet export: 90% compression ratio
- Memory allocator: 20-40% performance boost

### Notes

- Only 1 new dependency added (mimalloc)
- Fully backward compatible with v1.0.x
- All features opt-in via CMake configuration
- Version file updated: 1.0.1 → 1.1.0
- Full documentation: `docs/releases/v1.1.0.md`, `docs/analysis/VARIANT_STRATEGY_v1.1.0.md`

---

## [1.0.2] - 2025-12-14

### Fixed

- **Windows MSVC Release Build**
  - Fixed RocksDB linker error (unrecognized file format)
  - Root cause: DLL build mode attempted to link 1.2GB static rocksdb.lib
  - Solution: Intelligent RocksDB target selection in CMakeLists.txt
  - Uses `rocksdb-shared.dll` for shared builds, static for static builds
  - Default Windows build now uses static mode for reliability
  - Build output: `themis_server.exe` (10.1 MB)
  - Verified on Windows 11 with MSVC 19.44
  
- **Linux Build via WSL**
  - Re-validated and stabilized
  - Updated release artifacts and checksums

### Added

- **Documentation**
  - Troubleshooting guide: `docs/troubleshooting/rocksdb-windows-build-issues.md`
  - Build workarounds for Windows developers
  
- **Packaging**
  - Release artifacts for Windows/Linux
  - ZIP bundles with checksums
  - Release notes: `RELEASE_NOTES_v1.0.2.md`
  - Manifest: `MANIFEST_v1.0.2.txt`
  - SHA256 checksums: `SHA256SUMS.txt`

### Notes

- Patch release, compatible with v1.0.x
- No database migration required
- No breaking changes

---

## [1.0.1] - 2025-12-14

### Added

- **Docker Hub Multi-Architecture Images**
  - Published to `docker.io/themisdb/themisdb`
  - Tags: `v1.0.1`, `latest`
  - Platforms: `linux/amd64`, `linux/arm64`
  - Automatic platform detection on pull
  - Image size: ~150MB compressed
  - Build time: ~3.8 hours for both architectures
  
- **Docker Deployment Documentation**
  - New file: `docs/deployment/DOCKER_DEPLOYMENT.md`
  - Quick start with `docker pull` and `docker run`
  - Configuration reference (environment variables, ports)
  - Volume management and data persistence
  - Docker Compose examples
  - Production deployment best practices
  - Platform-specific instructions
  - Troubleshooting guide

### Fixed

- **Dockerfile**
  - Fixed `LD_LIBRARY_PATH` undefined variable warning
  - Updated runtime path: `/usr/local/lib/themisdb:/usr/local/lib`
  - Clean build with zero warnings
  
- **HTTP API Routing** (Critical)
  - Fixed `/entities/batch` endpoint returning 404
  - Root cause: Route classification prioritized parametrized patterns over exact matches
  - Solution: Reordered route matching in `HttpServer::classifyRoute()`
  - Impact: Batch operations now fully functional
  - Verified with 3/3 successful batch operations
  
- **Build System**
  - Fixed MSVC `setAttribute()` overload ambiguity
  - Added explicit `int64_t` casting for span attributes
  - Clean compilation across MSVC and GCC

### Changed

- **Build Configuration**
  - Optimized buildx configuration for multi-arch
  - Builder: `themis-multiarch` (docker-container driver)

### Security

- Release artifacts include GPG signatures
- SHA256 checksums for integrity verification

### Known Issues

- Minor: Tracer initialization warnings on startup (harmless, fix planned for v1.1)
  - Worker threads may process requests before Tracer::initialize() completes
  - Graceful fallback to no-op tracing
  - No impact on API functionality

### Notes

- Health check verified: `curl http://localhost:8080/health`
- Multi-arch images tested on AMD64 and ARM64
- Full backward compatibility with v1.0.0

---

## [1.0.0] - 2025-12-09

### Added

- **Release Management & Supply Chain Security**
  - SBOM (Software Bill of Materials) in CycloneDX 1.4 format
  - SHA256 hash verification for all artifacts
  - Machine-readable JSON: `SBOM_v1.0.0.json`
  - Human-readable manifest: `MANIFEST_v1.0.0.txt`
  - Enterprise release pipeline: `scripts/enterprise_release.ps1`
  - 50-item SLSA Level 1 compliance checklist
  - GitHub Actions release workflow: `.github/workflows/release.yml`
  - Automated SBOM generation: `scripts/generate_sbom.py`
  
- **Competitive Benchmarking Infrastructure**
  - Framework comparing vs PostgreSQL, MySQL, MariaDB, CockroachDB, TiDB, SingleStore
  - 87% performance gap closure target
  - Benchmarking documentation: `benchmarks/README.md`
  - Systematic performance optimization roadmap
  
- **Core Database Features**
  - ACID transactions with MVCC (Multi-Version Concurrency Control)
  - Multi-model support: Relational, Graph, Vector, Document
  - RocksDB TransactionDB storage engine
  - Secondary indexes (equality, composite, range, sparse, geo, TTL, fulltext)
  - Graph indexes (adjacency-based traversal)
  - HNSW vector index with persistence
  - Advanced Query Language (AQL)
  - Change Data Capture (CDC)
  - Time-series engine with Gorilla compression
  
- **Security Features**
  - TLS 1.3 with mTLS support
  - Role-Based Access Control (RBAC)
  - Field-level encryption
  - Audit logging with SIEM integration
  - Certificate pinning for HSM/TSA
  - HashiCorp Vault secrets management
  - Rate limiting and DoS protection
  - Input validation and sanitization
  - Security headers (CSP, HSTS, X-Frame-Options)
  
- **Distributed Features**
  - Horizontal sharding with consistent hashing (150 virtual nodes)
  - PKI-based operation signing (RSA-SHA256)
  - mTLS shard communication
  - etcd metadata store integration
  - Parallel scatter-gather queries
  - Cross-shard joins (Broadcast Hash, Co-Located)
  - P2P Gossip protocol (SWIM-based, optional)
  - Cassandra-inspired streaming (LZ4/Zstd compression)
  - Leader-follower replication
  - Multi-master replication with CRDTs
  - RAID-like redundancy modes
  - Kubernetes CRDs
  
- **GPU Acceleration** (Optional)
  - 10 backend options: CUDA, Vulkan, HIP, OpenCL, DirectX, OneAPI, ZLUDA, Metal, Graphics
  - 10-50x speedup for vector search
  - Automatic platform detection and fallback
  
- **Analytics Features**
  - Complex Event Processing (CEP) engine with EPL
  - Pattern matching: SEQUENCE, AND, OR, NOT, WITHIN
  - Windows: TUMBLING, SLIDING, SESSION, HOPPING
  - OLAP features: CUBE, ROLLUP, window functions
  - Stream-stream joins
  
- **Observability**
  - 44 Prometheus metrics for sharding and operations
  - Grafana dashboards (19 panels, 8 alert rules)
  - OpenTelemetry distributed tracing
  - RocksDB statistics export
  - Health check endpoints
  
- **Client SDKs**
  - 7 languages: Python, JavaScript, Rust, Go, Java, C#, Swift
  - Feature parity across all SDKs
  - CRUD, AQL, Vector, Graph, Transactions, URN-Routing
  - Connection pooling
  
- **Content Processing**
  - 10+ format processors: PDF, Office (DOCX/XLSX/PPTX/ODF), Video, Audio, Geo, Image, CAD, Text
  - Plugin interface for extensibility
  - YAML-based configuration

### Performance

- Entity PUT: 45,000 ops/s (p50: 0.02ms, p99: 0.15ms)
- Entity GET: 120,000 ops/s (p50: 0.008ms, p99: 0.05ms)
- Indexed Query: 8,500 queries/s (p50: 0.12ms, p99: 0.85ms)
- Graph Traverse (depth=3): 3,200 ops/s (p50: 0.31ms, p99: 1.2ms)
- Vector ANN (k=10): 1,800 queries/s (p50: 0.55ms, p99: 2.1ms)
- Index Rebuild: 12,000 entities/s

### Documentation

- Complete architecture documentation (456+ files)
- API reference (OpenAPI 3.0)
- Deployment guides (Docker, Kubernetes, ARM, QNAP)
- Security compliance documentation (BSI C5, ISO 27001, GDPR, SOC 2)
- Developer guides (build, test, contribute)
- Full documentation site: https://makr-code.github.io/ThemisDB/

### Notes

- Initial production release
- Comprehensive test coverage (85%+)
- GDPR/SOC2/HIPAA compliance-ready
- MIT License

---

## Template for Future Releases

```markdown
## [X.Y.Z] - YYYY-MM-DD

### Added
- New features

### Changed
- Changes to existing functionality

### Deprecated
- Soon-to-be removed features

### Removed
- Removed features

### Fixed
- Bug fixes

### Security
- Security improvements and vulnerability fixes

### Performance
- Performance metrics and improvements

### Notes
- Additional release notes
- Migration guides
- Breaking changes
```

---

[Unreleased]: https://github.com/makr-code/ThemisDB/compare/v1.2.0...HEAD
[1.2.0]: https://github.com/makr-code/ThemisDB/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/makr-code/ThemisDB/compare/v1.0.2...v1.1.0
[1.0.2]: https://github.com/makr-code/ThemisDB/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/makr-code/ThemisDB/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/makr-code/ThemisDB/releases/tag/v1.0.0


---

## File: docker\README.md

# ThemisDB - Official Docker Image

[![Release](https://img.shields.io/github/v/release/makr-code/ThemisDB?include_prereleases&sort=semver&color=blue)](https://github.com/makr-code/ThemisDB/releases)
[![License](https://img.shields.io/badge/license-MIT-green)](https://github.com/makr-code/ThemisDB/blob/main/LICENSE)
[![Platform](https://img.shields.io/badge/platform-linux%2Famd64%20%7C%20linux%2Farm64-brightgreen)](https://github.com/makr-code/ThemisDB)
[![Build](https://img.shields.io/badge/build-multi--stage--docker-success)](https://github.com/makr-code/ThemisDB/blob/main/Dockerfile)
[![Docker](https://img.shields.io/badge/Docker-✓-blue?logo=docker)](https://hub.docker.com/r/themisdb/themisdb)
[![GitHub Stars](https://img.shields.io/github/stars/makr-code/ThemisDB?style=social)](https://github.com/makr-code/ThemisDB)

ThemisDB ist ein High-Performance Multi-Modell-Datenbanksystem auf Basis von LSM-Tree-Architektur mit nativer Unterstützung für Vektorsuche, Graphoperationen, Geospatial-Abfragen und Volltextsuche.

**✨ NEU in v1.3.0:** Optionale native LLM-Integration mit llama.cpp - Führen Sie AI/LLM-Workloads direkt in Ihrer Datenbank aus!

**Aktuelle Version:** v1.3.0 (Dezember 2025)  
**Registry:** `docker.io/themisdb/themisdb`  
**Build-Status:** ✅ Erfolgreich (21.12.2025)

---

## Quick Start

```bash
# Image herunterladen
docker pull themisdb/themisdb:1.3.0

# ThemisDB starten
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 8765:8765 \
  -p 4318:4318 \
  -v themis_data:/data \
  themisdb/themisdb:1.3.0

# Verifyieren Sie den Start
curl http://localhost:8080/health
```

**Ports:**
- `8080` - REST API & GraphQL Interface (HTTP)
- `8765` - Binary Protocol (gRPC, Wire Protocol)
- `4318` - OpenTelemetry/Prometheus Metrics (OTLP)

**Volume:**
- `/data` - Datenbankdateien (müssen persistent sein!)

**Schnelle Überprüfung:**
```bash
docker logs themis        # Logs anschauen
docker ps | grep themis   # Container-Status prüfen
curl http://localhost:8080/health
```

---

## Verfügbare Tags

| Tag | Architektur | Base Image | Einsatz |
|-----|-------------|------------|---------|
| `latest` | amd64, arm64 | Ubuntu 22.04 | **Empfohlen** - Neueste stabile Version |
| `1.3.0` | amd64, arm64 | Ubuntu 22.04 | Stabile Release (Dez 2025) - LLM-Support |
| `1.3` | amd64, arm64 | Ubuntu 22.04 | Minor Version Track |
| `1.2.0` | amd64, arm64 | Ubuntu 22.04 | Vorherige stabile Version |
| `qnap` | amd64 | Ubuntu 20.04 | **QNAP NAS** optimiert (SSE4.2 Baseline) |
| `1.3.0-qnap` | amd64 | Ubuntu 20.04 | QNAP v1.3.0 Release |

**Multi-Architektur-Unterstützung:**
- `linux/amd64` - Intel/AMD x64 Prozessoren
- `linux/arm64` - ARM v8 (Raspberry Pi, Apple Silicon, AWS Graviton)

Docker wählt automatisch die passende Architektur für Ihre Plattform.

---

## Features

✅ **Multi-Modell Datenbank**
- Key-Value Store
- Document Store (JSON, BSON)
- Vector Search (Embeddings, Ähnlichkeitssuche mit HNSW)
- Graph Database (Vertices, Edges, Traversals)
- Geospatial (Points, Polygons, Spatial Indexes)
- Full-Text Search (Tokenization, Stemming, Ranking)
- **NEU: Optional LLM Integration** (v1.3.0) - Native AI Inference mit llama.cpp

✅ **Enterprise Features**
- ACID-Transaktionen mit Snapshot Isolation
- Horizontale Skalierung & Sharding
- Multi-Master Replication
- GPU-Beschleunigung (CUDA, Vulkan, ROCm)
- Real-Time Analytics (Complex Event Processing, OLAP)
- Client SDKs (Python, JavaScript, Rust, Go, Java, C#, Swift)
- **NEU: Protocol Support** (v1.3.0) - HTTP/2, WebSocket, MQTT, PostgreSQL Wire, MCP

✅ **Production-Ready**
- ~3.8 GB Docker Image (komprimiert ~150 MB)
- Integrierte Health-Checks
- OpenTelemetry Instrumentation
- DSGVO/GDPR Compliance Features
- Automatisierte Backups & Recovery
- Non-Root Container Security

---

## Docker Compose

### Basis-Setup

```yaml
version: '3.8'

services:
  themis:
    image: themisdb/themisdb:1.3.0
    container_name: themis
    ports:
      - "8080:8080"      # HTTP REST API
      - "8765:8765"      # Binary Protocol (Wire Protocol, gRPC)
      - "4318:4318"      # OpenTelemetry OTLP (Prometheus)
    volumes:
      - themis_data:/data
      - ./config.json:/etc/themis/config.json:ro
    environment:
      THEMIS_PORT: "8765"
      THEMIS_CONFIG_PATH: "/etc/themis/config.json"
    restart: unless-stopped
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8080/health"]
      interval: 30s
      timeout: 5s
      retries: 3
      start_period: 10s

volumes:
  themis_data:
    driver: local
```

**Starten:**
```bash
docker-compose up -d
docker-compose logs -f themis
```

### Production-Setup mit Monitoring

```yaml
version: '3.8'

services:
  themis:
    image: themisdb/themisdb:1.3.0
    ports:
      - "8080:8080"      # HTTP REST API
      - "8765:8765"      # Binary Protocol (Wire Protocol, gRPC)
      - "4318:4318"      # OpenTelemetry OTLP
    volumes:
      - themis_data:/data
      - ./config.json:/etc/themis/config.json:ro
    environment:
      THEMIS_PORT: "8765"
      THEMIS_CONFIG_PATH: "/etc/themis/config.json"
    deploy:
      resources:
        limits:
          cpus: '4'
          memory: 8G
        reservations:
          cpus: '2'
          memory: 4G
    restart: unless-stopped
    networks:
      - themis_net

  prometheus:
    image: prom/prometheus:latest
    ports:
      - "9090:9090"      # Prometheus UI & API
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml:ro
      - prometheus_data:/prometheus
    command:
      - '--config.file=/etc/prometheus/prometheus.yml'
      - '--storage.tsdb.path=/prometheus'
    networks:
      - themis_net
    restart: unless-stopped

  grafana:
    image: grafana/grafana:latest
    ports:
      - "3000:3000"      # Grafana UI
    environment:
      GF_SECURITY_ADMIN_PASSWORD: "admin"
      GF_INSTALL_PLUGINS: "grafana-clock-panel,grafana-simple-json-datasource"
    volumes:
      - grafana_data:/var/lib/grafana
    networks:
      - themis_net
    restart: unless-stopped

networks:
  themis_net:
    driver: bridge

volumes:
  themis_data:
    driver: local
  prometheus_data:
    driver: local
  grafana_data:
    driver: local
```

---

## Konfiguration

### Umgebungsvariablen

| Variable | Standard | Beschreibung |
|----------|----------|--------------|
| `THEMIS_CONFIG_PATH` | `/etc/themis/config.json` | Pfad zur Konfigurationsdatei |
| `THEMIS_PORT` | `8765` | Interner Server-Port (Binary Protocol) |
| `LD_LIBRARY_PATH` | `/usr/local/lib/themisdb:/usr/local/lib` | Pfad für Runtime-Bibliotheken |

### Custom Configuration

```bash
# Eigene Konfiguration einbinden
docker run -d \
  -v /path/to/config.json:/etc/themis/config.json:ro \
  themisdb/themisdb:1.3.0
```

**Konfiguration Beispiel:**
```json
{
  "server": {
    "port": 8765,
    "max_connections": 1000,
    "thread_pool_size": 8,
    "host": "0.0.0.0"
  },
  "storage": {
    "data_dir": "/data",
    "cache_size_mb": 512,
    "compression": "zstd"
  },
  "tracing": {
    "enabled": true,
    "service_name": "themis-server",
    "otlp_endpoint": "http://localhost:4318"
  },
  "features": {
    "vector_search": true,
    "graph_engine": true,
    "geo_spatial": true,
    "llm_engine": false
  }
}
```

---

## Plattform-spezifische Verwendung

### QNAP NAS

```bash
# QNAP-optimiertes Image herunterladen
docker pull themisdb/themisdb:qnap

# Starten (mit Port 8765)
docker run -d \
  --name themis \
  -p 8765:8765 \
  -v /share/Container/themis/data:/data \
  themisdb/themisdb:qnap
```

**QNAP Hinweise:**
- Ubuntu 20.04 Base (GLIBC 2.31)
- SSE4.2 CPU Baseline (erweiterte Kompatibilität)
- Optimiert für x86_64 QNAP NAS-Geräte
- Erfordert QTS 5.0+ oder QuTS hero h5.0+

### Raspberry Pi / ARM

```bash
# Wählt automatisch ARM64 Image aus
docker pull themisdb/themisdb:1.3.0
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 8765:8765 \
  -p 4318:4318 \
  -v themis_data:/data \
  themisdb/themisdb:1.3.0
```

### macOS (Docker Desktop)

```bash
# Funktioniert auf Intel und Apple Silicon
docker pull themisdb/themisdb:1.3.0
docker run -d \
  -p 8080:8080 \
  -p 8765:8765 \
  -p 4318:4318 \
  -v themis_data:/data \
  themisdb/themisdb:1.3.0
```

---

## Volumes & Persistence

### Data Volume

```bash
# Named Volume erstellen
docker volume create themis_data

# Mit Volume starten
docker run -d -v themis_data:/data themisdb/themisdb:1.3.0

# Volume sichern
docker run --rm -v themis_data:/data -v $(pwd):/backup \
  ubuntu tar czf /backup/themis_backup.tar.gz /data
```

### Wichtige Verzeichnisse

| Pfad | Zweck | Mount |
|------|-------|--------|
| `/data` | Datenbankdateien | ✅ Erforderlich |
| `/etc/themis` | Konfiguration | Optional |
| `/var/log/themis` | Anwendungs-Logs | Optional |

---

## Health Checks & Monitoring

### Built-in Health Check

```bash
# Container-Zustand prüfen
docker inspect --format='{{.State.Health.Status}}' themis

# Health Logs anschauen
docker inspect --format='{{json .State.Health}}' themis | jq
```

### Health Endpoint

```bash
curl http://localhost:8080/health
# Response: {"status":"ok","uptime":3600,"version":"1.3.0"}
```

### Ressourcen-Monitoring

```bash
# Live Statistiken
docker stats themis

# Detaillierte Ressourcennutzung
docker inspect themis | jq '.[0].HostConfig.Memory'
```

---

## Troubleshooting

### Container startet nicht

```bash
# Logs anschauen
docker logs themis

# Häufige Lösungen:
# 1. Port-Konflikt - Port-Mapping ändern
docker run -p 8081:8080 -p 18766:18765 ...

# 2. Berechtigungsproblem - Volume-Berechtigungen prüfen
docker run --user 0 ...  # Temporär als root starten

# 3. Ressourcen-Limit - Memory erhöhen
docker run --memory 4g ...
```

### Performance-Probleme

```bash
# Ressourcen erhöhen
docker update --memory 8g --cpus 4 themis
docker restart themis

# Ressourcennutzung prüfen
docker stats themis

# Für Production optimieren
docker run -d \
  --cpus="4" \
  --memory="8g" \
  --memory-swap="10g" \
  themisdb/themisdb:1.3.0
```

### Library Path Probleme

```bash
# Bibliotheken überprüfen
docker exec themis ldd /usr/local/bin/themis_server

# Umgebungsvariablen prüfen
docker exec themis printenv LD_LIBRARY_PATH
```

---

## Advanced Usage

### Ressourcen Limits

```bash
docker run -d \
  --cpus="4" \              # 4 CPU Cores
  --memory="8g" \           # 8GB RAM
  --memory-swap="10g" \     # 10GB insgesamt mit Swap
  --pids-limit=1000 \       # Prozess-Limit
  themisdb/themisdb:1.3.0
```

### Logging Konfiguration

```bash
# JSON file driver mit Rotation
docker run -d \
  --log-driver json-file \
  --log-opt max-size=10m \
  --log-opt max-file=3 \
  themisdb/themisdb:1.3.0

# Syslog driver
docker run -d \
  --log-driver syslog \
  --log-opt syslog-address=udp://localhost:514 \
  themisdb/themisdb:1.3.0
```

### Netzwerk Modi

```bash
# Host network (bessere Performance, weniger Isolation)
docker run -d --network host themisdb/themisdb:1.3.0

# Custom bridge network
docker network create themis_net
docker run -d --network themis_net themisdb/themisdb:1.3.0
```

---

## Eigenes Image bauen

```bash
# Repository klonen
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Standard Image bauen
docker build -t themis:custom .

# QNAP-optimiertes Image bauen
docker build -f docker/Dockerfile.qnap -t themis:qnap .

# Multi-Architektur Build (erfordert buildx)
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t themis:multiarch \
  --push .

# Mit lokaler LLM-Integration (llama.cpp)
docker build \
  --build-arg ENABLE_LLM=ON \
  --build-context llama=llama.cpp \
  -t themis:llm .
```

---

## Support & Ressourcen

**Dokumentation:**
- [ThemisDB GitHub](https://github.com/makr-code/ThemisDB)
- [Docker Deployment Guide](https://github.com/makr-code/ThemisDB/blob/main/docs/deployment/DOCKER_DEPLOYMENT.md)
- [API Reference](https://github.com/makr-code/ThemisDB/tree/main/openapi)

**Hilfe bekommen:**
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- GitHub Discussions: https://github.com/makr-code/ThemisDB/discussions

**Quellcode:**
- Repository: https://github.com/makr-code/ThemisDB
- Lizenz: [Siehe LICENSE](https://github.com/makr-code/ThemisDB/blob/main/LICENSE)

---

## Image Details

**Image Größe:** 
- Komprimiert: ~150 MB
- Unkomprimiert: ~400 MB
- Docker Umgebung: ~3.8 GB

**Build Info:**
- Builder: Docker Buildx (Multi-Stage)
- Compiler: GCC 11+ / Clang 14+
- C++ Standard: C++20
- vcpkg: Package Manager für Dependencies

**Sicherheit:**
- Non-Root User standardmäßig
- Minimale Angriffsfläche
- Regelmäßige Security Updates
- SBOM (Software Bill of Materials) vorhanden

**Inhalte der Runtime Image:**
- `themis_server` Binary (30 MB)
- Runtime-Bibliotheken (vcpkg Dependencies)
- Konfigurationen & Schemas
- Dokumentation & OpenAPI Specs
- Client SDKs & Beispiele
- Plugin-Signer Tools

---

## Lizenz

ThemisDB ist unter den Bedingungen der [LICENSE](https://github.com/makr-code/ThemisDB/blob/main/LICENSE) Datei lizenziert.

---

**Letztes Update:** 21. Dezember 2025  
**Betreuer:** ThemisDB Team  
**Docker Hub:** https://hub.docker.com/r/themisdb/themisdb  
**GitHub:** https://github.com/makr-code/ThemisDB


---

## File: docs\INDEX.md

# ThemisDB Documentation

**Version:** 1.3.0  
**Last Updated:** April 2026

Welcome to the ThemisDB documentation! This guide will help you find the information you need.

---

## 📚 Quick Navigation

**New to ThemisDB?**
- [Quick Start Guide](../guides/QUICK_START.md) - Get up and running in 5 minutes
- [Installation Guide](guides/INSTALLATION.md) - Complete installation instructions
- [Architecture Overview](architecture/OVERVIEW.md) - Understand how ThemisDB works

**Using ThemisDB:**
- [AQL Query Language](../aql/aql_syntax.md) - Learn the query language
- [REST API Reference](apis/REST_API.md) - HTTP API documentation
- [Client SDKs](../clients/README.md) - SDK documentation for 7 languages

**Operating ThemisDB:**
- [Configuration](operations/CONFIGURATION.md) - Configure your database
- [Monitoring](operations/MONITORING.md) - Monitor performance and health
- [Backup & Recovery](operations/BACKUP.md) - Protect your data

---

## 📖 Documentation Structure

### Getting Started

| Document | Description |
|----------|-------------|
| [Quick Start](../guides/QUICK_START.md) | 5-minute tutorial to get started |
| [Installation](guides/INSTALLATION.md) | Installation on Linux, Windows, macOS, Docker |
| [Configuration](operations/CONFIGURATION.md) | Configure ThemisDB for your needs |
| [First Query](guides/FIRST_QUERY.md) | Write your first AQL query |

### Core Concepts

| Document | Description |
|----------|-------------|
| [Architecture Overview](architecture/OVERVIEW.md) | High-level system architecture |
| [Multi-Model Design](../architecture/architecture_base_entity.md) | How ThemisDB handles multiple data models |
| [Transaction Model](../features/features_transactions.md) | ACID transactions with MVCC |
| [Storage Layer](architecture/architecture_storage.md) | RocksDB LSM-Tree storage |

### Features

| Document | Description |
|----------|-------------|
| [Feature Overview](../features/features_overview.md) | Complete feature catalog |
| [Vector Search](../features/features_vector_ops.md) | Similarity search with HNSW/FAISS |
| [Graph Operations](features/features_graph.md) | Graph traversal and pathfinding |
| [Time-Series](../features/features_time_series.md) | Time-series data and compression |
| [Hypertables](features/features_hypertables.md) | TimescaleDB-compatible time-series (v1.2+) |
| [Hybrid Search](features/features_hybrid_search.md) | RAG-optimized BM25+Vector search (v1.2+) |
| [Analytics](../observability/CEP_STREAMING_ANALYTICS.md) | CEP and OLAP analytics |

### Query Language (AQL)

| Document | Description |
|----------|-------------|
| [AQL Syntax](../aql/aql_syntax.md) | Complete AQL language reference |
| [AQL Examples](aql/aql_examples.md) | Common query patterns |
| [Query Optimization](../aql/aql_explain_profile.md) | EXPLAIN and PROFILE commands |

### API Reference

| Document | Description |
|----------|-------------|
| [REST API](apis/REST_API.md) | HTTP API endpoints |
| [GraphQL API](apis/api_graphql.md) | GraphQL interface |
| [Client SDKs](../clients/README.md) | SDKs for Python, JS, Rust, Go, Java, C#, Swift |

### Security & Compliance

| Document | Description |
|----------|-------------|
| [Security Overview](../security/security_implementation.md) | Enterprise security features |
| [TLS Setup](../guides/guides_tls_setup.md) | Configure TLS 1.3 and mTLS |
| [RBAC Configuration](../guides/guides_rbac.md) | Role-based access control |
| [Encryption](../security/security_encryption_strategy.md) | Data encryption at rest and in transit |
| [Audit Logging](../features/features_audit_logging.md) | Security event logging |
| [Compliance](../compliance/compliance_dashboard.md) | GDPR, SOC 2, HIPAA compliance |

### Operations

| Document | Description |
|----------|-------------|
| [Deployment Guide](operations/DEPLOYMENT.md) | Production deployment strategies |
| [Docker Deployment](../deployment/DOCKER_DEPLOYMENT.md) | Docker and Kubernetes deployment |
| [Configuration](operations/CONFIGURATION.md) | Configuration reference |
| [Monitoring](operations/MONITORING.md) | Prometheus metrics and alerting |
| [Backup & Recovery](operations/BACKUP.md) | Backup strategies and disaster recovery |
| [Troubleshooting](operations/TROUBLESHOOTING.md) | Common issues and solutions |
| [Performance Tuning](../performance/performance_memory.md) | Optimize for your workload |

### Development

| Document | Description |
|----------|-------------|
| [Contributing](../CONTRIBUTING.md) | How to contribute to ThemisDB |
| [Build Guide](../guides/guides_build_strategy.md) | Build from source |
| [Development Setup](development/SETUP.md) | Setup development environment |
| [Testing Guide](development/TESTING.md) | Run and write tests |
| [Code Style](development/CODE_STYLE.md) | Coding standards |
| [Architecture](../architecture/ARCHITECTURE_OVERVIEW.md) | Deep-dive into internals |

### Advanced Topics

| Document | Description |
|----------|-------------|
| [Sharding](sharding/sharding_overview.md) | Horizontal sharding and routing |
| [Replication](sharding/sharding_replication.md) | Leader-follower and multi-master |
| [GPU Acceleration](../performance/performance_gpu_plan.md) | CUDA, Vulkan, HIP backends |
| [vLLM Co-Location](../reports/VARIANT_STRATEGY_v1.1.0.md) | AI/ML workload optimization |
| [Content Processing](content/content_architecture.md) | Process PDFs, images, videos, etc. |

### Release Notes

| Document | Description |
|----------|-------------|
| [Changelog](../releases/CHANGELOG.md) | Version history and changes |
| [Roadmap](../roadmap/ROADMAP.md) | Future plans and features |
| [v1.3.0 Release](../releases/RELEASE_NOTES_v1.3.0.md) | Latest release notes |
| [v1.2.0 Release](../releases/v1.2.0.md) | Previous release |
| [v1.1.0 Release](../releases/v1.1.0.md) | Previous release |
| [Migration Guides](guides/MIGRATION.md) | Upgrade between versions |

---

## 🔍 Search by Topic

### By Use Case

**Building an Application:**
- [Quick Start](../guides/QUICK_START.md) → [REST API](apis/REST_API.md) → [Client SDKs](../clients/README.md)

**Analytics & BI:**
- [OLAP Features](observability/OLAP.md) → [Parquet Export](observability/olap.md) → [Time-Series](../features/features_time_series.md)

**AI/ML Applications:**
- [Vector Search](../features/features_vector_ops.md) → [Hybrid Search](features/features_hybrid_search.md) → [Embedding Cache](features/features_embedding_cache.md)

**Graph Applications:**
- [Graph Operations](features/features_graph.md) → [AQL Graph Queries](../aql/aql_syntax.md#graph-traversals) → [Path Algorithms](features/features_graph.md#algorithms)

**Production Deployment:**
- [Deployment Guide](operations/DEPLOYMENT.md) → [Monitoring](operations/MONITORING.md) → [Backup](operations/BACKUP.md) → [Security](../security/security_implementation.md)

### By Technology

**Docker/Kubernetes:**
- [Docker Deployment](../deployment/DOCKER_DEPLOYMENT.md)
- [Kubernetes Guide](deployment/deployment_kubernetes.md)
- [Helm Charts](../README.md)

**Cloud Platforms:**
- [AWS Deployment](deployment/deployment_aws.md)
- [Azure Deployment](deployment/deployment_azure.md)
- [GCP Deployment](deployment/deployment_gcp.md)

**ARM/Raspberry Pi:**
- [ARM Build Guide](../deployment/deployment_arm_build.md)
- [ARM Packages](../deployment/deployment_arm_packages.md)
- [Raspberry Pi Optimization](deployment/deployment_rpi.md)

---

## 📊 Performance & Benchmarks

- [Performance Overview](performance/performance_overview.md)
- [Benchmarking Guide](../README.md)
- [Memory Tuning](../performance/performance_memory.md)
- [GPU Performance](../performance/performance_gpu_plan.md)
- [Query Optimization](performance/performance_query.md)

---

## 🤝 Community Resources

- **GitHub Repository:** [github.com/makr-code/ThemisDB](https://github.com/makr-code/ThemisDB)
- **Issue Tracker:** [Report bugs](https://github.com/makr-code/ThemisDB/issues)
- **Discussions:** [Community forum](https://github.com/makr-code/ThemisDB/discussions)
- **Contributing:** [How to contribute](../CONTRIBUTING.md)
- **Security:** [Security policy](../SECURITY.md)

---

## 📝 Documentation Conventions

**Version Indicators:**
- No marker: Available in all versions
- (v1.1+): Available from version 1.1.0 onwards
- (v1.2+): Available from version 1.2.0 onwards
- (v1.3+): Available from version 1.3.0 onwards
- 🚧 Experimental: Not production-ready
- 📋 Planned: Future feature

**Code Examples:**
- Examples use generic placeholders like `localhost:8765`
- Adjust for your environment
- Most examples work with default configuration

**Conventions:**
- File paths use forward slashes (works on Windows too)
- Command examples assume bash shell (Windows users: use PowerShell equivalent)

---

## 🆘 Getting Help

1. **Check Documentation:** Search this documentation first
2. **Search Issues:** Check if someone else had the same problem
3. **Ask Community:** Use [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
4. **Report Bug:** Open an [issue](https://github.com/makr-code/ThemisDB/issues/new)

---

**Documentation Version:** 1.2.0  
**Last Updated:** April 2026  
**Next Review:** March 15, 2026


---

## File: docs\Home.md

# Welcome to ThemisDB

**A high-performance multi-model database with ACID guarantees**

[![Version](https://img.shields.io/badge/version-1.3.0-blue)](https://github.com/makr-code/ThemisDB/releases/tag/v1.3.0)
[![CI](https://github.com/makr-code/ThemisDB/actions/workflows/ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-MIT-green)](https://github.com/makr-code/ThemisDB/blob/main/LICENSE)

---

## Overview

ThemisDB is a production-ready multi-model database that combines **relational, graph, vector, and document** models in a single system with full ACID transaction support. Built on RocksDB with advanced security and compliance features.

**Key Capabilities:**
- 🔒 **ACID Transactions** - Full snapshot isolation with MVCC
- 🔍 **Multi-Model** - One database for relational, graph, vector, and documents
- 🚀 **High Performance** - 45K writes/s, 120K reads/s
- 🛡️ **Enterprise Security** - TLS 1.3, RBAC, encryption, audit logging
- 🌐 **Distributed** - Horizontal sharding, replication, Kubernetes-ready
- 🧠 **AI-Ready** - Hybrid search, embedding cache, GPU-accelerated

---

## Quick Links

### 🚀 Getting Started
- **[Quick Start Guide](guides-QUICK_START)** - Get running in 5 minutes
- **[Installation](guides-INSTALLATION)** - Install on Linux, Windows, macOS, or Docker
- **[Configuration](operations-CONFIGURATION)** - Configure for your needs
- **[First Query](guides-FIRST_QUERY)** - Write your first AQL query

### 📖 Learn ThemisDB
- **[Architecture Overview](architecture-OVERVIEW)** - Understand the design
- **[AQL Query Language](aql-aql_syntax)** - Learn the query syntax
- **[Feature Overview](features-features_overview)** - Explore all features
- **[REST API](api-REST_API)** - HTTP API reference

### 🚀 Deploy to Production
- **[Deployment Guide](operations-DEPLOYMENT)** - Production deployment
- **[Docker Guide](DOCKER_DEPLOYMENT)** - Run with Docker/Kubernetes
- **[Monitoring](operations-MONITORING)** - Monitor with Prometheus
- **[Security Hardening](security-security_implementation)** - Secure your deployment

---


## Core Features

### Multi-Model Database

**Relational:**
- Secondary indexes (equality, composite, range)
- SQL-like AQL queries
- ACID transactions

**Graph:**
- Native graph storage
- BFS, Dijkstra, A* traversals
- Path constraints and pruning

**Vector:**
- HNSW and FAISS indexes
- GPU-accelerated similarity search
- Hybrid search for RAG workflows

**Document:**
- JSON storage with flexible schema
- Fast field extraction
- Schema-based encryption

### Advanced Analytics

- **CEP Engine** - Complex Event Processing with pattern matching
- **OLAP** - CUBE, ROLLUP, window functions  
- **Time-Series** - Gorilla compression, continuous aggregates
- **Streaming** - Real-time data processing

### Enterprise Security

- **Authentication** - RBAC with 4-tier hierarchy, mTLS
- **Encryption** - AES-256-GCM at rest, TLS 1.3 in transit
- **Audit** - 65+ event types, SIEM integration
- **Compliance** - GDPR, SOC 2, HIPAA ready
- **Secrets** - HashiCorp Vault integration

### Distributed Capabilities

- **Sharding** - Consistent hashing, 150 virtual nodes
- **Replication** - Leader-follower and multi-master
- **Redundancy** - RAID-like modes (MIRROR, STRIPE, PARITY)
- **Kubernetes** - Operator with CRDs
- **Monitoring** - 44 Prometheus metrics, Grafana dashboards

---

## Performance Benchmarks

| Operation | Throughput | Latency (p50) |
|-----------|------------|---------------|
| Entity PUT | 45,000 ops/s | 0.02 ms |
| Entity GET | 120,000 ops/s | 0.008 ms |
| Indexed Query | 8,500 queries/s | 0.12 ms |
| Graph Traverse | 3,200 ops/s | 0.31 ms |
| Vector ANN (k=10) | 1,800 queries/s | 0.55 ms |

**[Full Benchmarks →](benchmarks-README)**

---

## Documentation Structure

This wiki is organized into the following sections:

### For Users
- **Getting Started** - Installation, quick start, configuration
- **Features** - Detailed feature documentation
- **Query Language** - AQL syntax and examples
- **API Reference** - REST, GraphQL, client SDKs

### For Operators
- **Operations** - Deployment, monitoring, backup
- **Security** - TLS, RBAC, encryption, compliance
- **Performance** - Tuning and optimization

### For Developers
- **Development** - Building, testing, contributing
- **Architecture** - System design and internals
- **Advanced Topics** - Sharding, GPU, plugins

---

## Roadmap

### Completed (v1.0 - v1.2)
- ✅ ACID transactions with MVCC
- ✅ Multi-model support (all 4 models)
- ✅ Horizontal sharding and replication
- ✅ GPU acceleration (10 backends)
- ✅ Enterprise security features
- ✅ Client SDKs (7 languages)
- ✅ Hypertables and hybrid search

### Current Focus (v1.3.0 - Q1 2026)
- 🚧 Query optimizer v2
- 🚧 RE2 integration for security
- 🚧 SDK publishing (PyPI, npm, crates.io)
- 🚧 Penetration testing phase 1

### Planned (v1.4.0+ - 2026)
- 📋 Multi-datacenter deployment
- 📋 Advanced ML/GNN features
- 📋 DuckDB OLAP integration
- 📋 Real-time materialized views

**[Full Roadmap →](roadmap-ROADMAP)**

---

## Community & Support

- **📖 Documentation:** [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
- **💬 Discussions:** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **🐛 Issues:** [Report bugs](https://github.com/makr-code/ThemisDB/issues)
- **🔒 Security:** [Security policy](SECURITY)
- **🤝 Contributing:** [Contributing guide](CONTRIBUTING)

---

## Quick Start Example

```bash
# Pull and run with Docker
docker pull themisdb/themisdb:latest
docker run -d -p 8765:8765 themisdb/themisdb:latest

# Create an entity
curl -X PUT http://localhost:8765/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Alice\",\"age\":30}"}'

# Query
curl -X POST http://localhost:8765/query \
  -H "Content-Type: application/json" \
  -d '{"table":"users","predicates":[{"column":"age","value":"30"}]}'
```

**[Full Quick Start →](guides-QUICK_START)**

---

## License

ThemisDB is open source under the [MIT License](https://github.com/makr-code/ThemisDB/blob/main/LICENSE).

---

**Ready to get started?** → **[Quick Start Guide](guides-QUICK_START)**

**Need help?** → **[Discussions](https://github.com/makr-code/ThemisDB/discussions)**


---

## File: docs\DOCUMENTATION_INDEX.md

# ThemisDB Dokumentations-Index

**Letzte Aktualisierung:** 20. Dezember 2025
**Version:** 1.3.0 (LLM Integration Release)

## 🎯 Schnelleinstieg nach Rolle

### Für Entwickler
1. [README.md](../README.md) - Projektübersicht & Quick Start
2. [guides/guides_build_strategy.md](../guides/guides_build_strategy.md) - Build-Toolchain (Windows/Linux/Docker)
3. [docs/guides/guides_build.md](../guides/guides_build.md) - Detaillierte Build-Anleitung
4. [DEVELOPMENT_AUDITLOG.md](../development/DEVELOPMENT_SUMMARY.md) - Aktueller Entwicklungsstand
5. [Enterprise Features](../README.md) - Enterprise Scalability Features

### Für Stakeholder
1. [THEMIS_SACHSTANDSBERICHT_2025.md](../reports/themis_sachstandsbericht_2025.md) - Executive Summary
2. ~~THEMIS_PROJECT_VALUATION.md~~ - 🔒 Confidential (available to licensed customers only)
3. [features/features_overview.md](../features/features_overview.md) - Feature-Übersicht mit Status
4. [ROADMAP.md](../roadmap/roadmap_overview.md) - Entwicklungs-Roadmap

### Für Compliance & Audits
1. [compliance/compliance_dashboard.md](../compliance/compliance_dashboard.md) - Executive Compliance Summary
2. [compliance/compliance_full_checklist.md](../compliance/compliance_full_checklist.md) - BSI C5, ISO 27001, DSGVO, eIDAS, SOC 2
3. [security/SECURITY_AUDIT_REPORT.md](../security/security_audit_report.md) - Security Audit Ergebnisse
4. [SECURITY.md](../SECURITY.md) - Vulnerability Disclosure Policy
5. [legal/LICENSE_COMPATIBILITY_ANALYSIS.md](../legal/LICENSE_COMPATIBILITY_ANALYSIS.md) - ⭐ License Compatibility (v1.3.0)
6. [THIRD_PARTY_LICENSES.md](../legal/THIRD_PARTY_LICENSES.md) - ⭐ Third-Party License Attribution (v1.3.0)

## 📚 Dokumentationsstruktur

### Root-Level Dokumente
```
/
├── README.md                        # Projektübersicht & Quick Start
├── LICENSE                          # MIT License with Government Clause
├── THIRD_PARTY_LICENSES.md          # ⭐ Third-Party License Attribution (v1.3.0)
├── aql/                             # ⭐ AQL EBNF Grammatik (v1.3.0)
│   ├── AQL_GRAMMAR.ebnf             # Vollständige formale Grammatik
│   └── README.md                    # AQL Übersicht
├── features/features_overview.md    # Feature-Liste mit Status
├── ROADMAP.md                       # Entwicklungs-Roadmap
├── CHANGELOG.md                     # Änderungshistorie
├── guides/guides_build_strategy.md  # Build-Toolchain & Strategie
├── INTEGRATION_ANALYSIS.md          # Enterprise Integration Analysis
├── TEST_REPORT.md                   # Vollständiger Test-Report
├── DEVELOPMENT_AUDITLOG.md          # Entwicklungsstand & Audit
├── DOCKER_DEPLOYMENT.md             # Docker Deployment Guide (v1.3.0)
└── CONTRIBUTING.md                  # Contribution Guidelines
```

### docs/ - Strukturierte Dokumentation
```
docs/
├── enterprise/                      # Enterprise Features
│   └── README.md                    # Enterprise Übersicht & Guide
├── performance/                     # Performance & Benchmarks
│   └── ENTERPRISE_SCALABILITY_STRATEGY.md
├── security/                        # Sicherheit & Compliance
├── legal/                           # ⭐ Legal & Licensing (v1.3.0)
│   └── LICENSE_COMPATIBILITY_ANALYSIS.md  # Dependency License Analysis
├── architecture/                    # Architektur-Dokumentation
├── api/                            # API-Dokumentation
└── guides/                         # User Guides
```

## 🚀 Enterprise Features

### Dokumentation
| Dokument | Zweck | Zielgruppe |
|----------|-------|------------|
| [enterprise/README.md](../README.md) | Übersicht & Quick Start | Entwickler, DevOps |
| [enterprise/enterprise_scalability.md](enterprise/enterprise_scalability.md) | Feature-Details & Code-Beispiele | Entwickler |
| [enterprise/enterprise_http_pool.md](enterprise/enterprise_http_pool.md) | HTTP Client Implementation | Entwickler |
| [enterprise/enterprise_final_report.md](enterprise/enterprise_final_report.md) | Implementation Summary | Stakeholder |
| [INTEGRATION_ANALYSIS.md](../reports/INTEGRATION_ANALYSIS.md) | Legacy Integration | Entwickler |

### Status
- ✅ **Token Bucket Rate Limiter** - Production Ready (5/5 Tests)
- ✅ **Per-Client Rate Limiter** - Production Ready (3/3 Tests)
- ✅ **Load Shedder** - Production Ready (5/5 Tests)
- ✅ **HTTP Client Pool** - Production Ready (6/6 Tests)
- ✅ **Batch Operations** - Production Ready (1/1 Tests)

**Test Coverage:** 20/20 (100%)

## 📖 Architektur & Design

### Kern-Architektur
- [architecture.md](architecture/architecture_overview.md) - System-Architektur Übersicht
- [storage/storage_rocksdb.md](../storage/storage_rocksdb.md) - RocksDB Storage Layout
- [mvcc_design.md](../architecture/architecture_mvcc.md) - MVCC Transaction Design
- [query_engine_aql.md](../aql/aql_query_engine.md) - Query Engine & AQL

### Spezielle Features
- [geo/GEO_ARCHITECTURE.md](../geo/geo_architecture.md) - Geo/Spatial Architecture
- [vector_ops.md](../features/features_vector_ops.md) - Vector Operations & HNSW
- [content_pipeline.md](../architecture/architecture_content_pipeline.md) - Content Processing Pipeline
- [search/hybrid_search_design.md](../search/hybrid_search_design.md) - Hybrid Search

## 🔒 Security & Compliance

### Security
- [security/security_overview.md](../security/security_overview.md) - Security Übersicht
- [encryption_strategy.md](../security/security_encryption_strategy.md) - Verschlüsselungsstrategie
- [security/security_key_management.md](../security/security_key_management.md) - Key Management
- [security/security_threat_model.md](../security/security_threat_model.md) - Threat Model
- [security_hardening_guide.md](../security/security_hardening.md) - Hardening Guide

### Compliance
- [compliance/compliance_dashboard.md](../compliance/compliance_dashboard.md) - Executive Dashboard
- [compliance/compliance_dpia.md](../compliance/compliance_dpia.md) - Datenschutz-Folgenabschätzung (DSGVO)
- [compliance/compliance_bcp_drp.md](../compliance/compliance_bcp_drp.md) - Business Continuity & Disaster Recovery
- [compliance_audit.md](../features/features_compliance_audit.md) - Compliance Audit
- [AUDIT_LOGGING.md](../features/features_audit_logging.md) - Audit Logging

### PKI & eIDAS
- [pki_integration_architecture.md](../security/security_pki_architecture.md) - PKI Integration
- [eidas_qualified_signatures.md](../security/security_eidas.md) - eIDAS Signaturen
- [security/pki_rsa_integration.md](../security/security_pki_rsa.md) - PKI RSA Integration

## 🛠️ Build & Deployment

### Build-Dokumentation
- [guides/guides_build_strategy.md](../guides/guides_build_strategy.md) - Build-Strategie & Plattformen
- [guides/guides_build.md](../guides/guides_build.md) - Detaillierte Build-Anleitung

### Deployment
- [deployment.md](../guides/guides_deployment.md) - Deployment-Strategien
- [DOCKER_MULTI_ARCH_STRATEGY.md](../deployment/deployment_docker_multiarch.md) - Multi-Arch Docker
- [docs/CI_CD_MULTIARCH.md](../deployment/deployment_cicd_multiarch.md) - Multi-Arch CI/CD

### Platform-Specific
- [ARM_RASPBERRY_PI_BUILD.md](../deployment/deployment_arm_build.md) - Raspberry Pi Build
- [ARM_BENCHMARKS.md](../deployment/deployment_arm_benchmarks.md) - ARM Performance
- [RASPBERRY_PI_TUNING.md](../deployment/deployment_raspberry_tuning.md) - Pi Tuning Guide

## 📊 Performance & Benchmarks

- [performance_benchmarks.md](../performance/performance_benchmarks.md) - Performance Übersicht
- [compression_benchmarks.md](../performance/performance_compression_benchmarks.md) - Kompression
- [encryption_metrics.md](../security/security_encryption_metrics.md) - Verschlüsselung Performance
- [performance/ENTERPRISE_SCALABILITY_STRATEGY.md](enterprise/enterprise_scalability.md) - Enterprise Strategy

## 🔍 API & Query Language

### AQL (Advanced Query Language)
- [aql_syntax.md](../aql/aql_syntax.md) - AQL Syntax
- [aql-hybrid-queries.md](../aql/aql_hybrid_queries.md) - Hybrid Queries
- [aql_explain_profile.md](../aql/aql_explain_profile.md) - EXPLAIN & PROFILE
- [recursive_path_queries.md](../features/features_recursive_path.md) - Rekursive Pfade
- [temporal_graphs.md](../features/features_temporal_graphs.md) - Temporale Graphen

### APIs
- [apis/openapi.md](../apis/apis_openapi.md) - REST API & OpenAPI Spec
- [apis/contentfs_api.md](../apis/apis_contentfs.md) - ContentFS API
- [apis/hybrid_search_api.md](../apis/apis_hybrid_search.md) - Hybrid Search API

## 👥 Client SDKs

- [clients/javascript_sdk_quickstart.md](../clients/clients_javascript_sdk.md) - JavaScript SDK
- [clients/python_sdk_quickstart.md](../clients/clients_python_sdk.md) - Python SDK
- [clients/rust_sdk_quickstart.md](../clients/clients_rust_sdk.md) - Rust SDK

## 📝 Development

### Guidelines
- [development/developers.md](../development/developers.md) - Developer Guide
- [code_quality.md](../guides/guides_code_quality.md) - Code Quality Pipeline
- [CONTRIBUTING.md](../CONTRIBUTING.md) - Contribution Guidelines

### Status & Planning
- [DEVELOPMENT_AUDITLOG.md](../development/DEVELOPMENT_SUMMARY.md) - Development Audit
- [development/implementation_status.md](../development/implementation_status.md) - Status
- [development/roadmap.md](../development/roadmap.md) - Roadmap
- [development/priorities.md](../development/priorities.md) - Prioritäten

### API Implementations
- [development/audit_api_implementation.md](../development/audit_api_implementation.md) - Audit API
- [development/saga_api_implementation.md](../development/saga_api_implementation.md) - SAGA API

## 🔗 External Resources

### GitHub
- **Repository:** https://github.com/makr-code/ThemisDB
- **Wiki:** https://github.com/makr-code/ThemisDB/wiki
- **Issues:** https://github.com/makr-code/ThemisDB/issues

### Badges
- [![CI](https://github.com/makr-code/ThemisDB/actions/workflows/ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/ci.yml)
- [![Code Quality](https://github.com/makr-code/ThemisDB/actions/workflows/code-quality.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/code-quality.yml)
- [![ARM Build](https://github.com/makr-code/ThemisDB/actions/workflows/arm-build.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/arm-build.yml)

## 📋 Navigation nach Thema

### Source-Module (16 Komponenten)

| Modul | README | Source | Headers | LOC |
|-------|--------|--------|---------|-----|
| analytics | [docs/observability/README.md](../README.md) | 2 | 3 | 3,742 |
| cache | [docs/storage/README.md](../README.md) | 1 | 6 | 492 |
| cdc | [docs/features/README.md](../README.md) | 1 | 1 | 510 |
| content | [docs/content/README.md](../README.md) | 15 | 16 | 9,091 |
| geo | [docs/geo/README.md](../README.md) | 3 | 2 | 304 |
| governance | [docs/governance/README.md](../README.md) | 1 | 1 | 259 |
| index | [docs/search/README.md](../README.md) | 11 | 12 | 14,629 |
| llm | [docs/llm/README.md](../README.md) | 2 | 2 | 679 |
| query | [docs/query/README.md](../README.md) | 12 | 12 | 12,560 |
| replication | [docs/storage/README.md](../README.md) | 1 | 2 | 1,612 |
| security | [docs/security/README.md](../README.md) | 16 | 16 | 8,138 |
| server | [docs/server/README.md](../README.md) | 20 | 20 | 18,282 |
| sharding | [docs/sharding/README.md](../README.md) | 19 | 21 | 12,278 |
| storage | [docs/storage/README.md](../README.md) | 10 | 9 | 4,591 |
| timeseries | [docs/timeseries/README.md](../README.md) | 8 | 7 | 2,767 |
| transaction | [docs/architecture/README.md](../README.md) | 2 | 2 | 895 |

**Gesamt:** 124 Source-Dateien, 132 Header-Dateien, 90,829 LOC

**Audit-Report:** [SOURCE_CODE_AUDIT.md](../development/SOURCE_CODE_AUDIT.md)

### Multi-Model Features
- **Graph:** [property_graph_model.md](../features/features_property_graph.md), [graph_index.cpp.md](../src/index/graph_index.cpp.md)
- **Geo/Spatial:** [GEO_ARCHITECTURE.md](../geo/geo_architecture.md), [geo_acceleration_3d_games.md](../geo/geo_acceleration_3d_games.md)
- **Time-Series:** [time_series.md](../features/features_time_series.md), [timeseries/continuous_agg.cpp.md](../src/timeseries/continuous_agg.cpp.md)
- **Document:** [content_pipeline.md](../architecture/architecture_content_pipeline.md), [content/content_manager.cpp.md](../src/content/content_manager.cpp.md)
- **Vector/Embedding:** [vector_ops.md](../features/features_vector_ops.md), [gnn_embeddings.md](../features/features_gnn_embeddings.md)

### Storage & Persistence
- **RocksDB:** [storage/storage_rocksdb.md](../storage/storage_rocksdb.md), [storage/rocksdb_wrapper.cpp.md](../src/storage/rocksdb_wrapper.cpp.md)
- **MVCC:** [mvcc_design.md](../architecture/architecture_mvcc.md)
- **Transactions:** [transactions.md](../features/features_transactions.md), [transaction/saga.cpp.md](../src/transaction/saga.cpp.md)
- **Compression:** [compression_strategy.md](../performance/performance_compression_strategy.md), [timeseries/gorilla.cpp.md](../src/timeseries/gorilla.cpp.md)

### Search & Indexing
- **Fulltext:** [search/fulltext_api.md](../search/fulltext_api.md), [search/stemming.md](../search/stemming.md)
- **Hybrid Search:** [search/hybrid_search_design.md](../search/hybrid_search_design.md)
- **Vector Search:** [vector_ops.md](../features/features_vector_ops.md), [index/vector_index.cpp.md](../src/index/vector_index.cpp.md)
- **Geo Indexing:** [geo/cpu_backend.cpp.md](../src/geo/cpu_backend.cpp.md)

### Governance & PII
- **PII Detection:** [security/pii_detection.md](../security/security_pii_detection.md), [pii_api.md](../security/security_pii_api.md)
- **Policies:** [security/security_policies.md](../security/security_policies.md), [governance/policy_engine.cpp.md](src/governance/policy_engine.cpp.md)
- **RBAC:** [rbac_authorization.md](../guides/guides_rbac.md), [RBAC.md](../guides/guides_rbac.md)
- **Retention:** [security/audit_and_retention.md](../security/security_audit_retention.md)

## ⚠️ Deprecated / Archive

Veraltete oder abgelöste Dokumentation:
- [archive/](archive/) - Archivierte Dokumente
- [reports/](reports/) - Git Merge Reports

## 🔄 Synchronisation

### Wiki Sync
```powershell
./sync-wiki.ps1
```

### Lokale Vorschau
```powershell
./build-docs.ps1  # MkDocs → site/
```

### GitHub Pages
- **Primär:** GitHub Wiki (maßgeblich)
- **Sekundär:** MkDocs Build (für Entwicklung)

## 📞 Support

- **Issues:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Diskussionen:** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Security:** Siehe [SECURITY.md](../SECURITY.md)

---

**Dokumentations-Status:** ✅ Konsolidiert (5. Dezember 2025)  
**Maintainer:** ThemisDB Team  
**Letzte Audit:** 5. Dezember 2025


---

## File: docs\guides\QUICK_START.md

# Quick Start Guide

**Get ThemisDB up and running in 5 minutes**

This guide will help you install ThemisDB, start the server, and run your first queries.

---

## Prerequisites

- **Docker** (recommended) OR
- **Linux/macOS/Windows** with build tools

---

## Installation

### Option 1: Docker (Recommended)

**Fastest way to get started:**

```bash
# Pull the latest image
docker pull themisdb/themisdb:latest

# Run ThemisDB
docker run -d \
  --name themisdb \
  -p 8765:8765 \
  -p 8080:8080 \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Check if it's running
curl http://localhost:8765/health
```

**Expected response:**
```json
{"status":"ok","uptime":5}
```

### Option 2: Pre-built Packages

**Debian/Ubuntu:**
```bash
wget https://github.com/makr-code/ThemisDB/releases/latest/download/themisdb_1.2.0-1_amd64.deb
sudo apt install ./themisdb_1.2.0-1_amd64.deb
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

### Option 3: Build from Source

**Clone and build:**
```bash
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Linux/macOS
./scripts/setup.sh
./scripts/build.sh

# Windows
.\scripts\setup.ps1
.\build.ps1

# Start server
./build/themis_server --config config.yaml
```

---

## Your First Queries

### 1. Check Server Health

```bash
curl http://localhost:8765/health
```

### 2. Create Your First Entity

```bash
curl -X PUT http://localhost:8765/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"name\":\"Alice\",\"age\":30,\"city\":\"Berlin\",\"role\":\"developer\"}"
  }'
```

**Response:**
```json
{"status":"success","key":"users:alice"}
```

### 3. Read the Entity

```bash
curl http://localhost:8765/entities/users:alice
```

**Response:**
```json
{
  "key": "users:alice",
  "blob": "{\"name\":\"Alice\",\"age\":30,\"city\":\"Berlin\",\"role\":\"developer\"}"
}
```

### 4. Create More Entities

```bash
# Create Bob
curl -X PUT http://localhost:8765/entities/users:bob \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Bob\",\"age\":35,\"city\":\"Berlin\",\"role\":\"manager\"}"}'

# Create Charlie
curl -X PUT http://localhost:8765/entities/users:charlie \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Charlie\",\"age\":28,\"city\":\"Munich\",\"role\":\"developer\"}"}'

# Create David
curl -X PUT http://localhost:8765/entities/users:david \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"David\",\"age\":32,\"city\":\"Hamburg\",\"role\":\"designer\"}"}'
```

### 5. Create an Index for Queries

```bash
curl -X POST http://localhost:8765/index/create \
  -H "Content-Type: application/json" \
  -d '{"table":"users","column":"city"}'
```

**Response:**
```json
{"status":"success","index":"users.city"}
```

### 6. Query by Index

**Find all users in Berlin:**
```bash
curl -X POST http://localhost:8765/query \
  -H "Content-Type: application/json" \
  -d '{
    "table": "users",
    "predicates": [{"column": "city", "value": "Berlin"}],
    "return": "entities"
  }'
```

**Response:**
```json
{
  "table": "users",
  "count": 2,
  "entities": [
    "{\"name\":\"Alice\",\"age\":30,\"city\":\"Berlin\",\"role\":\"developer\"}",
    "{\"name\":\"Bob\",\"age\":35,\"city\":\"Berlin\",\"role\":\"manager\"}"
  ]
}
```

### 7. Create a Range Index

```bash
curl -X POST http://localhost:8765/index/create \
  -H "Content-Type: application/json" \
  -d '{"table":"users","column":"age","type":"range"}'
```

### 8. Range Query with Sorting

**Find users aged 28-33, sorted by age:**
```bash
curl -X POST http://localhost:8765/query \
  -H "Content-Type: application/json" \
  -d '{
    "table": "users",
    "range": [{"column": "age", "gte": "28", "lte": "33"}],
    "order_by": {"column": "age", "desc": false},
    "return": "entities"
  }'
```

### 9. Using AQL (Advanced Query Language)

**AQL provides SQL-like syntax with graph and vector support:**

```bash
curl -X POST http://localhost:8765/query/aql \
  -H "Content-Type: application/json" \
  -d '{
    "query": "FOR u IN users FILTER u.city == \"Berlin\" AND u.age >= 30 RETURN u"
  }'
```

### 10. View Server Metrics

```bash
# JSON statistics
curl http://localhost:8765/stats

# Prometheus metrics
curl http://localhost:8765/metrics
```

---

## Next Steps

### Working with Graphs

**Create edges (relationships):**

```bash
# Alice knows Bob
curl -X PUT http://localhost:8765/entities/edge:e1 \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"id\":\"e1\",\"_from\":\"users:alice\",\"_to\":\"users:bob\",\"type\":\"knows\"}"
  }'

# Bob knows Charlie
curl -X PUT http://localhost:8765/entities/edge:e2 \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"id\":\"e2\",\"_from\":\"users:bob\",\"_to\":\"users:charlie\",\"type\":\"knows\"}"
  }'

# Graph traversal (BFS)
curl -X POST http://localhost:8765/graph/traverse \
  -H "Content-Type: application/json" \
  -d '{"start_vertex":"users:alice","max_depth":2}'
```

### Working with Vectors

**Create vector index:**
```bash
curl -X POST http://localhost:8765/vector/create-index \
  -H "Content-Type: application/json" \
  -d '{
    "table": "documents",
    "dimension": 128,
    "metric": "l2"
  }'
```

**Add vector embeddings:**
```bash
curl -X PUT http://localhost:8765/entities/documents:doc1 \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"title\":\"Introduction\",\"embedding\":[0.1,0.2,...]}"
  }'
```

**Vector similarity search:**
```bash
curl -X POST http://localhost:8765/vector/search \
  -H "Content-Type: application/json" \
  -d '{
    "table": "documents",
    "vector": [0.1, 0.2, ...],
    "k": 10
  }'
```

---

## Common Tasks

### Update an Entity

```bash
curl -X PUT http://localhost:8765/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Alice\",\"age\":31,\"city\":\"Berlin\",\"role\":\"senior developer\"}"}'
```

### Delete an Entity

```bash
curl -X DELETE http://localhost:8765/entities/users:alice
```

### Batch Operations

```bash
curl -X POST http://localhost:8765/entities/batch \
  -H "Content-Type: application/json" \
  -d '{
    "operations": [
      {"op":"PUT","key":"users:eve","blob":"{\"name\":\"Eve\"}"},
      {"op":"PUT","key":"users:frank","blob":"{\"name\":\"Frank\"}"},
      {"op":"DELETE","key":"users:old_user"}
    ]
  }'
```

### Backup Database

```bash
curl -X POST http://localhost:8765/admin/backup \
  -H "Content-Type: application/json" \
  -d '{"path":"/backups/backup-2025-12-15"}'
```

---

## Configuration

**Basic config.yaml:**

```yaml
storage:
  rocksdb_path: ./data/themis_server
  memtable_size_mb: 256
  block_cache_size_mb: 1024

server:
  host: 0.0.0.0
  port: 8765
  worker_threads: 8

vector_index:
  engine: hnsw
  hnsw_m: 16
  hnsw_ef_construction: 200
```

**See:** [Configuration Guide](../operations/CONFIGURATION.md)

---

## Troubleshooting

### Server won't start

**Check logs:**
```bash
# Docker
docker logs themisdb

# Package installation
sudo journalctl -u themisdb

# From source
./build/themis_server --log-level debug
```

### Connection refused

**Check if server is running:**
```bash
# Health check
curl http://localhost:8765/health

# Process check
ps aux | grep themis_server

# Docker check
docker ps | grep themisdb
```

### Port already in use

**Change port in config.yaml:**
```yaml
server:
  port: 8766  # Use different port
```

### Database not open error

**Check data directory permissions:**
```bash
# Linux/macOS
chmod 755 ./data
chown $USER:$USER ./data

# Docker - use volumes
docker run -v themis_data:/data themisdb/themisdb:latest
```

---

## Learn More

- **[Installation Guide](INSTALLATION.md)** - Detailed installation instructions
- **[Configuration Guide](../operations/CONFIGURATION.md)** - All configuration options
- **[AQL Documentation](../aql/aql_syntax.md)** - Learn the query language
- **[REST API Reference](../apis/REST_API.md)** - Complete API documentation
- **[Client SDKs](../README.md)** - Use ThemisDB from your favorite language

---

## Getting Help

- **Documentation:** [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
- **GitHub Issues:** [Report a bug](https://github.com/makr-code/ThemisDB/issues)
- **Discussions:** [Ask questions](https://github.com/makr-code/ThemisDB/discussions)
- **Security:** [Security policy](../../SECURITY.md)

---

**Congratulations!** You've completed the quick start guide. You now know how to:
- ✅ Install and run ThemisDB
- ✅ Create, read, update, and delete entities
- ✅ Create indexes and query data
- ✅ Work with graphs and vectors
- ✅ Monitor your database

**Next:** Explore the [full documentation](../INDEX.md) to learn about advanced features like transactions, sharding, replication, and GPU acceleration.


---

## File: docs\aql\aql_syntax.md

# AQL - THEMIS Query Language

**Version:** 1.0  
**Datum:** 30. Oktober 2025  
**Inspiriert von:** ArangoDB AQL, mit Fokus auf Multi-Modell-Queries

---

## �berblick

**AQL (Advanced Query Language)** ist eine deklarative SQL-�hnliche Sprache f�r THEMIS, optimiert f�r hybride Queries �ber relationale, Graph-, Vektor- und Dokument-Daten.

**Design-Prinzipien:**
- ? **Einfach:** SQL-�hnliche Syntax f�r schnelle Adoption
- ? **M�chtig:** Multi-Modell-Support (Relational, Graph, Vector)
- ? **Optimierbar:** Automatische Index-Auswahl via Optimizer
- ? **Erweiterbar:** Schrittweise Erweiterung (Aggregationen, Joins, Subqueries)

---

## Syntax-�bersicht

### Grundstruktur

```aql
FOR variable IN collection
  [LET var = expression [, ...]]
  [FILTER condition]
  [SORT expression [ASC|DESC] [, ...]]
  [LIMIT offset, count]
  [RETURN expression]
```

**Execution-Reihenfolge:**
1. `FOR` - Iteration �ber Collection/Index
2. `FILTER` - Pr�dikat-Evaluation (mit Index-Nutzung)
3. `SORT` - Sortierung (mit Index-Nutzung wenn m�glich)
4. `LIMIT` - Pagination/Offset
5. `RETURN` - Projektion (Felder/Objekte/Arrays)

---

## MVP-Einschr�nkungen und Hinweise

Damit Erwartungen klar sind, hier die wichtigsten Begrenzungen des aktuellen MVP:

- OR-Operator: Vollst�ndig unterst�tzt �ber DNF-Konvertierung. FULLTEXT kann in OR-Ausdr�cken verwendet werden.
- Feld-zu-Feld Vergleiche (z. B. `u.city == o.city`) sind im Translator nicht allgemein erlaubt. Ein spezieller Join-Pfad erlaubt jedoch Gleichheits-Joins �ber genau zwei FOR-Klauseln (siehe Abschnitt �Einfache Joins (MVP)�).
- LET in FILTER: Falls einfache LET-Bindungen in FILTER vorkommen, werden diese vor der �bersetzung extrahiert (�pre-extracted�). Bei `explain: true` signalisiert der Plan dies mit `plan.let_pre_extracted = true`.
- Subqueries, OR, komplexe Ausdr�cke/Funktionen sind (noch) eingeschr�nkt und werden iterativ erweitert.

## Kern-Klauseln

### 1. FOR - Collection Iteration

```aql
FOR doc IN users
  RETURN doc

FOR u IN users
  FILTER u.age > 18
  RETURN u.name
```

**Syntax:**
- `variable` - Beliebiger Bezeichner (lowercase empfohlen)
- `collection` - Table-Name aus Storage-Layer

**Multi-Collection (Joins - MVP seit 31.10.2025):**

Themis unterst�tzt Nested-Loop-Joins �ber mehrere Collections via sequenzielle `FOR`-Klauseln:

```aql
FOR u IN users
  FOR o IN orders
    FILTER o.user_id == u._key
    RETURN {user: u.name, order: o.id}
```

**Join-Arten (MVP):**
- **Equality Join:** Verkn�pfung �ber `FILTER var1.field == var2.field`
- **Cross Product + Filter:** Kartesisches Produkt mit nachtr�glicher Filterung

**Beispiel - User-City-Join:**
```aql
FOR user IN users
  FOR city IN cities
    FILTER user.city_id == city._key
    RETURN {
      user_name: user.name,
      city_name: city.name,
      country: city.country
    }
```

**Performance-Hinweise:**
- ?? Nested-Loop kann **teuer** sein bei gro�en Datasets (O(n�m) Komplexit�t)
- ?? Empfehlung: FILTER-Bedingungen so spezifisch wie m�glich
- ?? Zuk�nftig: Hash-Join-Optimierung f�r gro�e Collections geplant
- ?? Verwende Indizes auf Join-Spalten (z.B. `city_id`) wo m�glich

**Multi-FOR Limitierungen (MVP):**
- Maximal 2-3 FOR-Klauseln empfohlen (Performance)
- Join-Bedingung muss in FILTER sein (keine impliziten Joins)
- Nur Equality-Joins (`==`) optimiert

**Best Practices für Multi-FOR Joins:**
- 📌 **Explizite Join-Prädikate:** Verwende immer klare Gleichheitsbedingungen zwischen FOR-Variablen
  ```aql
  FOR u IN users
    FOR o IN orders
      FILTER o.user_id == u._key  -- Expliziter Join
      RETURN {user: u.name, order: o.id}
  ```
- 📌 **Reihenfolge optimieren:** Platziere kleinere Collections zuerst für bessere Performance
- 📌 **Index-Nutzung:** Stelle sicher, dass Join-Felder (z.B. `user_id`, `_key`) indexiert sind
- 📌 **Filter kombinieren:** Nutze zusätzliche FILTER-Bedingungen, um Zwischenergebnisse zu reduzieren
  ```aql
  FOR u IN users
    FILTER u.active == true       -- Reduziert äußere Loop
    FOR o IN orders
      FILTER o.user_id == u._key AND o.status == "shipped"
      RETURN {user: u.name, order: o.id}
  ```

---

### 2. FILTER - Bedingungen

**Vergleichsoperatoren:**
```aql
FILTER doc.age == 25          // Gleichheit
FILTER doc.age != 25          // Ungleichheit
FILTER doc.age > 18           // Gr��er
FILTER doc.age >= 18          // Gr��er-Gleich
FILTER doc.age < 65           // Kleiner
FILTER doc.age <= 65          // Kleiner-Gleich
```

**Logische Operatoren:**
```aql
FILTER doc.age > 18 AND doc.city == "Berlin"
FILTER doc.status == "active" OR doc.status == "pending"
FILTER NOT doc.deleted
```

**OR-Operator (v1.3):**
```aql
// Einfaches OR
FILTER doc.status == "active" OR doc.status == "pending"

// OR mit AND kombiniert
FILTER (doc.status == "active" AND doc.age >= 30) OR doc.city == "Berlin"

// Komplexe DNF-Expansion
FILTER (doc.city == "Berlin" OR doc.city == "Munich") AND doc.status == "active"
```

**IN-Operator:**
```aql
FILTER doc.status IN ["active", "pending", "approved"]
FILTER doc.age IN [18, 21, 25, 30]
```

**String-Operatoren:**
```aql
FILTER LIKE(doc.name, "Max%")           // Prefix-Match
FILTER CONTAINS(doc.description, "AI")  // Substring
FILTER REGEX_TEST(doc.email, ".*@example\.com")
```

**Fulltext-Suche (BM25-ranked):**
```aql
FILTER FULLTEXT(doc.content, "machine learning")              // Multi-term search
FILTER FULLTEXT(doc.title, '"exact phrase"')                  // Phrase search (escaped quotes)
FILTER FULLTEXT(doc.abstract, "neural networks", 50)          // Custom limit (default: 1000)

// **NEU v1.3:** FULLTEXT + AND Kombinationen (Hybrid Search)
FILTER FULLTEXT(doc.content, "AI") AND doc.year >= 2023
FILTER FULLTEXT(doc.title, "neural") AND doc.category == "Research" AND doc.views >= 1000
FILTER doc.lang == "en" AND FULLTEXT(doc.abstract, "machine learning")  // Order flexible
```

**FULLTEXT-Funktionsdetails:**
- **Argumente:** `FULLTEXT(field, query [, limit])`
  - `field` - Spaltenname mit Fulltext-Index
  - `query` - Suchquery (Tokens mit AND-Logik, oder `"phrase"` f�r exakte Phrasen)
  - `limit` - Optional: Max. Ergebnisse (default 1000)
- **Ranking:** BM25-Scoring (k1=1.2, b=0.75)
- **Features:** Stemming (EN/DE), Stopwords, Normalization (Umlaute)
- **Hybrid Queries (v1.3):** 
  - ? `FULLTEXT(...) AND <predicates>` - Intersection-based (BM25 n structural filters)
  - ? `FULLTEXT(...) OR <expr>` - Noch nicht unterst�tzt (geplant v1.4)
- **Execution Strategy:** Fulltext-Scan zuerst (BM25-ranked), dann Intersection mit strukturellen Filtern
- **Siehe:** `docs/search/fulltext_api.md` f�r Index-Erstellung und Konfiguration

**NULL-Checks:**
```aql
FILTER doc.email != null
FILTER doc.phone == null
```

---

### 3. SORT - Sortierung

**Einfache Sortierung:**
```aql
SORT doc.age                  // ASC (default)
SORT doc.age DESC
SORT doc.created_at DESC
```

**Multi-Column-Sort:**
```aql
SORT doc.city ASC, doc.age DESC
SORT doc.priority DESC, doc.created_at ASC
```

**Index-Nutzung:**
- Range-Index auf `age` ? effiziente Sortierung
- Composite-Index `(city, age)` ? optimale Multi-Column-Sort

---

### 4. LIMIT - Pagination

**Syntax:**
```aql
LIMIT count                   // Erste N Ergebnisse
LIMIT offset, count           // Pagination
```

**Beispiele:**
```aql
LIMIT 10                      // Erste 10
LIMIT 20, 10                  // Zeilen 21-30 (Seite 3)
```

**Best Practices:**
- Immer mit `LIMIT` arbeiten (verhindert Full-Scans)
- F�r gro�e Offsets: Cursor-basierte Pagination bevorzugen

---

### 5. RETURN - Projektion

**Ganzes Dokument:**
```aql
RETURN doc
```

**Einzelne Felder:**
```aql
RETURN doc.name
RETURN doc.email
```

**Objekt-Konstruktion:**
```aql
RETURN {
  name: doc.name,
  age: doc.age,
  city: doc.city
}
```

**Berechnete Felder:**
```aql
RETURN {
  name: doc.name,
  age_in_months: doc.age * 12,
  full_address: CONCAT(doc.street, ", ", doc.city)
}
```

**Arrays:**
```aql
RETURN [doc.name, doc.age, doc.city]
```

Unterst�tzte Ausdr�cke im MVP:
- Literale: Zahl, String, Bool, null
- Variablen und Feldzugriff: `doc`, `doc.field`
- Objekt- und Array-Literale (verschachtelt m�glich)
- Einfache Let-Bindings pro Zeile (siehe LET)

---

## Erweiterte Features (Phase 1.1+)

### LET - Variable Binding (MVP seit 31.10.2025)

Bindet pro Iteration Werte an Variablen, die in `FILTER` und `RETURN` genutzt werden k�nnen.

**Einfaches Beispiel:**
```aql
FOR u IN users
  LET city_name = u.city
  RETURN {name: u.name, city: city_name}
```

**Berechnungen mit LET:**
```aql
FOR product IN products
  LET total_value = product.price * product.quantity
  FILTER total_value > 1000
  RETURN {
    product: product.name,
    value: total_value
  }
```

**Mehrere LET-Bindungen:**
```aql
FOR sale IN sales
  LET net = sale.amount
  LET tax = net * 0.19
  LET gross = net + tax
  RETURN {sale_id: sale._key, net, tax, gross}
```

**LET in Joins:**
```aql
FOR user IN users
  FOR order IN orders
    FILTER order.user_id == user._key
    LET full_name = CONCAT(user.first_name, " ", user.last_name)
    RETURN {customer: full_name, order_id: order._key}
```

**MVP-Einschr�nkungen:**
- Unterst�tzt sind aktuell einfache Ausdr�cke: Literale, Variablen, Feldzugriffe, Bin�roperationen (+, -, *, /), Objekt-/Array-Literale
- LETs werden sequenziell ausgewertet; sp�tere LETs k�nnen fr�here verwenden
- Komplexe Funktionen (CONCAT, SUBSTRING, etc.) in Entwicklung
- Explain: Wenn `LET`-Variablen in `FILTER` zu einfachen Gleichheitspr�dikaten vor der �bersetzung extrahiert wurden, enth�lt der Plan das Flag `plan.let_pre_extracted = true`

---

### COLLECT - Aggregationen (MVP seit 31.10.2025)

Gruppiert Ergebnisse und berechnet Aggregatfunktionen.

**Einfaches GROUP BY:**
```aql
FOR user IN users
  COLLECT city = user.city
  RETURN {city, count: LENGTH(1)}
```

**COUNT-Aggregation:**
```aql
FOR user IN users
  COLLECT city = user.city WITH COUNT INTO total
  RETURN {city, total}
```

**SUM-Aggregation:**
```aql
FOR sale IN sales
  COLLECT category = sale.category
  AGGREGATE total_revenue = SUM(sale.amount)
  RETURN {category, total_revenue}
```

**Mehrere Aggregationen:**
```aql
FOR order IN orders
  COLLECT status = order.status
  AGGREGATE 
    total_count = COUNT(),
    total_amount = SUM(order.amount),
    avg_amount = AVG(order.amount),
    min_amount = MIN(order.amount),
    max_amount = MAX(order.amount)
  RETURN {status, total_count, total_amount, avg_amount, min_amount, max_amount}
```

**COLLECT mit FILTER:**
```aql
FOR user IN users
  FILTER user.age > 18
  COLLECT city = user.city
  AGGREGATE adult_count = COUNT()
  RETURN {city, adult_count}
```

**Unterst�tzte Aggregatfunktionen (MVP):**
- `COUNT()` - Anzahl der Gruppen-Elemente
- `SUM(expr)` - Summe eines numerischen Felds
- `AVG(expr)` - Durchschnitt eines numerischen Felds
- `MIN(expr)` - Minimum eines Felds
- `MAX(expr)` - Maximum eines Felds

**Performance-Hinweise:**
- Hash-basiertes Grouping: O(n) Komplexit�t
- FILTER vor COLLECT reduziert Datenvolumen (wird automatisch optimiert)
- F�r sehr gro�e Gruppen: Memory-Nutzung beachten

**Geplante Erweiterungen:**
- `STDDEV(expr)` - Standardabweichung
- `VARIANCE(expr)` - Varianz
- `PERCENTILE(expr, n)` - n-tes Perzentil
- `UNIQUE(expr)` - Distinct Values

Hinweise (MVP):
- Gruppierung erfolgt �ber exakte String-Matches der Group-Keys
- Mehrere GROUP BY-Felder via Tuple-Keys geplant
- HAVING-Clause (Post-Aggregation-Filter) in Entwicklung

---

## HTTP-spezifische Parameter f�r Pagination

Bei Nutzung des HTTP-Endpunkts `POST /query/aql` k�nnen optionale Felder zur Pagination mitgegeben werden:

```json
{
  "query": "FOR u IN users SORT u.age ASC LIMIT 10 RETURN u",
  "use_cursor": true,
  "cursor": "<token-aus-previous-response>",
  "allow_full_scan": false
}
```

- `use_cursor` (bool): Aktiviert Cursor-basierte Pagination. Antwortformat enth�lt `{items, has_more, next_cursor, batch_size}`.
- `cursor` (string): Token aus `next_cursor` der vorherigen Seite. G�ltig nur in Kombination mit `use_cursor: true`.
- `allow_full_scan` (bool): Optionaler Fallback f�r kleine Datenmengen/Tests; f�r gro�e Daten wird Index-basierte Sortierung empfohlen.

Weitere Details siehe `docs/cursor_pagination.md`.

---

## Spezial-Queries

### Graph-Traversierung

```aql
FOR v, e, p IN 1..3 OUTBOUND "users/alice" edges
  FILTER v.active == true
  RETURN {vertex: v, edge: e, path: p}
```

**Traversal-Richtungen:**
- `OUTBOUND` - Ausgehende Kanten (Alice ? Bob)
- `INBOUND` - Eingehende Kanten (Alice ? Bob)
- `ANY` - Beide Richtungen

**Depth-Limits:**
- `1..1` - Nur direkte Nachbarn
- `1..3` - Bis zu 3 Hops
- `2..5` - Min 2, Max 5 Hops

---

### Vektor-�hnlichkeitssuche

```aql
FOR doc IN users
  NEAR(doc.embedding, @query_vector, 10)
  FILTER doc.age > 18
  RETURN {name: doc.name, similarity: SIMILARITY()}
```

**Funktionen:**
- `NEAR(field, vector, k)` - k-NN-Suche
- `SIMILARITY()` - Aktueller Similarity-Score (0.0 - 1.0)

**Metriken:**
```aql
NEAR(doc.embedding, @query_vector, 10, "cosine")    // Cosine Similarity
NEAR(doc.embedding, @query_vector, 10, "euclidean") // L2-Distance
```

---

### Geo-Queries

```aql
FOR doc IN locations
  GEO_DISTANCE(doc.lat, doc.lon, 52.52, 13.405) < 5000
  RETURN {name: doc.name, distance: GEO_DISTANCE(doc.lat, doc.lon, 52.52, 13.405)}
```

**Funktionen:**
- `GEO_DISTANCE(lat1, lon1, lat2, lon2)` - Haversine-Distanz (Meter)
- `GEO_BOX(lat, lon, minLat, maxLat, minLon, maxLon)` - Bounding-Box-Check

---

### Fulltext-Suche (BM25)

**Einfache Multi-Term-Suche:**
```aql
FOR doc IN articles
  FILTER FULLTEXT(doc.content, "machine learning")
  LIMIT 10
  RETURN {title: doc.title, content: doc.content}
```

**Sortierung nach Score (BM25):**
```aql
FOR doc IN articles
  FILTER FULLTEXT(doc.content, "neural networks")
  SORT BM25(doc) DESC
  LIMIT 10
  RETURN {title: doc.title, score: BM25(doc)}
```

**Phrasensuche:**
```aql
FOR doc IN articles
  FILTER FULLTEXT(doc.abstract, '"neural networks"')
  LIMIT 20
  RETURN doc
```

**Mit benutzerdefiniertem Limit:**
```aql
FOR doc IN research_papers
  FILTER FULLTEXT(doc.content, "deep learning transformer", 50)
  RETURN {
    title: doc.title,
    authors: doc.authors,
    year: doc.year
  }
```

**Volltext + strukturierte Filter kombiniert:**
```aql
FOR doc IN articles
  FILTER FULLTEXT(doc.content, "AI") AND doc.year >= 2023
  LIMIT 10
  RETURN doc
```

**Volltext + OR-Kombinationen:**
```aql
FOR doc IN articles
  FILTER FULLTEXT(doc.content, "machine learning") OR doc.year < 2000
  LIMIT 10
  RETURN {title: doc.title, year: doc.year}
```

**Hinweise:**
- BM25-Ranking: Ergebnisse sind automatisch nach Relevanz sortiert (höchster Score zuerst)
- Score aus AQL zugreifbar: `BM25(doc)` liefert den Score für das aktuelle Dokument
- Index erforderlich: `POST /api/index/fulltext` (siehe `docs/search/fulltext_api.md`)
- Stemming/Stopwords/Normalisierung: Per Index konfigurierbar (EN/DE)
- Score-Ausgabe: Verf�gbar in RETURN via `FULLTEXT_SCORE()` (nur wenn ein `FULLTEXT(...)`-Filter in der Query vorhanden ist)
- AND/OR-Kombinationen: `FULLTEXT(...) AND ...` und `FULLTEXT(...) OR ...` vollständig produktiv

**Index-Erstellung (HTTP API):**
```json
POST /api/index/fulltext
{
  "table": "articles",
  "column": "content",
  "stemming_enabled": true,
  "language": "en",
  "stopwords_enabled": true,
  "normalize_german": false
}
```

---

### Fulltext-Suche

```aql
FOR doc IN articles
  FILTER FULLTEXT(doc.content, "machine learning AI")
  LIMIT 10
  RETURN {title: doc.title, score: FULLTEXT_SCORE()}
```

**Funktionen:**
- `FULLTEXT(field, query [, limit])` - Tokenisierte Suche mit optionalem Limit (Kandidatenzahl)
- `FULLTEXT_SCORE()` - Relevanz-Score (BM25) des aktuellen Treffers; nur g�ltig, wenn ein `FULLTEXT(...)`-Filter vorhanden ist

---

## Einfache Joins (MVP)

Unterst�tzt werden Equality-Joins �ber genau zwei `FOR`-Klauseln mit einem Gleichheitspr�dikat zwischen Variablen.

```aql
FOR u IN users
  FOR o IN orders
  FILTER u._key == o.user_id
  RETURN u
```

Eigenschaften und Einschr�nkungen (MVP):
- Genau zwei `FOR`-Klauseln; ein Equality-Pr�dikat `var1.field == var2.field` in `FILTER`.
- Zus�tzliche `FILTER` pro Seite sind erlaubt und werden vor dem Join angewendet.
- `RETURN` muss aktuell eine der Variablen zur�ckgeben (typisch `u` oder `o`).
- `LIMIT` wird nach dem Join angewendet. `SORT` im Join-Pfad ist derzeit nicht unterst�tzt.
- `explain: true` liefert einen Plan, der den Join-Pfad ausweist; bei LET-Pre-Extraction wird `plan.let_pre_extracted = true` gesetzt.

Projektion mit LET im Join-Kontext:

```aql
FOR u IN users
  FOR o IN orders
  FILTER u._key == o.user_id
  LET info = { user: u.name, order: o.id }
  RETURN info
```

Hinweis: Komplexe Projektionen k�nnen je nach Datenvolumen h�here Kosten verursachen; nutze `LIMIT` wo sinnvoll.

---

## Funktionen & Operatoren

### String-Funktionen

```aql
CONCAT(str1, str2, ...)       // "Hello" + " " + "World"
LOWER(str)                     // "HELLO" ? "hello"
UPPER(str)                     // "hello" ? "HELLO"
SUBSTRING(str, offset, length) // "Hello"[1:4] ? "ell"
LENGTH(str)                    // "Hello" ? 5
TRIM(str)                      // "  Hello  " ? "Hello"
```

### Numeric-Funktionen

```aql
ABS(num)                       // |-5| ? 5
CEIL(num) / FLOOR(num)         // 3.7 ? 4 / 3
ROUND(num, decimals)           // 3.14159, 2 ? 3.14
SQRT(num)                      // v16 ? 4
POW(base, exp)                 // 2^8 ? 256
```

### Aggregations (in COLLECT)

```aql
COUNT()                        // Anzahl Zeilen
SUM(expr)                      // Summe
AVG(expr)                      // Durchschnitt
MIN(expr) / MAX(expr)          // Minimum/Maximum
STDDEV(expr)                   // Standardabweichung
VARIANCE(expr)                 // Varianz
```

### Type-Checks

```aql
IS_NULL(value)
IS_NUMBER(value)
IS_STRING(value)
IS_ARRAY(value)
IS_OBJECT(value)
```

### Content/File-Funktionen

Ermöglichen Zugriff auf Metadaten und Chunks von ingestierten Dateien über die Content-Pipeline.

```aql
CONTENT_META(document_id)      // Gibt Metadaten-Objekt zurück (name, size, mimeType, etc.)
CONTENT_CHUNKS(document_id)    // Gibt Array von Chunks zurück (chunk_id, text, embedding, etc.)
```

**Beispiel - Dokument-Metadaten abfragen:**
```aql
FOR doc IN documents
  FILTER doc.status == "indexed"
  LET meta = CONTENT_META(doc._key)
  RETURN {
    id: doc._key,
    filename: meta.filename,
    size_bytes: meta.size,
    mime_type: meta.mimeType,
    pages: meta.pages
  }
```

**Beispiel - Chunks mit Volltext-Suche:**
```aql
FOR doc IN documents
  FILTER FULLTEXT(doc.content, "machine learning")
  LET chunks = CONTENT_CHUNKS(doc._key)
  RETURN {
    document: doc.title,
    chunk_count: LENGTH(chunks),
    first_chunk: chunks[0].text
  }
```

**Beispiel - Vektor-Suche über Chunks:**
```aql
FOR doc IN documents
  LET chunks = CONTENT_CHUNKS(doc._key)
  LET similar = VECTOR_SEARCH("chunks", @query_embedding, 5)
  FILTER doc._key IN similar
  RETURN {
    document: doc.title,
    relevant_chunks: chunks
  }
```

**MVP-Hinweise:**
- `CONTENT_META` und `CONTENT_CHUNKS` sind Funktionen, die vom Parser als `FunctionCallExpr` erkannt werden
- Engine-Integration erfordert Content-Storage-API (siehe `docs/content_architecture.md`)
- Chunks enthalten: `chunk_id`, `text`, `embedding`, `page_number`, `bbox` (optional)

---

## Beispiel-Queries

### 1. Einfache Filterung

```aql
FOR user IN users
  FILTER user.age > 18 AND user.city == "Berlin"
  SORT user.created_at DESC
  LIMIT 10
  RETURN {
    name: user.name,
    email: user.email,
    age: user.age
  }
```

**Optimizer:**
- Nutzt Composite-Index `(city, age)` falls vorhanden
- Fallback: Equality-Index `city` + Full-Scan-Filter `age`

---

### 2. Geo-Proximity-Search

```aql
FOR loc IN restaurants
  FILTER GEO_DISTANCE(loc.lat, loc.lon, 52.52, 13.405) < 2000
  FILTER loc.rating >= 4.0
  SORT GEO_DISTANCE(loc.lat, loc.lon, 52.52, 13.405) ASC
  LIMIT 5
  RETURN {
    name: loc.name,
    rating: loc.rating,
    distance: GEO_DISTANCE(loc.lat, loc.lon, 52.52, 13.405)
  }
```

**Optimizer:**
- Nutzt Geo-Index f�r Bounding-Box-Scan
- Post-Filter f�r exakte Distanz-Berechnung

---

### 3. Vektor-Suche mit Filter

```aql
FOR product IN products
  NEAR(product.embedding, @query_vector, 20)
  FILTER product.price < 100.0 AND product.in_stock == true
  SORT SIMILARITY() DESC
  LIMIT 10
  RETURN {
    name: product.name,
    price: product.price,
    similarity: SIMILARITY()
  }
```

**Pre-Filtering vs Post-Filtering:**
- Pre-Filter: Bitset f�r `price < 100 AND in_stock == true` ? k-NN
- Post-Filter: k-NN (20) ? Filter ? Top-10

---

### 4. Aggregationen

```aql
FOR order IN orders
  FILTER order.created_at >= "2025-01-01"
  COLLECT city = order.city
  AGGREGATE 
    total_revenue = SUM(order.amount),
    avg_order = AVG(order.amount),
    order_count = COUNT()
  SORT total_revenue DESC
  LIMIT 10
  RETURN {
    city,
    total_revenue,
    avg_order,
    order_count
  }
```

---

### 5. Graph-Traversierung

```aql
FOR vertex, edge, path IN 1..3 OUTBOUND "users/alice" friendships
  FILTER vertex.active == true
  RETURN {
    friend: vertex.name,
    connection_type: edge.type,
    path_length: LENGTH(path.edges)
  }
```

---

## Query-Execution & Optimizer

### Explain-Plan

```json
POST /query/aql
{
  "query": "FOR u IN users FILTER u.age > 18 SORT u.created_at DESC LIMIT 10",
  "explain": true
}
```

**Response:**
```json
{
  "plan": {
    "mode": "range_aware",
    "order": [
      { "column": "created_at", "value": "DESC" }
    ],
    "estimates": [
      { "column": "age", "value": "> 18", "estimatedCount": 1200, "capped": false }
    ],
    "let_pre_extracted": true
  }
}
```

### Index-Hints (sp�ter)

```aql
FOR doc IN users USE INDEX idx_age_city
  FILTER doc.age > 18
  RETURN doc
```

---

## AST-Struktur (Internal)

```cpp
// AST-Node-Typen
enum class ASTNodeType {
    ForNode,          // FOR variable IN collection
    FilterNode,       // FILTER condition
    SortNode,         // SORT expr [ASC|DESC]
    LimitNode,        // LIMIT offset, count
    ReturnNode,       // RETURN expression
    
    // Expressions
    BinaryOp,         // ==, !=, >, <, >=, <=, AND, OR
    UnaryOp,          // NOT, -
    FunctionCall,     // CONCAT, SUM, etc.
    FieldAccess,      // doc.field
    Literal,          // "string", 123, true, null
    Variable          // doc, user, etc.
};

// Beispiel-AST f�r: FOR u IN users FILTER u.age > 18 RETURN u.name
ForNode {
    variable: "u",
    collection: "users",
    
    filter: FilterNode {
        condition: BinaryOp {
            op: ">",
            left: FieldAccess("u", "age"),
            right: Literal(18)
        }
    },
    
    return_expr: ReturnNode {
        expression: FieldAccess("u", "name")
    }
}
```

---

## Implementierungs-Phasen

### Phase 1 (MVP - Woche 1-2):
- ? FOR, FILTER (Equality, Range, IN), SORT, LIMIT, RETURN
- ? Parser (PEGTL)
- ? AST ? QueryEngine-Translation
- ? HTTP-Endpoint `/query/aql`
- ? Unit-Tests

### Phase 2 (Woche 3-4):
- LET (Variable Binding)
- COLLECT (Aggregationen: COUNT, SUM, AVG)
- String-/Numeric-Funktionen
- Explain-Plan-Integration

### Phase 3 (Woche 5-6):
- Graph-Traversierung (FOR v, e, p IN ... OUTBOUND)
- Vektor-Suche (NEAR, SIMILARITY)
- Geo-Queries (GEO_DISTANCE, GEO_BOX)
- Fulltext (FULLTEXT, BM25)

### Phase 4 (sp�ter):
- Joins (Multi-Collection)
- Subqueries
- Transactions (BEGIN, COMMIT, ROLLBACK)
- INSERT, UPDATE, DELETE via AQL

---

## Performance-�berlegungen

**Index-Nutzung:**
- FILTER mit `==` ? Equality-Index
- FILTER mit `>`, `<` ? Range-Index
- FILTER mit `IN` ? Batch-Lookup
- SORT ? Range-Index (wenn vorhanden)

**Optimizer-Strategien:**
- **Filter-Pushdown:** FILTER vor SORT (reduziert Sortier-Kosten)
- **Index-Auswahl:** Kleinster gesch�tzter Index zuerst
- **Short-Circuit:** LIMIT fr�h anwenden (z.B. Top-K)

**Vermeiden:**
- Full-Table-Scans ohne LIMIT
- Sortierung ohne Index auf gro�en Datasets
- Aggregationen ohne COLLECT (ineffizient)

---

## Kompatibilit�t & Erweiterungen

**ArangoDB AQL:**
- �hnliche Syntax (FOR, FILTER, SORT, LIMIT, RETURN)
- Unterschiede: THEMIS nutzt natives MVCC, kein `_key` zwingend

**SQL-Vergleich:**
```sql
-- SQL
SELECT name, age FROM users WHERE age > 18 ORDER BY created_at DESC LIMIT 10;

-- AQL
FOR user IN users
  FILTER user.age > 18
  SORT user.created_at DESC
  LIMIT 10
  RETURN {name: user.name, age: user.age}
```

**Vorteile AQL:**
- Multi-Modell (Graph, Vector, Geo in einer Query)
- Explizite Execution-Reihenfolge (leichter zu optimieren)
- Schemalos (flexible Felder)

---

## Fehlerbehandlung

**Syntax-Errors:**
```json
{
  "error": "Syntax error at line 2, column 10: Expected 'IN' after variable name",
  "query": "FOR user users FILTER ...",
  "line": 2,
  "column": 10
}
```

**Semantic-Errors:**
```json
{
  "error": "Collection 'userz' does not exist (did you mean 'users'?)",
  "query": "FOR u IN userz RETURN u"
}
```

**Runtime-Errors:**
```json
{
  "error": "Division by zero in expression: amount / quantity",
  "entity_key": "orders:12345"
}
```

---

## Referenz-Links

- **Parser:** PEGTL (https://github.com/taocpp/PEGTL)
- **Inspiration:** ArangoDB AQL (https://www.arangodb.com/docs/stable/aql/)
- **Optimizer:** docs/query_optimizer.md
- **Index-Typen:** docs/indexes.md

---

**Status:** ? Syntax-Definition vollst�ndig  
**N�chster Schritt:** Parser-Implementation mit PEGTL

## Vollst�ndige Beispiele (MVP Features)

### Beispiel 1: User-City-Join mit Aggregation

**Szenario:** Finde alle User in ihren St�dten, gruppiert nach Land mit Z�hlung:

```aql
FOR user IN users
  FOR city IN cities
    FILTER user.city_id == city._key
    COLLECT country = city.country
    AGGREGATE user_count = COUNT()
    RETURN {country, user_count}
```

**Ergebnis:**
```json
[
  {"country": "Germany", "user_count": 125},
  {"country": "France", "user_count": 87},
  {"country": "Spain", "user_count": 43}
]
```

---

### Beispiel 2: Sales-Analyse mit LET und Aggregation

**Szenario:** Berechne Netto/Brutto-Ums�tze pro Kategorie:

```aql
FOR sale IN sales
  LET net = sale.amount
  LET tax = net * 0.19
  LET gross = net + tax
  COLLECT category = sale.category
  AGGREGATE 
    total_net = SUM(net),
    total_gross = SUM(gross),
    count = COUNT()
  RETURN {
    category,
    total_net,
    total_gross,
    avg_sale: total_net / count,
    count
  }
```

---

### Beispiel 3: Top-10 St�dte nach User-Count

**Szenario:** H�ufigste St�dte finden:

```aql
FOR user IN users
  COLLECT city_id = user.city_id WITH COUNT INTO user_count
  SORT user_count DESC
  LIMIT 10
  RETURN {city_id, user_count}
```

---

## Performance-Best-Practices (MVP)

### 1. JOIN-Optimierung

** Schlecht:** Kartesisches Produkt ohne Filter
** Gut:** Spezifische FILTER-Bedingungen, LIMIT verwenden

### 2. LET f�r Wiederverwendung

Berechnungen einmal durchf�hren, mehrfach nutzen:

```aql
FOR sale IN sales
  LET net = sale.amount
  LET tax = net * 0.19
  RETURN {net, tax, gross: net + tax}
```

### 3. FILTER vor COLLECT

Datenvolumen reduzieren bevor gruppiert wird.

---

## Implementation-Status (03.11.2025)

| Feature | Status | Notes |
|---------|--------|-------|
| **FOR** (Single) | ? Production | Vollst�ndig optimiert |
| **FOR** (Multi/Join) | ? MVP | Nested-Loop, Hash-Join geplant |
| **FILTER** | ? Production | Equality + Range + AND + OR + FULLTEXT |
| **OR-Operator** | ? Production | DNF-Konvertierung, Index-Merge |
| **FULLTEXT()** | ? Production | BM25-Ranking, Stemming, Phrasen |
| **FULLTEXT + AND** | ? Production | Hybrid Queries (BM25 n structural filters) |
| **SORT** | ? Production | Index-optimiert |
| **LIMIT** | ? Production | Offset + Count |
| **RETURN** | ? Production | Field/Object/Array |
| **LET** | ? MVP | Basis-Expressions, Arithmetik |
| **COLLECT** | ? MVP | Hash-Grouping, COUNT/SUM/AVG/MIN/MAX |
| **FULLTEXT + OR** | ?? Planned | Per-Disjunct FULLTEXT execution |
| **FULLTEXT_SCORE()** | ?? Planned | Score in RETURN-Expression |
| **Subqueries** | ?? Planned | Phase 1.4 |

---

**Dokumentations-Version:** 1.3 (03. November 2025)  
**Letzte Aktualisierung:** FULLTEXT + AND Hybrid Queries implementiert (13 Tests PASSED)



---

## File: docs\features\features_overview.md

# ThemisDB - Vollständige Features Liste

**Version:** 1.0.0  
**Stand:** 6. April 2026  
**Status-Legende:** ✅ Production-Ready | 🔧 Beta | 📋 Geplant

---

## 🎯 Übersicht

ThemisDB ist eine **Multi-Model Database** mit ACID-Garantien, die relationale, Graph-, Vektor- und Dokument-Datenmodelle in einem einheitlichen System vereint. Basierend auf RocksDB (LSM-Tree) mit erweiterter Sicherheits- und Compliance-Architektur.

**Kernmerkmale:**
- 🔒 **ACID-Transaktionen** mit MVCC (Snapshot Isolation)
- 🔍 **Multi-Model Support** (Relational, Graph, Vector, Document)
- 🚀 **High-Performance** (45K writes/s, 120K reads/s)
- 🛡️ **Enterprise Security** (TLS 1.3, RBAC, Verschlüsselung, Audit)
- 📊 **Advanced Query Language** (AQL mit Graph-Traversals, Aggregationen)
- 🌐 **Production-Ready** (85%+ Test Coverage, Comprehensive Monitoring)

---

## 📦 Storage & Data Model

### Canonical Storage Layer ✅
**Status:** Production-Ready | **Docs:** [`docs/architecture/base_entity.md`](docs/architecture/base_entity.md)

- **Base Entity** - Unified JSON/Binary blob storage für alle Datenmodelle
- **RocksDB TransactionDB** - LSM-Tree mit ACID-Garantien
- **VelocyPack/Bincode** - High-Performance Serialization
- **Multi-Format Support** - JSON, Binary, Custom Formats
- **Fast Field Extraction** - Optimierte Parsing-Pipeline

**Key Features:**
- Atomic updates über alle Index-Layer
- Write-optimiert (append-only LSM-Tree)
- Configurable compression (LZ4, ZSTD, Snappy)
- BlobDB support für große Objekte

### Multi-Model Mapping ✅
**Status:** Production-Ready

| Modell | Logical Entity | Physical Storage | Key Format |
|--------|----------------|------------------|------------|
| **Relational** | Row | (PK, Blob) | `table:pk` |
| **Document** | JSON Document | (PK, Blob) | `collection:pk` |
| **Graph (Nodes)** | Vertex | (PK, Blob) | `node:pk` |
| **Graph (Edges)** | Edge | (PK, Blob) | `edge:pk` |
| **Vector** | Embedding Object | (PK, Blob) | `object:pk` |

### External Blob Storage ✅
**Status:** Production-Ready | **Docs:** [`docs/storage/CLOUD_BLOB_BACKENDS.md`](docs/storage/CLOUD_BLOB_BACKENDS.md)

- **Filesystem Backend** - Hierarchische lokale Speicherung
- **WebDAV/ActiveDirectory** - SharePoint & Enterprise Integration
- **S3 Compatible** - Interface ready (AWS, MinIO, etc.)
- **Azure Blob** - Interface ready
- **Threshold-basierte Selektion** - Automatische Backend-Wahl
- **SHA256 Content Hashing** - Deduplizierung & Integrität

---

## 🔍 Indexing & Query

### Secondary Indexes ✅
**Status:** Production-Ready | **Docs:** [`docs/features/indexes.md`](docs/features/indexes.md)

**Index-Typen:**
- ✅ **Single-Column** - Equality-basierte Suche
- ✅ **Composite** - Multi-Spalten-Indizes
- ✅ **Range** - Bereichsabfragen (>, <, BETWEEN)
- ✅ **Sparse** - Nur für existierende Werte
- ✅ **Geo-Spatial** - R-Tree für räumliche Suche
- ✅ **TTL (Time-To-Live)** - Automatisches Expiration
- ✅ **Full-Text** - Inverted Index für Textsuche

**Features:**
- Automatic index maintenance mit MVCC
- Thread-safe operations
- Index statistics & cardinality estimation
- Rebuild & reindex operations
- Performance metrics

**API:**
```json
POST /index/create
{ "table": "users", "column": "age", "type": "range" }
```

### Graph Projections ✅
**Status:** Production-Ready | **Docs:** [`docs/features/recursive_path_queries.md`](docs/features/recursive_path_queries.md)

**Index-Strukturen:**
- **Outdex** - Ausgehende Kanten (`graph:out:node:edge`)
- **Indeg** - Eingehende Kanten (`graph:in:node:edge`)
- **Type-Aware** - Server-side Kantentyp-Filterung
- **Property Storage** - Edge properties mit Gewichtung

**Algorithmen:**
- ✅ **BFS (Breadth-First Search)** - Tiefenbegrenzte Traversierung
- ✅ **Dijkstra** - Kürzeste Pfade (gewichtet)
- ✅ **A*** - Heuristische Pfadsuche
- ✅ **Recursive Path Queries** - Variable Tiefe (1-N hops)
- ✅ **Temporal Graph Queries** - Zeitbereichs-Filter

**Path Constraints:**
- Last-Edge Constraints
- No-Vertex Repetition
- Type-based Pruning

### Vector Search ✅
**Status:** Production-Ready | **Docs:** [`docs/features/vector_ops.md`](docs/features/vector_ops.md)

**HNSW Index:**
- ✅ **Persistent HNSW** - Crash-safe, transactional
- ✅ **Distance Metrics** - L2, Cosine, Dot Product
- ✅ **Batch Operations** - Insert 500-1000 vectors
- ✅ **KNN Search** - Approximate Nearest Neighbors
- ✅ **Configurable Parameters** - M, efConstruction, efSearch

**Performance:**
- Throughput: 1,800 queries/s (CPU)
- Latency: p50 = 0.55ms, p99 = 2.1ms
- GPU Acceleration planned (50K+ q/s)

**API:**
```json
POST /vector/search
{ "vector": [0.1, 0.2, ...], "k": 10, "metric": "cosine" }
```

---

## 🔎 Query Language (AQL)

### Advanced Query Language ✅
**Status:** Production-Ready | **Docs:** [`docs/aql/syntax.md`](docs/aql/syntax.md)

**Syntax-Konstrukte:**
- ✅ **FOR/FILTER/SORT/LIMIT/RETURN** - SQL-ähnliche Semantik
- ✅ **Graph Traversals** - `FOR v,e,p IN 1..3 OUTBOUND start`
- ✅ **COLLECT/GROUP BY** - Aggregationen (COUNT, SUM, AVG, MIN, MAX)
- ✅ **Subqueries** - Nested queries mit IN/ALL/ANY
- ✅ **Pattern Matching** - Graph pattern expressions
- ✅ **Temporal Filters** - Zeitbereichs-Abfragen

**Query Optimizer:**
- ✅ **Cost-Based** - Index selection, predicate ordering
- ✅ **EXPLAIN** - Execution plan visualization
- ✅ **PROFILE** - Runtime metrics & bottleneck analysis
- ✅ **Parallelization** - Intel TBB task-based execution

**Metriken (PROFILE):**
- `edges_expanded` - Graph traversal expansion rate
- `prune_last_level` - Pruning effectiveness
- `index_scan_cost` - Index operation costs

### Hybrid Search ✅
**Status:** Production-Ready (Phase 4) | **Docs:** [`docs/apis/hybrid_search_api.md`](docs/apis/hybrid_search_api.md)

**Pre-Filtering:**
- Relational predicate → Candidate bitset
- Vector HNSW search über filtered candidates
- Graph expansion mit constraints

**Post-Filtering:**
- Global vector search → Top-K results
- Relational/Graph filters auf result set

**Use Cases:**
- "Finde ähnliche Dokumente (vector) aus Abteilung X (relational) mit Tag Y (graph)"
- Fusion von Similarity, Metadata und Relationships

---

## 🔒 Security & Compliance

### Enterprise Security Stack ✅
**Status:** Production-Ready (85% Coverage) | **Docs:** [`docs/security/implementation_summary.md`](docs/security/implementation_summary.md)

#### TLS/SSL Hardening ✅
- **TLS 1.3** default (TLS 1.2 fallback)
- **Strong Ciphers** - ECDHE-RSA-AES256-GCM-SHA384, ChaCha20-Poly1305
- **mTLS** - Client certificate verification
- **HSTS Headers** - `max-age=31536000; includeSubDomains`
- **Certificate Pinning** - SHA256 fingerprints für HSM/TSA

#### Rate Limiting & DoS Protection ✅
- **Token Bucket Algorithm** - 100 req/min default
- **Per-IP & Per-User Limits** - Configurable thresholds
- **HTTP 429 Responses** - Retry-After headers
- **Metrics** - Real-time monitoring

#### Input Validation ✅
- **JSON Schema Validation** - Strict type checking
- **AQL Injection Prevention** - Parameterized queries
- **Path Traversal Protection** - Sanitized file paths
- **Max Body Size** - 10MB default limit

#### Security Headers ✅
- `X-Frame-Options: DENY`
- `X-Content-Type-Options: nosniff`
- `X-XSS-Protection: 1; mode=block`
- `Content-Security-Policy` - Configurable
- **CORS Whitelisting** - Strict origin control

### RBAC (Role-Based Access Control) ✅
**Status:** Production-Ready | **Docs:** [`docs/security/implementation_summary.md`](docs/security/implementation_summary.md)

**Role Hierarchy:**
```
admin → operator → analyst → readonly
```

**Permissions:**
- `data:read`, `data:write`, `data:delete`
- `keys:rotate`, `keys:view`
- `audit:view`, `audit:export`
- `config:modify`
- Wildcard support: `*:*`

**Features:**
- JSON/YAML configuration
- User-role mapping store
- Resource-based access control

### Encryption ✅
**Status:** Production-Ready | **Docs:** [`docs/security/column_encryption.md`](docs/security/column_encryption.md)

#### Field-Level Encryption ✅
- **AES-256-GCM** - Authenticated encryption
- **Transparent Operations** - App-level abstraction
- **Schema-Based** - Selective field encryption
- **Index Compatibility** - Encrypted fields können indexiert werden

**Key Management:**
- ✅ **MockKeyProvider** - Development/Testing
- ✅ **HSMKeyProvider** - PKCS#11 HSM integration
- ✅ **VaultKeyProvider** - HashiCorp Vault

**Key Rotation:**
- ✅ **Lazy Re-Encryption** - Zero-downtime rotation
- ✅ **Transparent Migration** - Gradual re-encryption
- ✅ **Audit Trail** - Rotation tracking

**API:**
```json
PUT /config/encryption-schema
{
  "fields": {
    "ssn": { "encrypted": true, "algorithm": "AES-256-GCM" }
  }
}
```

#### Audit Log Encryption ✅
- **Encrypt-then-Sign** - Confidentiality + Integrity
- **Hash Chain** - Tamper-detection (Merkle-like)
- **PKI Signatures** - RSA-SHA256 (eIDAS-konform)

### Secrets Management ✅
**Status:** Production-Ready | **Docs:** [`docs/security/implementation_summary.md`](docs/security/implementation_summary.md)

**HashiCorp Vault Integration:**
- ✅ **KV v2 Engine** - Secret storage
- ✅ **AppRole Auth** - Service authentication
- ✅ **Auto Token Renewal** - Lease management
- ✅ **Rotation Callbacks** - Dynamic secret updates
- ✅ **Environment Fallback** - Development mode

### Audit Logging ✅
**Status:** Production-Ready | **Docs:** [`docs/features/audit_logging.md`](docs/features/audit_logging.md)

**Event Types (65+):**
- `LOGIN_FAILED`, `PRIVILEGE_ESCALATION_ATTEMPT`
- `DATA_ACCESS`, `DATA_MODIFIED`, `DATA_DELETED`
- `KEY_ROTATED`, `ENCRYPTION_FAILED`
- `UNAUTHORIZED_ACCESS`, `SCHEMA_CHANGED`

**Features:**
- ✅ **Severity Levels** - HIGH, MEDIUM, LOW
- ✅ **SIEM Integration** - Syslog RFC 5424, Splunk HEC
- ✅ **Tamper-Proof** - Hash chain verification
- ✅ **Retention Policies** - Auto-archival & purging

**API:**
```bash
GET /audit/logs?severity=HIGH&from=2025-01-01
```

### Compliance ✅
**Status:** Production-Ready | **Docs:** [`docs/features/compliance.md`](docs/features/compliance.md)

**GDPR/DSGVO:**
- ✅ Recht auf Löschung (Deletion API)
- ✅ Recht auf Auskunft (Data export)
- ✅ Pseudonymisierung (Field encryption)
- ✅ Data classification (4 Stufen: offen/vs-nfd/geheim/streng_geheim)

**SOC 2 Controls:**
- ✅ CC6.1 - Access Control (RBAC)
- ✅ CC6.7 - Audit Logs
- ✅ CC7.2 - Change Management

**HIPAA:**
- ✅ §164.312(a)(1) - Access Control
- ✅ §164.312(e)(1) - Transmission Security (TLS 1.3)

**PII Detection (7 Typen):**
- ✅ Email, Phone, SSN, Credit Card, IBAN, IP, URL
- ✅ Automatic pattern recognition
- ✅ YAML-configurable rules

### Multi-Tenancy ✅
**Status:** Production-Ready | **Docs:** [`docs/features/multi_tenancy.md`](docs/features/multi_tenancy.md)

**Features:**
- ✅ **Tenant Lifecycle** - Create, Update, Delete, Enable/Disable
- ✅ **Tenant Identification** - Header-based (`X-Tenant-ID`), Path-based
- ✅ **Resource Quotas** - Storage, Documents, Collections, Queries, Connections
- ✅ **Rate Limiting** - Per-tenant requests/sec with burst control
- ✅ **Feature Flags** - GPU, Vector, Graph, Timeseries, Geo, Full-Text
- ✅ **Encryption** - Tenant-specific keys, optional mandatory encryption
- ✅ **Usage Tracking** - Storage, Documents, Requests, Bandwidth
- ✅ **Billing Integration** - Prometheus metrics export
- ✅ **Data Isolation** - Complete tenant separation

---

## 📊 Time-Series & Analytics

### Time-Series Engine ✅
**Status:** Production-Ready | **Docs:** [`docs/features/time_series.md`](docs/features/time_series.md)

**Features:**
- ✅ **Gorilla Compression** - 10-20x compression ratio
- ✅ **Continuous Aggregates** - Pre-computed rollups (360-3600x speedup)
- ✅ **Retention Policies** - Auto-expiration
- ✅ **Downsampling** - Multi-resolution storage
- ✅ **Aggregate Scheduler** - Automatic background refresh
- ✅ **Query Optimizer** - Cost-based aggregate rewriting

**Performance:**
- 22/22 tests passing
- Sub-millisecond query latency (with aggregates)
- Efficient storage for metrics/logs

### OLAP Analytics ✅
**Status:** Production-Ready | **Docs:** [`docs/features/olap_analytics.md`](docs/features/olap_analytics.md)

**Features:**
- ✅ **Aggregations** - COUNT, SUM, AVG, MIN, MAX, STDDEV, VARIANCE, MEDIAN, PERCENTILE
- ✅ **Grouping Operators** - CUBE, ROLLUP, GROUPING SETS
- ✅ **Window Functions** - PARTITION BY, ORDER BY, ROWS/RANGE frames
- ✅ **Columnar Store** - Vektorisierte Aggregationen
- ✅ **Materialized Views** - Pre-computed aggregations

**Window Functions:**
- ROW_NUMBER, RANK, DENSE_RANK
- LAG, LEAD
- FIRST_VALUE, LAST_VALUE
- NTILE

### Temporal Graphs ✅
**Status:** Production-Ready | **Docs:** [`docs/features/temporal_graphs.md`](docs/features/temporal_graphs.md)

**Features:**
- ✅ **Temporal Filters** - `valid_from`, `valid_to`
- ✅ **Snapshot Queries** - Point-in-time graph state
- ✅ **Time-Range Aggregations** - Edge property rollups
- ✅ **Type-Aware Traversal** - Filter by edge type + timestamp

**API:**
```cpp
aggregateEdgePropertyInTimeRange(
  "user123", "FOLLOWS", "timestamp",
  from_ts, to_ts, AggregationType::COUNT
)
```

---

## 🔄 Transactions & Consistency

### MVCC (Multi-Version Concurrency Control) ✅
**Status:** Production-Ready (27/27 tests) | **Docs:** [`docs/architecture/mvcc_design.md`](docs/architecture/mvcc_design.md)

**Features:**
- ✅ **Snapshot Isolation** - Consistent reads
- ✅ **Write-Write Conflict Detection** - Automatic rollbacks
- ✅ **Atomic Updates** - Across all index layers
- ✅ **Optimistic Concurrency** - High throughput

**Guarantees:**
- **Atomicity** - All-or-nothing commits
- **Consistency** - Blob + Indexes transactional
- **Isolation** - Read Committed / Snapshot
- **Durability** - WAL-based recovery

### Transactions API ✅
**Status:** Production-Ready | **Docs:** [`docs/features/transactions.md`](docs/features/transactions.md)

**Features:**
- ✅ **Session-Based Transactions** - Long-lived sessions
- ✅ **Multi-Index Support** - Secondary, Graph, Vector
- ✅ **Isolation Levels** - `read_committed`, `snapshot`
- ✅ **Statistics** - Success rate, durations

**API:**
```bash
POST /transaction/begin
POST /transaction/commit
POST /transaction/rollback
GET /transaction/stats
```

---

## 📡 Change Data Capture (CDC)

### CDC Engine ✅
**Status:** Production-Ready | **Docs:** [`docs/features/change_data_capture.md`](docs/features/change_data_capture.md)

**Features:**
- ✅ **Append-Only Event Log** - All mutations captured
- ✅ **Incremental Consumption** - Checkpointing
- ✅ **SSE Streaming** - Real-time event delivery (experimental)
- ✅ **Backpressure Handling** - Flow control
- ✅ **Retention Policies** - Configurable TTL

**Event Types:**
- `INSERT`, `UPDATE`, `DELETE`
- Full entity snapshots
- Metadata (timestamp, user, transaction)

**API:**
```bash
GET /cdc/events?since=checkpoint_123
```

---

## 🚀 Performance & Optimization

### Memory Management ✅
**Status:** Production-Ready | **Docs:** [`docs/performance/memory_tuning.md`](docs/performance/memory_tuning.md)

**Storage Hierarchy:**
- **WAL on NVMe** - Minimum commit latency
- **Memtable in RAM** - Fast ingestion
- **Block Cache (RAM)** - Hot data caching (configurable size)
- **Bloom Filters (RAM)** - Probabilistic key existence checks
- **SSTables on SSD** - Persistent storage (LZ4/ZSTD compressed)

**Configuration:**
```yaml
storage:
  memtable_size_mb: 256
  block_cache_size_mb: 1024
  compression:
    default: lz4
    bottommost: zstd
```

### Compression ✅
**Status:** Production-Ready | **Docs:** [`docs/performance/compression_benchmarks.md`](docs/performance/compression_benchmarks.md)

**Algorithms:**
- **LZ4** - Balanced (33.8 MB/s write, 2.1x compression)
- **ZSTD** - Space-optimized (32.3 MB/s write, 2.8x compression)
- **Snappy** - Alternative option

**Strategie:**
- LZ4 für upper levels (schneller)
- ZSTD für bottommost level (besser komprimiert)

### Parallelization ✅
**Status:** Production-Ready | **Docs:** [`docs/performance/TBB_INTEGRATION.md`](docs/performance/TBB_INTEGRATION.md)

**Intel TBB Integration:**
- ✅ **Task-Based Execution** - Work-stealing scheduler
- ✅ **Batch Processing** - Parallel entity loading (batch size: 50)
- ✅ **Index Scans** - Parallel predicate evaluation
- ✅ **Throughput** - 3.5x speedup on 8-core systems

### GPU Acceleration ✅ (Optional Build)
**Status:** Available (Build Flag Required) | **Docs:** [`docs/performance/GPU_ACCELERATION_PLAN.md`](docs/performance/GPU_ACCELERATION_PLAN.md)

> ⚠️ **Build Requirement:** GPU acceleration requires explicit build flags:
> - `-DTHEMIS_ENABLE_CUDA=ON` for NVIDIA CUDA backend
> - `-DTHEMIS_ENABLE_GPU=ON` for general GPU support (Vulkan)

**CUDA Backend:**
- ✅ Faiss GPU Integration
- ✅ Vector distance computation (10-50x speedup)
- ✅ Batch queries (50K-100K q/s)

**Vulkan Backend:**
- ✅ Cross-platform GPU compute
- ✅ Multi-vendor support (NVIDIA, AMD, Intel)
- ✅ Compute shaders for vector operations

**Build Instructions:**
```bash
# NVIDIA CUDA Build
cmake -DTHEMIS_ENABLE_CUDA=ON -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Vulkan GPU Build
cmake -DTHEMIS_ENABLE_GPU=ON -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

---

## 🌐 APIs & Clients

### HTTP REST API ✅
**Status:** Production-Ready | **Docs:** [`docs/apis/openapi.md`](docs/apis/openapi.md)

**Core Endpoints:**
- ✅ **Entities:** `PUT/GET/DELETE /entities/{key}`
- ✅ **Indexes:** `POST /index/create`, `POST /index/drop`
- ✅ **Queries:** `POST /query` (relational), `POST /query/aql` (AQL)
- ✅ **Graph:** `POST /graph/traverse`
- ✅ **Vector:** `POST /vector/search`
- ✅ **Transactions:** `POST /transaction/*`
- ✅ **Admin:** `POST /admin/backup`, `GET /admin/stats`
- ✅ **Monitoring:** `GET /health`, `GET /stats`, `GET /metrics`

**Content-Type:**
- `application/json` (primary)
- `application/x-velocypack` (optional)

### OpenAPI 3.0 Specification ✅
**Status:** Production-Ready | **File:** [`docs/openapi.yaml`](docs/openapi.yaml)

- Complete API documentation
- Request/Response schemas
- Authentication schemes
- Error codes

### GraphQL API ✅
**Status:** Production-Ready | **Docs:** [`docs/apis/graphql.md`](docs/apis/graphql.md)

- ✅ **GraphQL Parser** - Query, Mutation, Subscription
- ✅ **Schema Introspection** - SDL Export
- ✅ **Field Resolution** - Nested selections
- ✅ **Built-in Types** - Document, Graph, Vector, Timeseries
- ✅ **Error Handling** - GraphQL spec compliant
- ✅ **HTTP Endpoint** - `POST /graphql`

### Client SDKs ✅
**Status:** Production-Ready | **Docs:** [`clients/`](clients/)

**Feature Parity across all 7 SDKs:**

| Feature | Python | JS/TS | Go | Rust | Java | C# | Swift |
|---------|--------|-------|----|----|------|----|----|
| Basic CRUD | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Transactions | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| AQL Queries | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Graph API | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Vector API | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Async/Await | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

**Graph API Methods:**
- `graphTraverse(startNode, maxDepth, edgeType)`
- `shortestPath(from, to, edgeType)`
- `neighbors(nodeId, direction, edgeType, limit)`

**Vector API Methods:**
- `vectorSearch(embedding, topK, filter)`
- `vectorUpsert(id, embedding, metadata)`
- `vectorDelete(id)`

📋 SDK Publishing (NPM, PyPI, NuGet, Maven, Crates.io) - Q1 2026

---

## 🛠️ Content Processing

### Content Architecture ✅
**Status:** Production-Ready | **Docs:** [`docs/architecture/content_architecture.md`](docs/architecture/content_architecture.md)

**Unified Ingestion Pipeline:**
- ✅ **ContentTypeRegistry** - MIME type detection
- ✅ **Processor Routing** - Domain-specific handlers
- ✅ **Metadata Extraction** - EXIF, GPS, Tags
- ✅ **Chunking** - Configurable strategies

### Content Processor Plugins ✅ (NEW)
**Status:** Production-Ready | **Docs:** [`docs/content/CONTENT_PROCESSOR_PLUGINS.md`](docs/content/CONTENT_PROCESSOR_PLUGINS.md)

**Plugin Architecture:**
- ✅ **DLL/SO Loading** - Dynamic plugin loading
- ✅ **YAML Configuration** - Per-processor settings (`config/processors/*.yaml`)
- ✅ **Unified Interface** - `IContentProcessorPlugin`
- ✅ **Health Checks** - Plugin status monitoring
- ✅ **Statistics** - Per-plugin metrics

**Implemented Processors:**

| Processor | Backend | MIME Types | Features |
|-----------|---------|------------|----------|
| **PDF** | poppler | `application/pdf` | Text extraction, metadata, page chunking |
| **Office** | libzip/pugixml | DOCX, XLSX, PPTX, ODF | Text, tables, metadata |
| **Video** | FFmpeg | MP4, WebM, MKV, MOV | Duration, codecs, thumbnails, subtitles |
| **Audio** | FFmpeg | MP3, WAV, FLAC, OGG | Duration, tags, waveform, transcription |
| **Geo** | GDAL | GeoJSON, KML, GPX, Shapefile | Coordinates, CRS, bounds, centroid |
| **Image** | libvips | JPEG, PNG, WebP, TIFF | EXIF, OCR, thumbnails, color analysis |
| **CAD** | OpenCASCADE | STEP, STL, IGES, OBJ | BOM, geometry, 3D preview |
| **Text** | Built-in | Plain text, Markdown | Sentence/paragraph chunking |

**Configuration Example:**
```yaml
# config/processors/pdf.yaml
name: pdf-processor
version: "1.0.0"
enabled: true
settings:
  extraction:
    text: true
    metadata: true
  thumbnail:
    generate: true
    max_width: 256
```

**API:**
```bash
POST /content/import
{
  "content": {...},
  "chunks": [...],
  "edges": [...],
  "blob": "..."
}
```

### Geo-Spatial Features ✅
**Status:** Production-Ready | **Docs:** [`docs/geo/`](docs/geo/)

**Capabilities:**
- ✅ **R-Tree Index** - Spatial search
- ✅ **Geohash** - Location encoding
- ✅ **GeoJSON Support** - Points, Lines, Polygons
- ✅ **GPX Processing** - Track/Route parsing
- ✅ **Distance Queries** - Radius search
- ✅ **Relational Schema** - Geo tables integration

---

## 📈 Observability & Monitoring

### Metrics & Statistics ✅
**Status:** Production-Ready | **Docs:** [`docs/observability/observability_prometheus.md`](../observability/observability_prometheus.md)

**Core Prometheus Metrics:**
- ✅ `vccdb_requests_total` (counter)
- ✅ `vccdb_errors_total` (counter)
- ✅ `vccdb_qps` (gauge)
- ✅ `rocksdb_block_cache_usage_bytes` (gauge)
- ✅ `rocksdb_estimate_num_keys` (gauge)
- ✅ `vccdb_page_fetch_time_ms_*` (histogram)

**Sharding Metrics (Phase 6 - NEW):** ✅ **Complete**

44 comprehensive metrics for distributed sharding:

**Shard Health & Topology:**
- `themis_shard_health_status{shard_id, status}` - Shard health indicator
- `themis_shard_certificate_expiry_seconds{shard_id}` - Certificate validity
- `themis_cluster_size` - Total number of shards
- `themis_virtual_nodes_total` - Consistent hash ring virtual nodes

**Routing Performance:**
- `themis_routing_requests_total{type}` - Requests by type (local/remote/scatter_gather)
- `themis_routing_errors_total{shard_id, error_type}` - Error tracking
- `themis_routing_latency_seconds{operation, quantile}` - Latency distribution

**Data Migration:**
- `themis_migration_records_total{operation_id}` - Records migrated
- `themis_migration_bytes_total{operation_id}` - Data transferred
- `themis_migration_progress_percent{operation_id}` - Migration progress
- `themis_migration_duration_seconds{operation_id}` - Total duration

**Cross-Shard Joins:**
- `themis_cross_shard_joins_total{strategy}` - Join operations (broadcast_hash/co_located)
- `themis_cross_shard_join_duration_seconds{strategy, quantile}` - Join latency
- `themis_hash_table_build_seconds{quantile}` - Hash table construction time

**Gossip Protocol:**
- `themis_gossip_messages_total{type}` - P2P messages (heartbeat/peer_list)
- `themis_gossip_peer_count` - Known peers
- `themis_gossip_roundtrip_seconds{quantile}` - Communication latency

**Cloud Agent:**
- `themis_datacenter_latency_seconds{datacenter, quantile}` - DC latency
- `themis_cross_dc_requests_total{source, target}` - Cross-DC traffic

**Configuration:**
```yaml
sharding:
  metrics:
    enabled: true
    enable_histograms: true
```

**Usage:**
```cpp
#include "sharding/prometheus_metrics.h"
#include "sharding/metrics_registry.h"

auto metrics = std::make_shared<PrometheusMetrics>(config);
ShardingMetricsRegistry::instance().registerMetrics(metrics);
```

**Monitoring Resources:**
- Alert Rules: [`deploy/kubernetes/monitoring/prometheus/alert-rules-sharding.yaml`](../../deploy/kubernetes/monitoring/prometheus/alert-rules-sharding.yaml)
- Grafana Dashboard: [`deploy/kubernetes/monitoring/grafana-dashboards/themisdb-sharding-dashboard.json`](../../deploy/kubernetes/monitoring/grafana-dashboards/themisdb-sharding-dashboard.json)
- Full Metrics List: [`docs/observability/observability_phase6_complete.md`](../observability/observability_phase6_complete.md)

**RocksDB Statistics:**
- Block cache hit/miss rates
- Compaction metrics
- Memtable sizes
- Files per level (L0-L6)

**API:**
```bash
GET /stats        # JSON format
GET /metrics      # Prometheus format (includes sharding metrics)
```

### OpenTelemetry Tracing ✅
**Status:** Production-Ready

**Features:**
- ✅ Distributed tracing
- ✅ Span context propagation
- ✅ Performance bottleneck detection
- ✅ OTLP exporter integration

### Logging ✅
**Status:** Production-Ready

**spdlog Integration:**
- ✅ Structured logging
- ✅ Log levels (TRACE, DEBUG, INFO, WARN, ERROR)
- ✅ File rotation
- ✅ Console + file outputs

---

## 🏗️ Deployment & Operations

### Deployment Options ✅
**Status:** Production-Ready | **Docs:** [`docs/guides/deployment.md`](docs/guides/deployment.md)

**Binary:**
```bash
themis_server --config /etc/themis/config.yaml
```

**Docker:**
```bash
docker run -p 8765:8765 \
  -v /data:/data \
  ghcr.io/makr-code/themisdb:latest
```

**Docker Compose:**
```bash
docker compose up --build
```

**Configuration Formats:**
- ✅ YAML (recommended)
- ✅ JSON
- ✅ Environment variables

### Container Images ✅
**Status:** Production-Ready

**Registries:**
- ✅ **GHCR:** `ghcr.io/makr-code/themisdb`
- ✅ **Docker Hub:** `themisdb/themisdb` (optional)

**Tags:**
- `latest` - Latest stable
- `g<shortsha>` - Git commit
- `latest-x64-linux`, `latest-arm64-linux` - Arch-specific

**Multi-Arch:**
- ✅ x86_64 (AMD64)
- ✅ ARM64 (aarch64)

### Backup & Recovery ✅
**Status:** Production-Ready | **Docs:** [`docs/guides/deployment.md`](docs/guides/deployment.md)

**Features:**
- ✅ **RocksDB Checkpoints** - Consistent snapshots
- ✅ **Point-in-Time Recovery** - WAL archiving
- ✅ **Incremental Backups** - Scripted automation
- ✅ **API Endpoint:** `POST /admin/backup`

**Scripts:**
- `scripts/backup.sh` (Linux)
- `scripts/backup.ps1` (Windows)

---

## 🧰 Admin Tools

### WPF Admin Tools Suite ✅
**Status:** Production-Ready | **Docs:** [`docs/admin_tools/user_guide.md`](../admin_tools/user_guide.md)

**Tools (7):**
1. ✅ **Audit Log Viewer** - Search, filter, export logs
2. ✅ **SAGA Verifier** - Distributed transaction consistency
3. ✅ **PII Manager** - GDPR data subject requests
4. ✅ **Key Rotation Dashboard** - LEK/KEK/DEK management
5. ✅ **Retention Manager** - Policy-based archival
6. ✅ **Classification Dashboard** - Data classification testing
7. ✅ **Compliance Reports** - Automated reporting

**Common Features:**
- Unified Themis Design System
- Dark/Light theme
- Export (CSV, PDF, Excel)
- Real-time search & filtering
- Error handling & validation

**Publish:**
```powershell
.\publish-all.ps1  # Build all tools to dist/
```

---

## 🔌 Plugin Architecture

### Plugin System ✅
**Status:** Production-Ready | **Docs:** [`docs/plugins/PLUGIN_MIGRATION.md`](../plugins/PLUGIN_MIGRATION.md)

**Unified Interface:**
- ✅ `IPlugin` - Base interface
- ✅ `PluginManager` - Discovery & loading
- ✅ Security verification (signature checking)
- ✅ Hot-reload support

**Plugin Categories:**
1. ✅ **Blob Storage** - Filesystem, WebDAV, S3, Azure
2. ✅ **Compute** - CUDA, Vulkan, DirectX
3. 📋 **Importers** - PostgreSQL, MySQL, CSV
4. 📋 **Embeddings** - Sentence-BERT, OpenAI, CLIP
5. 📋 **HSM** - PKCS#11, Luna, CloudHSM

**Benefits:**
- Modular binaries (Core < 50 MB)
- On-demand loading
- Third-party extensions
- Reduced dependencies

---

## 🧪 Testing & Quality

### Test Coverage ✅
**Status:** Production-Ready

**Overall Coverage:** 85%+

**Test Suites:**
- ✅ **Unit Tests** - Core components (269 files tested)
- ✅ **Integration Tests** - API endpoints, workflows
- ✅ **Performance Tests** - Benchmarks (Google Benchmark)
- ✅ **Security Tests** - Encryption, audit, HSM

**Test Frameworks:**
- Google Test (C++)
- Catch2 (alternative)
- Custom test harnesses

### Code Quality ✅
**Status:** Production-Ready | **Docs:** [`docs/development/code_audit_mockups_stubs.md`](../development/code_audit_mockups_stubs.md)

**Static Analysis:**
- ✅ **clang-tidy** - Modern C++ best practices
- ✅ **cppcheck** - Additional quality checks
- ✅ **Gitleaks** - Secret scanning

**Formatting:**
- ✅ **clang-format** - Consistent style
- ✅ `.clang-format` config (C++20, 4 spaces)

**CI/CD:**
- ✅ GitHub Actions (Linux + Windows)
- ✅ Coverage reporting
- ✅ Security scanning

**Scripts:**
```bash
./scripts/run_clang_quality_wsl.sh       # Linux/WSL
.\scripts\run_clang_quality.ps1          # Windows
```

---

## 📚 Documentation

### Documentation Suite ✅
**Status:** Comprehensive | **Location:** [`docs/`](docs/)

**Main Docs:**
- ✅ **GitHub Pages:** https://makr-code.github.io/ThemisDB/
- ✅ **Wiki:** https://github.com/makr-code/ThemisDB/wiki
- ✅ **Print View:** PDF export available
- ✅ **MkDocs:** Local preview support

**Categories:**
- **Architecture** - Design docs (base_entity, mvcc, content pipeline)
- **Features** - Feature guides (32+ docs)
- **Security** - Security architecture (10+ docs)
- **APIs** - API references (OpenAPI, ContentFS, Hybrid Search)
- **Admin Tools** - Tool guides & demos
- **Performance** - Tuning & benchmarks
- **Development** - Dev guides, audits

**Build Docs:**
```powershell
.\build-docs.ps1      # Generate site/
.\sync-wiki.ps1       # Sync to Wiki
```

---

## 🎯 Performance Benchmarks

### Typical Results ✅
**Platform:** Windows 11, i7-12700K, Release build

| Operation | Throughput | Latency (p50) | Latency (p99) |
|-----------|------------|---------------|---------------|
| **Entity PUT** | 45,000 ops/s | 0.02 ms | 0.15 ms |
| **Entity GET** | 120,000 ops/s | 0.008 ms | 0.05 ms |
| **Indexed Query** | 8,500 queries/s | 0.12 ms | 0.85 ms |
| **Graph Traverse** (depth=3) | 3,200 ops/s | 0.31 ms | 1.2 ms |
| **Vector ANN** (k=10) | 1,800 queries/s | 0.55 ms | 2.1 ms |
| **Index Rebuild** (100K) | 12,000 entities/s | - | - |

### Compression Performance ✅

| Algorithm | Write Throughput | Compression Ratio | Use Case |
|-----------|------------------|-------------------|----------|
| **None** | 34.5 MB/s | 1.0x | Development only |
| **LZ4** | 33.8 MB/s | 2.1x | Default (balanced) |
| **ZSTD** | 32.3 MB/s | 2.8x | Bottommost (storage) |

---

## 🗺️ Roadmap

### Q1 2026 (0-3 Monate)
**Focus:** Ecosystem & SDKs

- ✅ **v1.0.0 Production Release** - Alle P0/P1 Features komplett
- ✅ **GPU Acceleration (CUDA/Vulkan)** - 10-50x Vector speedup
- ✅ **Multi-Tenancy** - Complete tenant isolation
- ✅ **GraphQL API** - Full GraphQL server
- ✅ **OLAP Analytics** - CUBE, ROLLUP, Window Functions
- 🔧 **JavaScript/Python SDK** - Production-ready v1.0
- 🔧 **Content Processors** - PDF, Office support
- 🔧 **CI/CD Improvements** - Matrix builds, security scanning

### Q2-Q3 2026 (3-9 Monate)
**Focus:** Distributed Systems

- ✅ **Distributed Sharding (Phase 1-6)** - Vollständig inkl. Monitoring, Tests
- ✅ **Cassandra-inspired Streaming Protocol** - Chunk-basiert, LZ4/Zstd
- ✅ **RAID-like Redundancy** - MIRROR, STRIPE, PARITY, GEO_MIRROR
- ✅ **Granular Blob-Level Redundancy** - Per SST/WAL/Index
- ✅ **Adaptive Backpressure Protocol** - Load-aware sync deferral
- ✅ **Leader-Follower Replication** - WAL-based, Automatic Failover
- ✅ **Multi-Master Replication** - CRDTs, Vector Clocks, HLC
- ✅ **Complex Event Processing (CEP)** - EPL, Pattern Matching, Windows
- ✅ **Grafana Dashboards** - 19 Panels, 8 Alert Rules
- ✅ **SDK Feature Parity** - 7 SDKs (Graph + Vector API)

### Q4 2026+ (9+ Monate)
**Focus:** Innovation

- 📋 **Multi-DC Replication** - Geo-distributed
- 📋 **Kubernetes Operator Controller** - Full operator (CRDs ✅ done)
- 📋 **ML Integration** - GNNs, in-database training
- 📋 **Zero-Copy Transfer** - Advanced streaming optimization

**Siehe auch:** [`ROADMAP.md`](../roadmap/ROADMAP.md) für Details

---

## 🏆 Production-Ready Status

### P0 Features (Kritisch) ✅
**Status:** 100% Complete

- ✅ ACID Transactions (MVCC)
- ✅ Multi-Model Support (Relational, Graph, Vector, Document)
- ✅ Secondary Indexes (7 types)
- ✅ HNSW Persistence
- ✅ Graph Traversals (BFS, Dijkstra, A*)
- ✅ AQL Query Language
- ✅ Enterprise Security (TLS, RBAC, Encryption, Audit)
- ✅ Observability (Metrics, Tracing, Logging)
- ✅ Backup & Recovery

### Overall Progress
**Current Status:** ~98% Production-Ready

- **Core Engine:** 100%
- **Security Stack:** 85%
- **API Layer:** 95%
- **Documentation:** 95%
- **Client SDKs:** 95% (7 SDKs with feature parity)
- **Distributed Sharding:** 100% (Phase 1-6 Complete)
- **Replication:** 100% (Leader-Follower + Multi-Master)
- **Streaming Protocol:** 100%
- **RAID-like Redundancy:** 100%
- **CEP Engine:** 100%
- **GPU Acceleration:** 100% (Code Complete, Opt-in Build)

---

## 📦 Dependencies

### Core Libraries (vcpkg)

**Storage & Performance:**
- RocksDB - LSM-Tree storage
- Intel TBB - Parallelization
- Apache Arrow - Columnar analytics

**Serialization & Parsing:**
- simdjson - High-performance JSON
- VelocyPack - Binary serialization
- msgpack - Alternative serialization

**Vector Search:**
- HNSWlib - ANN index
- Faiss - GPU-accelerated search (optional)

**Networking:**
- Boost.Asio - Async I/O
- Boost.Beast - HTTP server
- libcurl - HTTP client (WebDAV, etc.)

**Security:**
- OpenSSL - TLS, encryption, PKI
- PKCS#11 - HSM integration

**Utilities:**
- spdlog - Logging
- yaml-cpp - YAML parsing
- nlohmann/json - JSON library

**Testing:**
- Google Test - Unit tests
- Google Benchmark - Performance tests

---

## 🔗 Referenzen

**Inspired by:**
- ArangoDB (Multi-model architecture)
- CozoDB (Hybrid relational-graph-vector)
- Azure Cosmos DB (Multi-model with ARS format)
- RocksDB (LSM-Tree foundation)
- Faiss (Vector search)

**Academic Foundations:**
- MVCC (PostgreSQL/Oracle design)
- LSM-Tree (Google Bigtable, LevelDB)
- HNSW (Malkov & Yashunin 2018)

---

## 📞 Support & Community

**Repository:** https://github.com/makr-code/ThemisDB  
**Issues:** https://github.com/makr-code/ThemisDB/issues  
**Discussions:** https://github.com/makr-code/ThemisDB/discussions  
**Wiki:** https://github.com/makr-code/ThemisDB/wiki

**Documentation:**
- Online: https://makr-code.github.io/ThemisDB/
- PDF: https://makr-code.github.io/ThemisDB/themisdb-docs-complete.pdf

---

## 📄 Lizenz

**MIT License** - See [`LICENSE`](LICENSE) file for details

---

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Letzte Aktualisierung:** 5. Dezember 2025


---

## File: docs\deployment\DOCKER_DEPLOYMENT.md

# ThemisDB Docker Deployment Guide

**Version:** 1.3.0  
**Last Updated:** April 2026  
**Status:** Production-Ready

## Quick Start

### Pull & Run (Docker Hub)

```bash
# Latest version
docker pull themisdb/themisdb:latest
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Specific version (v1.3.0 - LLM Integration)
docker pull themisdb/themisdb:v1.3.0
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themis_data:/data \
  themisdb/themisdb:v1.3.0
```

### Quick Start with GPU Support (v1.3.0+)

**NEW in v1.3.0:** Native LLM inference with GPU acceleration

```bash
# Pull GPU-enabled image
docker pull themisdb/themisdb:v1.3.0-gpu

# Run with NVIDIA GPU support
docker run -d \
  --name themis-gpu \
  --gpus all \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themis_data:/data \
  -v themis_models:/models \
  themisdb/themisdb:v1.3.0-gpu

# Verify GPU access
docker exec themis-gpu nvidia-smi
```

**Requirements:**
- NVIDIA GPU with CUDA support
- [NVIDIA Container Toolkit](https://github.com/NVIDIA/nvidia-docker) installed
- Docker 19.03+ with GPU support

**Performance:** 100x faster inference vs CPU-only mode


### Verify Running

```bash
# Check container status
docker ps | grep themis

# Health check
curl http://localhost:18765/health
# Expected: 200 OK with JSON response

# View logs
docker logs -f themis
```

---

## Docker Images

### Available Tags

| Tag | Architecture | Status | Use Case |
|-----|--------------|--------|----------|
| `latest` | amd64 + arm64 | ✅ Production | Recommended for most users (v1.3.0) |
| `v1.3.0` | amd64 + arm64 | ✅ Production | LLM Integration Release (December 2025) |
| `v1.3.0-gpu` | amd64 | ✅ Production | With CUDA support for GPU acceleration |
| `qnap` | amd64 | ✅ Production | QNAP NAS optimized (Ubuntu 20.04, SSE4.2 baseline) |
| `v1.2.0` | amd64 + arm64 | ✅ Production | Previous stable release |
| `v1.0.2` | amd64 + arm64 | ✅ Production | Legacy stable release |
| `v1` | amd64 + arm64 | ✅ Production | Major version track |

### Image Specs

**Registry:** `docker.io/themisdb/themisdb`

**Multi-Architecture Support:**
- `linux/amd64` - Intel/AMD x64 processors
- `linux/arm64` - ARM v8 processors (RPi, Apple Silicon, AWS Graviton, etc.)

**Image Size:** ~150MB (compressed)

**Build Configuration:**
```dockerfile
FROM ubuntu:22.04
VCPKG_ENABLE_ONLINE=OFF          # No internet access during build
VCPKG_TRIPLET=x64-linux          # For amd64
VCPKG_TRIPLET=arm64-linux        # For arm64
```
**Hinweis:** Die lokale Quelle `llama.cpp/` im Projekt‑Root ist per `.dockerignore` ausgeschlossen und wird nicht in das Build‑Context kopiert. Die LLM‑Funktionalität wird über die kompilierten Artefakte (ggml/llama) bereitgestellt; Modelle sollten als Volume (`/models`) gemountet werden.

---

## Configuration

### Environment Variables

```bash
docker run -e THEMIS_CONFIG_PATH=/etc/themis/config.json \
           -e THEMIS_PORT=18765 \
           -e LD_LIBRARY_PATH=/usr/local/lib/themisdb:/usr/local/lib \
           themisdb/themisdb:latest
```

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_CONFIG_PATH` | `/etc/themis/config.json` | Configuration file path |
| `THEMIS_PORT` | `18765` | Internal port (mapped via -p) |
| `LD_LIBRARY_PATH` | `/usr/local/lib/themisdb:/usr/local/lib` | Runtime library path |

### Custom Config

```bash
# Mount custom config
docker run -d \
  -v /path/to/config.json:/etc/themis/config.json:ro \
  themisdb/themisdb:latest
```

---

## Volumes

### Data Persistence

```bash
# Named volume (recommended)
docker volume create themis_data
docker run -d \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Bind mount (for development)
docker run -d \
  -v /local/data/path:/data \
  themisdb/themisdb:latest
```

### Directories in Container

| Path | Purpose | Persistence |
|------|---------|-------------|
| `/data` | Database storage | ✅ Volume |
| `/etc/themis` | Configuration | ✅ Config mount |
| `/var/log/themis` | Application logs | ✅ Volume |

---

## Networking

### Port Mapping

```bash
docker run -d \
  -p 8080:8080 \                # REST API (HTTP/HTTP/2)
  -p 18765:18765 \              # Internal protocol
  -p 9090:9090 \                # WebSocket
  -p 1883:1883 \                # MQTT
  -p 5432:5432 \                # PostgreSQL Wire Protocol
  themisdb/themisdb:latest
```

| Port | Protocol | Purpose | Default | Version |
|------|----------|---------|---------|---------|
| `8080` | HTTP/HTTP/2 | REST API, Web UI, Server Push | Required | v1.0+ |
| `18765` | Custom | Binary protocol | Required | v1.0+ |
| `9090` | WebSocket | Real-time CDC streaming | Optional | v1.3.0+ |
| `1883` | MQTT | Broker with WebSocket transport | Optional | v1.3.0+ |
| `5432` | PostgreSQL | Wire Protocol (SQL-to-Cypher) | Optional | v1.3.0+ |
| `3000` | MCP | Model Context Protocol | Optional | v1.3.0+ |

### Network Modes

```bash
# Host network (performance, less isolation)
docker run --network host themisdb/themisdb:latest

# Bridge network (default, recommended)
docker run --network bridge themisdb/themisdb:latest

# Custom network
docker network create themis_net
docker run --network themis_net themisdb/themisdb:latest
```

---

## Docker Compose

### Basic Setup

```yaml
# docker-compose.yml
version: '3.8'

services:
  themis:
    image: themisdb/themisdb:latest
    container_name: themis
    ports:
      - "8080:8080"
      - "18765:18765"
    volumes:
      - themis_data:/data
      - ./config/config.json:/etc/themis/config.json:ro
    environment:
      THEMIS_PORT: 18765
      THEMIS_CONFIG_PATH: /etc/themis/config.json
    restart: unless-stopped
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:18765/health"]
      interval: 10s
      timeout: 5s
      retries: 3

volumes:
  themis_data:
    driver: local
```

**Start:**
```bash
docker-compose up -d
docker-compose logs -f
```

### Multi-Service Stack

```yaml
version: '3.8'

services:
  themis:
    image: themisdb/themisdb:latest
    ports:
      - "8080:8080"
      - "18765:18765"
    volumes:
      - themis_data:/data
    networks:
      - themis_net

  # Optional: monitoring
  prometheus:
    image: prom/prometheus:latest
    ports:
      - "9090:9090"
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml:ro
    networks:
      - themis_net

networks:
  themis_net:
    driver: bridge

volumes:
  themis_data:
```

---

## Production Deployment

### Resource Allocation

```bash
docker run -d \
  --cpus="4" \                          # 4 CPU cores
  --memory="8g" \                       # 8GB RAM
  --memory-swap="10g" \                 # 10GB with swap
  -v themis_data:/data \
  themisdb/themisdb:latest
```

### Restart Policies

```bash
# Always restart
docker run --restart=always themisdb/themisdb:latest

# Restart unless manually stopped
docker run --restart=unless-stopped themisdb/themisdb:latest

# Restart with max retry count
docker run --restart=on-failure:5 themisdb/themisdb:latest
```

### Logging

```bash
# JSON file driver (max size limit)
docker run -d \
  --log-driver json-file \
  --log-opt max-size=10m \
  --log-opt max-file=3 \
  themisdb/themisdb:latest

# Syslog driver
docker run -d \
  --log-driver syslog \
  --log-opt syslog-address=udp://localhost:514 \
  themisdb/themisdb:latest

# View logs
docker logs --tail 100 --follow themis
```

---

## Platform-Specific Deployment

### Linux (x64)

```bash
# Ubuntu 22.04+
docker run -d \
  --platform linux/amd64 \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Debian
apt-get install docker.io
docker pull themisdb/themisdb:latest
```

### ARM/Raspberry Pi

```bash
# Auto-detects ARM64
docker pull themisdb/themisdb:latest

# Explicit pull
docker pull --platform linux/arm64 themisdb/themisdb:latest

# Run
docker run -d \
  --platform linux/arm64 \
  -v themis_data:/data \
  themisdb/themisdb:latest
```

### QNAP NAS

```bash
# Pull QNAP-optimized image (Ubuntu 20.04, SSE4.2 baseline)
docker pull themisdb/themisdb:qnap

# Run on QNAP (port 18765 to avoid conflicts)
docker run -d \
  --name themis \
  -p 18765:18765 \
  -v /share/Container/themis/data:/data \
  -v /share/Container/themis/config/config.qnap.json:/etc/themis/config.json:ro \
  themisdb/themisdb:qnap

# Or use docker-compose.qnap.yml
# See docker/docker-compose.qnap.yml for full setup
```

**QNAP Notes:**
- Use `qnap` or `v1.0.2-qnap` tags (optimized for older CPUs)
- Default port 18765 avoids QNAP service conflicts
- Mount volumes to `/share/Container/themis/`
- Requires GLIBC 2.31+ (QNAP QTS 5.0+)

### macOS (Apple Silicon/Intel)

```bash
# Auto-selects correct architecture
docker pull themisdb/themisdb:latest

# Explicitly specify
docker pull --platform linux/arm64 themisdb/themisdb:latest  # M-series
docker pull --platform linux/amd64 themisdb/themisdb:latest  # Intel
```

### Windows (Docker Desktop)

```powershell
# Pull image
docker pull themisdb/themisdb:latest

# Run
docker run -d `
  -p 8080:8080 `
  -p 18765:18765 `
  -v themis_data:C:\data `
  themisdb/themisdb:latest

# View logs
docker logs -f themis
```

---

## Troubleshooting

### Container Fails to Start

```bash
# Check logs
docker logs themis

# Common issues:
# 1. Port already in use
docker ps  # Find conflicting container
docker stop <container_id>

# 2. Insufficient disk space
docker system df  # Check usage

# 3. Broken config
docker exec themis cat /etc/themis/config.json
```

### Performance Issues

```bash
# Monitor container stats
docker stats themis

# Check resource limits
docker inspect themis | grep -i memory

# Increase memory allocation
docker update --memory 16g themis
docker restart themis
```

### Network Connectivity

```bash
# Test from host
curl http://localhost:8080/api/health

# Test from within container
docker exec themis curl http://localhost:18765/health

# Check port binding
docker port themis
```

### Library Path Issues

```bash
# Verify libraries are loaded
docker exec themis ldd /usr/local/bin/themis_server

# Check library path
docker exec themis echo $LD_LIBRARY_PATH

# Rebuild with updated lib path if needed
docker pull themisdb/themisdb:latest --force
```

---

## Build Your Own Image (Advanced)

### Build from Source

```bash
# Clone repo
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Build multi-arch (requires buildx setup)
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t themis:custom:latest .

# Build single-arch
docker build \
  -f Dockerfile \
  -t themis:custom:latest .
```

### Build Arguments

```dockerfile
# Customize build
docker build \
  --build-arg VCPKG_ENABLE_ONLINE=OFF \
  --build-arg THEMIS_VERSION=1.0.2 \
  -t themis:custom .
```

### QNAP-Specific Build

```bash
# Build QNAP-optimized image (Ubuntu 20.04, baseline CPU)
docker build \
  -f docker/Dockerfile.qnap \
  -t themis:qnap \
  .

# Tag and push
docker tag themis:qnap themisdb/themisdb:qnap
docker push themisdb/themisdb:qnap
```

---

## Best Practices

✅ **DO:**
- Use named volumes for data persistence
- Set resource limits (CPU, memory)
- Use health checks
- Enable restart policies
- Log to syslog or json-file with rotation
- Use specific version tags (not just `latest`)
- Run as non-root (built-in)
- Mount config as read-only

❌ **DON'T:**
- Run containers with `--privileged`
- Use `latest` tag in production (use specific versions)
- Store secrets in environment variables
- Ignore health check failures
- Disable restart policies
- Map unnecessary ports

---

## Support & Issues

**Docker Hub:** https://hub.docker.com/r/themisdb/themisdb

**GitHub Issues:** https://github.com/makr-code/ThemisDB/issues

**Deployment Strategy:** [Full Deployment Strategy](../deployment/deployment_strategy.md)

---

## Related Documentation

- [README.md](../README.md) - Main documentation
- [CHANGELOG.md](../releases/CHANGELOG.md) - Release notes
- [BUILD_ORGANIZATION.md](BUILD_ORGANIZATION.md) - Build system
- [Dockerfile](Dockerfile) - Build definition


---

## File: LICENSE

MIT License with Government Clause

Copyright (c) 2025 The ThemisDB Authors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

Government Clause (Sovereignty Protection):
Notwithstanding the permissions granted above, the Software is provided with the
express intent of ensuring digital sovereignty for public administration.
Therefore, any entity using this Software to provide a commercial managed service
or cloud offering ("Service Provider") agrees that:
a) The source code of any modifications, extensions, or derivative works created
   by the Service Provider to operate the Software as a service must be made
   publicly available under this same license (MIT with Government Clause)
   within 30 days of deployment.
b) Public sector entities (federal, state, and municipal governments) are granted
   a perpetual, irrevocable, royalty-free right to use, modify, and distribute
   the Software and any such derivative works for public purposes, regardless of
   any conflicting proprietary claims.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


---

