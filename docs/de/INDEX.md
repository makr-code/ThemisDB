# ThemisDB Documentation

> **📌 Navigation-Hinweis (2026-08-12):** Kanonischer Root-Index für `docs/de/` ist [`00_DOCUMENTATION_INDEX.md`](00_DOCUMENTATION_INDEX.md).
> Diese Datei (`INDEX.md`) bleibt als ergänzender Einstieg erhalten; für die vollständige Dokumentationsnavigation bitte den kanonischen Index nutzen.
> <!-- duplicate-notice: DOC-WEEKLY-2026-33 — DOCS-INV-005 -->

**Version:** 1.8.0-rc1  
**Last Updated:** April 2026

Welcome to the ThemisDB documentation! This guide will help you find the information you need.

---

## 🚀 Aktuell: v1.8.0-rc1

**Vollständige Informationen:**
- [Changelog](../../CHANGELOG.md) - Vollständige Änderungsliste
- [Release Notes v1.8.0](../releases/RELEASE_NOTES_v1.8.0.md) - Feature Release
- [Roadmap](../../ROADMAP.md) - Aggregierte Roadmap über alle 46 Module

**Highlights in v1.8.0-rc1:**
- 🌍 **Geo-Modul**: Vollständiges GeoJSON RFC 7946 Parsing + In-Memory R-Tree Spatial Index
- 🔐 **Auth**: German eID Online-Ausweisfunktion (BSI TR-03130/eIDAS) — v1.9.0
- 🔍 **Query**: ShardKey-Routing — point-lookup → 1 Shard statt scatter-gather — v1.9.0
- 🧠 **Analytics**: Forecasting Batch Prediction + Streaming Update + Parallel Auto-Tune — v1.9.0
- 🕸️ **Scraper Plugin**: v1.1.0 — Provenance-Felder auf allen Records; 56-Quellen-Katalog
- 🇩🇪 **German E-Gov**: OZG, XÖV, XDOMEA, eID Integrationen (v2.2.0/v1.9.0)

---

## 📚 Quick Navigation

**New to ThemisDB?**
- [Quick Start Guide](guides/QUICK_START.md) - Get up and running in 5 minutes
- [Installation Guide](guides/INSTALLATION.md) - Complete installation instructions
- [Architecture Overview](architecture/OVERVIEW.md) - Understand how ThemisDB works
- [🧠 LLM Complete Setup Guide](guides/LLM_COMPLETE_SETUP_GUIDE.md) - **NEU** Vollständiger Guide für LLM-Setup und Inferencing

**Module Documentation:**
Module documentation is located directly in the source code directories:
- Source modules: `../../src/<module>/README.md` and `../../src/<module>/FUTURE_ENHANCEMENTS.md`
- Header documentation: `../../include/<module>/README.md`

**Module-specific Documentation in docs/de/:**
All source code modules now have corresponding documentation directories for German-language guides, tutorials, and implementation notes:
- **Foundation Layer:** [core](core/), [storage](storage/), [transaction](transaction/), [themis](themis/), [base](base/), [utils](utils/)
- **Query & Index:** [query](query/), [aql](aql/), [index](index/), [search](search/), [temporal](temporal/), [timeseries](timeseries/)
- **Security:** [security](security/), [auth](auth/), [governance](governance/)
- **Server & Network:** [server](server/), [network](network/), [api](api/), [sharding](sharding/)
- **Intelligence:** [rag](rag/), [llm](llm/), [analytics](analytics/), [voice](voice/)
- **Operations:** [performance](performance/), [observability](observability/), [updates](updates/), [scheduler](scheduler/)
- **Data Integration:** [importers](importers/), [exporters](exporters/), [cdc](cdc/), [plugins](plugins/)
- **Distributed:** [replication](replication/), sharding
- **Specialized:** [graph](graph/), [chimera](chimera/), [geo](geo/), [acceleration](acceleration/)
- **Utility:** [metadata](metadata/), [gpu](gpu/), [cache](cache/), [content](content/)
- **Additional:** [prompt_engineering](prompt_engineering/)

**Business & Strategy:**
- [Strategiepapier: Industrie 4.0 & IoT](strategie/STRATEGIEPAPIER_INDUSTRIE_4_0_IOT.md) - ThemisDB für Smart Manufacturing & IoT-Anwendungen

**Using ThemisDB:**
- [AQL Query Language](aql/aql_syntax.md) - Learn the query language
- [REST API Reference](apis/HTTP_API_REFERENCE.md) - HTTP API documentation
- [Client SDKs](clients/README.md) - SDK documentation for 7 languages

**Operating ThemisDB:**
- [Configuration](operations/CONFIGURATION.md) - Configure your database
- [Monitoring](operations/MONITORING.md) - Monitor performance and health
- [Backup & Recovery](operations/BACKUP.md) - Protect your data

---

## 📖 Documentation Structure

### Getting Started

| Document | Description |
|----------|-------------|
| [Quick Start](guides/QUICK_START.md) | 5-minute tutorial to get started |
| [Installation](guides/INSTALLATION.md) | Installation on Linux, Windows, macOS, Docker |
| [Configuration](operations/CONFIGURATION.md) | Configure ThemisDB for your needs |
| [First Query](guides/FIRST_QUERY.md) | Write your first AQL query |

### Core Concepts

| Document | Description |
|----------|-------------|
| [Architecture Overview](architecture/OVERVIEW.md) | High-level system architecture |
| [Multi-Model Design](architecture/architecture_base_entity.md) | How ThemisDB handles multiple data models |
| [Transaction Model](features/features_transactions.md) | ACID transactions with MVCC |
| [Storage Layer](architecture/architecture_storage.md) | RocksDB LSM-Tree storage |

### Features

