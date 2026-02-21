# ThemisDB Source Code Roadmap

## Vision

ThemisDB aims to be a production-grade, multi-model database engine that unifies relational, document, graph, vector, geospatial, and timeseries data under a single query language (AQL). The development roadmap focuses on hardening production modules, graduating Beta components to Stable, and delivering enterprise-grade observability, security, and scalability.

---

## Overall System Status

| Module              | Status     | Notes                                              |
|---------------------|------------|----------------------------------------------------|
| acceleration        | Alpha      | CUDA/Vulkan GPU acceleration, early stage          |
| analytics           | Beta       | Analytical query pipeline, needs hardening         |
| api                 | Beta       | HTTP API server, stabilizing                       |
| aql                 | Production | AQL language engine, multi-paradigm query support  |
| auth                | Beta       | JWT/RBAC/SSO, enterprise integration in progress   |
| base                | Alpha      | Foundational abstractions, under active design     |
| cache               | Beta       | Semantic and query result caching                  |
| cdc                 | Beta       | Change Data Capture and changefeeds                |
| chimera             | Alpha      | Hybrid model layer, experimental                   |
| config              | Alpha      | Configuration management, early stage              |
| content             | Beta       | Content ingestion and processing pipelines         |
| core                | Beta       | Core database runtime, stabilizing                 |
| exporters           | Beta       | Data export (JSONL, LLM formats)                   |
| geo                 | Beta       | Geospatial query and indexing                      |
| governance          | Beta       | Policy engine and compliance governance            |
| gpu                 | Beta       | GPU compute integration                            |
| graph               | Beta       | Property graph queries and traversal               |
| importers           | Beta       | Data import (PostgreSQL, etc.)                     |
| index               | Production | HNSW, R-tree, adaptive indexing                    |
| ingestion           | Alpha      | Data ingestion pipeline, early stage               |
| llm                 | Beta       | LLM interaction storage and chain-of-thought       |
| metadata            | Beta       | Metadata management and catalog                    |
| network             | Alpha      | Network layer and peer communication               |
| observability       | Beta       | Metrics, tracing, and logging infrastructure       |
| performance         | Beta       | Benchmarking and performance optimization          |
| plugins             | Alpha      | Plugin system infrastructure                       |
| prompt_engineering  | Alpha      | LLM prompt management, experimental               |
| query               | Beta       | AQL optimizer, cost-based planner, execution       |
| rag                 | Beta       | Retrieval-Augmented Generation pipeline            |
| replication         | Beta       | Raft-based replication                             |
| scheduler           | Alpha      | Task and job scheduling, early stage               |
| search              | Beta       | Full-text and hybrid search                        |
| security            | Beta       | Encryption, key management, PKI integration        |
| server              | Beta       | Main server and API handler components             |
| sharding            | Beta       | Horizontal scaling and sharding                    |
| storage             | Production | RocksDB wrapper, MVCC, backup/recovery             |
| temporal            | Beta       | Temporal query and bitemporal data support         |
| themis              | Beta       | Core ThemisDB orchestration layer                  |
| timeseries          | Beta       | Time series management and compression             |
| training            | Alpha      | ML model training integration, experimental        |
| transaction         | Beta       | SAGA pattern and distributed transactions          |
| updates             | Beta       | Schema and data update management                  |
| utils               | Beta       | Shared utility functions and helpers               |
| voice               | Alpha      | Voice query interface, experimental                |

---

## Release Timeline

### v1.4.0 — Q1 2026
**Focus: Core Stabilization and Beta Graduation**
- Graduate `query`, `core`, `server`, and `transaction` from Beta → Stable
- Harden `auth` with full OAuth2/OIDC enterprise integration
- Improve `cache` hit rates and invalidation logic
- Stabilize `replication` with tested failover scenarios
- Deliver `observability` dashboards (Prometheus/Grafana integration)

### v1.5.0 — Q2 2026
**Focus: AI/ML Integration and Data Pipeline Hardening**
- Graduate `llm`, `rag`, and `search` from Beta → Stable
- Advance `prompt_engineering` and `training` from Alpha → Beta
- Harden `ingestion` and `cdc` pipelines for production workloads
- Finalize `geo` and `graph` production readiness
- Deliver `analytics` Stable milestone with OLAP query support

### v2.0.0 — Q4 2026
**Focus: Full Production Readiness and Enterprise Features**
- All core modules at Stable or Production status
- Graduate `acceleration` and `gpu` to Beta with validated benchmarks
- `sharding` and `replication` production-hardened for 10+ node clusters
- `governance` and `security` achieve compliance certifications (SOC2, FIPS 140-2)
- `plugins` API stabilized with documented extension points
- `voice` and `chimera` reach Beta with validated use cases
- Full end-to-end multi-model query benchmark suite published

