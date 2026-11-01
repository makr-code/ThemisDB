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
  - "src/search/hybrid_search.cpp"
  - "include/search/search_error_codes.h"
Related_Issue: "#7378"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Search Embedding and Dense Retrieval Guide

This document explains how the search module treats dense retrieval, vector similarity, and embedding-related quality issues. It is meant for developers updating the query layer, ANN integration, or reranking behavior.

## Dense retrieval responsibilities

The core dense path is implemented through:

- `HybridSearch::search()`
- `HybridSearch::setAnnFrontdoor()`
- `VectorIndexManager` interaction
- optional `AnnFrontdoor` queries in `src/search/hybrid_search.cpp`

The dense side is not a separate subsystem. It is a retrieval source that must be fused with the lexical result set and correctly reported as partial or failed when needed.

## Embedding contract assumptions

The search module assumes:

- the caller provides a valid vector and dimensionality when performing dense search
- ANN frontdoor output is comparable to vector-index output
- distance metrics can be converted into a score suitable for hybrid ranking
- embedding quality is eventually judged by candidate ordering rather than raw vector values alone

The implementation validates the vector-input path only indirectly: it checks whether the pointer is non-null and the dimension is greater than zero before attempting vector retrieval.

## Distance-to-score conversion

The module converts vector distances into similarity-like scores in the anonymous helper defined in `src/search/hybrid_search.cpp`:

- `COSINE` uses `1.0 - distance`
- `DOT` uses `distance`
- `L2` uses `1.0 / (1.0 + distance)`

This conversion is necessary so lexical scores and vector scores can be fused using the same ranking semantics. It is also a trade-off: it favors operational usability over perfect mathematical normalization across all distance metrics.

## ANN and fallback behavior

The code supports a dual-layer dense path:

1. ANN via `AnnFrontdoor`
2. fallback to `VectorIndexManager::searchKnn()` when the frontdoor result is empty and the strategy indicates a usable vector backend

This fallback ensures that a missing or empty frontdoor result does not automatically translate into a total retrieval failure. It is intentionally degraded but observable.

## Embedding quality and failure modes

Common dense retrieval failures include:

- null vector query
- zero-length vector path
- mismatched vector dimension
- ANN backend timeout or backend failure
- empty candidate set even when the vector path is enabled

The current implementation records whether the vector path ran without
exception, but does not populate the declared `SearchErrorCode` taxonomy or
`HybridSearch::SearchStats::primary_error_code`. An empty result is not by
itself recorded as a code. **TODO:** define and test whether empty/partial
backend responses are errors for each caller contract.

## Why the module uses bounded dense retrieval

The dense path is explicitly bounded with `k_vector` and `max_candidates` validation. This is not just a tuning preference; it is operationally necessary because dense retrieval can produce large candidate sets that would otherwise destabilize latency and memory use.

## Re-ranking implications

The LLM reranker consumes a narrowed set of candidates and reorders them based on a generated relevance score. This means the dense and lexical retrieval layers still matter most; the reranker is a second-stage improvement, not the primary retrieval source.

## Source-grounded references

- `src/search/hybrid_search.cpp`
- `include/search/hybrid_search.h`
- `include/search/search_error_codes.h`
- `src/search/llm_reranker.cpp`
- `include/search/layered_retrieval_orchestrator.h`
- `src/search/layered_retrieval_orchestrator.cpp`
