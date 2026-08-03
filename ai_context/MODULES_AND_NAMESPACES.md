# Modules & Namespaces Overview

**Datum:** 2026-08-03  
**Status:** Active  
**Primary:** src/<module>/ROADMAP.md, include/<module>/*.h  
**Bezug:** AI-Context für schnelle Modul-/Namespace-Navigation

## Quick Reference: 62 Modules × Namespaces

| Module | Namespace | Task | Docs |
|--------|-----------|------|------|
| acceleration | `themis::acceleration` | GPU dispatch, CUDA/OpenCL/TPU backends | [ROADMAP](../src/acceleration/ROADMAP.md) |
| access_model | `themis::access_model` | Data access patterns, security enforcement | [ROADMAP](../src/access_model/ROADMAP.md) |
| ai | `themis::ai` | AI/ML orchestration, inference core | [ROADMAP](../src/ai/ROADMAP.md) |
| analytics | `themis::analytics` | Time-series analysis, Arrow export | [ROADMAP](../src/analytics/ROADMAP.md) |
| api | `themis::api` | HTTP/gRPC/GraphQL transport, routing | [ROADMAP](../src/api/ROADMAP.md) |
| aql | `themis::aql` | Query language, parser, optimizer | [ROADMAP](../src/aql/ROADMAP.md) |
| auth | `themis::auth` | Authentication, authorization, principals | [ROADMAP](../src/auth/ROADMAP.md) |
| base | `themis::resource` | Resource mgmt, RAII, base types | [ROADMAP](../src/base/ROADMAP.md) |
| cache | `themis::cache` | Query/semantic/plan caching | [ROADMAP](../src/cache/ROADMAP.md) |
| cdc | `themis::cdc` | Change capture, streaming pipelines | [ROADMAP](../src/cdc/ROADMAP.md) |
| chaos | `themis::chaos` | Chaos testing, failure injection | [ROADMAP](../src/chaos/ROADMAP.md) |
| chimera | `themis::chimera` | Plugin system, adapter architecture | [ROADMAP](../src/chimera/ROADMAP.md) |
| config | `themis::config` | Configuration, environment parsing | [ROADMAP](../src/config/ROADMAP.md) |
| content | `themis::content` | Full-text indexing, content processing | [ROADMAP](../src/content/ROADMAP.md) |
| core | `themis::core` | Engine core, MVCC, transactions | [ROADMAP](../src/core/ROADMAP.md) |
| distributed_knowledge | `themis::distributed_knowledge` | Distributed knowledge graphs | [ROADMAP](../src/distributed_knowledge/ROADMAP.md) |
| distributed_tensor | `themis::distributed_tensor` | Distributed tensor ops, sharding | [ROADMAP](../src/distributed_tensor/ROADMAP.md) |
| document | `themis::document` | Document store, JSON/BSON | [ROADMAP](../src/document/ROADMAP.md) |
| ethics_ai | `themis::ethics_ai` | AI ethics, bias detection | [ROADMAP](../src/ethics_ai/ROADMAP.md) |
| evaluation | `themis::evaluation` | Model evaluation, benchmarking | [ROADMAP](../src/evaluation/ROADMAP.md) |
| execution | `themis::execution` | Query execution engine, operators | [ROADMAP](../src/execution/ROADMAP.md) |
| exporters | `themis::exporters` | Export (Parquet, Arrow, CSV, JSON) | [ROADMAP](../src/exporters/ROADMAP.md) |
| failover | `themis::failover` | Auto-failover, disaster recovery | [ROADMAP](../src/failover/ROADMAP.md) |
| geo | `themis::geo` | Geospatial indexing, PostGIS compat | [ROADMAP](../src/geo/ROADMAP.md) |
| governance | `themis::governance` | Compliance, audit, governance | [ROADMAP](../src/governance/ROADMAP.md) |
| gpu | `themis::gpu` | GPU memory, kernel dispatch | [ROADMAP](../src/gpu/ROADMAP.md) |
| graph | `themis::graph` | Graph model, traversal, GQL | [ROADMAP](../src/graph/ROADMAP.md) |
| importers | `themis::importers` | Data import, ETL, readers | [ROADMAP](../src/importers/ROADMAP.md) |
| index | `themis::index` | Index structures (HNSW, B-tree, R-tree) | [ROADMAP](../src/index/ROADMAP.md) |
| ingestion | `themis::ingestion` | Batch/streaming ingestion | [ROADMAP](../src/ingestion/ROADMAP.md) |
| llama_cpp | `themis::llama_cpp` | Llama.cpp binding, local inference | [ROADMAP](../src/llama_cpp/ROADMAP.md) |
| llm | `themis::llm` | LLM inference, model switching | [ROADMAP](../src/llm/ROADMAP.md) |
| llm_wiki | `themis::llm_wiki` | LLM-assisted wiki/RAG | [ROADMAP](../src/llm_wiki/ROADMAP.md) |
| maintenance | `themis::maintenance` | GC, compaction, maintenance | [ROADMAP](../src/maintenance/ROADMAP.md) |
| metadata | `themis::metadata` | Schema registry, catalog | [ROADMAP](../src/metadata/ROADMAP.md) |
| network | `themis::network` | Transport, conn pooling, messaging | [ROADMAP](../src/network/ROADMAP.md) |
| observability | `themis::observability` | Metrics, logging, tracing | [ROADMAP](../src/observability/ROADMAP.md) |
| onnx_clip | `themis::onnx_clip` | ONNX, CLIP multimodal embeddings | [ROADMAP](../src/onnx_clip/ROADMAP.md) |
| performance | `themis::performance` | Perf tracking, optimization | [ROADMAP](../src/performance/ROADMAP.md) |
| plugins | `themis::plugins` | Plugin loader, manifests | [ROADMAP](../src/plugins/ROADMAP.md) |
| process | `themis::process` | Process lifecycle, init/shutdown | [ROADMAP](../src/process/ROADMAP.md) |
| projects | `themis::projects` | Project/tenant management | [ROADMAP](../src/projects/ROADMAP.md) |
| prompt_engineering | `themis::prompt_engineering` | Prompt templates, engineering | [ROADMAP](../src/prompt_engineering/ROADMAP.md) |
| query | `themis::query` | Query planning, optimization | [ROADMAP](../src/query/ROADMAP.md) |
| rag | `themis::rag` | Retrieval-augmented generation | [ROADMAP](../src/rag/ROADMAP.md) |
| replication | `themis::replication` | Replication, consistency | [ROADMAP](../src/replication/ROADMAP.md) |
| retrieval | `themis::retrieval` | Vector/semantic search | [ROADMAP](../src/retrieval/ROADMAP.md) |
| rpc_grpc | `themis::rpc_grpc` | gRPC service defs, RPC contracts | [ROADMAP](../src/rpc_grpc/ROADMAP.md) |
| scheduler | `themis::scheduler` | Task scheduling, job coordination | [ROADMAP](../src/scheduler/ROADMAP.md) |
| scraper | `themis::scraper` | Web scraping, content drivers | [ROADMAP](../src/scraper/ROADMAP.md) |
| search | `themis::search` | Full-text & semantic search | [ROADMAP](../src/search/ROADMAP.md) |
| security | `themis::security` | Security primitives, encryption | [ROADMAP](../src/security/ROADMAP.md) |
| server | `themis::server` | HTTP server, request handling | [ROADMAP](../src/server/ROADMAP.md) |
| sharding | `themis::sharding` | Sharding strategy, distribution | [ROADMAP](../src/sharding/ROADMAP.md) |
| stable_diffusion | `themis::stable_diffusion` | Stable Diffusion image generation | [ROADMAP](../src/stable_diffusion/ROADMAP.md) |
| storage | `themis::storage` | RocksDB backend, K-V layer | [ROADMAP](../src/storage/ROADMAP.md) |
| temporal | `themis::temporal` | Temporal data, time optimization | [ROADMAP](../src/temporal/ROADMAP.md) |
| tensor | `themis::tensor` | Tensor ops, numerical computing | [ROADMAP](../src/tensor/ROADMAP.md) |
| themis | `themis` | Root namespace, aggregation | [ROADMAP](../src/themis/ROADMAP.md) |
| timeseries | `themis::timeseries` | Time-series indexing, retention | [ROADMAP](../src/timeseries/ROADMAP.md) |
| toolbox | `themis::toolbox` | Utilities, helpers, algorithms | [ROADMAP](../src/toolbox/ROADMAP.md) |
| training | `themis::training` | Model training, fine-tuning, LoRA | [ROADMAP](../src/training/ROADMAP.md) |
| transaction | `themis::transaction` | Transaction coordinator, 2PC/3PC/SAGA | [ROADMAP](../src/transaction/ROADMAP.md) |
| updates | `themis::updates` | Update ops, mutations, conflict resolution | [ROADMAP](../src/updates/ROADMAP.md) |
| user_storage_encrypted | `themis::user_storage_encrypted` | User data encryption, field-level | [ROADMAP](../src/user_storage_encrypted/ROADMAP.md) |
| utils | `themis::utils` | General utilities, string/math | [ROADMAP](../src/utils/ROADMAP.md) |
| voice | `themis::voice` | Voice I/O, speech synthesis | [ROADMAP](../src/voice/ROADMAP.md) |
| whisper | `themis::whisper` | OpenAI Whisper, speech-to-text | [ROADMAP](../src/whisper/ROADMAP.md) |

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
