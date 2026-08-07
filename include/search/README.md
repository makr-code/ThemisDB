> **Build:** `cmake --preset release && cmake --build build/release`

# Search Module - Public API

<!-- Status: current | validated: 2026-08-06 | commit: a14cdb2 -->
<!-- Primary: src/search/ | Secondary: docs/de/src/search/ -->

Public interface definitions for ThemisDB search functionality.

## Headers

### hybrid_search.h
**Purpose:** Hybrid search combining BM25 (full-text) and vector (semantic) search

**Key Classes:**
- `HybridSearch`: Main hybrid search engine with RRF
- `HybridSearch::Config`: Configuration for search behavior, resource limits, and vector metric
- `HybridSearch::Result`: Search result with individual and hybrid scores
- `HybridSearch::SearchStats`: Diagnostic information for partial-result detection

**Usage:**
```cpp
#include "search/hybrid_search.h"

using namespace themis;

// Configure
HybridSearch::Config config;
config.bm25_weight = 0.5;
config.vector_weight = 0.5;
config.use_rrf = true;
config.k = 10;
config.vector_metric = VectorIndexManager::Metric::COSINE;

// Create instance (constructor throws std::invalid_argument for invalid config)
HybridSearch search(fulltext_index, vector_index, config);

// Search (never throws; returns empty/partial results on backend error)
HybridSearch::SearchStats stats;
auto results = search.search("query text", vector, dim, &stats);

if (stats.partial_result) {
    // One backend failed – results are degraded but not empty
}

// Process results
for (const auto& r : results) {
    std::cout << "Doc: " << r.document_id
              << " Score: " << r.hybrid_score << std::endl;
}
```

**Features:**
- Reciprocal Rank Fusion (RRF) for optimal result merging
- Linear combination fallback with pre-normalization
- Configurable BM25/vector balance
- Consistent score normalization (including edge cases)
- Configurable vector distance metric (COSINE, DOT, L2)
- Hard resource limits to prevent unbounded memory / latency
- Graceful degradation with per-source diagnostic stats

---

## Core Types

### HybridSearch::Config
Configuration for hybrid search behaviour.

**Fields:**
- `bm25_weight`: Weight for BM25 scores (≥ 0.0; default 0.5)
- `vector_weight`: Weight for vector scores (≥ 0.0; default 0.5)
- `k`: Final result count (> 0; default 10)
- `k_bm25`: BM25 candidate count (≤ max_candidates; default 50)
- `k_vector`: Vector candidate count (≤ max_candidates; default 50)
- `use_rrf`: Use Reciprocal Rank Fusion — recommended (default true)
- `rrf_k`: RRF constant (> 0; default 60.0)
- `normalize_scores`: Normalize BM25/vector scores to [0,1] (default true)
- `max_k`: Hard upper bound for `k` (default 10,000)
- `max_candidates`: Hard upper bound for `k_bm25` and `k_vector` (default 10,000)
- `default_table`: Table name used for BM25 index lookup (non-empty; default "documents")
- `default_column`: Column name used for BM25 index lookup (non-empty; default "content")
- `vector_metric`: Distance metric for vector similarity (`COSINE` / `DOT` / `L2`; default `COSINE`)

The constructor throws `std::invalid_argument` if any of the constraints above are violated.

### HybridSearch::Result
Single search result with scores.

**Fields:**
- `document_id`: Document identifier
- `bm25_score`: BM25 relevance score (normalized to [0,1] when `normalize_scores` is true)
- `vector_score`: Vector similarity score (normalized to [0,1] when `normalize_scores` is true)
- `hybrid_score`: Combined final score (RRF or weighted linear combination)
- `bm25_rank`: Rank in BM25 results (-1 if not present in BM25 results)
- `vector_rank`: Rank in vector results (-1 if not present in vector results)
- `content`: Document content (optional)
- `geo_distance`: Geospatial distance (optional)

### HybridSearch::SearchStats
Diagnostic information about a `search()` call.

**Fields:**
- `bm25_ok`: True if the BM25 backend ran without error
- `vector_ok`: True if the vector backend ran without error
- `partial_result`: True when both backends were available but one failed while the other returned candidates
- `bm25_count`: Number of raw BM25 candidates before fusion
- `vector_count`: Number of raw vector candidates before fusion

---

### query_expander.h
**Purpose:** Query expansion, spelling correction, and zero-result fallback

**Key Classes / Structs:**
- `SpellingCorrection`: Ranked correction candidate (suggestion, edit_distance, confidence)
- `QueryExpander`: Expands a raw user query with synonyms, corrected tokens, and relaxed fallbacks
- `QueryExpander::Config`: Controls synonym expansion, spelling correction, max expansions
- `ExpandedQuery`: Output struct with original, corrected, synonyms, relaxed terms, and all_terms

**Usage:**
```cpp
#include "search/query_expander.h"

using namespace themis;

QueryExpander::Config cfg;
cfg.use_synonyms     = true;
cfg.correct_spelling = true;
cfg.max_expansions   = 5;
cfg.max_edit_distance = 2;

QueryExpander expander(cfg);
expander.addSynonyms("ml", {"machine learning", "artificial intelligence"});
expander.addVocabulary({"machine", "learning", "database", "index"});

auto expanded = expander.expand("mashine lerning");
// expanded.corrected  == "machine learning"
// expanded.synonyms   == {"artificial intelligence"}
// expanded.all_terms  contains all tokens + synonyms

// Ranked spelling corrections for a single word (new in v1.7.0)
// NOTE: "databse" and "qurey" are intentionally misspelled inputs to demonstrate correction
auto word_sug = expander.suggestSpellingCorrections("databse");
// [{suggestion="database", edit_distance=1, confidence=0.67}, ...]

// Ranked full-query corrections (new in v1.7.0)
auto query_sug = expander.suggestQueryCorrections("databse qurey");
// [{suggestion="database qurey", edit_distance=1, ...},
//  {suggestion="databse query",  edit_distance=2, ...},
//  {suggestion="database query", edit_distance=3, ...}]

// Suggest alternative phrasings via synonyms
auto alts = expander.suggestAlternatives("machine learning");

// Zero-result fallback: drop last token
auto relaxed = expander.relaxQuery("machine learning database");
// relaxed == "machine learning"
```

