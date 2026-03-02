# Search Module API - Future Enhancements

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

## Planned for v1.6.0

- **Neural LTR**: LambdaMART or small MLP scorer with offline batch training integration
- **Multi-namespace VectorIndexManager**: one instance per modality namespace
- **Streaming result delivery**: async/generator API for large `k` values

---

## See Also

- [Current API](README.md)
- [Implementation FUTURE_ENHANCEMENTS](../../src/search/FUTURE_ENHANCEMENTS.md)

---

*Last Updated: February 2026*  
*Current API Version: v1.9.0*  
*Next Target: v2.0.0*
