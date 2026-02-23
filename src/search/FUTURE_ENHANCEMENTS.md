# Search Module - Future Enhancements

## Delivered in v1.4.0

The following production-readiness improvements were delivered in v1.4.0:

- Configurable vector metric (COSINE / DOT / L2) — was hardcoded to COSINE
- Strict config validation with `std::invalid_argument` on bad parameters
- Hard resource limits (`max_k`, `max_candidates`) to bound memory / latency
- Score normalization edge-case fixes (range == 0 / single-result)
- Linear-combination pre-normalization (scores always in [0,1] before weighting)
- `SearchStats` struct for graceful degradation and partial-result detection
- Thread-safety and exception-safety documentation
- `normalizeScores` promoted to `public static` for testability
- Comprehensive test suite: `test_hybrid_search.cpp`, `test_rrf_fusion.cpp`,
  `test_score_normalization.cpp`, `test_hybrid_search_integration.cpp`
- `benchmark_hybrid_search.cpp` for algorithmic performance measurement

---

## Delivered in v1.5.0

The following high-priority features were delivered in v1.5.0:

### QueryExpander (`include/search/query_expander.h`)
- Synonym expansion with configurable maximum expansion count
- Per-token spelling correction using Levenshtein edit distance against a
  user-supplied vocabulary
- Alternative query generation by substituting tokens with their synonyms
- Zero-result fallback via `relaxQuery()` (drops the last token)
- Tests: `tests/test_query_expander.cpp`

### FuzzyMatcher (`include/search/fuzzy_matcher.h`)
- Levenshtein edit-distance search wrapping `SecondaryIndexManager::scanFulltextFuzzy`
- Public static utilities: `levenshtein()`, `soundex()`, `metaphone()`, `ngramSimilarity()`
- Unified [0,1] similarity score across all algorithms
- Bigram/N-gram overlap (Dice coefficient)
- Tests: `tests/test_fuzzy_matcher.cpp`

### FacetedSearch (`include/search/faceted_search.h`)
- `computeFacet()` — value-count facets for a single column
- `computeFacets()` — multi-column facets in one call
- `computeRangeFacet()` — numeric range buckets
- `applyFacetFilters()` — intersect candidate PK sets with active field=value constraints
- Tests: `tests/test_faceted_search.cpp`

### SearchAnalytics (`include/search/search_analytics.h`)
- Thread-safe query event logging with configurable max-events cap (circular eviction)
- `computeMetrics()` — average, p95, p99 latency; zero-result rate; top-20 queries
- `getZeroResultQueries()` — retrieve most-recent zero-result events for alerting
- `getRecentEvents()` — retrieve recent event log
- Tests: `tests/test_search_analytics.cpp`

### AutocompleteEngine (`include/search/autocomplete.h`)
- `suggestByPrefix()` — secondary-index prefix scan returning matching field values
- `suggestPopular()` — popular past queries starting with the prefix (backed by SearchAnalytics)
- `suggest()` — combined, deduplicated, score-ranked suggestions from both sources
- Tests: `tests/test_autocomplete.cpp`

### LearningToRank (`include/search/learning_to_rank.h`)
- `rerank()` — dot-product linear re-ranker over 6-dimensional `RankingFeatures`
- `recordClick()` / `train()` — pairwise gradient-descent training from click-through events
- `registerVariant()` / `selectVariant()` — deterministic A/B traffic splitting
- `rerankWithVariant()` — apply a named scorer; falls back to default if variant not found
- Tests: `tests/test_learning_to_rank.cpp`

### MultiModalSearch (`include/search/multi_modal_search.h`)
- Accepts `ModalQuery` components (TEXT, IMAGE, AUDIO, CUSTOM) each with a weight
- TEXT modality: delegates to `SecondaryIndexManager::scanFulltextWithScores`
- Embedding modalities: delegates to `VectorIndexManager::searchKnn`
- All result lists fused via weighted RRF; top-k returned
- `searchTextAndImage()` convenience method for the most common bi-modal case
- Tests: `tests/test_multi_modal_search.cpp`

---

## Delivered in v1.6.0

### LlmQueryRewriter (`include/search/llm_query_rewriter.h`)
- LLM-based query rewriting for improved recall via injected `LlmBackend` callback
- Builds structured prompts instructing the LLM to produce `n` numbered alternative phrasings
- Parses numbered lines (supporting `.`, `)`, and `:` separators) from LLM response
- Deduplication: case-insensitive comparison removes duplicate and original-equivalent rewrites
- Length guard: rewrites exceeding `max_rewrite_length` are silently dropped
- Graceful fallback: returns original query when backend is absent, throws, or produces no output
- `setBackend()`: runtime backend replacement (e.g. after model load)
- Tests: `tests/test_llm_query_rewriter.cpp`

---

## Delivered in v1.7.0

