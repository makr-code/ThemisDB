# Search Module API - Future Enhancements

## Delivered in v1.5.0

All features listed below were delivered in v1.5.0.  The actual public API is
documented in [`README.md`](README.md).

### Query Expansion API
**Status:** ✅ Delivered in v1.5.0 — see `include/search/query_expander.h`

`QueryExpander` provides synonym expansion, Levenshtein-based spelling
correction, alternative query generation, and zero-result relaxation.

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

## Planned for v1.6.0

- **Personalized autocomplete**: per-user click-history weighting in `AutocompleteEngine`
- **Neural LTR**: LambdaMART or small MLP scorer with offline batch training integration
- **Multi-namespace VectorIndexManager**: one instance per modality namespace
- **Streaming result delivery**: async/generator API for large `k` values

---

## See Also

- [Current API](README.md)
- [Implementation FUTURE_ENHANCEMENTS](../../src/search/FUTURE_ENHANCEMENTS.md)

---

*Last Updated: February 2026*  
*Current API Version: v1.5.0*  
*Next Target: v1.6.0*