**Config Fields:**
- `use_synonyms`: Expand tokens with registered synonyms (default true)
- `correct_spelling`: Apply Levenshtein-based spelling correction against vocabulary (default true)
- `detect_phrases`: Preserve multi-word synonym phrases (default true)
- `synonym_weight`: Relative weight of synonym terms — informational (default 0.8)
- `max_expansions`: Maximum synonym terms to add per token (default 5)
- `max_edit_distance`: Maximum edit distance for spelling correction (default 2)

**SpellingCorrection Fields:**
- `suggestion`: Corrected word or full query string
- `edit_distance`: Levenshtein distance from input (lower is better)
- `confidence`: Normalized score in [0,1] — `1 - edit_distance / (max_edit_distance + 1)`

---

### fuzzy_matcher.h
**Purpose:** Enhanced fuzzy search with Levenshtein, Soundex, Metaphone, and N-gram algorithms

**Key Classes:**
- `FuzzyMatcher`: Wraps `SecondaryIndexManager::scanFulltextFuzzy` with algorithm selection and unified scoring
- `FuzzyMatcher::Config`: Algorithm choice, max distance, N-gram size, phonetic pre-filter
- `FuzzyMatch`: Single result with document_id, matched_token, score [0,1], and edit_distance

**Usage:**
```cpp
#include "search/fuzzy_matcher.h"

using namespace themis;

FuzzyMatcher::Config cfg;
cfg.algorithm    = FuzzyMatcher::Algorithm::LEVENSHTEIN;
cfg.max_distance = 2;

FuzzyMatcher matcher(&secondary_index_mgr, cfg);
auto [status, matches] = matcher.search("douments", "docs", "body");
for (const auto& m : matches) {
    std::cout << m.document_id << " score=" << m.score << "\n";
}

// Static algorithm utilities
int dist  = FuzzyMatcher::levenshtein("colour", "color");    // 1
auto sx   = FuzzyMatcher::soundex("Smith");                  // "S530"
auto mp   = FuzzyMatcher::metaphone("Knight");               // "NT"
double ng = FuzzyMatcher::ngramSimilarity("database", "databases"); // ~0.89
```

**Config Fields:**
- `algorithm`: `LEVENSHTEIN` / `SOUNDEX` / `METAPHONE` / `NGRAM` (default LEVENSHTEIN)
- `max_distance`: Maximum edit distance or minimum overlap threshold (default 2)
- `ngram_size`: N-gram size for NGRAM algorithm (default 2)
- `phonetic_prefilter`: Apply Soundex/Metaphone pre-filter before edit distance (default false)

---

### faceted_search.h
**Purpose:** Multi-dimensional facet computation for drill-down navigation

**Key Classes:**
- `FacetedSearch`: Computes per-field value counts and range-bucket facets
- `FacetResult`: Field name + `value_counts` map + total_docs
- `FacetedSearch::RangeBucket`: Labelled numeric range (low, high)
- `FacetedSearch::ActiveFacet`: A field=value drill-down constraint

**Usage:**
```cpp
#include "search/faceted_search.h"

using namespace themis;

FacetedSearch facets(&secondary_index_mgr);

// Collect PKs from search results
std::vector<std::string> pks = {"pk1", "pk2", "pk3"};

// Categorical facet
auto [st, brand_facet] = facets.computeFacet("products", "brand", pks);
for (const auto& [val, cnt] : brand_facet.value_counts) {
    std::cout << val << ": " << cnt << "\n";
}

// Multiple facets at once
auto [st2, all_facets] = facets.computeFacets("products", {"brand", "category"}, pks);

// Range facet (price buckets)
std::vector<FacetedSearch::RangeBucket> buckets = {
    {"$0-$50",   0,  50},
    {"$50-$200", 50, 200},
    {"$200+",   200, 1e9},
};
auto [st3, price_facet] = facets.computeRangeFacet("products", "price", buckets, pks);

// Apply filters (intersection)
std::vector<FacetedSearch::ActiveFacet> filters = {{"brand", "Acme"}};
auto [st4, filtered_pks] = facets.applyFacetFilters("products", pks, filters);
```

---

### search_analytics.h
**Purpose:** Thread-safe query log, performance metrics, and zero-result detection

**Key Classes:**
- `SearchAnalytics`: Thread-safe event log with configurable capacity (circular eviction)
- `SearchAnalytics::Config`: `max_events` capacity bound (default 10,000)
- `SearchEvent`: Recorded query event (query, timestamp, result_count, latency_ms, is_zero_result)
- `SearchMetrics`: Snapshot of aggregated stats (total/zero queries, avg/p95/p99 latency, top queries)

**Usage:**
```cpp
#include "search/search_analytics.h"

using namespace themis;

SearchAnalytics analytics;   // default max_events = 10,000

// Record from your search loop
auto t0 = std::chrono::steady_clock::now();
auto results = hs.search(query, vec.data(), vec.size());
double ms = std::chrono::duration<double, std::milli>(
    std::chrono::steady_clock::now() - t0).count();
analytics.record(query, results.size(), ms);

// Zero-result alerting
auto zero_queries = analytics.getZeroResultQueries(10);

// Aggregated metrics
SearchMetrics m = analytics.computeMetrics();
if (m.zero_result_rate > 0.1) {
    // Alert: more than 10% zero-result rate
}
std::cout << "p99 latency: " << m.p99_latency_ms << " ms\n";
```

**Notes:**
- **Thread-safe**: all methods protected by an internal `std::mutex`
- Bounded memory: oldest events are evicted when `max_events` is reached

---

### autocomplete.h
**Purpose:** Real-time query completion from index prefix scans and popular-query history

**Key Classes:**
- `AutocompleteEngine`: Combines prefix-index and popular-query suggestions
- `AutocompleteEngine::Config`: Suggestion count, prefix length, popular boost, deduplication
- `Suggestion`: text, relevance score, is_popular flag