### Ranked Spelling Correction Suggestions (`include/search/query_expander.h`)
- `SpellingCorrection` struct: `suggestion`, `edit_distance`, `confidence` (normalized [0,1])
- `QueryExpander::suggestSpellingCorrections(word, max_suggestions)` — returns up to
  `max_suggestions` ranked candidates from the registered vocabulary within `max_edit_distance`,
  sorted by ascending edit distance (alphabetical tiebreak); confidence decays linearly with distance
- `QueryExpander::suggestQueryCorrections(query, max_suggestions)` — tokenizes the query,
  builds per-token substitution variants and one all-corrected variant, deduplicates, and sorts
  by total edit distance ascending
- Tests: `tests/test_query_expander.cpp` (14 new test cases)

---

## Planned Features

### Query Expansion and Rewriting
**Priority:** High  
**Status:** ✅ Delivered in v1.5.0 — see `include/search/query_expander.h`

Automatically expand and rewrite queries for better results.

**Features:**
- Synonym expansion using thesaurus
- Query relaxation for zero-result queries
- Spelling correction and suggestion
- Query intent detection
- Automatic phrase detection

**Implementation:**
```cpp
class QueryExpander {
public:
    struct ExpansionConfig {
        bool use_synonyms = true;
        bool correct_spelling = true;
        bool detect_phrases = true;
        double synonym_weight = 0.8;
        size_t max_expansions = 5;
    };
    
    Result<ExpandedQuery> expand(
        const std::string& query,
        const ExpansionConfig& config
    );
    
    Result<std::string> correctSpelling(
        const std::string& query
    );
    
    Result<std::vector<std::string>> suggestAlternatives(
        const std::string& query
    );
};
```

---

### Advanced Fuzzy Matching
**Priority:** Medium  
**Status:** ✅ Delivered in v1.5.0 — see `include/search/fuzzy_matcher.h`

Enhanced fuzzy search with phonetic algorithms.

**Features:**
- Levenshtein distance for edit distance
- Soundex and Metaphone for phonetic matching
- N-gram matching
- Configurable fuzziness levels
- Performance optimization with BK-trees

**Expected Performance:**
- Fuzzy search overhead: <50% vs exact search
- Support 1-2 edit distance efficiently

---

### Multi-Modal Search
**Priority:** Medium  
**Status:** ✅ Delivered in v1.5.0 — see `include/search/multi_modal_search.h`

Search across text, images, and other modalities.

**Features:**
- Image search using CLIP embeddings
- Audio search for voice queries
- Cross-modal retrieval (text→image, image→text)
- Unified embedding space

---

### Learning to Rank (LTR)
**Priority:** Medium  
**Status:** ✅ Delivered in v1.5.0 — see `include/search/learning_to_rank.h`

Machine learning-based result ranking.

**Features:**
- Training from click-through data
- Multiple ranking features
- Online learning and adaptation
- A/B testing framework

---

### Search Analytics
**Priority:** High  
**Status:** ✅ Delivered in v1.5.0 — see `include/search/search_analytics.h`

Track and analyze search performance.

**Features:**
- Query log analysis
- Click-through rate tracking
- Zero-result query detection
- Search performance metrics
- User behavior analysis

---

### Faceted Search
**Priority:** High  
**Status:** ✅ Delivered in v1.5.0 — see `include/search/faceted_search.h`

Multi-dimensional filtering and navigation.

**Features:**
- Dynamic facet generation
- Range facets (price, date)
- Hierarchical facets (categories)
- Facet count computation
- Drill-down navigation

---

### Autocomplete and Suggestions
**Priority:** Medium  
**Status:** ✅ Delivered in v1.5.0 — see `include/search/autocomplete.h`

Real-time query suggestions.

**Features:**
- Prefix-based suggestions
- Popular query suggestions
- Personalized suggestions
- Context-aware completion
- Instant search results

---

## Performance Roadmap

### v1.5.0 Achieved
- Query expansion overhead: <20% (in-memory synonym+spelling correction)
- Fuzzy search: Within 2x of exact search (Levenshtein wrapped over BM25 index)
- Faceted search: <50ms for 10 facets (secondary index range scans)
- Autocomplete latency: <10ms (prefix range scan + analytics top-queries)
- LTR inference: <1ms per query (linear dot product, 6 features)
- Multi-modal search: sub-millisecond RRF fusion (embeddings pre-computed by caller)

### v1.6.0 Targets
- Personalized autocomplete (per-user click history)
- Neural LTR (LambdaMART or small MLP) with offline batch training
- Multi-namespace VectorIndexManager (one instance per modality)
- Streaming result delivery for large k values

---

## See Also

- [README.md](README.md) - Current module documentation
- [Header Documentation](../../include/search/README.md) - Public API
- [Index Module](../index/FUTURE_ENHANCEMENTS.md) - Index improvements

---

*Last Updated: February 2026*  
*Module Version: v1.7.0*  
*Next Review: v1.8.0 Release*
