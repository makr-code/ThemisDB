# Vector Advanced Features

This directory contains advanced vector search capabilities for ThemisDB. These modules extend the basic vector similarity search with sophisticated algorithms for radius-based search and multi-vector queries.

## Status: GAP-006 Implementation (Stub Phase)

**Current Version:** Stub/Placeholder Implementation  
**Implementation Date:** February 2026  
**Status:** Interface definitions complete, full implementations planned for future releases

These modules provide complete interface definitions and documentation but return `NOT_IMPLEMENTED` errors. They serve as:
- Clear interface contracts for future implementations
- Documentation of planned features
- Foundation for incremental development
- API stability for dependent code

## Modules Overview

### 1. Approximate Radius Search (`approximate_radius_search.h/cpp`)

Efficient search for all vectors within a distance threshold.

**Features:**
- Approximate radius-based search (faster than exact)
- Multiple distance metrics (L2, Cosine, Dot Product)
- Max results limiting
- Quality guarantees (recall threshold)
- Batch processing support
- Dynamic radius adjustment

**Example Usage:**
```cpp
#include "index/approximate_radius_search.h"

using namespace themis::vector;

ApproximateRadiusSearch radius_search(vector_manager);

ApproximateRadiusSearch::SearchConfig config;
config.radius = 0.3f;              // Distance threshold
config.metric = Metric::COSINE;
config.max_results = 1000;
config.min_recall = 0.95f;         // 95% recall guarantee

// Future: Full implementation will enable
// auto result = radius_search.search(query_vector, config);
// for (const auto& item : result.value().results) {
//     std::cout << item.id << ": " << item.distance << std::endl;
// }
```

**Use Cases:**
- Finding all similar documents within a similarity threshold
- Duplicate detection with distance threshold
- Clustering pre-processing (find neighbors)
- Anomaly detection (find items with few neighbors)
- Local density estimation

**vs. k-NN Search:**
| Feature | k-NN | Radius Search |
|---------|------|---------------|
| Result count | Fixed (k) | Variable (within radius) |
| Query parameter | k neighbors | distance threshold |
| Use case | "Top-k similar" | "All within threshold" |
| Performance | O(log n) with HNSW | Similar with optimization |

### 2. Multi-Vector Search (`multi_vector_search.h/cpp`)

Complex similarity queries involving multiple vectors.

**Features:**
- Multiple query vectors (ensemble search)
- Multiple vector fields per item (multi-modal)
- Various fusion strategies (linear, rank-based, RRF)
- Hybrid search (vector + keyword)
- Query expansion support
- Weight optimization

**Fusion Strategies:**
1. **Linear Combination**: `score = w1*s1 + w2*s2 + ...`
2. **Rank Fusion**: Borda count method
3. **Reciprocal Rank Fusion (RRF)**: `score = Σ 1/(k + rank_i)`
4. **Max/Min/Avg**: Simple aggregation
5. **Learned Fusion**: ML-optimized weights (future)

**Example Usage:**
```cpp
#include "index/multi_vector_search.h"

using namespace themis::vector;

MultiVectorSearch multi_search(vector_manager);

// Example 1: Multi-query search
MultiVectorSearch::MultiQuery query;
query.vectors = {query_vec1, query_vec2, query_vec3};
query.weights = {0.5f, 0.3f, 0.2f};

MultiVectorSearch::SearchConfig config;
config.fusion = FusionStrategy::LINEAR_COMBINATION;
config.top_k = 10;

// Future: Full implementation will enable
// auto result = multi_search.search(query, config);

// Example 2: Multi-field search
// auto result = multi_search.searchMultiField(
//     query_vector, 
//     {"title_embedding", "content_embedding"},
//     config
// );

// Example 3: Hybrid search (vector + keywords)
// std::unordered_map<std::string, float> keyword_scores = {
//     {"doc1", 0.8f}, {"doc2", 0.6f}
// };
// auto result = multi_search.hybridSearch(
//     query_vector, keyword_scores, config);
```

**Use Cases:**
- Multi-modal search (text + image + audio)
- Ensemble retrieval (multiple query formulations)
- Hybrid search (semantic + keyword)
- Multi-aspect similarity (title + content + metadata)
- Cross-lingual search (multiple language embeddings)
- Recommendation systems (multiple user preference vectors)

