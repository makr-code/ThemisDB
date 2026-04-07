<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Search Module

All notable changes to public headers are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Implementation details in `../../src/search/CHANGELOG.md`.

## [2.3.0] — 2026-02

### Added
- `distributed_hybrid_search.h` — `DistributedHybridSearch` multi-node hybrid search with mTLS, RRF merging. `SearchStats` adds `shards_failed` and `shards_queried` fields.

## [2.2.0] — 2026-01

### Added
- `negative_keyword_filter.h` — `NegativeKeywordFilter` with NOT operator and minus-prefix (`-keyword`) exclusion syntax.

## [2.1.0] — 2025-11

### Added
- `search_highlighter.h` — `SearchHighlighter` with `highlight()` for inline spans and `snippet()` for contextual text snippets; `tokenize()` helper.

## [2.0.0] — 2025-09

### Changed
- Major version: architecture refactored to support distributed, multi-modal, and neural sparse retrieval.

### Added
- `neural_sparse_retrieval.h` — `NeuralSparseRetrieval` SPLADE-style neural sparse retrieval.
- `multi_modal_search.h` — `MultiModalSearch` text + image + audio.
- `llm_query_rewriter.h` — `LlmQueryRewriter` LLM-based query intent rewriting.
- `llm_reranker.h` — `LlmReranker` LLM-based result reranking.
- `learning_to_rank.h` — `LearningToRank` LTR model training and scoring.
- `personalized_ranker.h` — `PersonalizedRanker` user-preference ranking.
- `cross_lingual_search.h` — `CrossLingualSearch` cross-language query translation.
- `search_analytics.h` — `SearchAnalytics` CTR, zero-result tracking.

## [1.0.0] — 2025-01

### Added
- `autocomplete.h` — `Autocomplete` prefix autocomplete.
- `faceted_search.h` — `FacetedSearch` facet extraction and filtering.
- `fuzzy_matcher.h` — `FuzzyMatcher` edit-distance fuzzy matching.
- `hybrid_search.h` — `HybridSearch` single-node BM25 + vector.
- `multi_field_search.h` — `MultiFieldSearch` weighted multi-field search.
- `query_expander.h` — `QueryExpander` synonym and semantic expansion.
