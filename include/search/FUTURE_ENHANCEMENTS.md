# Search Module API - Future Enhancements

## Scope

- API-level enhancements to `include/search/` headers
- LTR (learning-to-rank) interface (`LearningToRank`, model hot-swap API)
- Neural sparse retrieval API (compile-time optional, `NeuralSparseRetriever`)
- Faceted search interface (`FacetedSearch`, composable drill-down API)
- Cross-lingual search API (`CrossLingualSearch`, multi-embedding fusion)
- Query rewriting hook (`QueryRewriter`, pluggable ordered rewrite chain)

## Design Constraints

- [ ] LTR model is hot-swappable without server restart
- [ ] Faceted search API is composable — facets can be combined via intersection
- [ ] Neural sparse API is compile-time optional (`THEMIS_NEURAL_SPARSE` flag)
- [ ] No breaking changes to existing `HybridSearch` API
- [ ] All new interfaces use `Result<T>` for error propagation (no exceptions)
- [ ] Query rewriting hooks are ordered and individually enable/disable-able

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `LearningToRank` | Search result re-ranking pipelines | Hot-swappable model; online click-through training |
| `NeuralSparseRetriever` | Neural IR search backends | Compile-time optional via `THEMIS_NEURAL_SPARSE` |
| `FacetedSearch` | Browse/filter UIs | Composable facet intersection; ≤ 5 ms aggregation |
| `CrossLingualSearch` | Multilingual applications | RRF fusion over multilingual embeddings |
| `QueryRewriter` | Query preprocessing pipeline | Ordered hook chain; ≤ 20 ms total rewrite budget |

## Delivered in v1.5.0

All features listed below were delivered in v1.5.0.  The actual public API is
documented in [`README.md`](README.md).

### Query Expansion API
**Status:** ✅ Delivered in v1.5.0 — see `include/search/query_expander.h`

`QueryExpander` provides synonym expansion, Levenshtein-based spelling
correction, alternative query generation, and zero-result relaxation.

---

### Ranked Spelling Correction Suggestions API
**Status:** ✅ Delivered in v1.7.0 — see `include/search/query_expander.h`

`QueryExpander::suggestSpellingCorrections()` returns a ranked list of
`SpellingCorrection` candidates (suggestion, edit_distance, confidence) for
a single misspelled word.  `QueryExpander::suggestQueryCorrections()` returns
ranked full-query correction strings by substituting each misspelled token
with its best correction, plus an all-corrected variant.

---

### Fuzzy Search API
**Status:** ✅ Delivered in v1.5.0 — see `include/search/fuzzy_matcher.h`

`FuzzyMatcher` supports Levenshtein, Soundex, Metaphone, and N-gram
similarity as a thin wrapper over `SecondaryIndexManager::scanFulltextFuzzy`.

---

### Faceted Search API
**Status:** ✅ Delivered in v1.5.0 — see `include/search/faceted_search.h`

`FacetedSearch` computes categorical value-count facets, numeric range
bucket facets, and supports active-facet drill-down filter intersection.

---

### Search Analytics API
**Status:** ✅ Delivered in v1.5.0 — see `include/search/search_analytics.h`

Thread-safe query log with p95/p99 latency, zero-result rate, and
per-query frequency tracking.

---

### Autocomplete API
**Status:** ✅ Delivered in v1.5.0 — see `include/search/autocomplete.h`

`AutocompleteEngine` provides prefix-index suggestions and popular-query
suggestions, combined and deduplicated.

---

### Learning to Rank API
**Status:** ✅ Delivered in v1.5.0 — see `include/search/learning_to_rank.h`

Linear feature-vector re-ranker with online click-through training and
deterministic A/B variant routing.

---

### Multi-Modal Search API
**Status:** ✅ Delivered in v1.5.0 — see `include/search/multi_modal_search.h`

Unified TEXT + embedding (IMAGE/AUDIO/CUSTOM) search with weighted RRF
fusion across all modalities.

---

## Delivered in v1.9.0

### MultiFieldBoostedSearch API (`include/search/multi_field_search.h`)
**Status:** ✅ Delivered in v1.9.0

`MultiFieldBoostedSearch` provides configurable per-field boost weighting over BM25
full-text scores, implementing the "title > body > tags" search priority.

Key API surface:
- `search(query, fields)` — per-field BM25 + normalization + boost-weighted combination
- `defaultFields(table)` — canonical `title/body/tags` preset (boosts 3.0 / 1.0 / 0.5)
- `normalizeScores(scored)` — public static helper, directly unit-testable
- `setConfig(config)` — runtime config replacement

---

## Delivered in v2.0.0

### PersonalizedRanker API (`include/search/personalized_ranker.h`)
**Status:** ✅ Delivered in v2.0.0