## Integration with Existing Systems

### Vector Index Manager Integration

Both modules integrate with existing `VectorIndexManager`:

```cpp
#include "index/vector_index.h"
#include "index/approximate_radius_search.h"
#include "index/multi_vector_search.h"

// Initialize vector manager
VectorIndexManager vector_manager(db);
vector_manager.init("documents", 768, VectorIndexManager::Metric::COSINE);

// Create advanced search modules
ApproximateRadiusSearch radius_search(vector_manager);
MultiVectorSearch multi_search(vector_manager);
```

### Advanced Vector Index Integration

Works with `advanced_vector_index.h` for optimized operations:

```cpp
#include "index/advanced_vector_index.h"

// Advanced features can leverage GPU acceleration
// and other optimizations from advanced vector index
```

## Planned Implementation Timeline

### Phase 1: Approximate Radius Search (Q2 2026)
- Implement HNSW-based radius search
- Add distance threshold filtering
- Optimize for common radius values
- Add batch processing
- Comprehensive testing

### Phase 2: Multi-Vector Search - Basic (Q3 2026)
- Implement linear combination fusion
- Add rank-based fusion (Borda count)
- Implement RRF (Reciprocal Rank Fusion)
- Multi-query search support
- Performance optimization

### Phase 3: Multi-Vector Search - Advanced (Q4 2026)
- Multi-field search implementation
- Hybrid search (vector + keyword)
- Query expansion support
- Weight learning/optimization
- GPU acceleration

### Phase 4: Production Features (2027)
- Distributed search for massive datasets
- Advanced caching strategies
- Real-time index updates
- A/B testing framework
- ML-based fusion strategies

## Algorithm Complexity

### Approximate Radius Search
| Operation | Time Complexity | Space Complexity | Notes |
|-----------|----------------|------------------|-------|
| Search | O(log n + r) | O(r) | r = results in radius |
| Batch Search | O(m × log n + R) | O(R) | m = queries, R = total results |
| Estimate Count | O(log n × s) | O(1) | s = sample size |

### Multi-Vector Search
| Operation | Time Complexity | Space Complexity | Notes |
|-----------|----------------|------------------|-------|
| Linear Fusion | O(k × m × log n) | O(k × m) | k = top-k, m = vectors |
| Rank Fusion | O(k × m × log n + k × log k) | O(k × m) | Includes sorting |
| RRF | O(k × m × log n) | O(k × m) | Constant time fusion |
| Hybrid Search | O(k × log n + h) | O(k + h) | h = keyword matches |

## Performance Considerations

### Radius Search Optimization
1. **Index Structure**: HNSW with radius-aware navigation
2. **Early Termination**: Stop when distance exceeds radius
3. **Batch Processing**: Reuse computations across queries
4. **Caching**: Cache frequent radius values
5. **GPU Acceleration**: Parallel distance computations

### Multi-Vector Fusion Optimization
1. **Lazy Evaluation**: Only compute needed similarities
2. **Score Caching**: Cache individual vector scores
3. **Parallel Search**: Execute searches in parallel
4. **Approximate Fusion**: Use sampling for large result sets
5. **Index Sharing**: Reuse index structures across queries

## Fusion Strategy Comparison

| Strategy | Complexity | Quality | Tuning Needed | Best For |
|----------|-----------|---------|---------------|----------|
| Linear | O(m) | High | Yes (weights) | Known importance |
| Max | O(m) | Medium | No | Any match acceptable |
| Avg | O(m) | Medium | No | Equal importance |
| Rank | O(m log m) | High | Optional | Unknown importance |
| RRF | O(m) | High | Minimal (k param) | General purpose |

## Error Handling

All methods return `Result<T>` for consistent error handling:

```cpp
auto result = multi_search.search(query, config);
if (!result) {
    std::cerr << "Error: " << result.error().message << std::endl;
    return;
}

const auto& search_result = result.value();
std::cout << "Found " << search_result.results.size() << " results\n";
```

Current stub implementations return:
```cpp
ErrorRegistry::ErrorCode::NOT_IMPLEMENTED
"Method is not yet implemented. This is a stub for GAP-006..."
```

