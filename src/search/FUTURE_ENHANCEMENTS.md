> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Search Module - Future Enhancements

- BM25 hybrid search with Reciprocal Rank Fusion (RRF) blending lexical and semantic results; configurable k1/b/RRF-k constants
- HNSW vector indexing supporting Cosine, Dot Product, and L2 distance metrics with 85%+ recall@10
- LLM-based query rewriting to improve recall for underspecified or noisy queries
- Faceted search with per-field value-count facets and numeric range buckets with drill-down filtering
- Learning-to-rank (LTR) re-ranker using a 6-dimensional feature vector with online pairwise gradient-descent training
- Multi-field boosting (title > body > tags) with configurable per-field weight vectors
- Neural sparse retrieval (SPLADE) for vocabulary expansion and out-of-vocabulary term handling
- Cross-lingual semantic search via multilingual embedding models with automatic language detection

## Design Constraints

- [ ] Hybrid search (BM25 + HNSW RRF) must return top-10 results in ≤ 20 ms at p99 for indices up to 10 M documents
- [ ] BM25 default parameters (k1=1.2, b=0.75) must remain unchanged; parameter changes must be explicit
- [ ] `HybridSearch` API (RRF weights, mode selection) must remain stable from v1.2.0 onwards; no breaking changes
- [ ] LLM query rewriting must degrade gracefully to the original query when the LLM is unavailable or exceeds 200 ms
- [ ] LTR model updates must be applied online (no index rebuild) and must not affect in-flight queries
- [ ] SPLADE sparse vectors must be stored in a compressed sparse row (CSR) format to cap memory at ≤ 4 GB per 10 M docs
- [ ] Cross-lingual search must support at least 50 languages via a single multilingual model with no per-language index
- [ ] Facet counting must complete in ≤ 5 ms for up to 1,000 distinct facet values on a result set of 100,000 documents

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `HybridSearch::search(query, top_k, mode)` | HTTP API, SDK | Modes: TEXT / VECTOR / HYBRID; returns `SearchResult` list |
| `LlmQueryRewriter::rewrite(query)` | HybridSearch pre-processor | Returns rewritten query string; 200 ms timeout with original fallback |
| `FacetedSearch::facet(results, fields)` | API response builder | Returns `FacetMap`; supports value-count and numeric range buckets |
| `LearningToRank::rerank(query, results)` | Result post-processor | Online pairwise training; model update via `train(feedback)` |
| `MultiFieldBoostedSearch::search(query, weights)` | API handler | Per-field boost weights map; falls back to equal weights if empty |
| `NeuralSparseRetriever::retrieve(query, top_k)` | Hybrid retrieval pipeline | SPLADE sparse vectors; CSR storage format |
| `CrossLingualSearch::search(query, lang_hint)` | Multilingual API | Auto-detects language; uses multilingual embedding model |
| `SearchAnalytics::record(query, latency_ms)` | Request middleware | Thread-safe; exports avg/p95/p99 latency and zero-result rate |

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

### PersonalizedRanker (`include/search/personalized_ranker.h`)
- Per-user interaction history tracking for personalized search result re-ranking
- `InteractionType` enum: VIEW (0.2), CLICK (0.5), BOOKMARK (1.0), LIKE (1.0), DISLIKE (-0.5)
- `UserInteraction` struct: `user_id`, `document_id`, `type`, `timestamp`
- Time-decayed scoring: `score += type_weight * exp(-decay_rate * age_days)` — more recent interactions contribute more
- Configurable decay rate (default 0.05 ≈ half-weight after ~14 days), boost weight, and per-user buffer size
- Per-user history bounded by `Config::max_interactions_per_user` with oldest-first eviction
- `computeScore(user_id, doc_id)` — returns personalization score in [-1, 1] for any (user, document) pair
- `applyPersonalization(user_id, candidates)` — adjusts `final_score` of ranked candidates and re-sorts
- `getUserInteractions(user_id)` — inspect stored interactions (most-recent first)
- `clearUser(user_id)` / `clear()` — GDPR-compatible user-data removal
- Thread-safe: all methods protected by a shared `std::mutex`
- Tests: `tests/test_personalized_ranker.cpp` (28 test cases)
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

