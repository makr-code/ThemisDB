# Critical Data Flow Paths

This document records the main cross-module execution paths that currently matter for release review and architecture gating. It complements the include-derived dependency inventory in `MODULE_INTEGRATION_CONTRACTS.md` with explicit runtime handler/orchestrator/coordinator evidence, and it should be read together with `src/MODULE_FUNCTION_USAGE_MAP.md` when broader repository-wide symbol-consumer coverage is needed.

## 1. Query -> Storage -> Transaction

| Step | Modules | Evidence |
|---|---|---|
| Edge ingress | `server` -> `query` + `storage` + `index` + `llm` | `src/server/query_api_handler.cpp`, `src/server/http_server.cpp` |
| Query execution core | `query` reads directly from RocksDB-backed storage surfaces | `include/query/query_engine.h`, `src/query/query_engine.cpp`, `src/query/query_optimizer.cpp` |
| Transactional mutation wrapper | `query` wraps `MutationExecutor::StorageContext` in `MutationTransactionContext` for rollback-aware multi-statement execution | `include/query/mutation_executor.h`, `include/query/mutation_transaction.h`, `src/query/aql_runner.cpp` |
| Commit / recovery substrate | `transaction` binds ACID and crash-recovery behavior to RocksDB primitives | `include/transaction/transaction_manager.h`, `src/transaction/transaction_manager.cpp`, `src/transaction/crash_recovery_manager.cpp`, `src/transaction/saga.cpp` |

**Release note:** this path is runtime-critical because query-side mutations are only atomic when the storage context is transaction-backed.

## 2. Sharding <-> Transaction 2PC

| Step | Modules | Evidence |
|---|---|---|
| Classical shard-level 2PC | `sharding` owns a `DistributedTransactionCoordinator` with `PREPARE -> COMMIT/ABORT` flow and TrueTime timestamps | `include/sharding/distributed_transaction.h`, `src/sharding/distributed_transaction.cpp` |
| Cross-shard coordinator | `sharding` also ships `CrossShardTransactionCoordinator` with its own WAL + snapshot contract | `include/sharding/cross_shard_transaction.h`, `src/sharding/cross_shard_transaction.cpp`, `src/sharding/transaction_snapshot.cpp` |
| WAL durability | shard-transaction lifecycle records persist through a dedicated transaction WAL wrapper | `include/sharding/transaction_wal.h`, `src/sharding/transaction_wal.cpp` |
| Region/global coordination | `transaction` contributes `GlobalTransactionManager` and recoverable 2PC contracts built on sharding TrueTime/WAL types | `include/transaction/global_transaction_manager.h`, `include/transaction/recoverable_two_phase_coordinator.h`, `src/transaction/global_transaction_manager.cpp` |

**Release note:** The repository currently documents multiple independent 2PC coordinator implementations, so recovery compatibility is not universal across them.

## 3. ANN -> Tensor -> Graph -> LLM (RAG path)

| Step | Modules | Evidence |
|---|---|---|
| Retrieval contract | `search` exposes a four-layer orchestrator: ANN -> Tensor -> Graph -> LLM | `include/search/layered_retrieval_orchestrator.h`, `src/search/layered_retrieval_orchestrator.cpp` |
| ANN and vector substrate | active vector runtime lives in `index` and not in the docs-only `src/vector_search/` path | `include/index/advanced_vector_index.h`, `src/index/advanced_vector_index.cpp`, `src/index/multi_vector_search.cpp`, `src/index/vector_index.cpp` |
| RAG assembly and summarization | retrieved documents are summarized and normalized before prompt injection | `include/rag/document_summarizer.h`, `include/rag/streaming_retriever.h`, `src/rag/streaming_retriever.cpp`, `src/rag/hybrid_retriever.cpp` |
| LLM / wiki augmentation | retrieval can flow into LLM and wiki-backed reasoning surfaces | `include/rag/wiki_index_store.h`, `src/llm_wiki/wikipedia/llm_wiki_plugin_impl.cpp`, `src/llm_wiki/wikipedia/wiki_workspace_orchestrator.cpp` |

**Release note:** `src/vector_search/` is a docs-only module path today; release decisions for vector retrieval must follow the canonical `index`/`search`/`rag` source paths above.

## 4. Server <-> LLM coupling

| Step | Modules | Evidence |
|---|---|---|
| HTTP/API ingress | `server` wires LLM, prompt, query, and storage dependencies directly into request handlers | `src/server/llm_api_handler.cpp`, `src/server/query_api_handler.cpp`, `src/server/http_server.cpp` |
| Query-assisted LLM flows | query handlers persist interaction and prompt-state context next to normal query execution | `src/server/query_api_handler.cpp`, `include/query/query_engine.h`, `src/query/aql_runner.cpp` |
| Streaming response surface | live streaming implementation currently sits in `llm`, not in docs-only `src/llm_streaming/` | `include/llm/streaming_handler.h`, `src/llm/streaming_handler.cpp` |
| Protocol variants | server-side LLM exposure spans HTTP plus dedicated gRPC services | `src/server/llm_grpc_service.cpp`, `include/server/llm_grpc_service.h`, `include/server/http3_production_config.h` |

**Release note:** `src/llm_streaming/` remains a docs-only module path; production streaming evidence must be mapped to the canonical `llm` and `server` runtime files above.
