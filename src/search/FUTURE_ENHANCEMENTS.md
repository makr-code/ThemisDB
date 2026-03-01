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

## Delivered in v1.8.0

### LlmReranker (`include/search/llm_reranker.h`)
- Configurable re-ranking with LLM feedback loop, closing Phase 3 of the Search Roadmap
- Injected `LlmBackend` callable (same pattern as `LlmQueryRewriter`) — no hard LLM library dependency
- `LlmRerankCandidate` and `LlmRerankResult` value types; `rerank()` accepts any result source
- Batched prompting: splits candidates into configurable `batch_size` groups for cost control
- Score parsing: per-document 0–10 integer scores from the LLM response, normalised to [0, 1]
- Out-of-range score clamping (values outside [0, 10] clamped before normalisation)
- Configurable blending: `final = llm_weight * llm_score + (1 - llm_weight) * initial_score`
- `min_score_threshold` filter: removes low-confidence results from the output
- Graceful fallback: returns original order when backend is absent, throws, or produces no output
- `setBackend()`: runtime backend replacement (e.g. after model load)
- `toClickEvents()` static method: converts LLM relevance judgments to `ClickEvent` objects
  for direct consumption by `LearningToRank::recordClick()`, closing the feedback loop
- Tests: `tests/test_llm_reranker.cpp`

### HybridSearch LlmReranker integration (`include/search/hybrid_search.h`)
- `HybridSearch::setReranker()` attaches an `LlmReranker` to the search pipeline
- Applied as a post-fusion step after RRF (or linear combination): top-N results are re-scored
  by the LLM and returned in LLM-determined order with updated `hybrid_score`
- Null backend disables re-ranking; `search()` returns RRF order unchanged
- `HybridSearch::Result::content` field forwarded to `LlmRerankCandidate::content`
- Tests: `tests/test_hybrid_search.cpp` (`HybridSearchReranker` test group)

---

## Delivered in v1.9.0

### MultiFieldBoostedSearch (`include/search/multi_field_search.h`)
- Multi-field boosted full-text search with per-field BM25 scoring and configurable boost weights
- Implements the "title > body > tags" priority: title boost=3.0, body boost=1.0, tags boost=0.5
- `FieldConfig` value type: `{table, column, boost}` — fully configurable, any field combination
- `search(query, fields)` — queries each field independently, normalises BM25 scores to [0, 1]
  per field, multiplies by the field boost, and accumulates into a combined per-document score
- Score combination formula: `score(doc) = Σ_f( boost_f × normalised_bm25_score_f(doc) )`
- Documents appearing in only a subset of fields are still ranked (missing fields contribute 0)
- `defaultFields(table)` — convenience factory: title/body/tags with the canonical boost hierarchy
- `Result` struct: `document_id`, combined `score`, and per-field `field_scores` breakdown
- Graceful handling: negative-boost fields are skipped with a warning; empty query/fields return
  immediately; null index returns empty without throwing; all exceptions caught inside `search()`
- Exception safety: `search()` never throws; errors logged via `THEMIS_WARN`/`THEMIS_ERROR`
- Tests: `tests/test_multi_field_search.cpp`

---

## Delivered in v2.0.0

### CrossLingualSearch (`include/search/cross_lingual_search.h`)
- Cross-lingual semantic search via multilingual embeddings (Phase 4 of the Search Roadmap)
- Model-agnostic design: callers supply pre-computed float vectors (e.g. from
  `paraphrase-multilingual-mpnet-base-v2` or LaBSE); no hard embedding-library dependency
- `search(embedding, hints)` — single-embedding kNN query with distance-to-similarity
  conversion (`1 / (1 + distance)`), optional per-language boost, score threshold filter, k-cap
- `searchMultiEmbedding(queries, hints)` — issues independent kNN queries for each
  `EmbeddingQuery` (embedding + weight), fuses ranked lists via weighted Reciprocal Rank Fusion
  (RRF), then applies language boosts and threshold filtering
- `setLanguageMap(map)` — supplies a `doc_id → language_code` (ISO 639-1) mapping for result
  annotation (`Result::language`) and `LanguageHint` boost application
- `LanguageHint` struct: `{language_code, boost}` — multiplies the final score of results in
  the specified language; zero or negative boost hints are silently ignored
- `EmbeddingQuery` struct: `{embedding, weight}` — weight drives the RRF contribution term
- Resource safety: `k` and `candidates` are clamped to `max_k` / `max_candidates` at
  construction time; `search()` and `searchMultiEmbedding()` never throw
- Config validation: throws `std::invalid_argument` on k=0, candidates=0, rrf_k≤0
- Tests: `tests/test_cross_lingual_search.cpp`
### NeuralSparseRetrieval (`include/search/neural_sparse_retrieval.h`)
- SPLADE / BERT-based neural sparse retrieval engine, closing Phase 4 item of the Search Roadmap
- `SparseVector` type alias: `unordered_map<string, float>` — non-zero learned term weights
- `SparseEncoderBackend` callable type — same injected-backend pattern as `LlmReranker::LlmBackend`
- `addDocument(doc_id, sparse_vec)` — indexes a pre-computed sparse vector; negative weights clamped
- `addDocumentText(doc_id, text)` — encodes text via attached backend then indexes it
- `removeDocument(doc_id)` — clean removal from both forward and inverted indexes
- `search(query_vec, k)` — inverted-index dot-product accumulation; O(|q| × postings)
- `searchText(query_text, k)` — encodes query then calls `search()`; never throws
- Scoring: `score(q, d) = Σ_t( q[t] * d[t] )` — standard inner product over shared terms
- `Config::max_terms_per_doc` — soft cap; top-weighted terms kept, remainder discarded
- `Config::score_threshold` — pre-normalization minimum score filter
- `Config::normalize_scores` — optional linear rescaling to [0, 1] (same edge-case handling as
  `HybridSearch::normalizeScores`)
- `normalizeScores()` promoted to `public static` for direct unit testing
- Result struct: `document_id`, `score` (normalized), `raw_score` (inner product before normalization)
- Graceful fallback: no encoder → no-op for text methods; encoder throws → empty result, no propagation
- Tests: `tests/test_neural_sparse_retrieval.cpp`

---

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
