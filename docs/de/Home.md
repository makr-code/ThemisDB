# Welcome to ThemisDB

**A high-performance multi-model database with ACID guarantees**

[![Version](https://img.shields.io/badge/version-1.8.0--rc1-blue)](https://github.com/makr-code/ThemisDB/releases/tag/v1.8.0-rc1)
[![CI](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml)
[![License](https://img.shields.io/badge/license-MIT-green)](https://github.com/makr-code/ThemisDB/blob/main/LICENSE)

---

## 🚀 Current Release: v1.8.0-rc1 (April 2026)

**Key Highlights:**
- 🌍 **Geo Module** - Full GeoJSON RFC 7946 parsing (all 7 geometry types) + in-memory R-tree spatial index
- 🔐 **Auth: German eID** - Online-Ausweisfunktion per BSI TR-03130/eIDAS (v1.9.0)
- 🔍 **Query: ShardKey Routing** - Point-lookup routed to 1 shard instead of scatter-gather (v1.9.0)
- 🧠 **Analytics: Forecasting** - Batch prediction, streaming update, parallel auto-tune (v1.9.0)
- 🕸️ **Scraper Plugin v1.1.0** - Provenance fields on all records; 56-source knowledge catalog
- 🇩🇪 **German E-Gov** - OZG/XÖV/XDOMEA/eID connectors (v2.2.0/v1.9.0)
- 🧪 **Training v1.6.0** - AdaLoRA (importance-based rank pruning), LoRAAdapterMerger (TIES merge), LoRA+

**Documentation:**
- [Changelog](../../CHANGELOG.md)
- [Roadmap](../../ROADMAP.md)
- [Compendium Update Notes](../reports/V1.4.0_ALPHA_UPDATE_NOTES.md)

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
