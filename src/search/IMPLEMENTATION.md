---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "include/search/hybrid_search.h"
  - "include/search/distributed_hybrid_search.h"
  - "include/search/search_error_codes.h"
  - "src/search/hybrid_search.cpp"
  - "src/search/distributed_hybrid_search.cpp"
  - "tests/search/CMakeLists.txt"
Related_Issue: "#7378"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Search Module Implementation Guide

This document describes the implementation expectations for the ThemisDB search module. It is intended to be read alongside the source files and the public API contracts rather than as a marketing summary.

## Module boundaries

The search module owns the query and ranking composition logic. It does not own the underlying storage or vector index internals. The implementation is organized around the following responsibilities:

- lexical retrieval (`HybridSearch`)
- vector retrieval and ANN integration (`HybridSearch`, `AnnFrontdoor`)
- distributed merge across shards (`DistributedHybridSearch`)
- candidate reranking (`LlmReranker`)
- layered retrieval orchestration (`LayeredRetrievalOrchestrator`)
- error taxonomy and diagnostics (`SearchErrorCode`, `SearchStats`)

## Implementation invariants

The codebase consistently follows these invariants:

1. Search methods prioritize explicit degradation metadata over silent failure.
2. `HybridSearch::search()` catches backend/fusion exceptions and returns available or empty results; the current implementation does not populate all declared degradation/error fields.
3. Bottleneck behavior is controlled with configuration validation and bounded candidate limits.
4. LLM reranking remains optional and recoverable.
5. Distributed queries report partial success clearly rather than pretending that all shards succeeded.

## Public API behavior

Key public contracts come from `include/search/hybrid_search.h` and `include/search/layered_retrieval_orchestrator.h`.

### HybridSearch

`HybridSearch` enforces configuration safety at construction time. It rejects invalid values such as:

- `k == 0`
- `max_k == 0`
- `k > max_k`
- `max_candidates == 0`
- non-positive `rrf_k`
- invalid weights or empty table/column names

This is important because the module expects stable, deterministic configuration and treats invalid config as an immediate construction failure.

### SearchStats and error taxonomy

`HybridSearch::search()` currently populates backend status, partial-result,
and candidate-count fields. The header also declares `primary_error_code`,
`fusion_failed`, and `rerank_fallback`, but the inspected implementation does
not write them. `SearchErrorCode` is declared in a public header but is not
currently consumed by the inspected search `.cpp` files. Treat the latter
fields as declared API surface, not active runtime diagnostics; wiring and
tests are TODO.

### DistributedHybridSearch

The distributed implementation batches remote shard queries, tracks shard-specific errors, and records timeout behavior. It continues operating with survivors instead of failing the entire query when remote shards fail.

### LayeredRetrievalOrchestrator

This orchestrator is intentionally more conservative than a simple ranker. It imposes per-layer deadlines and prunes expensive work using guardrails. The layered flow is fail-safe and logs decisions for do-no-harm behavior under high-latency or backend-failure conditions.

## Edge cases intentionally handled

The implementation covers several high-impact edge cases:

- empty lexical query input
- empty vector input
- missing or null LLM backend
- malformed LLM reranker output
- shard timeout during distributed execution
- over-limit candidate counts
- zero results after fusion
- degraded fallback without throwing

The module preserves these cases in code paths, not just in issue trackers or release notes.

## Threading and lifecycle expectations

The public header files document the expected lifecycle rules:

- `HybridSearch` is not thread-safe as a single instance
- `LayeredRetrievalOrchestrator` requires safe backend wiring before concurrent `execute()` calls
- constructors and initial validations are strict and early
- result objects are not expected to be shared across threads without caller-owned synchronization

## Implementation quality goals

The search module favors:

- deterministic ranking behavior
- bounded fan-out
- explicit diagnostics
- fail-safe partial responses
- robust local fallback to original ranking when optional systems fail

This is the practical design target for the code in `src/search`.

## Source-grounded references

- `include/search/hybrid_search.h`
- `src/search/hybrid_search.cpp`
- `include/search/layered_retrieval_orchestrator.h`
- `src/search/layered_retrieval_orchestrator.cpp`
- `include/search/search_error_codes.h`
- `src/search/distributed_hybrid_search.cpp`
- `src/search/llm_reranker.cpp`