**Usage:**
```cpp
#include "search/autocomplete.h"

using namespace themis;

AutocompleteEngine::Config cfg;
cfg.max_suggestions  = 10;
cfg.popular_boost    = 1.5;

AutocompleteEngine ac(&secondary_index_mgr, &analytics, cfg);

// Combined suggestions (prefix + popular)
auto suggestions = ac.suggest("data", "products", "name");

// Prefix-only (from index)
auto prefix_only = ac.suggestByPrefix("data", "products", "name", 20);

// Popular-only (from SearchAnalytics query history)
auto popular_only = ac.suggestPopular("data", 20);
```

**Config Fields:**
- `max_suggestions`: Maximum completions returned (default 10)
- `min_prefix_length`: Minimum prefix length to trigger completion (default 1)
- `popular_boost`: Score multiplier for popular-query suggestions (default 1.5)
- `include_popular`: Include popular-query suggestions (default true)
- `include_prefix`: Include prefix-index suggestions (default true)
- `deduplicate`: Remove duplicate suggestion texts (default true)

---

### learning_to_rank.h
**Purpose:** Linear feature-based re-ranker with click-through training and A/B variant selector

**Key Classes:**
- `LearningToRank`: Dot-product linear scorer, online gradient-descent training, A/B variants
- `LearningToRank::Config`: Learning rate, click buffer size, L2 regularization
- `RankingFeatures`: 6-dimensional feature vector (bm25, vector, rrf, recency, click_count, popularity)
- `RankedResult`: Candidate with features + final_score
- `ClickEvent`: Click-through event (query, document_id, result_position)
- `LearningToRank::Variant`: Named scoring function + traffic_fraction for A/B splits

**Usage:**
```cpp
#include "search/learning_to_rank.h"

using namespace themis;

LearningToRank::Config cfg;
cfg.learning_rate  = 0.01;
cfg.regularization = 0.001;
LearningToRank ltr(cfg);

// Build candidates from HybridSearch results
std::vector<RankedResult> candidates;
for (const auto& r : hs_results) {
    RankedResult rr;
    rr.document_id = r.document_id;
    rr.features.bm25_score   = r.bm25_score;
    rr.features.vector_score = r.vector_score;
    rr.features.rrf_score    = r.hybrid_score;
    candidates.push_back(rr);
}

// Re-rank using current weights
auto ranked = ltr.rerank(candidates);

// Record click and train
ltr.recordClick({"machine learning", "doc_42", 3});
size_t trained = ltr.train();

// A/B testing
ltr.registerVariant({"ltr_v2", my_scorer, 0.1});
auto variant = ltr.selectVariant(session_id);  // deterministic hash routing
auto ab_ranked = ltr.rerankWithVariant(candidates, variant);
```

**Config Fields:**
- `learning_rate`: Gradient-descent step size (default 0.01)
- `max_click_buffer`: Maximum stored click events before auto-eviction (default 1000)
- `regularization`: L2 regularization coefficient (default 0.001)

---

### multi_modal_search.h
**Purpose:** Unified search across text, image, audio, and arbitrary-embedding modalities with RRF fusion

**Key Classes:**
- `MultiModalSearch`: Dispatches TEXT queries to fulltext index and embedding queries to VectorIndexManager; fuses via RRF
- `MultiModalSearch::Config`: `k`, `rrf_k`, `candidates_per_modal`
- `ModalQuery`: Component query (modality, text/embedding, namespace, weight)
- `MultiModalResult`: document_id, fused score, matched_modality
- `Modality`: enum `TEXT` / `IMAGE` / `AUDIO` / `CUSTOM`

**Usage:**
```cpp
#include "search/multi_modal_search.h"

using namespace themis;

MultiModalSearch::Config cfg;
cfg.k = 10;
MultiModalSearch mms(&sec_index_mgr, &vec_index_mgr, cfg);

// Text + image query
std::vector<ModalQuery> queries = {
    { Modality::TEXT,  "sunset beach", {},         "text_ns",  0.6 },
    { Modality::IMAGE, "",    clip_embedding, "image_ns", 1.0 },
};
auto results = mms.search(queries, "photos", "caption");

// Convenience: single text + single image
auto results2 = mms.searchTextAndImage(
    "sunset over mountains",
    clip_embedding,
    "image_ns",
    "photos", "caption"
);
```

**Config Fields:**
- `k`: Number of fused results to return (default 10)
- `rrf_k`: RRF smoothing constant (default 60.0)
- `candidates_per_modal`: How many candidates to fetch per modality before fusion (default 100)

---

### multi_field_search.h
**Purpose:** Multi-field boosted full-text search that ranks documents by combining BM25 scores
across several fields (e.g. title, body, tags) with per-field boost weights.

**Key Classes:**
- `MultiFieldBoostedSearch`: Executes per-field BM25 queries, normalizes scores, applies boosts, returns top-k
- `MultiFieldBoostedSearch::Config`: `k`, `candidates_per_field`
- `MultiFieldBoostedSearch::FieldConfig`: `table`, `column`, `boost`
- `MultiFieldBoostedSearch::Result`: `document_id`, combined `score`, per-field `field_scores`

**Usage:**
```cpp
#include "search/multi_field_search.h"

using namespace themis;

MultiFieldBoostedSearch::Config cfg;
cfg.k = 10;
MultiFieldBoostedSearch mfs(&sec_index_mgr, cfg);

// Use the canonical title / body / tags preset (boosts 3.0 / 1.0 / 0.5)
auto fields = MultiFieldBoostedSearch::defaultFields("articles");
auto results = mfs.search("database engine", fields);

// Or specify custom fields
std::vector<MultiFieldBoostedSearch::FieldConfig> custom = {
    {"posts", "title",   3.0},
    {"posts", "summary", 2.0},
    {"posts", "body",    1.0},
};
auto results2 = mfs.search("open source", custom);

for (const auto& r : results) {
    std::cout << r.document_id << " score=" << r.score << "\n";
}
```

**Config Fields:**
- `k`: Maximum number of results to return (default 10)
- `candidates_per_field`: BM25 candidates fetched per field before score combination (default 100)

**Score Combination:**
```
score(doc) = Σ_f( boost_f × normalized_bm25_f(doc) )
```
where `normalized_bm25_f` is the per-field BM25 score linearly rescaled to [0, 1].

