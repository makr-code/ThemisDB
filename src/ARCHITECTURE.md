# Architecture - Source Root

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The root of src is a documentation and aggregation boundary above individual modules. It does not own one runtime subsystem; instead, it organizes cross-module source structure, source-wide plans, aggregated audit/security context, and cross-module static-analysis artifacts.

## Main Planes

1. Module plane
- 62 top-level source modules own feature-local implementation and behavior contracts under src/<module>/

2. Aggregation plane
- source-wide ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, SECURITY, and inventory-oriented reports summarize cross-module state

3. Analysis plane
- MODULE_FUNCTION_USAGE_MAP, UNUSED_FUNCTIONS_REPORT, STUB_INVENTORY, and related artifacts support review, refactoring, and governance workflows

## Contracts

| Contract | Behavior |
|---|---|
| module contract | implementation ownership stays in src/<module>/ docs |
| aggregation contract | root docs summarize cross-module state without replacing module-local contracts |
| analysis contract | root reports inform review and planning but do not override source truth in owning modules |

## Failure Semantics

- stale root aggregation docs create governance and navigation drift, not direct runtime behavior changes.
- module-local docs remain the source of truth for feature-local behavior.
- inventory and aggregation docs must be regenerated when root or module summaries materially change.

## Sourcecode Verification (Scope: src/<root>/architecture)

- Verified files:
  - src/ROADMAP.md
  - src/FUTURE_ENHANCEMENTS.md
  - src/AUDIT.md
  - src/SECURITY.md
  - src/MODULE_FUNCTION_USAGE_MAP.md
  - src/UNUSED_FUNCTIONS_REPORT.md
  - ai_working/developer_docs_inventory_report.md
- Verified architecture claims:
  - root docs sit above module-local docs as aggregation artifacts
  - the inventory distinguishes module rows from the <root> filename-matrix row
  - ownership of runtime behavior remains module-local
---

## Unified Dependency Map (source-validated 2026-09-09)

This section documents the cross-module dependency graph derived from a full
`include/` and `src/` header-include scan on 2026-09-09. It supersedes informal
descriptions in older docs.

### Module Layers

| Layer | Modules | Role |
|-------|---------|------|
| **0 — Foundation** | `base`, `core`, `performance`, `utils` | Stateless utilities; no ThemisDB upstream deps |
| **0b — Leaf Runtime Services** | `acceleration` | Hardware acceleration leaf module consumed by index/geo/graph/llm; no ThemisDB upstream deps |
| **1 — Persistent Storage** | `storage`, `cache`, `metadata`, `timeseries`, `temporal` | RocksDB wrapper, MVCC, blob/columnar, schema management |
| **2 — Distributed Infrastructure** | `sharding`, `replication`, `network`, `transaction`, `cdc`, `failover`, `maintenance` | Consensus (Raft/Paxos/Gossip), WAL shipping, 2PC, ACID, changefeed |
| **3 — Indexing & Search** | `index`, `search`, `geo`, `graph` | HNSW/vector/spatial/graph indexes, BM25+vector hybrid search |
| **4 — Data Processing** | `query`, `aql`, `analytics`, `execution`, `content`, `ingestion`, `toolbox`, `document` | AQL/SQL pipeline, operator execution, multimodal ingestion |
| **5 — AI / LLM** | `llm`, `rag`, `retrieval`, `llm_wiki`, `training`, `prompt_engineering`, `llama_cpp`, `onnx_clip`, `whisper`, `stable_diffusion`, `distributed_knowledge`, `distributed_tensor` | Inference, RAG evaluation, LoRA, embeddings, vision, multi-modal |
| **6 — Application & Protocol** | `server`, `api`, `auth`, `security`, `plugins`, `rpc_grpc` | HTTP/gRPC servers, 50+ API handlers, plugin system |
| **Cross-cutting** | `access_model`, `governance`, `observability`, `scheduler`, `security`, `chaos`, `config`, `ethics_ai`, `chimera`, `process`, `projects`, `updates` | Policy, observability, governance, scheduling |

