> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Search Module - Public API

<!-- Status: current | validated: 2026-04-06 | commit: a14cdb2 -->
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

## Performance Characteristics

- **Latency:** 5-20ms for typical queries
- **Throughput:** 500-2K queries/second
- **Memory:** O(k) per query
- **Scalability:** Handles millions of documents

---

## See Also

- [Implementation Documentation](../../src/search/README.md)
- [Index Module](../index/README.md)
- [Future Enhancements](FUTURE_ENHANCEMENTS.md)

---

*Last Updated: April 2026*
*API Version: v2.3.0*

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