**Notes:**
- `normalizeScores()` is a public static method for direct unit testing.
- Fields with negative boost are skipped with a warning; fields with boost = 0.0 contribute 0 to the score.
- `search()` never throws; all index exceptions are caught and logged.

---

### personalized_ranker.h
**Purpose:** Per-user interaction history tracking with time-decayed personalization scoring for search result re-ranking

**Key Classes:**
- `PersonalizedRanker`: Records user interactions and computes personalization boosts for ranked candidates
- `PersonalizedRanker::Config`: `decay_rate`, `max_interactions_per_user`, `boost_weight`
- `UserInteraction`: `user_id`, `document_id`, `type` (InteractionType), `timestamp`
- `InteractionType`: `VIEW` (0.2), `CLICK` (0.5), `BOOKMARK` (1.0), `LIKE` (1.0), `DISLIKE` (-0.5)

**Usage:**
```cpp
#include "search/personalized_ranker.h"

using namespace themis;

PersonalizedRanker::Config cfg;
cfg.decay_rate   = 0.05;   // half-weight after ~14 days
cfg.boost_weight = 0.2;    // how much to shift final_score
PersonalizedRanker pr(cfg);

// Record interactions as users browse (e.g. from click/session logs)
pr.recordInteraction({"alice", "doc_42", InteractionType::LIKE,
                       std::chrono::system_clock::now()});
pr.recordInteraction({"alice", "doc_7",  InteractionType::DISLIKE,
                       std::chrono::system_clock::now()});

// After LTR re-ranking, apply user-specific personalization
auto ranked = ltr.rerank(candidates);
pr.applyPersonalization("alice", ranked);  // re-sorts by personalized final_score

// Query personalization score for a single document
double score = pr.computeScore("alice", "doc_42");  // returns value in [-1, 1]

// GDPR: remove all data for a user
pr.clearUser("alice");
```

**Config Fields:**
- `decay_rate`: Exponential decay rate per day (default 0.05; 0 = no decay)
- `max_interactions_per_user`: Maximum stored interactions per user, oldest evicted (default 500)
- `boost_weight`: Multiplier applied to the [-1,1] personalization score when adjusting `final_score` (default 0.2)

**Score Model:**
```
personalization_score = clamp(Σ type_weight * exp(-decay_rate * age_days), -1, 1)
final_score += boost_weight * personalization_score
```
### cross_lingual_search.h
**Purpose:** Cross-lingual semantic search using multilingual embeddings to retrieve documents
across language boundaries in a shared vector space.

**Key Classes:**
- `CrossLingualSearch`: Issues kNN queries on multilingual embeddings; optionally fuses multiple
  language-variant queries via RRF; applies per-language boost factors; annotates results with
  language metadata
- `CrossLingualSearch::Config`: `k`, `candidates`, `score_threshold`, `rrf_k`, `max_k`, `max_candidates`
- `CrossLingualSearch::LanguageHint`: `language_code`, `boost`
- `CrossLingualSearch::EmbeddingQuery`: `embedding`, `weight`
- `CrossLingualSearch::Result`: `document_id`, `score`, `language`

**Usage:**
```cpp
#include "search/cross_lingual_search.h"

using namespace themis;

CrossLingualSearch::Config cfg;
cfg.k = 10;
cfg.score_threshold = 0.3;  // optional: filter out low-confidence results
CrossLingualSearch cls(&vec_index_mgr, cfg);

// Optional: annotate results with per-document language information
cls.setLanguageMap({{"doc1", "en"}, {"doc2", "de"}, {"doc3", "fr"}});

// Single-embedding search (e.g. paraphrase-multilingual-mpnet-base-v2 output)
std::vector<CrossLingualSearch::LanguageHint> hints = {
    {"en", 1.2},  // slight preference for English results
};
auto results = cls.search(query_embedding, hints);

// Multi-embedding fusion across language variants
CrossLingualSearch::EmbeddingQuery qEn{en_embedding, 1.0};
CrossLingualSearch::EmbeddingQuery qDe{de_embedding, 0.8};
auto results2 = cls.searchMultiEmbedding({qEn, qDe}, hints);

for (const auto& r : results) {
    std::cout << r.document_id
              << " score=" << r.score
              << " lang="  << r.language << "\n";
}
```

**Config Fields:**
- `k`: Maximum results to return (default 10)
- `candidates`: kNN candidates retrieved per query before filtering (default 100)
- `score_threshold`: Minimum similarity score in [0, 1] (default 0.0)
- `rrf_k`: RRF smoothing constant for multi-embedding fusion (default 60.0)
- `max_k` / `max_candidates`: Hard resource limits; `k` and `candidates` are clamped at construction

**Notes:**
- Model-agnostic: callers supply pre-computed float vectors from any multilingual embedding model.
- `search()` and `searchMultiEmbedding()` never throw; index exceptions are caught and logged.
- `setLanguageMap()` populates `Result::language` and enables `LanguageHint` boost lookup.
- RRF formula per list i: `score(doc) += weight_i / (rrf_k + rank_i(doc))`

---

### distributed_hybrid_search.h
**Purpose:** Distributed hybrid search across multiple ThemisDB shards with cross-shard
Reciprocal Rank Fusion (RRF) result merging and mTLS-secured inter-node communication.

**Key Classes:**
- `DistributedHybridSearch`: Distributes hybrid search to all healthy shards in parallel and
  merges results via cross-shard RRF
- `DistributedHybridSearch::Config`: `k`, `rrf_k`, `shard_timeout_ms`, `max_concurrent_shards`,
  `skip_failed_shards`, `local_shard_id`, `search_endpoint`
- `DistributedHybridSearch::ShardSearchResult`: Per-shard outcome with results, success flag,
  error message, and execution time
- `DistributedHybridSearch::SearchStats`: Diagnostics — `shards_queried`, `shards_succeeded`,
  `shards_failed`, `partial_result`

