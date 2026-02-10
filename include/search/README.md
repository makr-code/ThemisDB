# Search Module - Public API

Public interface definitions for ThemisDB search functionality.

## Headers

### hybrid_search.h
**Purpose:** Hybrid search combining BM25 (full-text) and vector (semantic) search

**Key Classes:**
- `HybridSearch`: Main hybrid search engine with RRF
- `HybridSearch::Config`: Configuration for search behavior
- `HybridSearch::Result`: Search result with scores

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

// Create instance
HybridSearch search(fulltext_index, vector_index, config);

// Search
auto results = search.search("query text", vector, dim);

// Process results
for (const auto& r : results) {
    std::cout << "Doc: " << r.document_id 
              << " Score: " << r.hybrid_score << std::endl;
}
```

**Features:**
- Reciprocal Rank Fusion (RRF) for optimal result merging
- Configurable BM25/vector balance
- Score normalization
- Geospatial filtering support

---

## Core Types

### HybridSearch::Config
Configuration for hybrid search behavior.

**Fields:**
- `bm25_weight`: Weight for BM25 scores (0.0-1.0)
- `vector_weight`: Weight for vector scores (0.0-1.0)
- `k`: Final result count
- `k_bm25`: BM25 candidate count
- `k_vector`: Vector candidate count
- `use_rrf`: Use Reciprocal Rank Fusion (recommended)
- `rrf_k`: RRF constant (default: 60.0)
- `normalize_scores`: Normalize scores to [0,1]

### HybridSearch::Result
Single search result with scores.

**Fields:**
- `document_id`: Document identifier
- `bm25_score`: BM25 relevance score
- `vector_score`: Vector similarity score
- `hybrid_score`: Combined final score
- `bm25_rank`: Rank in BM25 results
- `vector_rank`: Rank in vector results
- `content`: Document content (optional)
- `geo_distance`: Geospatial distance (optional)

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
- All search operations are thread-safe
- Can be called concurrently from multiple threads
- Index managers must be thread-safe

---

## Examples

### Basic Hybrid Search
```cpp
HybridSearch::Config config;
config.use_rrf = true;
config.k = 10;

HybridSearch search(fulltext_idx, vector_idx, config);

auto results = search.search(
    "machine learning",
    query_vector,
    vector_dim
);
```

### BM25-Only Mode
```cpp
HybridSearch::Config config;
config.bm25_weight = 1.0;
config.vector_weight = 0.0;

HybridSearch search(fulltext_idx, vector_idx, config);
// Only uses BM25, vector search disabled
```

### With Geospatial Filter
```cpp
auto results = search.searchWithGeoFilter(
    "restaurants",
    query_vector,
    vector_dim,
    37.7749,  // lat
    -122.4194,  // lon
    5.0  // radius in km
);
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
*API Version: v1.3.0*