## Delivered in v2.1.0

### SearchHighlighter (`include/search/search_highlighter.h`)
- Highlight and snippet generation for matched search terms (Issue #2457)
- `highlight(text, terms, config)` — wraps every occurrence of each query term in configurable
  open/close tags (default `<mark>…</mark>`); case-insensitive matching controlled by `Config::case_insensitive`
- `snippet(text, terms, config)` — extracts the best passage from a document; uses a sliding-window
  scorer (`bestWindowOffset()`) to find the position covering the most matches; pads excerpt with
  configurable ellipsis (`Config::ellipsis`) at boundaries
- `tokenize(text, lowercase)` static helper — punctuation-aware tokeniser; optional case folding
- `applyHighlight(text, offsets, open, close)` static helper — applies a sorted list of (start, len)
  match offsets in a single linear pass, without overlapping or duplicated tags
- `bestWindowOffset(text, terms, window_size)` static helper — sliding-window scorer returning the
  character offset where the most query terms appear within a window of `window_size` characters
- Fully stateless after construction; `highlight()` and `snippet()` are `noexcept`
- Config fields: `highlight_open`, `highlight_close`, `ellipsis`, `min_window`, `max_snippet_len`,
  `case_insensitive`
- Tests: `tests/test_search_highlighter.cpp`

---

## Delivered in v2.2.0

### NegativeKeywordFilter (`include/search/negative_keyword_filter.h`)
- `NOT` / minus-prefix operator for search queries (Issue #2003)
- `ParsedQuery` struct: `positive_query` (remaining text) + `negative_terms` (excluded words)
- `parseQuery(raw_query)` static helper — extracts minus-prefixed (`-term`) and `NOT`-keyword
  tokens from the raw user query, returning positive and negative parts separately; supports
  mixed syntax (`"machine learning -neural NOT crash"`)
- `filter(table, column, candidate_pks, negative_terms)` — looks up posting lists for each
  negative term in the secondary index, collects all document PKs that contain any excluded term,
  and returns the candidate PK list with those documents removed
- `Config::max_scan` — per-term scan limit to bound resource usage (default 10 000)
- Null-index safety: `filter()` returns an error `Status` and the original `candidate_pks`
  unchanged when constructed with a null `SecondaryIndexManager` pointer
- Thread safety: `NegativeKeywordFilter` holds only a non-owning pointer to the index; concurrent
  calls to `filter()` are safe as long as the underlying index is not modified concurrently
- Tests: `tests/test_negative_keyword_filter.cpp`

---

## Delivered in v2.3.0

### DistributedHybridSearch (`include/search/distributed_hybrid_search.h`)
- Distributed hybrid search across multiple ThemisDB shards with cross-shard RRF result
  merging (Issue #2280)
- Extends local `HybridSearch`: runs a local search, then dispatches the same query in parallel
  to every healthy remote shard via `RemoteExecutor::post()` (REST, mTLS-secured)
- Cross-shard Reciprocal Rank Fusion: per-shard ranked lists are merged globally; ties broken
  by `hybrid_score`; top-k globally merged results returned
- `ShardResult` struct: per-shard results and HTTP status
- `Config::skip_failed_shards` (default `true`) — unreachable / timed-out shards are silently
  skipped; degraded results are still returned from surviving shards
- `Config::search_endpoint` — override for the HTTP POST path on remote shards
  (default `/api/v1/search/hybrid`); request body: `{"query":"…","k":<int>,"vector_query":[…]}`
- `SearchStats::shards_queried` / `shards_failed` observability fields
- mTLS authentication: all inter-node traffic routed through the injected `RemoteExecutor`
- Thread safety: NOT thread-safe; concurrent `search()` calls must be serialised externally
- Tests: `tests/test_distributed_hybrid_search.cpp`

---

## Delivered in v2.4.0

### ConversationalSearch (`include/search/conversational_search.h`)
- Multi-turn conversational search with context-aware query reformulation (Phase 5)
- `search(query)` — reformulates the query using `context_window` most recent turns, dispatches
  to the underlying `HybridSearch`, and appends the turn to the session history
- `reformulate(query)` — public static-equivalent helper; builds context-enriched query string
  by prepending up to `context_window` prior queries separated by `Config::context_separator`
- `Turn` struct: `query`, `reformulated_query`, `results` — full turn record
- `getHistory()` — inspect full deque of turns (oldest first)
- `clearHistory()` — remove all history (GDPR-compatible session reset)
- `Config::context_window` — number of prior turns to include in context (0 = stateless)
- `Config::max_history` — maximum retained turns; oldest are evicted (min 1)
- `Config::context_separator` — separator between historical query terms (default `" "`)
- Exception safety: `search()` never throws; null `HybridSearch` returns empty results
- Tests: `tests/test_search_future_interfaces.cpp` (17 tests — ConversationalSearch*)

---

### FederatedSearch (`include/search/federated_search.h`)
- Federated search across multiple isolated per-tenant `HybridSearch` indexes (Phase 5)
- `registerTenant(id, hs)` — register a named tenant index (non-owning pointer)
- `removeTenant(id)` / `setTenantWeight(id, w)` — dynamic tenant management
- `search(query, vector_query, tenant_stats)` — queries all non-skipped tenants, merges via
  weighted Reciprocal Rank Fusion (RRF), returns top-k globally ranked results
- `mergeTenantResults(map)` — public RRF merge helper; directly unit-testable without network
- `Result` struct: `document_id`, `tenant_id`, `score`, `bm25_score`, `vector_score`
- `TenantStats` struct: per-tenant diagnostics (`results_count`, `skipped`)
- Tenant weights clamped to [0, 1]; weight=0 excludes tenant from results
- `Config::skip_null_tenants` — silently skip null-pointer tenants (default true)
- Exception safety: `search()` never throws; per-tenant errors are caught and logged
- Tests: `tests/test_search_future_interfaces.cpp` (17 tests — FederatedSearch*)

---

### SearchResultStream (`include/search/search_result_stream.h`)
- Cursor-based streaming pagination over large `HybridSearch` result sets (Phase 5)
- `open(query)` — fetches up to `Config::total_k` results, resets cursor to 0
- `nextPage()` — returns up to `Config::page_size` results and advances cursor
- `hasMore()` / `reset()` / `close()` — cursor and stream lifecycle management
- `forEachResult(callback)` — streaming iteration; return `false` from callback for early stop
- `totalResults()` / `cursorPosition()` — inspection accessors
- Temporarily adjusts the underlying `HybridSearch` config k to `total_k` during `open()`
  and restores it afterwards; transparent to the caller
- Exception safety: `open()` and `nextPage()` never throw; callback exceptions are caught
- Tests: `tests/test_search_future_interfaces.cpp` (17 tests — SearchResultStream*)

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

## Test Strategy

- Unit test coverage ≥ 80% for `HybridSearch`, `BM25Ranker`, `LlmQueryRewriter`, `FacetedSearch`, `LearningToRank`, `NeuralSparseRetriever`, and `CrossLingualSearch`
- BM25 correctness tests: compute BM25 scores for 10 hand-crafted query-document pairs and assert results match the reference formula within ± 0.001
- Hybrid recall@10 integration test: evaluate against a 1 M-document corpus; assert recall@10 ≥ 85% in HYBRID mode
- LLM query rewriter fallback test: simulate LLM timeout (> 200 ms); assert original query is used and no exception is thrown
- LTR online training test: feed 1,000 pairwise preference events; assert MRR@10 improves by ≥ 5% relative to the untrained model after 500 updates
- Faceted search tests: assert value-count facets are correct for 1,000 distinct values and numeric range buckets respect configured boundaries
- SPLADE neural sparse retrieval test: assert retrieved document IDs overlap ≥ 70% with BM25 top-20 on the BEIR TREC-COVID split
- Cross-lingual search tests: submit queries in 5 languages (EN, DE, FR, ES, ZH); assert top-1 result is in the correct language-agnostic answer set

## Performance Targets

- Hybrid search (BM25 + HNSW RRF) latency: ≤ 20 ms at p99 for top-10 results on an index of up to 10 M documents
- HNSW index build throughput: ≥ 10,000 vectors/s for 768-dimensional embeddings on a single CPU node
- BM25 ranking throughput: ≥ 50,000 queries/s for simple term queries against a 1 M-document index on a 16-core host
- LLM query rewriter overhead: ≤ 200 ms added latency at p99; 0 ms when LLM is unavailable (passthrough)
- Facet counting latency: ≤ 5 ms for up to 1,000 distinct facet values on a result set of 100,000 documents
- LTR re-ranking latency: ≤ 2 ms for re-ranking top-100 candidates with the 6-dimensional linear model
- SPLADE index memory: ≤ 4 GB for a 10 M-document corpus stored in CSR format
- Autocomplete suggestion latency: ≤ 5 ms at p99 for prefix queries against a 1 M-term dictionary

---

## Identified Gaps (from AI_ML_IMPACT_ASSESSMENT.md)

### Gap 2 — LlmQueryRewriter: Semantic Output Validator and Structured Fallback (Target: Q3 2026)

**Source:** `AI_ML_IMPACT_ASSESSMENT.md §7, Gap 2 (Severity: Medium/S2)`
**Status:** ✅ Implemented (2026-04-21)

**Problem:** `LlmQueryRewriter::rewrite()` had no semantic output validator: an LLM
response that is syntactically parseable but semantically nonsensical was accepted
and returned as-is with no signal to callers.

**Implemented changes:**
- `RewriteQuality` enum (`OK` / `FALLBACK`) added to `llm_query_rewriter.h`.
- `RewrittenQuery::quality` field added (default: `OK`).
- `Config::min_token_overlap_ratio` (default: `0.2`) — rewrites below the Jaccard
  token-overlap threshold are discarded; when all are discarded, `quality=FALLBACK`
  and the original query is returned via `fallback_to_original`.
- `LlmQueryRewriter::jaccardTokenOverlap()` + `applyOverlapFilter()` private helpers
  added to `llm_query_rewriter.h` / `.cpp`.
- Tests: `test_llm_query_rewriter_validator.cpp` (LQR_VAL_01..06) registered as
  `LlmQueryRewriterValidatorFocusedTests`.

**Inputs:** `std::string query`, LLM-generated rewrite lines.
**Outputs:** `RewrittenQuery { rewrites, quality=OK|FALLBACK }`.
**Constraints:** Existing `fallback_to_original` behaviour on error/timeout paths unchanged.
**Perf target:** ≤ 1 ms additional overhead at p99 (one `unordered_set` per rewrite).

---

## Security / Reliability

- Query injection prevention: all user-supplied query strings must be length-limited (≤ 4,096 bytes) and stripped of control characters before being passed to the BM25 tokenizer or LLM rewriter
- Resource exhaustion: `max_k` and `max_candidates` hard limits must be enforced before HNSW traversal begins; requests exceeding these limits return HTTP 400, not a partial result
- LTR model integrity: the learned weight vector must be persisted with a checksum; a corrupted model file must be detected on load and the system must fall back to uniform weights
- SPLADE vocabulary: the sparse vocabulary index must be loaded from a read-only memory-mapped file; writes to the vocabulary at runtime are rejected
- Cross-lingual model: the multilingual embedding model binary must be verified against a SHA-256 hash at startup; a hash mismatch aborts loading and falls back to the BM25-only mode
- `SearchAnalytics` query log must not store raw query strings containing PII; queries are hashed (SHA-256) before storage
- Distributed shard search must authenticate inter-shard requests with mTLS; unauthenticated shard calls are rejected with a 401 error

---

## Scientific References

### Neural Sparse Retrieval (SPLADE)

[1] T. Formal, B. Piwowarski, J. Genthial, and S. Clinchant, "SPLADE: Sparse Lexical and Expansion Model for First Stage Ranking," in *Proc. 44th Int. ACM SIGIR Conf. Res. Dev. Inf. Retrieval (SIGIR)*, pp. 2288–2292, 2021. https://doi.org/10.1145/3404835.3463098

[2] T. Formal, C. Lassance, B. Piwowarski, and S. Clinchant, "From Distillation to Hard Negative Sampling: Making Sparse Neural IR Models More Effective," in *Proc. 45th Int. ACM SIGIR Conf. Res. Dev. Inf. Retrieval (SIGIR)*, pp. 2353–2359, 2022. https://doi.org/10.1145/3477495.3531857

### Cross-Lingual Semantic Search

[3] F. Feng, Y. Yang, D. Cer, N. Arivazhagan, and W. Wang, "Language-Agnostic BERT Sentence Embedding," in *Proc. 60th Annu. Meet. Assoc. Comput. Linguistics (ACL)*, pp. 878–891, 2022. https://doi.org/10.18653/v1/2022.acl-long.62

[4] J. Reimers and I. Gurevych, "Making Monolingual Sentence Embeddings Multilingual using Knowledge Distillation," in *Proc. 2020 Conf. Empirical Methods Nat. Lang. Process. (EMNLP)*, pp. 4512–4525, 2020. https://doi.org/10.18653/v1/2020.emnlp-main.365

### Learning to Rank

[5] C. J. C. Burges, T. Shaked, E. Renshaw, A. Lazier, M. Deeds, N. Hamilton, and G. Hullender, "Learning to Rank using Gradient Descent," in *Proc. 22nd Int. Conf. Mach. Learn. (ICML)*, pp. 89–96, 2005. https://doi.org/10.1145/1102351.1102363

[6] Z. Cao, T. Qin, T.-Y. Liu, M.-F. Tsai, and H. Li, "Learning to Rank: From Pairwise Approach to Listwise Approach," in *Proc. 24th Int. Conf. Mach. Learn. (ICML)*, pp. 129–136, 2007. https://doi.org/10.1145/1273496.1273513

### Distributed Search

[7] N. Craswell, D. Hawking, and P. Thistlewaite, "Merging Results from Isolated Search Engines," in *Proc. 10th Australasian Database Conf. (ADC)*, pp. 189–200, 1999.

[8] M. Shokouhi and L. Si, "Federated Search," *Foundations and Trends in Information Retrieval*, vol. 5, no. 1, pp. 1–102, 2011. https://doi.org/10.1561/1500000010

### Query Expansion and Rewriting

[9] C. Carpineto and G. Romano, "A Survey of Automatic Query Expansion in Information Retrieval," *ACM Comput. Surv.*, vol. 44, no. 1, pp. 1–50, 2012. https://doi.org/10.1145/2071389.2071390

[10] W. Yu et al., "Generate Rather Than Retrieve: Large Language Models Are Strong Context Augmenters," in *Proc. 11th Int. Conf. Learn. Representations (ICLR)*, 2023. https://openreview.net/forum?id=ZuEW6ue886

### Re-Ranking

[11] R. Nogueira and K. Cho, "Passage Re-ranking with BERT," *arXiv preprint arXiv:1901.04085*, 2019. https://arxiv.org/abs/1901.04085

[12] Y. Zhuang, B. Liu, B. Koopman, and G. Zuccon, "SPLADE-v3: New Baselines for SPLADE," *arXiv preprint arXiv:2310.14276*, 2023. https://arxiv.org/abs/2310.14276

---

## See Also

- [README.md](README.md) - Current module documentation
- [Header Documentation](../../include/search/README.md) - Public API
- [Index Module](../index/FUTURE_ENHANCEMENTS.md) - Index improvements

---

*Last Updated: April 2026*
*Module Version: v1.7.0*
*Next Review: v1.8.0 Release*