**Usage:**
```cpp
#include "search/distributed_hybrid_search.h"
#include "sharding/remote_executor.h"
#include "sharding/urn_resolver.h"

using namespace themis;

// 1. Configure mTLS-secured remote executor
sharding::RemoteExecutor::Config exec_cfg;
exec_cfg.cert_path    = "/etc/themis/tls/shard.crt";
exec_cfg.key_path     = "/etc/themis/tls/shard.key";
exec_cfg.ca_cert_path = "/etc/themis/tls/ca.crt";
auto executor = std::make_shared<sharding::RemoteExecutor>(exec_cfg);

// 2. Create distributed search engine
DistributedHybridSearch::Config dhs_cfg;
dhs_cfg.k              = 20;
dhs_cfg.local_shard_id = "shard_001";
// search_endpoint defaults to "/search/hybrid" (HTTP server route)

DistributedHybridSearch dhs(
    &local_hybrid_search,  // local HybridSearch instance (may be nullptr)
    resolver.get(),        // URNResolver for enumerating healthy shards
    executor.get(),        // RemoteExecutor (mTLS-configured)
    dhs_cfg
);

// 3. Search (never throws)
DistributedHybridSearch::SearchStats stats;
auto results = dhs.search("machine learning", query_embedding, &stats);

if (stats.partial_result) {
    // At least one shard was unavailable; results come from surviving shards only
    THEMIS_WARN("{} of {} shards failed", stats.shards_failed, stats.shards_queried);
}

for (const auto& r : results) {
    std::cout << r.document_id << " score=" << r.hybrid_score << "\n";
}
```

**Config Fields:**
- `k`: Maximum globally merged results to return (default 10)
- `rrf_k`: RRF smoothing constant for cross-shard fusion (default 60.0)
- `shard_timeout_ms`: Per-shard HTTP request timeout (default 5000)
- `max_concurrent_shards`: Maximum shards queried concurrently per batch (default 10)
- `skip_failed_shards`: When true (default), failed shards are silently skipped
- `local_shard_id`: This node's shard ID (used to avoid double-querying the local shard)
- `search_endpoint`: HTTP POST endpoint on each shard (default `"/search/hybrid"`)

**Notes:**
- `search()` never throws; all network and parsing errors are caught internally.
- Remote shards receive: `POST /search/hybrid` with `{"query", "k", "vector_query"}`.
- `mergeShardResults()` is public for direct unit testing without network infrastructure.
- `SearchStats::partial_result` is true when at least one shard succeeded and at least one failed.
- When `skip_failed_shards = false`, any shard failure causes `search()` to return `{}`.
- Requires the remote shards to expose `POST /search/hybrid` (matches `HttpServer::handleHybridSearch`).

---

## Phase 5 Components

### conversational_search.h
**Purpose:** Multi-turn conversational search with context-aware query reformulation. Maintains a per-session conversation history and enriches each new query with the most recent turns.

**Key Classes:**
- `ConversationalSearch`: Manages session history and context-enriched search
- `ConversationalSearch::Config`: `context_window`, `max_history`, `context_separator`
- `ConversationalSearch::Turn`: `query`, `reformulated_query`, `results`

**Usage:**
```cpp
#include "search/conversational_search.h"

using namespace themis;

ConversationalSearch::Config cfg;
cfg.context_window = 3;  // include last 3 turns in context
ConversationalSearch cs(&hybrid_search, cfg);

// Turn 1
auto results1 = cs.search("machine learning");

// Turn 2 — query is enriched with prior context
auto results2 = cs.search("what about overfitting?");

// Inspect history
for (const auto& turn : cs.getHistory()) {
    std::cout << turn.query
              << " (reformulated: " << turn.reformulated_query << ")"
              << " -> " << turn.results.size() << " results\n";
}

// Reset session (e.g. for a new user session)
cs.clearHistory();
```

**Config Fields:**
- `context_window`: Number of past query turns appended to the new query (default 3; 0 = stateless per-turn search)
- `max_history`: Maximum turns retained; oldest evicted when limit is reached (default 50)
- `context_separator`: String inserted between historical terms in the reformulated query (default `" "`)

**Notes:**
- `search()` and `reformulate()` never throw; the constructor throws `std::invalid_argument` on invalid config.
- Not thread-safe; create a separate instance per session or thread.

**@since v2.4.0**

---

### federated_search.h
**Purpose:** Federated search across isolated per-tenant `HybridSearch` indexes. Executes a single query concurrently across registered tenants and merges results via cross-tenant Reciprocal Rank Fusion (RRF).

**Key Classes:**
- `FederatedSearch`: Cross-tenant search orchestrator with per-tenant isolation and weighting
- `FederatedSearch::Config`: `k`, `rrf_k`, `skip_null_tenants`
- `FederatedSearch::Result`: `document_id`, `tenant_id`, `score`, `bm25_score`, `vector_score`
- `FederatedSearch::TenantStats`: Per-tenant diagnostics (`tenant_id`, `results_count`, `skipped`)

**Usage:**
```cpp
#include "search/federated_search.h"

using namespace themis;

FederatedSearch::Config cfg;
cfg.k = 20;
FederatedSearch fs(cfg);

fs.registerTenant("tenant_A", &hs_a);
fs.registerTenant("tenant_B", &hs_b);
fs.setTenantWeight("tenant_A", 0.8);  // slightly downweight tenant A

std::vector<FederatedSearch::TenantStats> stats;
auto results = fs.search("machine learning", {}, &stats);

for (const auto& r : results) {
    std::cout << r.tenant_id << "/" << r.document_id
              << " score=" << r.score << "\n";
}
```

**Config Fields:**
- `k`: Final merged result count across all tenants (default 10)
- `rrf_k`: RRF smoothing constant for cross-tenant fusion (default 60.0)
- `skip_null_tenants`: Silently skip tenants with null `HybridSearch` pointers (default true)

**Notes:**
- A weight of 0.0 excludes the tenant from the merged results. Weights are clamped to [0.0, 1.0].
- `search()` never throws; per-tenant exceptions are caught internally.
- Results from each tenant are fully isolated: data from one tenant cannot influence another's scores.

**@since v2.4.0**

---

### llm_query_rewriter.h
**Purpose:** LLM-based query rewriting that generates semantically equivalent alternative phrasings to broaden search recall beyond synonym dictionary coverage.

**Key Types:**
- `RewriteQuality`: `OK` (LLM rewrites accepted) / `FALLBACK` (all rewrites discarded; original used)
- `RewrittenQuery`: `original`, `rewrites`, `llm_used`, `quality`
- `LlmQueryRewriter`: Generates alternative queries via an injected `LlmBackend` callable
- `LlmQueryRewriter::Config`: `num_rewrites`, `max_tokens`, `temperature`, `fallback_to_original`, `max_rewrite_length`, `min_token_overlap_ratio`

