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

# Search Module API Contract

This document summarizes the public search interfaces and the invariants they enforce. It is intentionally aligned to the source definitions in the `include/search` directory.

## Primary API surfaces

### HybridSearch

File: `include/search/hybrid_search.h`

`HybridSearch` is the canonical local hybrid retrieval API. It combines:

- BM25/full-text candidate generation
- vector search candidates
- rank fusion
- optional LLM reranking

The main entry points are:

- `HybridSearch::search(...)`
- `HybridSearch::reciprocalRankFusion(...)`
- `HybridSearch::setAnnFrontdoor(...)`
- `HybridSearch::setReranker(...)`

Contract highlights:

- constructor validates configuration and may throw `std::invalid_argument`
- `search()` is implemented as a fail-safe operation and does not propagate backend exceptions
- the result includes `SearchStats` metadata for partial results and degradation visibility
- `Result` carries lexical, vector, and hybrid scoring information

### DistributedHybridSearch

File: `include/search/distributed_hybrid_search.h`

This API extends the hybrid search path to multiple shards. It is intended for cluster or distributed query execution where shard failure and timeout handling matter.

Important invariants:

- local shard is evaluated first and remote shards are executed in bounded batches
- failures are tracked individually and aggregated in `SearchStats`
- partial results are preferred over a total query failure when some shards are unavailable
- the input configuration requires a valid `search_endpoint` and positive shard-concurrency settings

### SearchResultStream

File: `include/search/search_result_stream.h`

This API provides streaming result delivery with cursor-style pagination. It keeps result materialization bounded and exposes timeout settings for long-running queries.

### LayeredRetrievalOrchestrator

File: `include/search/layered_retrieval_orchestrator.h`

This production retrieval chain executes the stages:

- ANN
- Tensor
- Graph
- LLM

The orchestrator records per-layer decisions and enforces timeouts and guardrails. It is designed around an explicit fail-safe model rather than exhaustive success assumptions.

## Error taxonomy

The module error taxonomy is defined in `include/search/search_error_codes.h`.

It is grouped into:

- retrieval errors (`0x0000-0x0FFF`)
- fusion errors (`0x1000-0x1FFF`)
- distributed merge errors (`0x2000-0x2FFF`)
- utility errors (`0x3000-0x3FFF`)
- analytics errors (`0x4000-0x4FFF`)

The string conversion helper `searchErrorCodeToString()` makes these states easier to log and interpret in deployments.

## Threading and safety expectations

Public documentation in the headers states that:

- a single `HybridSearch` instance is not thread-safe
- `LayeredRetrievalOrchestrator` requires safe backend wiring before concurrent execution
- the module favors caller-owned synchronization instead of hidden locks

This is an intentional design trade-off: the retrieval objects remain lightweight and predictable, while safety edges remain explicit.

## Typical usage pattern

A caller typically:

1. configure `HybridSearch::Config`
2. attach the lexical and vector backends
3. optionally attach an ANN frontdoor and LLM reranker
4. call `search()` with a text query and, if available, a vector query
5. inspect `SearchStats` for partial-result conditions
6. when using distributed search, handle shard anomalies and merge degradation explicitly

## Source-grounded references

- `include/search/hybrid_search.h`
- `include/search/search_error_codes.h`
- `include/search/layered_retrieval_orchestrator.h`
- `src/search/hybrid_search.cpp`
- `src/search/distributed_hybrid_search.cpp`
- `src/search/llm_reranker.cpp`
