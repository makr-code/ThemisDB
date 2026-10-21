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

# Search Module Modeling Notes

This module models retrieval as a bounded, observable ranking problem. The production behavior is defined by a combination of lexical score generation, dense vector scoring, hybrid fusion, and distributed shard merge.

## Core model

The effective search model is:

1. generate lexical candidates from BM25/full-text retrieval
2. generate vector candidates using ANN or vector index search
3. convert distance to a comparable score using the configured metric
4. merge the two candidate sets using RRF or linear combination
5. apply optional LLM reranking
6. enforce candidate counts and final `k` caps
7. report outcome health through `SearchStats` and error codes

This flow is visible in `HybridSearch::search()` and in the distributed variant.

## Candidate generation assumptions

### Lexical retrieval

The BM25 path consumes:

- a `text_query`
- a configured default table and column
- a capped `k_bm25` candidate window

The exact implementation in `HybridSearch::search()` attempts a `scanFulltextWithScores()` call and populates `Result` objects with:

- `document_id`
- `bm25_score`
- `bm25_rank`

If the backend is unavailable or raises, the result is logged and the module proceeds without failing the call.

### Vector retrieval

The dense path consumes:

- a `vector_query` pointer and `vector_dim`
- an optional `AnnFrontdoor`
- a fallback to `VectorIndexManager`

The distance-to-score conversion appears in the anonymous helper in `hybrid_search.cpp`:

- COSINE -> `1.0 - distance`
- DOT -> `distance`
- L2 -> `1.0 / (1.0 + distance)`

This conversion makes dense similarity comparable with lexical ranking before fusion.

## Fusion models

### Reciprocal Rank Fusion (RRF)

The default model in `HybridSearch::reciprocalRankFusion()` is the standard approach:

- each result receives a score based on its position in the lexical and vector lists
- the final score is the sum of the reciprocal rank contributions
- the constant `rrf_k` controls sensitivity to rank differences

This approach is robust when both retrieval sources have noisy or incomplete rankings.

### Linear combination fallbacks

The code supports a weighted linear combination when configured. The weight model is explicit and bounded, and the constructor validates that:

- weights are non-negative
- `k` and `max_k` are consistent
- `k_bm25` and `k_vector` do not exceed `max_candidates`
- `rrf_k` is positive

This is important because invalid configuration should fail early rather than produce unstable output.

## Distributed model

`DistributedHybridSearch` extends the local hybrid model by querying remote shards and merging results. Important production assumptions:

- the local shard is always queried first
- healthy shards are discovered from the resolver
- remote shards run concurrently in batches
- shard timeouts are explicit and recorded as failures
- results from surviving shards remain usable even when some shards fail

The module does not assume all shards will be healthy. Instead, it treats shard failures as a distributed-query concern, not as a fatal condition, and records them in `ShardSearchResult` and aggregate debug statistics.

## Layered retrieval model

The `LayeredRetrievalOrchestrator` introduces a higher-level retrieval chain:

- ANN
- Tensor
- Graph
- LLM

Each layer decides whether it executed, timed out, or was pruned by guardrails. The worker uses a deadline wrapper and records the decision internally. This makes the layered chain harder to reason about than a simple two-source fusion, but it enables explicit failure semantics and per-layer observability.

## Failure and guardrail model

The module assumes that retrieval quality depends on both quality and control. Failure surfaces include:

- unmatched or empty candidate sets
- partial backend results
- incorrect or malformed reranker output
- cross-shard merge deficits
- overloaded query workloads

The guardrail design in `LayeredRetrievalOrchestrator` acknowledges this by bounding `max_ann_candidates`, `max_tensor_candidates`, `max_graph_edges`, and `max_prompt_chars` when enabled.

## The model is intentionally fail-safe

This module does not treat retrieval as a single all-or-nothing call. It is modeled as a degraded signal pipeline with explicit observability. The required contract is not simply "best result set" but "best result set with explicit quality metadata and bounded failure modes." This is why the code uses the `SearchErrorCode` taxonomy and `SearchStats` diagnostics so directly.

## Source-grounded references

- `include/search/hybrid_search.h`
- `include/search/search_error_codes.h`
- `src/search/hybrid_search.cpp`
- `src/search/distributed_hybrid_search.cpp`
- `include/search/layered_retrieval_orchestrator.h`
- `src/search/layered_retrieval_orchestrator.cpp`