**Usage:**
```cpp
#include "search/llm_query_rewriter.h"

using namespace themis;

LlmQueryRewriter::Config cfg;
cfg.num_rewrites = 3;
cfg.min_token_overlap_ratio = 0.2f;  // discard semantically nonsensical rewrites

LlmQueryRewriter rewriter(cfg, [&](const std::string& prompt) {
    return my_llm.generate(prompt, 256);
});

auto result = rewriter.rewrite("fast db insert");
// result.rewrites might contain:
//   "high-throughput database insertion"
//   "quick record insertion in a database"

if (result.quality == RewriteQuality::FALLBACK) {
    // LLM output was unusable; original query is the sole entry in rewrites
}

// Swap backend at runtime (e.g. after model hot-reload)
rewriter.setBackend(new_backend);
```

**Config Fields:**
- `num_rewrites`: Alternative rewrites to request from the LLM (default 3; must be > 0)
- `max_tokens`: Max token hint for the LLM backend (default 256; enforcement is backend-specific)
- `temperature`: Sampling temperature hint (default 0.7; enforcement is backend-specific)
- `fallback_to_original`: Append original query when no usable LLM output is produced (default true)
- `max_rewrite_length`: Character budget per rewrite; longer strings are dropped (default 256)
- `min_token_overlap_ratio`: Minimum Jaccard token-overlap between a rewrite and the original query (Jaccard = |A∩B| / |A∪B| where A and B are whitespace-token sets); rewrites below this threshold are discarded (default 0.2; set 0.0 to disable)

---

### llm_reranker.h
**Purpose:** Configurable LLM-based re-ranker that scores query-document pairs (0–10), blends the LLM scores with upstream retrieval scores, and optionally converts relevance judgments into `ClickEvent` objects for closed-loop LTR training.

**Key Types:**
- `LlmRerankCandidate`: `document_id`, `content` (snippet), `initial_score`
- `LlmRerankResult`: `document_id`, `llm_score` (0–1), `initial_score`, `final_score`, `llm_scored`
- `LlmReranker`: Batched LLM-based re-ranker with configurable score blending
- `LlmReranker::Config`: `batch_size`, `llm_weight`, `max_snippet_length`, `fallback_to_original`, `max_tokens`, `temperature`, `min_score_threshold`

**Usage:**
```cpp
#include "search/llm_reranker.h"

using namespace themis;

LlmReranker::Config cfg;
cfg.llm_weight = 0.7;  // final = 0.7 * llm_score + 0.3 * initial_score
LlmReranker reranker(cfg, [](const std::string& prompt) {
    return my_model.generate(prompt);
});

std::vector<LlmRerankCandidate> candidates;
for (const auto& r : hs_results) {
    candidates.push_back({r.document_id, r.content, r.hybrid_score});
}
auto reranked = reranker.rerank("fast db insert", candidates);

// Optionally close the LTR feedback loop
auto clicks = LlmReranker::toClickEvents("fast db insert", reranked, 0.5);
for (const auto& ev : clicks) ltr.recordClick(ev);
ltr.train();
```

**Config Fields:**
- `batch_size`: Candidates per LLM prompt batch (default 5)
- `llm_weight`: Blending weight for LLM score; `final = llm_weight * llm_score + (1 - llm_weight) * initial_score` (default 0.7)
- `max_snippet_length`: Max characters of each document snippet included in the prompt (default 200)
- `fallback_to_original`: Return candidates in original order when the LLM fails (default true)
- `max_tokens`: Max token hint for the LLM response (default 256)
- `temperature`: Sampling temperature hint; 0.0 omits the hint from the prompt (default 0.0)
- `min_score_threshold`: Minimum `final_score` for inclusion in output; 0.0 returns all candidates (default 0.0)

---

### negative_keyword_filter.h
**Purpose:** NOT-operator negative keyword filtering. Parses `-term` / `NOT term` syntax from the raw query, runs the positive part through BM25/hybrid search, then removes any result document that contains an excluded term.

**Key Classes:**
- `NegativeKeywordFilter`: Stateless filter; holds only a non-owning pointer to the secondary index
- `NegativeKeywordFilter::Config`: `max_exclude_scan`
- `NegativeKeywordFilter::ParsedQuery`: `positive_query`, `negative_terms`

**Supported query syntax:**

| Syntax | Example | Meaning |
|---|---|---|
| Minus prefix (`-`) | `"machine learning -neural"` | Exclude "neural" |
| `NOT` keyword | `"machine learning NOT neural"` | Equivalent to minus prefix |
| Mixed | `"database -slow NOT crash"` | Exclude "slow" and "crash" |

**Usage:**
```cpp
#include "search/negative_keyword_filter.h"

using namespace themis;

// 1. Parse negative terms from raw user query
auto pq = NegativeKeywordFilter::parseQuery("machine learning -neural");
// pq.positive_query == "machine learning"
// pq.negative_terms == {"neural"}

// 2. Run search on positive query
auto results = hybrid_search.search(pq.positive_query, vec.data(), vec.size());

// 3. Filter by NOT terms
std::vector<std::string> pks;
for (auto& r : results) pks.push_back(r.document_id);

NegativeKeywordFilter nkf(&sec_index);
auto [status, filtered_pks] = nkf.filter(
    "documents", "content", pks, pq.negative_terms);

// 4. Retain only results that survived the filter
std::unordered_set<std::string> keep(filtered_pks.begin(), filtered_pks.end());
results.erase(
    std::remove_if(results.begin(), results.end(),
        [&keep](const auto& r){ return keep.find(r.document_id) == keep.end(); }),
    results.end());
```

**Config Fields:**
- `max_exclude_scan`: Maximum documents fetched per negative term; prevents unbounded memory use (default 100,000; set to 0 to disable the limit — only in controlled environments or with small corpora, as very common negative terms can exhaust memory)

**Notes:**
- `parseQuery()` is a pure static function and is fully thread-safe.
- `filter()` never throws. When the index pointer is null, it returns an error Status and the original PKs unchanged.

