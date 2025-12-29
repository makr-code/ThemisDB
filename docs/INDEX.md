<div align="center">

# ThemisDB Documentation Index (Language Selector)

</div>

This file is now language-neutral. Please choose your language:

- 🇩🇪 Deutsch (primär): [de/INDEX.md](de/INDEX.md)
- 🇬🇧 English: [en/INDEX.md](en/INDEX.md)

> [!NOTE]
> German docs are authoritative; translations may lag.

---

## 📖 Documentation Structure

### Getting Started

| Document | Description |
|----------|-------------|
| [Quick Start](de/guides/QUICK_START.md) | 5-minute tutorial to get started |
| [Installation](de/guides/guides_build_strategy.md) | Installation on Linux, Windows, macOS, Docker |
| [Configuration](de/deployment/README.md) | Configure ThemisDB for your needs |
| [First Query](de/guides/QUICK_START.md) | Write your first AQL query |

### Core Concepts

| Document | Description |
|----------|-------------|
| [Architecture Overview](de/architecture/README.md) | High-level system architecture |
| [Multi-Model Design](de/architecture/README.md) | How ThemisDB handles multiple data models |
| [Transaction Model](de/architecture/README.md) | ACID transactions with MVCC |
| [Storage Layer](de/storage/README.md) | RocksDB LSM-Tree storage |

### Features

| Document | Description |
|----------|-------------|
| [Feature Overview](de/features/features_overview.md) | Complete feature catalog |
| [Vector Search](de/search/README.md) | Similarity search with HNSW/FAISS |
| [Graph Operations](de/features/README.md) | Graph traversal and pathfinding |
| [Time-Series](de/timeseries/README.md) | Time-series data and compression |
| [Hybrid Search](de/search/hybrid_search_design.md) | RAG-optimized BM25+Vector search (v1.2+) |
| [Analytics](de/observability/CEP_STREAMING_ANALYTICS.md) | CEP and OLAP analytics |

### Query Language (AQL)

| Document | Description |
|----------|-------------|
| [AQL Syntax](de/aql/README.md) | Complete AQL language reference |
| [AQL Examples](de/aql/README.md) | Common query patterns |
| [Query Optimization](de/query/README.md) | EXPLAIN and PROFILE commands |

### API Reference

| Document | Description |
|----------|-------------|
| [REST API](de/apis/HTTP_API_REFERENCE.md) | HTTP API endpoints |
| [GraphQL API](de/apis/apis_graphql.md) | GraphQL interface |
| [Client SDKs](de/clients/README.md) | SDKs for Python, JS, Rust, Go, Java, C#, Swift |

### Security & Compliance

| Document | Description |
|----------|-------------|
| [Security Overview](de/security/README.md) | Enterprise security features |
| [TLS Setup](de/guides/guides_tls_setup.md) | Configure TLS 1.3 and mTLS |
| [RBAC Configuration](de/guides/guides_rbac.md) | Role-based access control |
| [Encryption](de/security/security_encryption_strategy.md) | Data encryption at rest and in transit |
| [Audit Logging](de/security/SECURITY_AUDIT_REPORT.md) | Security event logging |
| [Compliance](de/compliance/compliance_dashboard.md) | GDPR, SOC 2, HIPAA compliance |

### Operations

| Document | Description |
|----------|-------------|
| [Deployment Guide](de/deployment/README.md) | Production deployment strategies |
| [Docker Deployment](de/deployment/DOCKER_DEPLOYMENT.md) | Docker and Kubernetes deployment |
| [Configuration](de/deployment/README.md) | Configuration reference |
| [Monitoring](de/observability/README.md) | Prometheus metrics and alerting |
| [Backup & Recovery](de/deployment/README.md) | Backup strategies and disaster recovery |
| [Troubleshooting](de/guides/rocksdb-windows-build-issues.md) | Common issues and solutions |
| [Performance Tuning](de/performance/README.md) | Optimize for your workload |

### Development

| Document | Description |
|----------|-------------|
| [Contributing](../CONTRIBUTING.md) | How to contribute to ThemisDB |
| [Build Guide](guides/guides_build_strategy.md) | Build from source |
| [Development Setup](development/SETUP.md) | Setup development environment |
| [Testing Guide](development/TESTING.md) | Run and write tests |
| [Code Style](development/CODE_STYLE.md) | Coding standards |
| [Architecture](architecture/ARCHITECTURE_OVERVIEW.md) | Deep-dive into internals |

### Advanced Topics

| Document | Description |
|----------|-------------|
| [Sharding](sharding/sharding_overview.md) | Horizontal sharding and routing |
| [Replication](sharding/sharding_replication.md) | Leader-follower and multi-master |
| [GPU Acceleration](performance/performance_gpu_plan.md) | CUDA, Vulkan, HIP backends |
| [vLLM Co-Location](analysis/VARIANT_STRATEGY_v1.1.0.md) | AI/ML workload optimization |
| [Content Processing](content/content_architecture.md) | Process PDFs, images, videos, etc. |

### Release Notes

| Document | Description |
|----------|-------------|
| [Changelog](../CHANGELOG.md) | Version history and changes |
| [Roadmap](roadmap/ROADMAP.md) | Future plans and features |
| [v1.3.0 Release](../RELEASE_NOTES_v1.3.0.md) | Latest release notes |
| [v1.2.0 Release](releases/v1.2.0.md) | Previous release |
| [v1.1.0 Release](releases/v1.1.0.md) | Previous release |
| [Migration Guides](guides/MIGRATION.md) | Upgrade between versions |

---

## 🔍 Search by Topic

### By Use Case

**Building an Application:**
- [Quick Start](guides/QUICK_START.md) → [REST API](api/REST_API.md) → [Client SDKs](../clients/README.md)

**Analytics & BI:**
- [OLAP Features](analytics/OLAP.md) → [Parquet Export](analytics/olap.md) → [Time-Series](features/features_time_series.md)

**AI/ML Applications:**
- [Vector Search](features/features_vector_ops.md) → [Hybrid Search](features/features_hybrid_search.md) → [Embedding Cache](features/features_embedding_cache.md)

**Graph Applications:**
- [Graph Operations](features/features_graph.md) → [AQL Graph Queries](aql/aql_syntax.md#graph-traversals) → [Path Algorithms](features/features_graph.md#algorithms)

**Production Deployment:**
- [Deployment Guide](operations/DEPLOYMENT.md) → [Monitoring](operations/MONITORING.md) → [Backup](operations/BACKUP.md) → [Security](security/security_implementation.md)

### By Technology

**Docker/Kubernetes:**
- [Docker Deployment](deployment/DOCKER_DEPLOYMENT.md)
- [Kubernetes Guide](deployment/deployment_kubernetes.md)
- [Helm Charts](../helm/README.md)

**Cloud Platforms:**
- [AWS Deployment](deployment/deployment_aws.md)
- [Azure Deployment](deployment/deployment_azure.md)
- [GCP Deployment](deployment/deployment_gcp.md)

**ARM/Raspberry Pi:**
- [ARM Build Guide](deployment/deployment_arm_build.md)
- [ARM Packages](deployment/deployment_arm_packages.md)
- [Raspberry Pi Optimization](deployment/deployment_rpi.md)

---

## 📊 Performance & Benchmarks

- [Performance Overview](performance/performance_overview.md)
- [Benchmarking Guide](../benchmarks/README.md)
- [Memory Tuning](performance/performance_memory.md)
- [GPU Performance](performance/performance_gpu_plan.md)
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
**Last Updated:** December 15, 2025  
**Next Review:** March 15, 2026
