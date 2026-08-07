# Modules & Namespaces Overview

**Datum:** 2026-08-03  
**Status:** Active  
**Primary:** src/<module>/ROADMAP.md, include/<module>/*.h  
**Bezug:** AI-Context für schnelle Modul-/Namespace-Navigation

## Quick Reference: 62 Integrated Modules × Namespaces × Tier Classification

**See also:** [ARCHITECTURE_CLASSIFICATION.md](./ARCHITECTURE_CLASSIFICATION.md) for comprehensive tier definitions (T0–T5 model including 5 core T0 modules).

| Module | Namespace | Tier | Task | Docs | Plugin Version |
|--------|-----------|------|------|------|----------------|
| **base** | `themis::resource` | **T0** | Resource mgmt, RAII, base types | [ROADMAP](../src/base/ROADMAP.md) | — |
| **core** | `themis::core` | **T0** | Engine core, MVCC, transactions | [ROADMAP](../src/core/ROADMAP.md) | — |
| **plugins** | `themis::plugins` | **T0** | Plugin loader, manifests | [ROADMAP](../src/plugins/ROADMAP.md) | — |
| **themis** | `themis` | **T0** | Root namespace, aggregation, edition mgmt | [ROADMAP](../src/themis/ROADMAP.md) | — |
| **utils** | `themis::utils` | **T0** | General utilities, string/math | [ROADMAP](../src/utils/ROADMAP.md) | — |
| **aql** | `themis::aql` | **T1** | Query language, parser, optimizer | [ROADMAP](../src/aql/ROADMAP.md) | — |
| **cache** | `themis::cache` | **T1** | Query/semantic/plan caching | [ROADMAP](../src/cache/ROADMAP.md) | — |
| **execution** | `themis::execution` | **T1** | Query execution engine, operators | [ROADMAP](../src/execution/ROADMAP.md) | — |
| **index** | `themis::index` | **T1** | Index structures (HNSW, B-tree, R-tree) | [ROADMAP](../src/index/ROADMAP.md) | — |
| **metadata** | `themis::metadata` | **T1** | Schema registry, catalog | [ROADMAP](../src/metadata/ROADMAP.md) | — |
| **query** | `themis::query` | **T1** | Query planning, optimization | [ROADMAP](../src/query/ROADMAP.md) | — |
| **storage** | `themis::storage` | **T1** | RocksDB backend, K-V layer | [ROADMAP](../src/storage/ROADMAP.md) | — |
| **acceleration** | `themis::acceleration` | **T3** | GPU dispatch, CUDA/OpenCL/TPU backends | [ROADMAP](../src/acceleration/ROADMAP.md) | — |
| **access_model** | `themis::access_model` | **T3** | Data access patterns, security enforcement | [ROADMAP](../src/access_model/ROADMAP.md) | — |
| **ai** | `themis::ai` | **T3** | AI/ML orchestration, inference core | [ROADMAP](../src/ai/ROADMAP.md) | — |
| **analytics** | `themis::analytics` | **T3** | Time-series analysis, Arrow export | [ROADMAP](../src/analytics/ROADMAP.md) | — |
| **api** | `themis::api` | **T3** | HTTP/gRPC/GraphQL transport, routing | [ROADMAP](../src/api/ROADMAP.md) | — |
| **auth** | `themis::auth` | **T3** | Authentication, authorization, principals | [ROADMAP](../src/auth/ROADMAP.md) | — |
| **cdc** | `themis::cdc` | **T3** | Change capture, streaming pipelines | [ROADMAP](../src/cdc/ROADMAP.md) | — |
| **chaos** | `themis::chaos` | **T3** | Chaos testing, failure injection | [ROADMAP](../src/chaos/ROADMAP.md) | — |
| **chimera** | `themis::chimera` | **T3** | Adapter architecture, compatibility | [ROADMAP](../src/chimera/ROADMAP.md) | — |
| **config** | `themis::config` | **T3** | Configuration, environment parsing | [ROADMAP](../src/config/ROADMAP.md) | — |
| **content** | `themis::content` | **T3** | Full-text indexing, content processing | [ROADMAP](../src/content/ROADMAP.md) | — |
| **distributed_knowledge** | `themis::distributed_knowledge` | **T3** | Distributed knowledge graphs | [ROADMAP](../src/distributed_knowledge/ROADMAP.md) | — |
| **distributed_tensor** | `themis::distributed_tensor` | **T3** | Distributed tensor ops, sharding | [ROADMAP](../src/distributed_tensor/ROADMAP.md) | — |
| **document** | `themis::document` | **T3** | Document store, JSON/BSON | [ROADMAP](../src/document/ROADMAP.md) | — |
| **ethics_ai** | `themis::ethics_ai` | **T3** | AI ethics, bias detection | [ROADMAP](../src/ethics_ai/ROADMAP.md) | ✅ `plugins/ethics_ai/`, 🔒 `themisdb_ethic_ai/` (Wave 1) |
| **evaluation** | `themis::evaluation` | **T3** | Model evaluation, benchmarking | [ROADMAP](../src/evaluation/ROADMAP.md) | — |
| **exporters** | `themis::exporters` | **T3** | Export (Parquet, Arrow, CSV, JSON) | [ROADMAP](../src/exporters/ROADMAP.md) | ✅ `plugins/exporters/` |
| **failover** | `themis::failover` | **T3** | Auto-failover, disaster recovery | [ROADMAP](../src/failover/ROADMAP.md) | — |
| **geo** | `themis::geo` | **T3** | Geospatial indexing, PostGIS compat | [ROADMAP](../src/geo/ROADMAP.md) | ✅ `plugins/themisdb_geo/` (Wave 1 candidate) |
| **governance** | `themis::governance` | **T3** | Compliance, audit, governance | [ROADMAP](../src/governance/ROADMAP.md) | — |
| **gpu** | `themis::gpu` | **T3** | GPU memory, kernel dispatch | [ROADMAP](../src/gpu/ROADMAP.md) | — |
| **graph** | `themis::graph` | **T3** | Graph model, traversal, GQL | [ROADMAP](../src/graph/ROADMAP.md) | — |
| **importers** | `themis::importers` | **T3** | Data import, ETL, readers | [ROADMAP](../src/importers/ROADMAP.md) | ✅ `plugins/importers/`, 🔒 `themisdb_importer/` (Wave 1) |
| **ingestion** | `themis::ingestion` | **T3** | Batch/streaming ingestion | [ROADMAP](../src/ingestion/ROADMAP.md) | — |
| **llama_cpp** | `themis::llama_cpp` | **T3** | Llama.cpp binding, local inference | [ROADMAP](../src/llama_cpp/ROADMAP.md) | ✅ `plugins/llama_cpp/` |
| **llm** | `themis::llm` | **T3** | LLM inference, model switching | [ROADMAP](../src/llm/ROADMAP.md) | — |
| **llm_wiki** | `themis::llm_wiki` | **T3** | LLM-assisted wiki/RAG | [ROADMAP](../src/llm_wiki/ROADMAP.md) | 🔒 `themisdb_llm_wiki/` (Wave 1) |
| **maintenance** | `themis::maintenance` | **T3** | GC, compaction, maintenance | [ROADMAP](../src/maintenance/ROADMAP.md) | — |
| **network** | `themis::network` | **T3** | Transport, conn pooling, messaging | [ROADMAP](../src/network/ROADMAP.md) | — |
| **observability** | `themis::observability` | **T3** | Metrics, logging, tracing | [ROADMAP](../src/observability/ROADMAP.md) | — |
| **onnx_clip** | `themis::onnx_clip` | **T3** | ONNX, CLIP multimodal embeddings | [ROADMAP](../src/onnx_clip/ROADMAP.md) | — |
| **performance** | `themis::performance` | **T3** | Perf tracking, optimization | [ROADMAP](../src/performance/ROADMAP.md) | — |
| **process** | `themis::process` | **T3** | Process lifecycle, init/shutdown | [ROADMAP](../src/process/ROADMAP.md) | — |
| **projects** | `themis::projects` | **T3** | Project/tenant management | [ROADMAP](../src/projects/ROADMAP.md) | — |
| **prompt_engineering** | `themis::prompt_engineering` | **T3** | Prompt templates, engineering | [ROADMAP](../src/prompt_engineering/ROADMAP.md) | — |
| **rag** | `themis::rag` | **T3** | Retrieval-augmented generation | [ROADMAP](../src/rag/ROADMAP.md) | — |
| **replication** | `themis::replication` | **T3** | Replication, consistency | [ROADMAP](../src/replication/ROADMAP.md) | — |
| **retrieval** | `themis::retrieval` | **T3** | Vector/semantic search | [ROADMAP](../src/retrieval/ROADMAP.md) | — |
| **rpc_grpc** | `themis::rpc_grpc` | **T3** | gRPC service defs, RPC contracts | [ROADMAP](../src/rpc_grpc/ROADMAP.md) | — |
| **scheduler** | `themis::scheduler` | **T3** | Task scheduling, job coordination | [ROADMAP](../src/scheduler/ROADMAP.md) | — |
| **scraper** | `themis::scraper` | **T3** | Web scraping, content drivers | [ROADMAP](../src/scraper/ROADMAP.md) | ✅ `plugins/scraper/` |
| **search** | `themis::search` | **T3** | Full-text & semantic search | [ROADMAP](../src/search/ROADMAP.md) | — |
| **security** | `themis::security` | **T3** | Security primitives, encryption | [ROADMAP](../src/security/ROADMAP.md) | — |
| **server** | `themis::server` | **T3** | HTTP server, request handling | [ROADMAP](../src/server/ROADMAP.md) | — |
| **sharding** | `themis::sharding` | **T3** | Sharding strategy, distribution | [ROADMAP](../src/sharding/ROADMAP.md) | — |
| **stable_diffusion** | `themis::stable_diffusion` | **T3** | Stable Diffusion image generation | [ROADMAP](../src/stable_diffusion/ROADMAP.md) | ✅ `plugins/stable_diffusion/` |
| **temporal** | `themis::temporal` | **T3** | Temporal data, time optimization | [ROADMAP](../src/temporal/ROADMAP.md) | — |
| **tensor** | `themis::tensor` | **T3** | Tensor ops, numerical computing | [ROADMAP](../src/tensor/ROADMAP.md) | — |
| **timeseries** | `themis::timeseries` | **T3** | Time-series indexing, retention | [ROADMAP](../src/timeseries/ROADMAP.md) | ✅ `plugins/themisdb_timeseries/` (Wave 1 candidate) |
| **toolbox** | `themis::toolbox` | **T3** | Utilities, helpers, algorithms | [ROADMAP](../src/toolbox/ROADMAP.md) | — |
| **training** | `themis::training` | **T3** | Model training, fine-tuning, LoRA | [ROADMAP](../src/training/ROADMAP.md) | — |
| **transaction** | `themis::transaction` | **T3** | Transaction coordinator, 2PC/3PC/SAGA | [ROADMAP](../src/transaction/ROADMAP.md) | — |
| **updates** | `themis::updates` | **T3** | Update ops, mutations, conflict resolution | [ROADMAP](../src/updates/ROADMAP.md) | — |
| **user_storage_encrypted** | `themis::user_storage_encrypted` | **T3** | User data encryption, field-level | [ROADMAP](../src/user_storage_encrypted/ROADMAP.md) | ✅ `plugins/user_storage_encrypted/`, 🔒 `themisdb_storage/` (Wave 1) |
| **voice** | `themis::voice` | **T3** | Voice I/O, speech synthesis | [ROADMAP](../src/voice/ROADMAP.md) | — |
| **whisper** | `themis::whisper` | **T3** | OpenAI Whisper, speech-to-text | [ROADMAP](../src/whisper/ROADMAP.md) | ✅ `plugins/whisper/` |

**Legend:**
- **Tier**: T0 (Trusted Core), T1–T2 (Engine), T3–T4 (Infrastructure)
- **Plugin**: ✅ = public plugin available, 🔒 = private plugin (Wave 1+), — = no plugin version
- See [ARCHITECTURE_CLASSIFICATION.md](./ARCHITECTURE_CLASSIFICATION.md) for full tier definitions

## Namespace Hierarchy Pattern

```
themis::                           // root, forward decls
  ├── <module>::                   // public API for each module
  │   ├── detail::                 // private impl details
  │   ├── interface::              // abstract contracts (for plugins)
  │   └── internal::               // cross-module helpers
  └── <category>::                 // e.g., themis::network, themis::security
```

## Separation of Concerns (SOC) Boundaries

| Layer | Modules | Ownership |
|-------|---------|-----------|
| **Transport** | server, api, network, rpc_grpc | HTTP/gRPC/WebSocket protocols |
| **Auth & Security** | auth, security, governance, access_model | Identity, encryption, policy |
| **Query** | aql, query, execution | Parsing, optimization, execution |
| **Indexing** | index, cache, content, search, retrieval, geo | Data structures, search ops |
| **Data Model** | document, graph, timeseries, temporal, tensor | Multi-model storage contracts |
| **Storage** | storage, replication, sharding, failover, cdc | Persistence, distribution, HA |
| **AI/ML** | llm, ai, training, evaluation, rag, prompt_engineering | Inference, fine-tuning, ops |
| **Infrastructure** | config, observability, chaos, performance, scheduler | Cross-cutting concerns |

## Consumer Relationships (High-Level Dependencies)

- **api** → server, auth, query, execution, aql
- **query** → aql, execution, index, cache, storage
- **execution** → index, retrieval, graph, timeseries, tensor
- **storage** → core, replication, sharding, maintenance
- **llm** → ai, retrieval, rag, prompt_engineering, training
- **index** → storage, cache, performance, acceleration
- **auth** → governance, security, access_model, observability

---

**Zuletzt geprueft (Modules & Namespaces):** 2026-08-03