`PersonalizedRanker` records per-user interaction events and uses them to
compute time-decayed personalization boosts for search result re-ranking.

Key API surface:
- `recordInteraction(interaction)` — record a VIEW, CLICK, BOOKMARK, LIKE, or DISLIKE event
- `computeScore(user_id, doc_id)` — personalization score in [-1, 1] with exponential time decay
- `applyPersonalization(user_id, candidates)` — boost/suppress `RankedResult::final_score` and re-sort
- `getUserInteractions(user_id)` — inspect stored per-user history (most-recent first)
- `clearUser(user_id)` / `clear()` — GDPR-compatible data removal
- Configurable `decay_rate`, `boost_weight`, and `max_interactions_per_user`
- Thread-safe via shared `std::mutex`
### CrossLingualSearch API (`include/search/cross_lingual_search.h`)
**Status:** ✅ Delivered in v2.0.0

`CrossLingualSearch` provides cross-lingual semantic retrieval by operating on
multilingual embedding vectors, enabling queries in one language to match
documents written in any language stored in the same vector space.

Key API surface:
- `search(embedding, hints)` — kNN query with distance-to-similarity conversion,
  per-language boost factors, score threshold filter, and k-cap
- `searchMultiEmbedding(queries, hints)` — fuses multiple query embeddings (e.g.
  one per language variant) via weighted Reciprocal Rank Fusion (RRF) before
  applying language boosts and threshold filtering
- `setLanguageMap(map)` — supplies `doc_id → language_code` mapping for result
  annotation (`Result::language`) and `LanguageHint` boost lookup
- `setConfig(config)` — runtime config replacement
- `LanguageHint` struct: `{language_code, boost}` — ISO 639-1 code + score multiplier
- `EmbeddingQuery` struct: `{embedding, weight}` — pre-computed vector + RRF weight
- `Result` struct: `{document_id, score, language}` — enriched with language metadata

---

## Delivered in v2.1.0

