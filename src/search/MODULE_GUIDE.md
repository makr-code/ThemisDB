---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# Search Module Guide

This guide is the working map for developers who need to understand, change, or debug the search subsystem without starting from raw code dumps.

## Module purpose

The search module turns user requests into ranked and bounded retrieval results. It combines:

- lexical candidate generation
- dense/vector retrieval
- hybrid fusion
- shard-aware distributed execution
- optional LLM re-ranking and layered retrieval

## Entry points

### Local hybrid search

Primary entry point: `HybridSearch::search()` in `include/search/hybrid_search.h`.

Use this when the request is local to a process and the system already has a full-text index and vector index backing store.

### Distributed hybrid search

Primary entry point: `DistributedHybridSearch::search()`.

Use this when the query must span more than one shard or remote search endpoint. It explicitly handles shard timeouts and partial failures.

### Layered retrieval

Primary entry point: `LayeredRetrievalOrchestrator::execute()`.

Use this when the operation needs a multi-stage retrieval chain and explicit guardrails for latency and prompt size.

## File map

- `src/search/hybrid_search.cpp` — local lexical + vector fusion logic
- `src/search/distributed_hybrid_search.cpp` — remote shard coordination and result merge
- `src/search/llm_reranker.cpp` — LLM-based candidate reordering
- `src/search/layered_retrieval_orchestrator.cpp` — staged retrieval pipeline
- `include/search/search_error_codes.h` — category-based error taxonomy
- `src/search/query_expander.cpp` — query relaxation helpers
- `src/search/faceted_search.cpp` — facet constraints and aggregation
- `src/search/search_result_stream.cpp` — streaming/pagination behavior

## Typical task paths

### Adding or changing fusion behavior

Read:

- `include/search/hybrid_search.h`
- `src/search/hybrid_search.cpp`
- `include/search/search_error_codes.h`

Then verify the impact on:

- `SearchStats::partial_result`
- `fusion_failed`
- `primary_error_code`

### Debugging distributed shard issues

Read:

- `src/search/distributed_hybrid_search.cpp`
- `include/search/search_error_codes.h`

Pay attention to:

- `shard_timeout_ms`
- `skip_failed_shards`
- batch execution and timeout paths
- merge underflow and overlap metadata

### Investigating LLM re-ranking regressions

Read:

- `src/search/llm_reranker.cpp`
- `include/search/hybrid_search.h`

Check the fallback logic and verify the backend contract and score output remain valid.

## Working conventions

- Prefer deterministic and bounded result sets over unbounded candidate growth.
- Treat degraded results as explicit states, not hidden outcomes.
- Preserve observability when a component is partial or fallback-based.
- Keep the public API contract in sync with the source behavior.

## Related docs

- `src/search/ARCHITECTURE.md`
- `src/search/IMPLEMENTATION.md`
- `src/search/API.md`
- `src/search/RUNBOOK.md`
- `src/search/OPERATIONS.md`
- `src/search/SECURITY.md`
