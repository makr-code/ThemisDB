# ThemisDB Module Architecture Index

**Version:** 1.0  
**Last Updated:** 2026-04-06  
**Status:** Complete

---

## Overview

Each `src/<module>/` directory contains an `ARCHITECTURE.md` file that documents its
design, components, data flows, integration points, configuration, and security
considerations. This file is the index of those per-module guides.

For the overall system architecture see the root [`ARCHITECTURE.md`](../../ARCHITECTURE.md).
For the source directory inventory see [`SOURCE_DIRECTORY_GUIDE.md`](./SOURCE_DIRECTORY_GUIDE.md).

---

## Module Architecture Guides

### Core Infrastructure

| Module | Guide | Status |
|---|---|---|
| `src/base/` | [ARCHITECTURE.md](../../src/base/ARCHITECTURE.md) | Base classes, module loader |
| `src/utils/` | [ARCHITECTURE.md](../../src/utils/ARCHITECTURE.md) | Shared utilities: audit, PII, HKDF, cron, SIMD |
| `src/core/` | [ARCHITECTURE.md](../../src/core/ARCHITECTURE.md) | ConcernsContext, logger, error handling |
| `src/config/` | [ARCHITECTURE.md](../../src/config/ARCHITECTURE.md) | Configuration management |
| `src/themis/` | [ARCHITECTURE.md](../../src/themis/ARCHITECTURE.md) | Main orchestration, edition manager |

### Storage & Persistence

| Module | Guide | Status |
|---|---|---|
| `src/storage/` | [ARCHITECTURE.md](../../src/storage/ARCHITECTURE.md) | RocksDB wrapper, MVCC, BlobDB, key schema |
| `src/cache/` | [ARCHITECTURE.md](../../src/cache/ARCHITECTURE.md) | Multi-level cache, TieredCacheManager |
| `src/transaction/` | [ARCHITECTURE.md](../../src/transaction/ARCHITECTURE.md) | ACID, SAGA, 2PC, branch/merge |
| `src/temporal/` | [ARCHITECTURE.md](../../src/temporal/ARCHITECTURE.md) | Bitemporal, time-travel queries |
| `src/timeseries/` | [ARCHITECTURE.md](../../src/timeseries/ARCHITECTURE.md) | Gorilla compression, TSStore, continuous aggregation |

### Query & Processing

| Module | Guide | Status |
|---|---|---|
| `src/query/` | [ARCHITECTURE.md](../../src/query/ARCHITECTURE.md) | AQL parser, cost-based optimizer, execution engine |
| `src/aql/` | [ARCHITECTURE.md](../../src/aql/ARCHITECTURE.md) | AQL NL-to-query, BPMN, process mining |
| `src/analytics/` | [ARCHITECTURE.md](../../src/analytics/ARCHITECTURE.md) | OLAP, NLP, graph analytics |
| `src/metadata/` | [ARCHITECTURE.md](../../src/metadata/ARCHITECTURE.md) | Schema catalog, INFORMATION_SCHEMA, statistics |

### Index & Search

| Module | Guide | Status |
|---|---|---|
| `src/index/` | [ARCHITECTURE.md](../../src/index/ARCHITECTURE.md) | Vector (HNSW/IVF), B-tree, graph, spatial, inverted |
| `src/search/` | [ARCHITECTURE.md](../../src/search/ARCHITECTURE.md) | Hybrid BM25+vector, RRF, LLM reranker |
| `src/geo/` | [ARCHITECTURE.md](../../src/geo/ARCHITECTURE.md) | Geospatial: PostGIS-compatible ST_* functions |
| `src/graph/` | [ARCHITECTURE.md](../../src/graph/ARCHITECTURE.md) | Property graph, BFS/DFS/PageRank, BPMN |

### LLM & AI

| Module | Guide | Status |
|---|---|---|
| `src/llm/` | [ARCHITECTURE.md](../../src/llm/ARCHITECTURE.md) | LLM inference, paged KV-cache, LoRA, grammar, vision |
| `src/rag/` | [ARCHITECTURE.md](../../src/rag/ARCHITECTURE.md) | RAG pipeline, hybrid retrieval, multi-judge evaluation |
| `src/prompt_engineering/` | [ARCHITECTURE.md](../../src/prompt_engineering/ARCHITECTURE.md) | Prompt lifecycle, A/B testing, auto-optimization |
| `src/training/` | [ARCHITECTURE.md](../../src/training/ARCHITECTURE.md) | Auto-labeling, LoRA training, graph enrichment |
| `src/acceleration/` | [ARCHITECTURE.md](../../src/acceleration/ARCHITECTURE.md) | CUDA/OpenCL/Metal GPU compute backends |
| `src/gpu/` | [ARCHITECTURE.md](../../src/gpu/ARCHITECTURE.md) | GPU lifecycle, VRAM management |

### Server & API