## Testing

### Current Tests
Basic interface tests verify:
- Constructor/destructor behavior
- Method signatures and return types
- Error handling for unimplemented methods
- Integration with VectorIndexManager

### Planned Tests
Comprehensive test suites will include:
- Correctness tests with known vectors
- Recall/precision measurements
- Performance benchmarks
- Scalability tests (1K to 1M vectors)
- Fusion strategy comparisons
- Integration tests

Test datasets:
- SIFT1M: 1M 128-dim vectors
- GIST1M: 1M 960-dim vectors
- Deep1B: 1B 96-dim vectors (subset)
- Custom synthetic datasets

## References

### Academic Papers
1. **HNSW**: Malkov, Y. A., & Yashunin, D. A. (2018). "Efficient and robust approximate nearest neighbor search"
2. **FAISS**: Johnson, J., et al. (2019). "Billion-scale similarity search with GPUs"
3. **RRF**: Cormack, G. V., et al. (2009). "Reciprocal rank fusion outperforms condorcet and individual rank learning methods"
4. **CombSUM/CombMNZ**: Fox, E. A., & Shaw, J. A. (1994). "Combination of multiple searches"
5. **Multi-Modal**: Dosovitskiy, A., et al. (2020). "An Image is Worth 16x16 Words: Transformers for Image Recognition at Scale"

### Similar Systems
- **Milvus**: Multi-vector and hybrid search support
- **Weaviate**: Hybrid search with BM25 + vector
- **Pinecone**: Metadata filtering and hybrid search
- **Qdrant**: Payload-based filtering and scoring
- **FAISS**: Radius search with HNSW and IVF

## Example Applications

### Application 1: Multi-Modal Product Search
```cpp
// Search products by image + text description
MultiVectorSearch::MultiQuery query;
query.vectors = {image_embedding, text_embedding};
query.weights = {0.7f, 0.3f};  // Prefer image similarity
query.field_names = {"image_vec", "desc_vec"};

auto results = multi_search.search(query, config);
```

### Application 2: Duplicate Detection
```cpp
// Find all potential duplicates within 0.1 distance
ApproximateRadiusSearch::SearchConfig config;
config.radius = 0.1f;
config.metric = Metric::COSINE;

auto results = radius_search.search(document_vector, config);
// Check results for exact duplicates
```

### Application 3: Query Expansion
```cpp
// Generate multiple query formulations
std::vector<std::vector<float>> variants = {
    original_query,
    reformulation1,
    reformulation2
};

auto results = multi_search.searchWithExpansion(variants, config);
// Combines evidence from all formulations
```

## Contributing

When implementing these algorithms:

1. **Follow patterns**: Use `Result<T>`, integrate with `VectorIndexManager`
2. **Benchmark**: Compare with baseline implementations
3. **Document**: Include complexity analysis and references
4. **Test**: Correctness, recall, precision, performance
5. **Optimize**: Consider GPU acceleration from start

## License

Part of ThemisDB - Multi-Model Database System

## Related Documentation

- [Vector Index](./vector_index.h) - Core vector storage and search
- [Advanced Vector Index](./advanced_vector_index.h) - GPU-accelerated operations
- [Error Handling](../utils/expected.h) - Result<T> pattern
- [GAP Analysis](../../docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md) - Implementation gaps

## Future Enhancements

### Advanced Features
- **Learned Metrics**: Train custom distance functions
- **Dynamic Indexing**: Real-time index updates
- **Approximate Algorithms**: Trade accuracy for speed
- **Distributed Search**: Partition vectors across nodes
- **Streaming Search**: Process vector streams

### Optimization Features
- **Query Cache**: Cache frequent queries
- **Prefetching**: Predict and prefetch results
- **Adaptive Fusion**: Learn weights from user feedback
- **GPU Batching**: Optimize GPU memory usage
- **Index Compression**: Reduce memory footprint

### Integration Features
- **AQL Integration**: Native query language support
- **REST API**: HTTP endpoints for search
- **gRPC API**: High-performance RPC interface
- **Monitoring**: Real-time metrics and alerts
- **A/B Testing**: Compare fusion strategies