---

### neural_sparse_retrieval.h
**Purpose:** SPLADE / BERT-based neural sparse retrieval. Documents and queries are encoded into sparse term-weight vectors; retrieval uses inverted-index dot-product accumulation for efficiency.

**Key Types:**
- `SparseVector`: `std::unordered_map<std::string, float>` — sparse term-to-weight representation
- `SparseEncoderBackend`: `std::function<SparseVector(const std::string&)>` — injected text encoder
- `NeuralSparseRetrieval`: In-memory sparse retrieval engine with indexing and search
- `NeuralSparseRetrieval::Config`: `k`, `max_terms_per_doc`, `score_threshold`, `normalize_scores`
- `NeuralSparseRetrieval::Result`: `document_id`, `score` (normalised), `raw_score`

**Usage:**
```cpp
#include "search/neural_sparse_retrieval.h"

using namespace themis;

NeuralSparseRetrieval::Config cfg;
cfg.k = 10;
NeuralSparseRetrieval nsr(cfg);

// Attach a SPLADE-compatible encoder
nsr.setEncoder([&](const std::string& text) {
    return my_splade_model.encode(text);
});

// Index documents
nsr.addDocumentText("doc1", "fast in-memory database engine");
nsr.addDocumentText("doc2", "neural sparse retrieval with SPLADE");

// Query via text (encoder called internally)
auto results = nsr.searchText("database performance");
for (auto& r : results)
    std::cout << r.document_id << "  score=" << r.score << "\n";

// Or supply pre-computed sparse vectors directly
SparseVector qvec = {{"database", 1.2f}, {"performance", 0.8f}};
auto results2 = nsr.search(qvec);
```

**Config Fields:**
- `k`: Maximum results to return (default 10)
- `max_terms_per_doc`: Soft cap on terms per document sparse vector; only top-weighted terms are kept (default 512)
- `score_threshold`: Minimum raw inner-product score for inclusion (default 0.0)
- `normalize_scores`: Rescale result scores to [0, 1] before returning (default true)

**Notes:**
- Scoring model: `score(q, d) = Σ_t( q[t] * d[t] )` — standard sparse dot product, matching SPLADE/uniCOIL.
- `search()` and `searchText()` never throw; encoder errors are caught and result in an empty return.
- `addDocumentText()` propagates exceptions from the encoder backend.

---

### search_highlighter.h
**Purpose:** Stateless search result highlighter and best-passage snippet extractor. Wraps matched terms in configurable HTML tags and extracts the highest-coverage passage from a document.

**Key Classes:**
- `SearchHighlighter`: Stateless; all methods safe to call concurrently
- `SearchHighlighter::Config`: `highlight_open`, `highlight_close`, `ellipsis`, `min_window`, `max_snippet_len`, `case_insensitive`

**Usage:**
```cpp
#include "search/search_highlighter.h"

using namespace themis;

SearchHighlighter::Config cfg;
cfg.highlight_open  = "<mark>";
cfg.highlight_close = "</mark>";
cfg.max_snippet_len = 300;
SearchHighlighter hl(cfg);

std::string doc = "ThemisDB provides fast hybrid search combining BM25 and vector search.";
std::vector<std::string> terms = {"hybrid", "search"};

// Wrap all matches in <mark> tags
std::string marked = hl.highlight(doc, terms);
// -> "ThemisDB provides fast <mark>hybrid</mark> <mark>search</mark> ..."

// Extract the best-matching passage
std::string snip = hl.snippet(doc, terms, 100);
// -> "...fast <mark>hybrid</mark> <mark>search</mark> combining BM25..."

// Static helpers (exposed for pipeline reuse)
auto tokens       = SearchHighlighter::tokenize(doc, true);
size_t best_start = SearchHighlighter::bestWindowOffset(doc, tokens, 100);
```

**Config Fields:**
- `highlight_open`: Opening HTML tag around matched terms (default `<mark>`)
- `highlight_close`: Closing HTML tag around matched terms (default `</mark>`)
- `ellipsis`: Boundary marker inserted at snippet edges (default `...`)
- `min_window`: Characters on each side of a match in the snippet (default 40)
- `max_snippet_len`: Maximum returned snippet length in characters (default 300)
- `case_insensitive`: Case-insensitive term matching (default true)

**Static helpers:**
- `tokenize(text, case_insensitive)` — split text into lowercase tokens
- `applyHighlight(text, offsets, open_tag, close_tag)` — insert tags at given byte ranges
- `bestWindowOffset(text, terms, window_size)` — find the start offset maximising term coverage

**Notes:**
- `highlight()` and `snippet()` are `noexcept`; invalid inputs produce empty output.
- Overlapping matches are merged before tagging to prevent nested tags.

---

### search_result_stream.h
**Purpose:** Cursor-based streaming pagination over a `HybridSearch` result set. Avoids materialising all results at once by delivering them one page at a time via `nextPage()` or a callback via `forEachResult()`.

**Key Classes:**
- `SearchResultStream`: Stateful streaming wrapper around `HybridSearch`
- `SearchResultStream::Config`: `total_k`, `page_size`
- `SearchResultStream::ResultCallback`: `std::function<bool(const HybridSearch::Result&)>` — return `false` to stop early

**Usage:**
```cpp
#include "search/search_result_stream.h"

using namespace themis;

SearchResultStream::Config cfg;
cfg.total_k   = 10000;  // materialise up to 10k results internally
cfg.page_size = 100;    // deliver 100 at a time
SearchResultStream stream(&hybrid_search, cfg);

// Page-based iteration
stream.open("machine learning");
while (stream.hasMore()) {
    auto page = stream.nextPage();
    for (const auto& r : page) { process(r); }
}

// Callback streaming with early termination
stream.reset();  // rewind without re-querying
stream.forEachResult([](const HybridSearch::Result& r) -> bool {
    process(r);
    return true;  // return false to stop early
});

// Release buffered results
stream.close();
```

**Config Fields:**
- `total_k`: Total maximum results to materialise from `HybridSearch` (default 1,000)
- `page_size`: Results per `nextPage()` call (default 100)

