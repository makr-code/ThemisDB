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

# Search Module Research Notes

This document captures the design assumptions and trade-offs that underpin the ThemisDB search stack. It is intended for developers working on retrieval quality, failure handling, or distributed ranking behavior.

## Scope

The module is centered on three production surfaces:

- lexical retrieval via BM25/full-text search
- vector retrieval via ANN or vector index access
- hybrid and distributed fusion across local and remote shards

The primary source files are:

- `src/search/hybrid_search.cpp`
- `src/search/distributed_hybrid_search.cpp`
- `src/search/llm_reranker.cpp`
- `src/search/layered_retrieval_orchestrator.cpp`
- `include/search/search_error_codes.h`
- `include/search/hybrid_search.h`
- `include/search/layered_retrieval_orchestrator.h`

## Research and design assumptions

### 1. Multi-signal retrieval is preferable to single-signal ranking

The module intentionally combines keyword and dense signals instead of depending on a single retrieval strategy. `HybridSearch` accepts both `text_query` and a dense `vector_query` and then merges the resulting candidates. This is consistent with the production contract in `include/search/hybrid_search.h` and the behavior in `src/search/hybrid_search.cpp`.

The search stack assumes that a user query often contains lexical specificity and semantic intent simultaneously. BM25 addresses exact term alignment; vector retrieval addresses notionally similar but differently phrased content. The loss of either signal is treated as a degradation event, not a hidden fallback.

### 2. Failure transparency matters more than perfect recall

The module explicitly tracks partial-result states (`SearchStats::partial_result`, `fusion_failed`, `rerank_fallback`, `failed_shard_reasons`) rather than silently returning a best-effort score. This design preference is rooted in how the error taxonomy is defined in `include/search/search_error_codes.h` and how `HybridSearch::search()` and `DistributedHybridSearch::search()` handle backend timeouts or failures.

The design principle is: return the most useful data available while preserving enough metadata for the caller to understand whether the answer was degraded.

### 3. Result quality must remain bounded under load

The configuration validation in `HybridSearch` and `DistributedHybridSearch` prevents unbounded candidate counts and unbounded final result sizes. This is crucial for both latency control and memory stability. The module assumes that retrieval quality is still valuable at smaller bounded sets, especially when the caller configures `k`, `max_k`, and candidate caps explicitly.

### 4. LLM re-ranking is optional and fail-safe

`LlmReranker` is designed as a final-stage improvement, not a required dependency. It accepts a backend callable and falls back to the original ranking when the backend is absent, fails, or returns malformed output. This reduces the operational risk of making the core search pipeline dependent on an external LLM service.

## Evaluation posture

The module uses a pragmatic evaluation model rather than an academic benchmark-only model:

- retrieval quality is evaluated in terms of rank order and result relevance under fused lexical/vector signals
- degradation signals are treated as first-class observations
- distributed search is expected to be partially degraded rather than hard-failing under shard loss
- layer execution is bounded with timeouts and guardrails in `LayeredRetrievalOrchestrator`

This aligns with the implemented contracts around `SearchErrorCode`, `SearchStats`, and the layered orchestration pipeline.

## Trade-offs

### Lexical + dense fusion

Benefits:

- better ranking for ambiguous or acronym-heavy queries
- improved recall for short queries that lack exact lexical overlap
- ability to mix BM25 precision with vector semantic recall

Costs:

- harder tuning of fusion weights and candidate counts
- more operational uncertainty when one backend is late or partial
- sensitivity to metric choice (COSINE, DOT, L2) and candidate count thresholds

### Distributed search

Benefits:

- greater coverage across shards
- parallelization of shard queries
- reduced single-shard bottleneck when using `RemoteExecutor`

Costs:

- merge conflicts and overlap variance must be measured explicitly
- shard timeout behavior must stay observable
- fallback semantics can hide partial failure if callers ignore `SearchStats`

### LLM-assisted reranking

Benefits:

- higher semantic quality after candidate set narrowing
- better alignment with user intent for difficult or nuanced queries

Costs:

- latency sensitivity
- backend availability risk
- optional but not guaranteed signal quality

## Operational implications

The module avoids the assumption that every query must succeed in the strongest form. The production contract is to surface partial outcomes and keep degradation explicit. This is reflected in:

- fail-safe `search()` implementations
- bounded candidate caps
- timeout handling in distributed and layered retrieval
- explicit error taxonomy for downstream diagnosis

For further context see:

- `src/search/ARCHITECTURE.md`
- `src/search/IMPLEMENTATION.md`
- `src/search/API.md`
- `src/search/RUNBOOK.md`
