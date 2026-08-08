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
| conversational_search.cpp | context-aware multi-turn query behavior |
| federated_search.cpp | multi-tenant search with RRF aggregation |
| llm_query_rewriter.cpp | LLM-assisted query rewrite behavior |
| llm_reranker.cpp | LLM-assisted reranking behavior |
| learning_to_rank.cpp | trainable ranking behavior |
| search_analytics.cpp | query/result metric tracking behavior |
| multi_field_search.cpp | field-weighted search behavior |
| multi_modal_search.cpp | modality-mixed search behavior |
| neural_sparse_retrieval.cpp | sparse neural retrieval behavior |
| search_result_stream.cpp | streaming/cursor result behavior |
| cross_lingual_search.cpp | language-aware query translation and search |
| negative_keyword_filter.cpp | exclusion-based result filtering |
| personalized_ranker.cpp | user-context-aware result ranking |
| search_highlighter.cpp | result excerpt highlighting and context extraction |

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

**Complete Implementation Status (2026-08-08):** All 20 search module implementations now integrated into build configuration.

- Verified files (20 implementations):
  - src/search/autocomplete.cpp (193 lines, 6 methods)
  - src/search/conversational_search.cpp (126 lines, 6 methods) ✨ NEW
  - src/search/cross_lingual_search.cpp (222 lines)
  - src/search/distributed_hybrid_search.cpp (489 lines)
  - src/search/faceted_search.cpp (230 lines)
  - src/search/federated_search.cpp (198 lines, 9 methods) ✨ NEW
  - src/search/fuzzy_matcher.cpp (305 lines)
  - src/search/hybrid_search.cpp (450 lines)
  - src/search/learning_to_rank.cpp (232 lines)
  - src/search/llm_query_rewriter.cpp (297 lines)
  - src/search/llm_reranker.cpp (272 lines)
  - src/search/multi_field_search.cpp (187 lines)
  - src/search/multi_modal_search.cpp (262 lines)
  - src/search/negative_keyword_filter.cpp (179 lines)
  - src/search/neural_sparse_retrieval.cpp (306 lines)
  - src/search/personalized_ranker.cpp (174 lines)
  - src/search/query_expander.cpp (452 lines)
  - src/search/search_analytics.cpp (177 lines)
  - src/search/search_highlighter.cpp (301 lines)
  - src/search/search_result_stream.cpp (171 lines, 9 methods) ✨ NEW
  
- Total Lines of Code: 5,223 across all implementations
- Build Configuration Status: ✅ All 20 files integrated into cmake/ModularBuild.cmake (2026-08-08)
- Production Quality: All implementations marked PRODUCTION-READY (85-95% maturity scores)
- Verified behavior surfaces:
  - hybrid/distributed merge, ranking, utility, and observability paths
  - conversational context tracking and result curation
  - federated search with multi-tenant isolation
  - streaming and pagination support
- Note:
  - Build integration completed via commit 8888dc81 (feat: add missing search module implementations)
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - Wave B dependency planning for search enhancements is tracked in issue `#5039`
  - upstream planning context links: Wave C `#5040`, Wave A `#5038`
  - historical entries remain in CHANGELOG.md
  - Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
  - dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
  - follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
