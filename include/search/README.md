# Search Module - Public API

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

*Last Updated: February 2026*
*API Version: v1.4.0*
