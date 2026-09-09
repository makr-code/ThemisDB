# MODULE_ARCHITECTURE

## Scope and Method

This document maps all **72 first-level modules under `src/`** and summarizes direct module dependencies derived from in-repo include relationships.

- Module inventory evidence: `src/` directory listing (72 module directories).
- Dependency evidence source: include edges in `src/<module>/**/*.(cpp|h|hpp|cc|cxx|hh)`.
- SCC/cycle analysis basis: directed include graph built from the same source tree.

## Layered Architecture (Foundation → Application)

```text
Foundation -> Storage -> Distribution -> Indexing -> Processing -> AI -> Application
```

Layer baseline from root architecture/governance context:
- `ARCHITECTURE.md`
- `MODULE_INDEX.md`
- module-level `src/<module>/ROADMAP.md`

## 72-Module Map with Direct Dependencies

| Module | Layer | Direct dependencies (include-derived) | Evidence |
|---|---|---|---|
| `acceleration` | AI | geo, index, llm, storage, themis, utils | `src/acceleration/` |
| `access_model` | Application | utils | `src/access_model/` |
| `ai` | AI | utils | `src/ai/` |
| `ai_working` | Application | none detected | `src/ai_working/` |
| `analytics` | Processing | security, storage, themis, utils | `src/analytics/` |
| `api` | Application | index, query, server, storage, themis, transaction, utils | `src/api/` |
| `aql` | Processing | analytics, distributed_knowledge, index, llm, prompt_engineering, query, sharding, storage, utils | `src/aql/` |
| `auth` | Application | security, server, utils | `src/auth/` |
| `base` | Foundation | acceleration, observability, themis, utils | `src/base/` |
| `cache` | Storage | index, observability, security, storage, utils | `src/cache/` |
| `cdc` | Distribution | storage, utils | `src/cdc/` |
| `chaos` | Foundation | none detected | `src/chaos/` |
| `chimera` | Processing | index, query, utils | `src/chimera/` |
| `config` | Foundation | observability | `src/config/` |
| `content` | Processing | config, llm, security, storage, utils | `src/content/` |
| `core` | Foundation | observability, security, themis, utils | `src/core/` |
| `distributed_knowledge` | Distribution | none detected | `src/distributed_knowledge/` |
| `distributed_tensor` | Distribution | observability, rag, tensor | `src/distributed_tensor/` |
| `document` | Processing | none detected | `src/document/` |
| `ethics_ai` | AI | query, storage, utils | `src/ethics_ai/` |
| `evaluation` | AI | none detected | `src/evaluation/` |
| `execution` | Processing | none detected | `src/execution/` |
| `exporters` | Processing | governance, query, security, utils | `src/exporters/` |
| `failover` | Distribution | none detected | `src/failover/` |
| `geo` | Indexing | storage, temporal, themis, utils | `src/geo/` |
| `governance` | Application | observability, security, utils | `src/governance/` |
| `gpu` | Application | acceleration, themis, utils | `src/gpu/` |
| `graph` | Indexing | index, llm, observability, query, security, storage, utils | `src/graph/` |
| `image_analysis` | AI | plugins | `src/image_analysis/` |
| `importers` | Processing | content, plugins, utils | `src/importers/` |
| `index` | Indexing | acceleration, config, llm, observability, security, storage, themis, utils | `src/index/` |
| `ingestion` | Processing | governance, index, storage, utils | `src/ingestion/` |
| `llama_cpp` | AI | llm, rag, utils | `src/llama_cpp/` |
| `llm` | AI | acceleration, cache, core, ethics_ai, governance, index, llama_cpp, metadata, observability, performance, query, rag, security, server, sharding, storage, themis, utils | `src/llm/` |
| `llm_streaming` | Application | none detected | `src/llm_streaming/` |
| `llm_wiki` | AI | config, importers, plugins | `src/llm_wiki/` |
| `maintenance` | Distribution | observability, scheduler, storage, utils | `src/maintenance/` |
| `metadata` | Storage | cdc, index, observability, storage | `src/metadata/` |
| `network` | Distribution | index, query, security, storage, themis, timeseries, transaction, utils | `src/network/` |
| `observability` | Application | api, core, security, utils | `src/observability/` |
| `onnx_clip` | AI | plugins | `src/onnx_clip/` |
| `performance` | Foundation | storage | `src/performance/` |
| `plugins` | Application | acceleration, themis, utils | `src/plugins/` |
| `process` | Processing | index, storage, utils | `src/process/` |
| `projects` | Processing | none detected | `src/projects/` |
| `prompt_engineering` | AI | distributed_knowledge, metadata, security, storage, utils | `src/prompt_engineering/` |
| `query` | Processing | analytics, aql, geo, index, llm, metadata, observability, performance, security, sharding, storage, themis, utils | `src/query/` |
| `rag` | AI | distributed_knowledge, document, ingestion, llm, observability, performance, prompt_engineering, security, themis, training, utils | `src/rag/` |
| `replication` | Distribution | utils | `src/replication/` |
| `retrieval` | AI | none detected | `src/retrieval/` |
| `rpc_grpc` | Application | plugins | `src/rpc_grpc/` |
| `scheduler` | Processing | cdc, query, security, storage, themis, timeseries, utils | `src/scheduler/` |
| `scraper` | Processing | llm | `src/scraper/` |
| `search` | Indexing | core, graph, index, llm, storage, tensor, themis, utils | `src/search/` |
| `security` | Application | auth, core, server, storage, themis, utils | `src/security/` |
| `server` | Application | analytics, api, aql, auth, cache, cdc, config, content, exporters, geo, governance, graph, index, llm, maintenance, metadata, network, observability, performance, plugins, prompt_engineering, query, rag, scheduler, security, sharding, storage, themis, timeseries, transaction, updates, utils, voice | `src/server/` |
| `sharding` | Distribution | distributed_knowledge, storage, themis, transaction, utils | `src/sharding/` |
| `stable_diffusion` | AI | plugins, utils | `src/stable_diffusion/` |
| `storage` | Storage | cdc, index, performance, sharding, temporal, tensor, transaction, utils | `src/storage/` |
| `temporal` | Storage | none detected | `src/temporal/` |
| `tensor` | Indexing | index, observability, storage, utils | `src/tensor/` |
| `themis` | Foundation | acceleration, index, network, query, timeseries, utils | `src/themis/` |
| `timeseries` | Storage | storage, utils | `src/timeseries/` |
| `toolbox` | Processing | aql, ingestion, rag, utils | `src/toolbox/` |
| `training` | AI | analytics, graph, index, llm, query, storage, utils | `src/training/` |
| `transaction` | Distribution | index, plugins, storage, utils | `src/transaction/` |
| `updates` | Storage | utils | `src/updates/` |
| `user_storage_encrypted` | Storage | security | `src/user_storage_encrypted/` |
| `utils` | Foundation | config, observability, security, storage, themis | `src/utils/` |
| `vector_search` | Indexing | none detected | `src/vector_search/` |
| `voice` | AI | llm, utils | `src/voice/` |
| `whisper` | AI | plugins | `src/whisper/` |

