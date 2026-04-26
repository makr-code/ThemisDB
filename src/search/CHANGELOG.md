> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Search Module

All notable changes documented here. Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [2.3.0] — 2026-03-11
### Added
- `DistributedHybridSearch` — distributed hybrid search across shards with cross-shard RRF result merging and mTLS-secured inter-node communication via `RemoteExecutor`
- `SearchStats::shards_failed` / `shards_queried` observability fields for degraded-result detection
- `Config::skip_failed_shards` fault-tolerance flag (default `true`)
- `Config::search_endpoint` override for custom shard API paths
- Tests: `tests/test_distributed_hybrid_search.cpp`

## [2.2.0] — 2026-03-10
### Added
- `NegativeKeywordFilter` — `NOT` / minus-prefix operator support: parses negative terms from raw queries, fetches posting-list candidates for each excluded term and removes them from the result set
- `ParsedQuery` struct with `positive_query` and `negative_terms` fields
- `parseQuery()` static helper for minus-prefix (`-`) and `NOT`-keyword extraction
- `filter()` method: scans the secondary index for documents containing any negative term and removes them from the candidate PK list
- `Config::max_scan` scan-limit guard
- Null-index safety: returns error `Status` and original candidates unchanged when index is null
- Tests: `tests/test_negative_keyword_filter.cpp`

## [2.1.0] — 2026-03-09
### Added
- `SearchHighlighter` — highlight and snippet generation for matched search terms
- `highlight()` — wraps matched terms in configurable HTML open/close tags (default `<mark>…</mark>`)
- `snippet()` — extracts the best passage from a document containing the most query-term matches; pads with configurable ellipsis at boundaries
- `tokenize()` static helper — case-folding, punctuation-aware tokeniser
- `applyHighlight()` static helper — low-level token-offset applicator
- `bestWindowOffset()` static helper — sliding-window scorer for optimal snippet position
- Fully stateless: instances safe for concurrent use after construction; `highlight()` and `snippet()` are `noexcept`
- Tests: `tests/test_search_highlighter.cpp`

## [2.0.0] — 2026-03-08
### Added
- `PersonalizedRanker` — per-user interaction history tracking with time-decayed scoring and result re-ranking
- `InteractionType` enum: VIEW (0.2), CLICK (0.5), BOOKMARK (1.0), LIKE (1.0), DISLIKE (−0.5)
- `computeScore(user_id, doc_id)` — returns personalization score in [−1, 1]
- `applyPersonalization(user_id, candidates)` — adjusts `final_score` and re-sorts
- GDPR-compatible `clearUser()` / `clear()` removal
- `CrossLingualSearch` — cross-lingual semantic search via multilingual embeddings
- `search(embedding, hints)` — kNN with distance-to-similarity, per-language boost, score threshold, k-cap
- `searchMultiEmbedding(queries, hints)` — weighted RRF fusion across multiple embeddings
- `setLanguageMap()` for `Result::language` annotation and `LanguageHint` boost lookup
- `NeuralSparseRetrieval` — SPLADE / BERT-based neural sparse retrieval
- `addDocument()` / `addDocumentText()` / `removeDocument()` index management
- `search()` / `searchText()` — inverted-index dot-product accumulation with optional score normalisation
- `SparseEncoderBackend` injectable callback (same pattern as `LlmReranker`)
- Tests: `tests/test_personalized_ranker.cpp`, `tests/test_cross_lingual_search.cpp`, `tests/test_neural_sparse_retrieval.cpp`

## [1.9.0] — 2026-03-05
### Added
- `MultiFieldBoostedSearch` — multi-field boosted full-text search with per-field BM25 scoring
- Default "title > body > tags" priority: title boost=3.0, body boost=1.0, tags boost=0.5
- `FieldConfig` value type: `{table, column, boost}` — fully configurable
- `search(query, fields)` — per-field normalised BM25 score × boost accumulated per document
- `defaultFields(table)` convenience factory
- Per-field `field_scores` breakdown in `Result`
- Tests: `tests/test_multi_field_search.cpp`

## [1.8.0] — 2026-02-28
### Added
- `LlmReranker` — configurable re-ranking with LLM feedback loop
- `LlmRerankCandidate` / `LlmRerankResult` value types
- Batched prompting with configurable `batch_size`
- Score parsing: per-document 0–10 integer scores normalised to [0, 1]; out-of-range values clamped
- Configurable blending: `final = llm_weight × llm_score + (1 − llm_weight) × initial_score`
- `min_score_threshold` post-blend filter
- `toClickEvents()` static bridge to `LearningToRank::ClickEvent`
- `setReranker()` on `HybridSearch` — attaches an `LlmReranker` as a post-fusion step
- Tests: `tests/test_llm_reranker.cpp`; `HybridSearchReranker` group in `tests/test_hybrid_search.cpp`

## [1.7.0] — 2026-02-15
### Added
- `QueryExpander::suggestSpellingCorrections(word, max_suggestions)` — ranked candidates within `max_edit_distance`, sorted by ascending edit distance; `confidence` decays linearly with distance
- `QueryExpander::suggestQueryCorrections(query, max_suggestions)` — per-token substitution variants plus one all-corrected variant, deduplicated and sorted by total edit distance
- `SpellingCorrection` struct: `suggestion`, `edit_distance`, `confidence`
- 14 new test cases in `tests/test_query_expander.cpp`

## [1.6.0] — 2026-02-01
### Added
- `LlmQueryRewriter` — LLM-based query rewriting for improved recall
- Structured prompts instructing the LLM to produce `n` numbered alternative phrasings
- Numbered-line parser supporting `.`, `)`, and `:` separators
- Case-insensitive deduplication of rewrites against each other and the original query
- `max_rewrite_length` guard: oversized rewrites silently dropped
- Graceful fallback to original query when backend is absent, throws, or produces no output
- `setBackend()` for runtime backend replacement
- Tests: `tests/test_llm_query_rewriter.cpp`

## [1.5.0] — 2026-01-20
### Added
- BM25 full-text search with field boosting
- Semantic vector search with cosine similarity
- Hybrid search (dense + sparse fusion)
- Faceted search and aggregated search results
- Auto-suggest and query completion
- `QueryExpander` — synonym expansion, Levenshtein spelling correction, zero-result relaxation
- `FuzzyMatcher` — Levenshtein, Soundex, Metaphone, N-gram (Dice) similarity
- `FacetedSearch` — value-count facets, numeric range buckets, drill-down filtering
- `SearchAnalytics` — thread-safe query log; avg/p95/p99 latency, zero-result rate, top-20 queries
- `AutocompleteEngine` — prefix-index + popular-query suggestions, score-ranked output
- `LearningToRank` — linear re-ranker over 6-dimensional feature vector; online pairwise gradient-descent
- `MultiModalSearch` — TEXT/IMAGE/AUDIO/CUSTOM modalities; weighted RRF fusion

## [1.0.0] — 2024-01-01
### Added
- Initial implementation
