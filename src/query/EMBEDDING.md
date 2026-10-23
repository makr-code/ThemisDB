---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Query Module Embedding and Similarity Notes

## Purpose

The Query module integrates embedding-based retrieval and semantic caching into a broader declarative execution pipeline. This file explains the embedding-related surfaces used by the module and the assumptions those surfaces rely on.

## Where embeddings appear in the module

The key embedding-aware surfaces are:

- `src/query/semantic_cache.cpp` — exact-match and approximate semantic cache logic.
- `src/query/query_cache_manager.cpp` — cache coordination across exact and adaptive strategies.
- `src/query/vectorized_execution.cpp` — execution pathways for vectorized and similarity-aware evaluation.
- `src/query/optimizer_cost_model.cpp` and adaptive optimizer files — plan choices when vector or adjacency constraints are present.

The module does not treat similarity as a disconnected feature. It is instead part of the runtime decision-making stack that includes cache retrieval, query planning, and result filtering.

## Semantic caching model

`SemanticQueryCache` computes a query embedding and uses a vector index to find a similar prior query. The general flow is:

1. normalize the query text,
2. compute a vector embedding,
3. search the vector index for near neighbors,
4. accept the result if similarity exceeds the configured threshold,
5. return the cached result only if the entry is not expired.

This creates a useful optimization for repeated or paraphrased user queries, especially for natural-language or semantic retrieval scenarios. The system still preserves a fast exact-match check first and uses the semantic match as a fallback path when exact lookup fails.

## Embedding assumptions and trade-offs

The design depends on a few critical assumptions:

- query embeddings are deterministic enough for repeated queries to map to the same region of embedding space,
- the index can search efficiently enough to keep the semantic-cache fallback bounded,
- the similarity threshold is tuned to avoid overmatching unrelated queries,
- the cached result remains correct and valid for the current data state.

The trade-off is explicit: semantic caching improves repeated-query latency but adds a second source of correctness risk. A high false-positive threshold will return stale or semantically off-topic answers, while a low threshold reduces recall of available cache hits.

## Relationship to planning and execution

Embedding-aware queries are not isolated from the execution stack. The planner must understand whether a query is primarily vector-driven, predicate-driven, or hybrid. That matters because a query that mixes a vector search with a geographic filter or graph traversal should not be treated as a single binary operation.

In practice, the Query module expects the optimizer to reason about:

- the cost of candidate generation,
- the cost of re-ranking or post-filtering,
- the cost of I/O from the underlying storage or index backend,
- the chance that the semantic-cache path can avoid expensive recomputation.

## Failure and safety concerns

The embedding surfaces are intentionally limited by correctness and safety checks:

- cache entries have explicit expiration logic,
- memory and entry limits are enforced,
- stale entries can be removed instead of acting as valid hits,
- vector index lookup is only accepted above configured similarity thresholds,
- result reuse is not allowed to hide data freshness violations or query policy constraints.

This is especially important for user-facing retrieval where the result should remain explainable and bounded.

## Operational guidance

When adding or modifying embedding-aware query logic, the following should remain true:

- the semantic cache must not silently bypass access checks,
- query embedding and result reuse must remain under the same runtime controls as ordinary query execution,
- vector search thresholds must be explicit and reviewed when workload characteristics change,
- cache invalidation must remain correct as data model or schema changes appear.

This keeps the embedding layer aligned with the rest of the Query module rather than treating it as a separate domain with its own hidden assumptions.
