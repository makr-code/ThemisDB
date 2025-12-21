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
> - See [Build Guide](docs/guides/guides_build_strategy.md) for setup instructions

### Key Features (When LLM Support Enabled)

- 🧠 **Embedded LLM Engine** - llama.cpp integration for LLaMA/Mistral/Phi-3 (1B-70B params)
- ⚡ **GPU Acceleration** - NVIDIA CUDA support with significant speedup vs CPU
- 💾 **PagedAttention** - Advanced memory management with memory savings
- 🎯 **Continuous Batching** - Handle concurrent inference requests
- 🔧 **Quantization Support** - Q4_K_M, Q5_K_M, Q8_0 for efficient memory usage
- 📊 **Production Monitoring** - Grafana dashboards with metrics and alert rules
- 🔌 **Plugin Architecture** - Extensible LLM backend system
- 🌐 **Distributed RPC Framework** - Inter-shard communication for distributed LLM operations

### Performance Highlights (GPU Acceleration)

- **Significant speedup** with GPU acceleration vs CPU
- **Memory savings** with PagedAttention and prefix caching
- **Kernel fusion** for additional performance gains
- **Comprehensive test coverage** with unit tests

**[→ See LLM Integration Guide](docs/llm/LLAMA_CPP_INTEGRATION.md)**  
**[→ See Complete LLM Documentation](docs/llm/README.md)**

---

## Overview

ThemisDB is a production-ready multi-model database that combines relational, graph, vector, and document models in a single system with full ACID transaction support. Built on RocksDB with advanced security and compliance features.

**Key Features:**

- 🔒 **ACID Transactions** - Full snapshot isolation with MVCC
- 🔍 **Multi-Model** - Relational, Graph, Vector, Document in one database
- 🚀 **High Performance** - 45K writes/s, 120K reads/s, GPU-accelerated vector search
- 🛡️ **Enterprise Security** - TLS 1.3, RBAC, field-level encryption, audit logging
- 📊 **Advanced Analytics** - Complex Event Processing (CEP), OLAP, Time-series
- 🌐 **Distributed** - Horizontal sharding, replication, Kubernetes-ready
- 🧠 **AI-Ready** - Hybrid search (RAG), embedding cache, FAISS integration, **optional LLM engine with llama.cpp** (v1.3.0+)
- 🌐 **Modern Protocols** - HTTP/1.1, GraphQL, SSE, gRPC (v1.3.0), **HTTP/2 with Server Push** ✅, **WebSocket** ✅, **MQTT** ✅, **HTTP/3** 🚧, **PostgreSQL Wire** ✅, **MCP** ✅

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
./setup.sh
./build.sh

# Setup and build (Windows)
.\setup.ps1
.\build.ps1

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
- [Installation Guide](docs/guides/guides_deployment.md)
- [Docker Deployment](DOCKER_DEPLOYMENT.md)
- [Quick Start Tutorial](docs/guides/quick_start.md)

**Core Concepts:**
- [Architecture Overview](docs/architecture/ARCHITECTURE_OVERVIEW.md)
- [Multi-Model Design](docs/architecture/architecture_base_entity.md)
- [Transaction Management](docs/features/features_transactions.md)
- [AQL Query Language](docs/aql/aql_syntax.md)

**Features:**
- [Vector Search](docs/features/features_vector_ops.md)
- [Graph Operations](docs/features/features_graph.md)
- [Time-Series Engine](docs/features/features_time_series.md)
- [Security & Compliance](docs/security/security_implementation.md)
- [Feature Overview](docs/features/features_overview.md)

**Operations:**
- [Configuration Guide](docs/guides/guides_configuration.md)
- [Monitoring & Metrics](docs/observability/observability_prometheus.md)
- [Backup & Recovery](docs/guides/guides_deployment.md#backup--recovery)
- [Performance Tuning](docs/performance/performance_memory.md)

**Development:**
- [Build Guide](docs/guides/guides_build_strategy.md)
- [Contributing](CONTRIBUTING.md)
- [API Reference](docs/api/api_reference.md)
- [Client SDKs](clients/README.md)

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

**[→ Detailed Roadmap](docs/roadmap/ROADMAP.md)**  
**[→ Modularization Plan](docs/architecture/MODULARIZATION_PLAN.md)** (post-v1.3.0)

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
**[→ Benchmark Suite Documentation](COMPREHENSIVE_BENCHMARK_GUIDE.md)**

---

## Community & Support

- **Documentation**: [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
- **GitHub Issues**: [Report bugs or request features](https://github.com/makr-code/ThemisDB/issues)
- **Discussions**: [Community discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Contributing**: [Contributing guidelines](CONTRIBUTING.md)
- **Security**: [Security policy](SECURITY.md)

---

## License

ThemisDB is released under the [MIT License](LICENSE).

---

## Acknowledgments

ThemisDB is inspired by and builds upon the ideas from:
- **ArangoDB** - Multi-model architecture
- **CozoDB** - Hybrid relational-graph-vector design
- **Azure Cosmos DB** - Multi-model with unified API
- **RocksDB** - High-performance LSM-Tree storage
- **FAISS** - Efficient similarity search

**For a complete list of third-party libraries and feature attributions, see [ATTRIBUTIONS.md](ATTRIBUTIONS.md).**

---

**Built with ❤️ for the database community**