### Confirmed Cross-Module Dependency Graph

```
server       ──→ query, transaction, storage, sharding, llm, rag, search,
                  index, graph, security, auth, cache, cdc, analytics,
                  content, governance, observability, plugins, maintenance,
                  network, timeseries
query        ──→ storage, index, sharding (shard_router), analytics,
                  llm/lora_framework, distributed_knowledge (federated_rag_merger),
                  scheduler, security, utils
transaction  ──→ storage (rocksdb_wrapper, history_manager), index,
                  sharding (truetime, wal_manager), plugins, utils
storage      ──→ cdc, index, performance, sharding (redundancy_strategy),
                  temporal, tensor, transaction (snapshot_manager), utils
sharding     ──→ storage, transaction (recoverable_two_phase_coordinator),
                  distributed_knowledge, utils
replication  ──→ cdc (schema_registry), utils
llm          ──→ acceleration, cache, index, storage, query (!!circular),
                  server (mcp_server.h — !!circular), rag, sharding,
                  ethics_ai, governance, metadata, observability, security
rag          ──→ llm, distributed_knowledge, observability, prompt_engineering,
                  training, security, index/vector, storage, graph
search       ──→ index (vector, ann_frontdoor), metadata, llm (reranker — injected)
index        ──→ acceleration, storage, security, observability, llm/lora_framework
```

### Confirmed Circular Dependencies

| Pair | Evidence | Mitigation |
|------|----------|-----------|
| `llm` ↔ `server` | `src/llm/mcp_tool_bridge.cpp` includes `server/mcp_server.h`; `src/server/lora_api_handler.cpp` includes `llm/lora_framework/lora_orchestrator.h` | Compile order dependency; no clean boundary — design debt |
| `llm` ↔ `query` | `src/llm/` includes `query/` for AQL integration; `src/query/` includes `llm/lora_framework/lora_orchestrator.h` | Forward declarations used in some paths |
| `sharding` ↔ `transaction` | Both include the other's cross-shard 2PC headers | Isolated via `recoverable_two_phase_coordinator` (transaction-owned, shard-called) |

### Key Abstract Interface Boundaries

| Interface | Location | Decoupling Boundary |
|-----------|----------|---------------------|
| `IStorageEngine` | `include/themis/base/interfaces/storage_interface.h` | query, transaction → storage |
| `IQueryEngine` + `IExpressionEvaluator` | `include/themis/base/interfaces/query_interface.h` | consumers → query |
| `IIndexManager`, `IVectorIndex`, `ISecondaryIndex`, `IGraphIndex` | `include/themis/base/interfaces/index_interface.h` | query, server → index |
| `ILLMPlugin` | `include/llm/llm_plugin_interface.h` | llm → llama_cpp, onnx_clip, whisper |
| `IngestionToolbox` | `include/toolbox/ingestion_toolbox.h` | aql, rag → toolbox → ingestion (one-way enforced) |

### Design Bottlenecks

1. **`server/http_server.h` monolithic fan-in** — imports 50+ cross-module headers directly. Any leaf change forces `HttpServer` recompilation. No planned fix prior to Q1 2027.
2. **`llm` ↔ `server` circular dep** — `mcp_tool_bridge.cpp` is the coupling point. Must be resolved before `llm` can be compiled independently.
3. **`sharding` ↔ `transaction` bidirectional** — managed but not eliminated; 2PC correctness depends on ordered initialization.

### Docs-Only Module Paths (no source implementation, 2026-09-09)

| Module | Path | Status |
|--------|------|--------|
| `llm_streaming` | `src/llm_streaming/` | `.gitkeep` only; implementation externalized or planned |
| `vector_search` | `src/vector_search/` | `.gitkeep` only; implementation externalized or planned |
| `chimera` | `src/chimera/` | Planned (Batch 6+); adapter factory not yet implemented |

All implementation claims for these modules must be treated as aspirational until source is delivered.
