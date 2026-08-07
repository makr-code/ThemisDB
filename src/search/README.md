# ThemisDB Search Module

<!-- Status: current | validated: 2026-08-06 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The search module provides lexical, vector, and hybrid retrieval behavior for ThemisDB, including RRF-based fusion, distributed shard result merging, query expansion and reranking helpers, and result-focused search utilities.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| hybrid_search.cpp | hybrid lexical/vector fusion behavior |
| distributed_hybrid_search.cpp | cross-shard merge and distributed hybrid behavior |
| faceted_search.cpp | facet aggregation and filter-path behavior |
| query_expander.cpp | query expansion and relaxation behavior |
| fuzzy_matcher.cpp | fuzzy/approximate token matching behavior |
| autocomplete.cpp | prefix suggestion behavior |
| llm_query_rewriter.cpp | LLM-assisted query rewrite behavior |
| llm_reranker.cpp | LLM-assisted reranking behavior |
| learning_to_rank.cpp | trainable ranking behavior |
| search_analytics.cpp | query/result metric tracking behavior |
| multi_field_search.cpp | field-weighted search behavior |
| multi_modal_search.cpp | modality-mixed search behavior |
| neural_sparse_retrieval.cpp | sparse neural retrieval behavior |
| search_result_stream.cpp | streaming/cursor result behavior |

## Scope

In scope:
- lexical/vector/hybrid retrieval and ranking behavior
- shard merge and distributed search result composition
- search utility layers (facet, fuzzy, expand, rerank, analytics, stream)

Out of scope:
- underlying index storage internals owned by index/storage modules
- external model serving ownership outside module contracts
- non-search domain orchestration logic

## Runtime Behavior and Limits

- hybrid and distributed paths are bounded by configured candidate limits.
- merge/fusion behavior is deterministic per ranking configuration.
- optional LLM augmentation paths remain explicit and degradable.
- analytics/stream behavior remains observable and non-silent.

## Sourcecode Verification (Module: search/readme)

- Verified files:
  - src/search/hybrid_search.cpp
  - src/search/distributed_hybrid_search.cpp
  - src/search/faceted_search.cpp
  - src/search/query_expander.cpp
  - src/search/fuzzy_matcher.cpp
  - src/search/autocomplete.cpp
  - src/search/llm_query_rewriter.cpp
  - src/search/llm_reranker.cpp
  - src/search/learning_to_rank.cpp
  - src/search/search_analytics.cpp
  - src/search/multi_field_search.cpp
  - src/search/multi_modal_search.cpp
  - src/search/neural_sparse_retrieval.cpp
  - src/search/search_result_stream.cpp
- Verified behavior surfaces:
  - hybrid/distributed merge, ranking, utility, and observability paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - Wave B dependency planning for search enhancements is tracked in issue `#5039`
  - upstream planning context links: Wave C `#5040`, Wave A `#5038`
  - historical entries remain in CHANGELOG.md
  - historical entries remain in CHANGELOG.md
  - Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
  - dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
  - follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