---

## Cross-Cutting Initiatives

### Performance Optimization
- GPU-accelerated vector search via `acceleration` and `gpu` modules
- Query plan caching and adaptive re-optimization in `query`
- Storage compression improvements in `storage` and `timeseries`
- Benchmark-driven regression detection via `performance` module

### Security Hardening
- Field-level encryption maturation across `storage` and `security`
- `auth` module: MFA enforcement, session management hardening
- `governance` module: policy-as-code and audit trail completeness
- Regular penetration testing and CVE response process

### Observability Improvements
- Distributed tracing (OpenTelemetry) across all request paths
- Per-module metrics exposed via `observability` Prometheus exporters
- Structured logging with correlation IDs throughout `server` and `query`
- Alerting runbooks for common failure modes

### Testing Infrastructure
- Unit test coverage targets: 80%+ for Production/Stable modules
- Integration test harness across all module boundaries
- Fuzz testing (`fuzz/`) expansion to cover AQL parser edge cases
- Chaos engineering for `replication`, `sharding`, and `transaction`

---

## Module-Specific Roadmaps

Individual modules with detailed roadmaps are linked below. Each `src/*/ROADMAP.md` provides granular milestone tracking and technical design decisions for that module.

- [acceleration/ROADMAP.md](acceleration/ROADMAP.md)
- [analytics/ROADMAP.md](analytics/ROADMAP.md)
- [api/ROADMAP.md](api/ROADMAP.md)
- [aql/ROADMAP.md](aql/ROADMAP.md)
- [auth/ROADMAP.md](auth/ROADMAP.md)
- [base/ROADMAP.md](base/ROADMAP.md)
- [cache/ROADMAP.md](cache/ROADMAP.md)
- [cdc/ROADMAP.md](cdc/ROADMAP.md)
- [chimera/ROADMAP.md](chimera/ROADMAP.md)
- [config/ROADMAP.md](config/ROADMAP.md)
- [content/ROADMAP.md](content/ROADMAP.md)
- [core/ROADMAP.md](core/ROADMAP.md)
- [exporters/ROADMAP.md](exporters/ROADMAP.md)
- [geo/ROADMAP.md](geo/ROADMAP.md)
- [governance/ROADMAP.md](governance/ROADMAP.md)
- [gpu/ROADMAP.md](gpu/ROADMAP.md)
- [graph/ROADMAP.md](graph/ROADMAP.md)
- [importers/ROADMAP.md](importers/ROADMAP.md)
- [index/ROADMAP.md](index/ROADMAP.md)
- [ingestion/ROADMAP.md](ingestion/ROADMAP.md)
- [llm/ROADMAP.md](llm/ROADMAP.md)
- [metadata/ROADMAP.md](metadata/ROADMAP.md)
- [network/ROADMAP.md](network/ROADMAP.md)
- [observability/ROADMAP.md](observability/ROADMAP.md)
- [performance/ROADMAP.md](performance/ROADMAP.md)
- [plugins/ROADMAP.md](plugins/ROADMAP.md)
- [prompt_engineering/ROADMAP.md](prompt_engineering/ROADMAP.md)
- [query/ROADMAP.md](query/ROADMAP.md)
- [rag/ROADMAP.md](rag/ROADMAP.md)
- [replication/ROADMAP.md](replication/ROADMAP.md)
- [scheduler/ROADMAP.md](scheduler/ROADMAP.md)
- [search/ROADMAP.md](search/ROADMAP.md)
- [security/ROADMAP.md](security/ROADMAP.md)
- [server/ROADMAP.md](server/ROADMAP.md)
- [sharding/ROADMAP.md](sharding/ROADMAP.md)
- [storage/ROADMAP.md](storage/ROADMAP.md)
- [temporal/ROADMAP.md](temporal/ROADMAP.md)
- [themis/ROADMAP.md](themis/ROADMAP.md)
- [timeseries/ROADMAP.md](timeseries/ROADMAP.md)
- [training/ROADMAP.md](training/ROADMAP.md)
- [transaction/ROADMAP.md](transaction/ROADMAP.md)
- [updates/ROADMAP.md](updates/ROADMAP.md)
- [utils/ROADMAP.md](utils/ROADMAP.md)
- [voice/ROADMAP.md](voice/ROADMAP.md)