### SearchHighlighter API (`include/search/search_highlighter.h`)
**Status:** ✅ Delivered in v2.1.0 (Issue #2457)

`SearchHighlighter` provides highlight and snippet generation for matched search
terms in document text, enabling rich result presentation.

Key API surface:
- `highlight(text, terms)` — wraps every occurrence of a query term in `text` with
  configurable open/close markup tags (default `<em>` / `</em>`); case-insensitive,
  word-boundary matching, original capitalisation preserved
- `highlight(text, query)` — convenience overload: tokenises the raw query string first
- `highlight(text, terms_vector)` — overload accepting a pre-split term list
- `snippet(text, terms)` — extracts a short excerpt (≤ `Config::window_size` chars)
  centred on the densest cluster of query-term matches, with terms highlighted and
  `Config::separator` prepended/appended where the text is truncated
- `snippet(text, query)` / `snippet(text, terms_vector)` — convenience overloads
- `snippet(..., window_size)` — per-call window size override
- `setConfig(config)` — runtime config replacement
- `tokenize(query)` — public static helper: splits and lower-cases a raw query string
- `applyHighlight(text, terms, open_tag, close_tag)` — public static helper, directly testable
- `bestWindowOffset(lower_text, terms, window_size)` — public static helper for snippet offset
- `Config` fields: `open_tag`, `close_tag`, `separator`, `window_size`, `max_window_size`

---

## Delivered in v2.2.0

### NegativeKeywordFilter API (`include/search/negative_keyword_filter.h`)
**Status:** ✅ Delivered in v2.2.0 (Issue #2003)

`NegativeKeywordFilter` implements the `NOT` / minus-prefix operator for full-text search
queries, enabling callers to exclude documents that contain specific terms.

Supported query syntax:
- Minus prefix: `"machine learning -neural"` → excludes docs containing "neural"
- `NOT` keyword: `"machine learning NOT neural"` → equivalent
- Mixed: `"database -slow NOT crash"` → excludes both "slow" and "crash"

Key API surface:
- `ParsedQuery` struct: `positive_query` (terms to search for) + `negative_terms` (terms to exclude)
- `parseQuery(raw_query)` — static; parses `-term` and `NOT term` syntax, lowercases negatives,
  handles dangling `NOT`, lone `-` as positive token, case-insensitive `NOT` keyword
- `filter(table, column, candidate_pks, negative_terms)` — uses secondary index
  `scanFulltext()` to collect documents containing excluded terms, then removes them
  from `candidate_pks` while preserving order
- Null-index safety: returns error Status but preserves `candidate_pks` unchanged
- Empty `negative_terms`: passes through all PKs with OK Status
- Exception safety: `filter()` never throws; all index exceptions are caught and logged

Typical pipeline usage:
1. `parseQuery()` to split raw user query into positive + negative parts
2. Run BM25 / hybrid search on `positive_query`
3. `filter()` on the result PKs with `negative_terms`
4. Retain only results in the filtered PK set

---

## Delivered in v2.3.0

### DistributedHybridSearch API (`include/search/distributed_hybrid_search.h`)
**Status:** ✅ Delivered in v2.3.0 (Issue #2280)

`DistributedHybridSearch` extends the local `HybridSearch` engine to operate across a
cluster of ThemisDB shards.  It dispatches hybrid search requests in parallel to all
healthy remote shards via mTLS-secured HTTP POST (`RemoteExecutor`), then merges the
per-shard result lists into a single globally ranked result set using cross-shard
Reciprocal Rank Fusion (RRF).

Key API surface:
- `search(text_query, vector_query, stats)` — runs local + all remote shards in parallel,
  merges via cross-shard RRF, returns top-k globally ranked results.  Never throws.
- `mergeShardResults(shard_results)` — public RRF merge helper; directly unit-testable
  without network infrastructure
- `parseShardResponse(json_data)` — public static JSON deserializer; accepts direct array
  or `{"results": [...]}` wrapped format; exposed primarily for unit testing
- `ShardSearchResult` struct: `{shard_id, results, success, error_msg, execution_time_ms}`
- `SearchStats` struct: `{shards_queried, shards_succeeded, shards_failed, partial_result}`
- `Config` fields: `k`, `rrf_k`, `shard_timeout_ms`, `max_concurrent_shards`,
  `skip_failed_shards`, `local_shard_id`, `search_endpoint` (default: `"/search/hybrid"`)
- Fault tolerance: `skip_failed_shards=true` (default) skips timed-out / unreachable shards
  and returns results from surviving shards; callers inspect `SearchStats::partial_result`
- mTLS: all inter-node traffic is routed through the injected `RemoteExecutor` which is
  constructed with mTLS certificates — no additional TLS configuration required

Typical pipeline usage:
```cpp
RemoteExecutor::Config exec_cfg;
exec_cfg.cert_path = "/etc/themis/tls/shard.crt";
exec_cfg.key_path  = "/etc/themis/tls/shard.key";
exec_cfg.ca_cert_path = "/etc/themis/tls/ca.crt";
auto executor = std::make_shared<RemoteExecutor>(exec_cfg);

DistributedHybridSearch::Config dhs_cfg;
dhs_cfg.k               = 20;
dhs_cfg.local_shard_id  = "shard_001";
DistributedHybridSearch dhs(&local_hs, resolver.get(), executor.get(), dhs_cfg);

DistributedHybridSearch::SearchStats stats;
auto results = dhs.search("machine learning", query_embedding, &stats);
// stats.shards_failed > 0 indicates degraded mode
```

---

- **Neural LTR**: LambdaMART or small MLP scorer with offline batch training integration
- **Multi-namespace VectorIndexManager**: one instance per modality namespace
- **Streaming result delivery**: async/generator API for large `k` values

---

## Test Strategy

- Unit tests for `LearningToRank` re-ranking with synthetic feature vectors and known score ordering
- Model hot-swap tests verifying zero-downtime swap under concurrent search load
- Faceted search composability tests with nested filter combinations across multiple facet types
- Cross-lingual search tests using parallel corpora (query in language A, results in language B)
- Query rewriter hook ordering tests ensuring deterministic rewrite output
- Compile-time exclusion tests confirming `NeuralSparseRetriever` is absent without `THEMIS_NEURAL_SPARSE`

## Performance Targets

- LTR re-ranking ≤ 10 ms for top-100 candidates (p99)
- Faceted aggregation ≤ 5 ms for up to 10 facets over 1 M documents
- Query rewriting pipeline ≤ 20 ms end-to-end
- Neural sparse retrieval ≤ 50 ms for vocabulary size ≤ 30,000 terms
- `CrossLingualSearch::searchMultiEmbedding()` RRF fusion ≤ 15 ms for 5 query embeddings

## Security / Reliability

- Search query strings hashed (SHA-256) before storage in analytics log — raw query never persisted
- No query content included in error messages or stack traces
- LTR model updates validated against a held-out test set before hot-swap activation
- Faceted filter inputs sanitized to prevent injection into underlying index scans
- Rate limiting applied at the search API boundary to prevent denial-of-service via expensive queries

## See Also

- [Current API](README.md)
- [Implementation FUTURE_ENHANCEMENTS](../../src/search/FUTURE_ENHANCEMENTS.md)

---

*Last Updated: April 2026*
*Current API Version: v2.2.0*
*Next Target: v2.3.0*

<!-- validated: 2026-04-06 | commit: a14cdb2 -->