| Module | Guide | Status |
|---|---|---|
| `src/server/` | [ARCHITECTURE.md](../../src/server/ARCHITECTURE.md) | HTTP/gRPC/WS/MQTT/PG wire, 40+ handlers |
| `src/api/` | [ARCHITECTURE.md](../../src/api/ARCHITECTURE.md) | Client-side API wrappers |
| `src/network/` | [ARCHITECTURE.md](../../src/network/ARCHITECTURE.md) | Binary wire protocol, TLS, QoS |
| `src/voice/` | [ARCHITECTURE.md](../../src/voice/ARCHITECTURE.md) | Voice assistant, Whisper STT, voice-to-AQL |

### Security & Auth

| Module | Guide | Status |
|---|---|---|
| `src/auth/` | [ARCHITECTURE.md](../../src/auth/ARCHITECTURE.md) | JWT, OIDC, RBAC, MFA, session |
| `src/security/` | [ARCHITECTURE.md](../../src/security/ARCHITECTURE.md) | AES-256-GCM, HSM/Vault, RBAC, RLS, PII, zero-trust |
| `src/governance/` | [ARCHITECTURE.md](../../src/governance/ARCHITECTURE.md) | Policy engine, compliance, AI ethics |

### Distributed Systems

| Module | Guide | Status |
|---|---|---|
| `src/sharding/` | [ARCHITECTURE.md](../../src/sharding/ARCHITECTURE.md) | Raft/Gossip/Paxos, SAGA, ShardRepairEngine |
| `src/replication/` | [ARCHITECTURE.md](../../src/replication/ARCHITECTURE.md) | Leader-follower Raft, WAL shipping, PITR |
| `src/cdc/` | [ARCHITECTURE.md](../../src/cdc/ARCHITECTURE.md) | Change Data Capture, SSE, Kafka, webhooks |

### Observability

| Module | Guide | Status |
|---|---|---|
| `src/observability/` | [ARCHITECTURE.md](../../src/observability/ARCHITECTURE.md) | Prometheus, OpenTelemetry, profiler, alerting |
| `src/performance/` | [ARCHITECTURE.md](../../src/performance/ARCHITECTURE.md) | Cycle metrics, WiscKey, Cicada, LIRS, NUMA |

### Data Integration

| Module | Guide | Status |
|---|---|---|
| `src/ingestion/` | [ARCHITECTURE.md](../../src/ingestion/ARCHITECTURE.md) | Multi-source data ingestion pipeline |
| `src/importers/` | [ARCHITECTURE.md](../../src/importers/ARCHITECTURE.md) | Format importers: CSV, JSON, Parquet, XML |
| `src/exporters/` | [ARCHITECTURE.md](../../src/exporters/ARCHITECTURE.md) | Format exporters: CSV, JSON, Parquet, Arrow |
| `src/content/` | [ARCHITECTURE.md](../../src/content/ARCHITECTURE.md) | Binary content: PDF, DOCX, images, STT |

### Extensibility & Lifecycle

| Module | Guide | Status |
|---|---|---|
| `src/plugins/` | [ARCHITECTURE.md](../../src/plugins/ARCHITECTURE.md) | Plugin loading, Ed25519 signing, hot-plug |
| `src/scheduler/` | [ARCHITECTURE.md](../../src/scheduler/ARCHITECTURE.md) | Cron scheduler, hybrid retention manager |
| `src/updates/` | [ARCHITECTURE.md](../../src/updates/ARCHITECTURE.md) | Hot-reload, schema migration, canary rollout |
| `src/chimera/` | [ARCHITECTURE.md](../../src/chimera/ARCHITECTURE.md) | Chimera benchmark and evaluation |

---

## How to Use These Guides

1. **New contributors** – start with `ARCHITECTURE.md` (root), then read the guides for
   the modules you are working on.
2. **Feature development** – check the module guide for the target module to understand
   how the component fits in, what interfaces to implement against, and how to test.
3. **Bug investigation** – the Data Flow section of each guide traces the path of a
   request; use it to identify which component handles a given operation.
4. **Cross-module changes** – the Integration Points table in each guide lists what a
   module depends on and what consumes it.

---

## Guide Conventions

Each `ARCHITECTURE.md` follows this structure:

1. **Overview** – what the module does
2. **Design Principles** – key architectural decisions
3. **Component Architecture** – key files and a component diagram
4. **Data Flow** – how data moves through the module
5. **Integration Points** – what the module depends on and exposes
6. **Threading & Concurrency Model** – how concurrency is handled
7. **Performance Architecture** – key optimization techniques
8. **Security Considerations** – security properties and threats
9. **Configuration** – key configuration parameters
10. **Error Handling** – how errors are handled
11. **Known Limitations & Future Work** – open items
12. **References** – links to READMEs, docs, papers

---

*Last updated automatically by the architecture guide generation process.*
