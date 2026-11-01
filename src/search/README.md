---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
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
  - "DOCUMENTATION_GOVERNANCE.md"
  - "src/search/README.md"
  - "cmake/ModularBuild.cmake"
  - "tests/search/CMakeLists.txt"
  - "tests/CMakeLists.txt"
Related_Issue: "#7378"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# ThemisDB Search Module

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
- analytics and streaming APIs expose per-class diagnostics; verify each header
  for its exact observability and buffering contract.

## Build and test evidence

- The root `cmake/ModularBuild.cmake` query-source list includes search implementations, but it does not list every `.cpp` under `src/search/`. Do not infer that every module file is compiled into every target.
- `tests/search/CMakeLists.txt` declares glob-based focused-test targets with a 120-second CTest timeout and the `search` label (plus conditional labels for selected tests). `tests/CMakeLists.txt` auto-discovers this module CMake file and adds `search` when tests are configured; target generation, compilation, and execution still require verification in a configured build.
- The directory currently contains 16 `test_*.cpp` files. Their presence and CMake declarations are not evidence that they compiled or ran. No runtime test result is asserted here.
- For actual API and behavioral details, use the public headers and implementation files cited by [ARCHITECTURE.md](ARCHITECTURE.md); maturity percentages and earlier line-count inventories in this README are retired because they are not current source evidence.

The local source set includes optional query rewriting/reranking and layered retrieval, but backend availability and runtime configuration are supplied by callers. Search-owned code does not establish an embedding model's quality or a production latency/SLA.