**Stream lifecycle:** `open(query)` → `nextPage()` / `forEachResult()` → `reset()` (rewind) → `close()` (release).

**Notes:**
- `open()` and `nextPage()` never throw; the constructor throws `std::invalid_argument` on invalid config.
- Not thread-safe; concurrent calls must be serialised externally.

---

## Integration Points

### With Index Module
```cpp
#include "search/hybrid_search.h"
#include "index/secondary_index.h"
#include "index/vector_index.h"

// Create indexes
SecondaryIndexManager fulltext(db);
VectorIndexManager vectors(db);

// Create hybrid search
HybridSearch search(&fulltext, &vectors, config);
```

---

## API Conventions

### Namespace
```cpp
namespace themis {
    class HybridSearch { /* ... */ };
}
```

### Thread Safety
A single `HybridSearch` instance is **not thread-safe**. `search()` and `setConfig()`
must not be called concurrently on the same instance. The recommended pattern is to
create one `HybridSearch` instance per thread, since the class is lightweight (it holds
only a `Config` and two non-owning index pointers).

### Exception Safety
- The constructor offers **strong exception safety**: it throws `std::invalid_argument`
  for an invalid `Config`, and the object is never partially constructed.
- `search()` is **unconditionally noexcept at runtime**: all exceptions from the index
  backends and from the fusion stage are caught internally, logged, and an empty or
  partial result vector is returned rather than propagating the exception.

---

## Examples

### Basic Hybrid Search
```cpp
HybridSearch::Config config;
config.use_rrf = true;
config.k = 10;
config.vector_metric = VectorIndexManager::Metric::COSINE;

HybridSearch search(fulltext_idx, vector_idx, config);

auto results = search.search(
    "machine learning",
    query_vector,
    vector_dim
);
```

### Hybrid Search with Diagnostics
```cpp
HybridSearch::SearchStats stats;
auto results = search.search(
    "machine learning",
    query_vector,
    vector_dim,
    &stats
);

if (stats.partial_result) {
    // Log or alert: one backend failed
    log.warn("Partial search result: bm25_ok={} vector_ok={}",
             stats.bm25_ok, stats.vector_ok);
}
```

### BM25-Only Mode
```cpp
HybridSearch::Config config;
config.bm25_weight = 1.0;
config.vector_weight = 0.0;

HybridSearch search(fulltext_idx, nullptr, config);
// Only uses BM25; pass nullptr for vector_index to skip vector search
```

### L2 Vector Metric
```cpp
HybridSearch::Config config;
config.vector_metric = VectorIndexManager::Metric::L2;
HybridSearch search(fulltext_idx, vector_idx, config);
```

---

## Troubleshooting

### `std::invalid_argument` thrown on construction

All search engine constructors validate their `Config` at construction time and throw
`std::invalid_argument` for invalid values.  Common causes:

| Class | Typical cause |
|---|---|
| `HybridSearch` | `k == 0`, `rrf_k <= 0`, `default_table` or `default_column` is empty, `k > max_k` (hard upper bound, default 10,000), or `k_bm25 / k_vector > max_candidates` (default 10,000); see `HybridSearch::Config` above |
| `ConversationalSearch` | Reserved for future validation (currently all default configs are valid) |
| `FederatedSearch` | Reserved for future validation |
| `LlmQueryRewriter` | `num_rewrites == 0` |
| `LlmReranker` | `batch_size == 0` or `llm_weight` outside [0, 1] |
| `NeuralSparseRetrieval` | `k == 0`, `max_terms_per_doc == 0`, or `score_threshold < 0` |
| `SearchResultStream` | `page_size == 0` or `total_k == 0` |

### Empty results from `HybridSearch::search()`

1. **Partial result** — check `SearchStats::partial_result`. One backend may have failed; inspect logs for `THEMIS_ERROR` messages.
2. **Config weights are zero** — ensure at least one of `bm25_weight` or `vector_weight` is > 0.
3. **BM25 index not built** — call `SecondaryIndexManager::createIndex()` with `FULLTEXT` type before the first search. See [Index Module documentation](../index/README.md) for details.
4. **Vector index empty** — ensure documents have been upserted with embedding vectors before querying.

### Low recall / poor result quality

1. **Unbalanced RRF weights** — start with `bm25_weight = 0.5` and `vector_weight = 0.5`; adjust based on evaluation.
2. **Embedding model mismatch** — query and document vectors must be produced by the same model.
3. **Small `k_bm25` / `k_vector`** — increasing candidate counts improves recall at the cost of higher latency.
4. **Missing query expansion** — attach a `QueryExpander` to handle synonyms and spelling errors.

### LLM components return fallback / original results

- `LlmQueryRewriter` and `LlmReranker` silently fall back to the original order/query when no backend is set, or when the backend throws.
- Verify the backend is set via `setBackend()` and that the LLM service is reachable.
- Inspect `RewrittenQuery::quality == RewriteQuality::FALLBACK` or `LlmRerankResult::llm_scored == false`.

### `NegativeKeywordFilter::filter()` returns error Status

- The index pointer passed to the constructor is null.  Pass a valid `SecondaryIndexManager*`.
- The `table` / `column` combination has not been indexed.  Ensure a FULLTEXT index exists.

### `SearchResultStream` delivers no pages after `open()`

- Check that the underlying `HybridSearch` instance is valid and its index is populated.
- Verify `Config::total_k > 0` and `Config::page_size > 0`.
- Call `hasMore()` before each `nextPage()`; after the last page `hasMore()` returns false.

### Thread-safety violations

All search classes in this module are **not thread-safe** by default.  If you observe data
races or crashes in multi-threaded code:
- Create one instance per thread (recommended for `HybridSearch` — it is lightweight).
- Or protect shared instances with an external `std::mutex` around every call.

---

## Performance Characteristics

- **Latency:** 5-20ms for typical queries
- **Throughput:** 500-2K queries/second
- **Memory:** O(k) per query
- **Scalability:** Handles millions of documents

---

## See Also

- [Implementation Documentation](../../src/search/README.md)
- [Index Module](../index/README.md)
- [Future Enhancements](../../src/search/FUTURE_ENHANCEMENTS.md)

---

*Last Updated: May 2026*
*API Version: v2.4.0*

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