| Document | Description |
|----------|-------------|
| [Feature Overview](features/features_overview.md) | Complete feature catalog |
| [Vector Search](features/features_vector_ops.md) | Similarity search with HNSW/FAISS |
| [Graph Operations](features/features_graph.md) | Graph traversal and pathfinding |
| [Time-Series](features/features_time_series.md) | Time-series data and compression |
| [Hypertables](features/features_hypertables.md) | TimescaleDB-compatible time-series (v1.2+) |
| [Hybrid Search](search/hybrid_search_design.md) | RAG-optimized BM25+Vector search (v1.2+) |
| [Analytics](observability/CEP_STREAMING_ANALYTICS.md) | CEP and OLAP analytics |

### Query Language (AQL)

| Document | Description |
|----------|-------------|
| [AQL Syntax](aql/aql_syntax.md) | Complete AQL language reference |
| [AQL Examples](aql/aql_examples.md) | Common query patterns |
| [Query Optimization](aql/aql_explain_profile.md) | EXPLAIN and PROFILE commands |

### API Reference

| Document | Description |
|----------|-------------|
| [REST API](apis/HTTP_API_REFERENCE.md) | HTTP API endpoints |
| [GraphQL API](apis/apis_graphql.md) | GraphQL interface |
| [Client SDKs](clients/README.md) | SDKs for Python, JS, Rust, Go, Java, C#, Swift |

### Security & Compliance

| Document | Description |
|----------|-------------|
| [Security Overview](security/security_implementation.md) | Enterprise security features |
| [TLS Setup](guides/guides_tls_setup.md) | Configure TLS 1.3 and mTLS |
| [RBAC Configuration](guides/guides_rbac.md) | Role-based access control |
| [Encryption](security/security_encryption_strategy.md) | Data encryption at rest and in transit |
| [Audit Logging](features/features_audit_logging.md) | Security event logging |
| [Compliance](compliance/compliance_dashboard.md) | GDPR, SOC 2, HIPAA compliance |

### Enterprise & Governance

| Document | Description |
|----------|-------------|
| [CMS Strategy Paper](strategie/CMS_STRATEGY_PAPER.md) | ThemisDB für Content Management (Government & Enterprise) |
| [Enterprise Edition](enterprise/README.md) | Enterprise features and licensing |
| [Governance Overview](governance/README.md) | Data governance and policies |

### Operations

| Document | Description |
|----------|-------------|
| [Deployment Guide](deployment/README.md) | Production deployment strategies |
| [Docker Deployment](deployment/DOCKER_DEPLOYMENT.md) | Docker and Kubernetes deployment |
| [Configuration](operations/CONFIGURATION.md) | Configuration reference |
| [Monitoring](operations/MONITORING.md) | Prometheus metrics and alerting |
| [Backup & Recovery](operations/BACKUP.md) | Backup strategies and disaster recovery |
| [Troubleshooting](operations/TROUBLESHOOTING.md) | Common issues and solutions |
| [Performance Tuning](performance/performance_memory.md) | Optimize for your workload |

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
| [vLLM Co-Location](reports/VARIANT_STRATEGY_v1.1.0.md) | AI/ML workload optimization |
| [Content Processing](content/content_architecture.md) | Process PDFs, images, videos, etc. |

### Release Notes

| Document | Description |
|----------|-------------|
| [Changelog](releases/CHANGELOG.md) | Version history and changes |
| [Roadmap](roadmap/roadmap_overview.md) | Future plans and features |
| [v1.8.0 Release](releases/RELEASE_NOTES_v1.8.0.md) | Latest release notes |
| [v1.7.0 Release](releases/RELEASE_NOTES_v1.7.0.md) | Previous release |
| [v1.5.0 Release](releases/RELEASE_NOTES_v1.5.0.md) | Previous release |
| [v1.3.0 Release](releases/RELEASE_NOTES_v1.3.0.md) | Previous release |
| [v1.2.0 Release](releases/v1.2.0.md) | Previous release |
| [v1.1.0 Release](releases/v1.1.0.md) | Previous release |
| [Migration Guides](guides/MIGRATION.md) | Upgrade between versions |

---

## 🔍 Search by Topic

### By Use Case

**Building an Application:**
- [Quick Start](guides/QUICK_START.md) → [REST API](apis/HTTP_API_REFERENCE.md) → [Client SDKs](clients/README.md)

**Analytics & BI:**
- [OLAP Features](observability/README.md) → [Parquet Export](observability/README.md) → [Time-Series](timeseries/README.md)

**AI/ML Applications:**
- [Vector Search](search/README.md) → [Hybrid Search](search/hybrid_search_design.md) → [Embedding Cache](storage/README.md)

**Graph Applications:**
- [Graph Operations](features/README.md) → [AQL Graph Queries](aql/README.md) → [Path Algorithms](features/README.md)

**Production Deployment:**
- [Deployment Guide](deployment/README.md) → [Monitoring](observability/README.md) → [Backup](deployment/README.md) → [Security](security/README.md)

### By Technology

**Docker/Kubernetes:**
- [Docker Deployment](deployment/DOCKER_DEPLOYMENT.md)
- [Kubernetes Guide](deployment/README.md)
- [Helm Charts](README.md)

**Cloud Platforms:**
- [AWS/Azure/GCP Deployment](deployment/README.md)

**ARM/Raspberry Pi:**
- [ARM Build Guide](build/README.md)
- [ARM Deployment](deployment/README.md)

---

## 📊 Performance & Benchmarks

- [Performance Overview](performance/README.md)
- [Benchmarking Guide](README.md)
- [Memory Tuning](performance/README.md)
- [GPU Performance](performance/GPU_ACCELERATION_PLAN.md)
- [Query Optimization](performance/README.md)

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