## Critical Integration Points

### 1) Query -> Storage -> Transaction

Evidence chain:
- HTTP/API query entry and query handler wiring: `include/server/http_server.h:58`, `include/server/http_server.h:116-117`
- Query handler constructs/uses query engine and storage paths: `src/server/query_api_handler.cpp:171`, `src/server/query_api_handler.cpp:329`, `src/server/query_api_handler.cpp:877`
- Query engine storage coupling: `src/query/query_engine.cpp:31-34`, `src/query/query_engine.cpp:234-248`
- Transaction API and distributed transaction endpoints: `src/server/transaction_api_handler.cpp:94`, `src/server/distributed_txn_api_handler.cpp:17`, `src/server/distributed_txn_api_handler.cpp:212`

### 2) Sharding <-> Transaction (Cross-shard 2PC)

Evidence chain:
- Transaction-side 2PC coordinator contract: `include/transaction/distributed_transaction_manager.h:16-33`
- Transaction manager uses sharding WAL primitives: `include/transaction/distributed_transaction_manager.h:49-51`
- Sharding 2PC coordinator references transaction recovery contracts: `src/sharding/two_phase_commit_coordinator.cpp:31`
- PREPARE/COMMIT RPC participant protocol: `src/sharding/shard_rpc_client.cpp:304-354`
- Cross-shard boundary tests: `tests/cross/test_cross_shard_coordinator.cpp` (2PC/3PC/SAGA/Percolator paths), `tests/cross/test_cross_shard_ssi.cpp:294-367`

### 3) RAG pipeline (LLM -> RAG -> Search -> Index -> Storage)

Evidence chain:
- LLM orchestration drives RAG context and generateRAG: `src/llm/ai_orchestrator.cpp:919`, `src/llm/ai_orchestrator.cpp:1372-1379`
- Search layered orchestrator includes ANN/tensor/graph/LLM stages: `src/search/layered_retrieval_orchestrator.cpp:3`, `src/search/layered_retrieval_orchestrator.cpp:9-12`
- Search->index integration (`HybridSearch`): `src/search/hybrid_search.cpp:14-16`, `src/search/hybrid_search.cpp:189`, `src/search/hybrid_search.cpp:215`
- Storage-side index manager surface (index retrieval and storage get): `src/storage/storage_engine.cpp:161-204`, `src/storage/storage_engine.cpp:380-393`
- RAG integration helpers include index + storage contracts: `include/rag/rag_integration_helpers.h:15-16`

### 4) LLM <-> Server coupling

Evidence chain:
- Server directly includes many LLM surfaces: `src/server/http_server.cpp:78-80`, `src/server/http_server.cpp:152-155`
- LLM module includes server MCP contract: `src/llm/mcp_tool_bridge.cpp:15`
- This creates a direct bidirectional module dependency pair in include-graph analysis (see cycle section).

## Circular Dependency Analysis (include-graph SCC)

### Largest strongly connected component

- **1 SCC with 41 modules** detected:
  `acceleration, analytics, api, aql, auth, cache, cdc, config, content, core, ethics_ai, exporters, geo, governance, graph, index, ingestion, llama_cpp, llm, maintenance, metadata, network, observability, performance, plugins, prompt_engineering, query, rag, scheduler, security, server, sharding, storage, tensor, themis, timeseries, training, transaction, updates, utils, voice`

### Representative mutual dependency pairs

- `api <-> server`
- `aql <-> query`
- `auth <-> security`
- `llm <-> server`
- `llm <-> query`
- `llm <-> rag`
- `storage <-> transaction`
- `storage <-> sharding`
- `index <-> storage`
- `themis <-> utils`

Evidence examples:
- `src/server/http_server.cpp:177-181` and `src/llm/mcp_tool_bridge.cpp:15`
- `include/transaction/distributed_transaction_manager.h:49-51` and `src/sharding/two_phase_commit_coordinator.cpp:31`
- `src/query/query_engine.cpp:31-34` and `src/storage/storage_engine.cpp:161-204`

## Notes

- `src/llm_streaming/` and `src/vector_search/` are currently docs-only module paths (`.gitkeep`, `README.md`, `ARCHITECTURE.md`, `ROADMAP.md`) per module-level source evidence.
- Architecture status and release implications are tracked in `docs/architecture/RELEASE_ARCHITECTURE_STATUS.md`.
