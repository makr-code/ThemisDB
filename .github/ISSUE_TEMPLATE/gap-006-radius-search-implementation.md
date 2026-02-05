---
name: GAP-006 Approximate Radius Search Implementation
about: Implement approximate radius search for vector similarity queries
title: '[GAP-006] Implement ApproximateRadiusSearch Algorithm'
labels: ['type:feature', 'area:vector', 'gap:006', 'priority:P2', 'status:ready']
assignees: ''
---

## Implementation Task
Implement the ApproximateRadiusSearch algorithm to find all vectors within a distance threshold (radius) from a query vector.

**Current Status:** Stub implementation (returns NOT_IMPLEMENTED)  
**Target Release:** Q3-Q4 2026  
**Priority:** P2 - High  
**Effort:** Large (2-3 weeks)

## Background

ApproximateRadiusSearch provides radius-based vector similarity search (find ALL vectors within threshold) as opposed to k-NN search (find k nearest vectors). The stub interface is defined in:
- `include/index/approximate_radius_search.h` - Complete interface definition
- `src/index/approximate_radius_search.cpp` - Stub implementation (needs real algorithm)

## Key Difference: Radius Search vs k-NN

| Feature | k-NN Search | Radius Search |
|---------|-------------|---------------|
| Query parameter | k (number of neighbors) | radius (distance threshold) |
| Result count | Fixed (k results) | Variable (all within radius) |
| Use case | "Find 10 similar items" | "Find all items within 0.3 distance" |
| Existing impl | ✅ VectorIndexManager | ❌ Not yet implemented |

## Required Implementation

### 1. Core Algorithm - HNSW-Based Radius Search

**File:** `src/index/approximate_radius_search.cpp`

Implement the main search method:

```cpp
Result<ApproximateRadiusSearch::SearchResult> 
ApproximateRadiusSearch::search(
    const std::vector<float>& query_vector,
    const SearchConfig& config) {
    
    // IMPLEMENTATION STEPS:
    
    // 1. Validate inputs
    //    - Check query_vector dimensions match index
    //    - Validate radius > 0
    //    - Validate max_results > 0
    
    // 2. Access HNSW index from VectorIndexManager
    //    - Get index for configured metric (L2, COSINE, DOT)
    //    - Check if index exists and is initialized
    
    // 3. Initialize search structures
    //    - Result vector (up to max_results)
    //    - Visited set to avoid duplicates
    //    - Priority queue for candidates
    
    // 4. Modified HNSW search with radius constraint:
    //    Starting from entry point:
    //    - Navigate HNSW graph layers (top to bottom)
    //    - At each layer:
    //      * Explore neighbors
    //      * Calculate distances to query
    //      * Add to candidates if distance <= radius
    //      * Continue exploration if promising
    //    - At bottom layer (layer 0):
    //      * Exhaustively check neighborhood
    //      * Collect all vectors within radius
    
    // 5. Post-processing:
    //    - Sort results by distance (if config.sort_results)
    //    - Truncate to max_results
    //    - Calculate statistics (total_candidates, actual_max_distance)
    //    - Check if truncated flag should be set
    
    // 6. Return SearchResult
}
```

### 2. HNSW Radius Search Algorithm

Key modifications to standard HNSW search:

```cpp
// Pseudocode for radius search
void radiusSearchLayer(layer_num, query, radius, candidates, visited) {
    ep = entry_point[layer_num];
    
    while (has_closer_neighbor) {
        current = get_closest_unvisited(candidates);
        visited.insert(current);
        
        for (neighbor : get_neighbors(current, layer_num)) {
            if (visited.contains(neighbor)) continue;
            
            distance = compute_distance(query, neighbor);
            
            // KEY DIFFERENCE: Check radius instead of maintaining k-nearest
            if (distance <= radius) {
                candidates.add(neighbor, distance);
                // Don't stop - keep exploring to find ALL within radius
            }
            
            // Pruning: Stop if distance is much larger than radius
            if (distance > radius * expansion_factor) {
                continue;  // Don't explore this branch further
            }
        }
    }
}
```

### 3. Distance Metrics Support

Implement for all three metrics:

```cpp
float computeDistance(const std::vector<float>& a, 
                     const std::vector<float>& b,
                     Metric metric) {
    switch (metric) {
        case Metric::L2:
            return euclideanDistance(a, b);
        case Metric::COSINE:
            return 1.0f - cosineSimilarity(a, b);
        case Metric::DOT_PRODUCT:
            return -dotProduct(a, b);  // Negative for max->min
    }
}
```

### 4. Additional Search Methods

Implement convenience methods:

```cpp
// Search by ID (lookup vector first)
Result<SearchResult> searchById(
    std::string_view query_id,
    const SearchConfig& config) {
    // 1. Lookup vector from VectorIndexManager
    // 2. Call search() with retrieved vector
}

// Batch search (multiple queries)
Result<std::vector<SearchResult>> batchSearch(
    const std::vector<std::vector<float>>& query_vectors,
    const SearchConfig& config) {
    // 1. For each query vector:
    //    - Call search()
    //    - Collect result
    // 2. Optional: Parallelize for performance
}

// Dynamic radius adjustment to hit target count
Result<SearchResult> searchWithTargetCount(
    const std::vector<float>& query_vector,
    int target_count,
    const SearchConfig& config) {
    // 1. Start with initial radius
    // 2. Binary search on radius:
    //    - If too few results, increase radius
    //    - If too many, decrease radius
    //    - Iterate until close to target_count
}

// Estimate result count (sampling-based)
Result<size_t> estimateResultCount(
    const std::vector<float>& query_vector,
    float radius,
    Metric metric) {
    // 1. Sample N points from index
    // 2. Count how many fall within radius
    // 3. Extrapolate to full index size
}
```

### 5. Integration with VectorIndexManager

**Dependencies:**
```cpp
#include "index/vector_index.h"

// Use existing VectorIndexManager methods:
// - Access HNSW index
// - Get vector by ID
// - Query index statistics
// - Check index initialization status
```

**Key Integration Points:**
```cpp
// In constructor
ApproximateRadiusSearch::ApproximateRadiusSearch(VectorIndexManager& vm)
    : vector_manager_(vm) {
    // Initialize statistics
}

// Access HNSW index internals (may need friend class or accessor methods)
auto* hnsw_index = vector_manager_.getHNSWIndex();
```

### 6. Performance Optimizations

Implement these optimizations:

1. **Adaptive Exploration**
   - Use expansion_factor to control search breadth
   - Start conservative, expand if needed
   - Prune branches unlikely to have results

2. **Early Termination**
   - Stop if max_results reached and min_recall satisfied
   - Use beam search to limit candidates

3. **Batch Processing**
   - Process multiple queries in parallel
   - Share distance computations when possible
   - Use SIMD for distance calculations

4. **Caching**
   - Cache frequently accessed neighborhoods
   - Cache distance computations
   - LRU cache for vector data

5. **GPU Acceleration** (Future)
   - Offload distance computations to GPU
   - Batch multiple queries for GPU efficiency

## Testing Requirements

### Unit Tests

**File:** `tests/test_approximate_radius_search.cpp`

```cpp
TEST(ApproximateRadiusSearchTest, BasicRadiusSearch) {
    // Create small vector index
    // Query with known radius
    // Verify all results within radius
}

TEST(ApproximateRadiusSearchTest, MetricsL2Cosine) {
    // Test with L2 metric
    // Test with Cosine metric
    // Verify distance calculations
}

TEST(ApproximateRadiusSearchTest, MaxResultsTruncation) {
    // Set max_results < actual results
    // Verify truncation works correctly
}

TEST(ApproximateRadiusSearchTest, EmptyResults) {
    // Query with very small radius
    // Verify returns empty result set
}

TEST(ApproximateRadiusSearchTest, BatchSearch) {
    // Query with multiple vectors
    // Verify results for each query
}

TEST(ApproximateRadiusSearchTest, DynamicRadius) {
    // Use searchWithTargetCount
    // Verify hits target (within tolerance)
}

TEST(ApproximateRadiusSearchTest, EstimateCount) {
    // Call estimateResultCount
    // Verify estimate is reasonable
}
```

### Integration Tests

**File:** `tests/integration/test_radius_search_integration.cpp`

```cpp
TEST(RadiusSearchIntegrationTest, RealVectorData) {
    // Load actual embeddings (e.g., SIFT1M subset)
    // Perform radius searches
    // Verify correctness and recall
}

TEST(RadiusSearchIntegrationTest, PerformanceBenchmark) {
    // Test with 100K vectors
    // Measure query time
    // Verify < 50ms per query
}
```

### Recall Testing

Verify approximate search quality:

```cpp
TEST(RadiusSearchRecallTest, RecallVsExactSearch) {
    // Perform exact brute-force radius search
    // Perform approximate HNSW radius search
    // Calculate recall@radius
    // Verify recall >= min_recall (e.g., 95%)
}
```

## Algorithm Complexity

**Target Complexity:**
- **Time:** O(log N + M) where N is index size, M is results within radius
  - HNSW navigation: O(log N)
  - Result collection: O(M)
  - Sorting (if enabled): O(M log M)
  
- **Space:** O(M) for storing M results

**Performance Targets:**
- Query time < 50ms for 1M vectors (single query)
- Recall >= 95% compared to exact search
- Support radius values from 0.01 to 2.0 (for cosine distance 0-2)

## Implementation Checklist

### Phase 1: Core Radius Search (Week 1)
- [ ] Implement basic radius search algorithm
- [ ] Support L2 metric
- [ ] Basic HNSW navigation with radius constraint
- [ ] Unit tests for basic functionality

### Phase 2: Extended Features (Week 2)
- [ ] Support COSINE and DOT_PRODUCT metrics
- [ ] Implement searchById
- [ ] Implement batchSearch
- [ ] Implement estimateResultCount
- [ ] Comprehensive unit tests

### Phase 3: Advanced Features (Week 3)
- [ ] Implement searchWithTargetCount (dynamic radius)
- [ ] Add performance optimizations (pruning, caching)
- [ ] Integration tests
- [ ] Recall testing vs exact search
- [ ] Performance benchmarks

### Phase 4: Polish & Documentation
- [ ] Edge case handling
- [ ] Statistics tracking
- [ ] Documentation updates
- [ ] Usage examples
- [ ] Code review and optimization

## Success Criteria

- [ ] Core search method fully implemented
- [ ] All distance metrics supported (L2, COSINE, DOT)
- [ ] Unit test coverage > 90%
- [ ] Integration tests passing
- [ ] Recall >= 95% on test datasets
- [ ] Performance targets met (< 50ms per query on 1M vectors)
- [ ] No memory leaks
- [ ] Documentation complete with examples

## References

### Academic Papers
1. Malkov, Y. A., & Yashunin, D. A. (2018). "Efficient and robust approximate nearest neighbor search using HNSW"
2. Johnson, J., et al. (2019). "Billion-scale similarity search with GPUs" (FAISS)
3. Muja, M., & Lowe, D. G. (2014). "Scalable nearest neighbor algorithms for high dimensional data"

### Existing Code to Study
- `src/index/vector_index.cpp` - VectorIndexManager implementation
- `include/index/advanced_vector_index.h` - Advanced vector operations
- Tests: `tests/test_vector_index.cpp`

### Similar Implementations
- FAISS: Range search API
- Hnswlib: radius_search extension
- Annoy: get_nns_by_vector with search_k parameter

## Notes

- HNSW index must support layer-by-layer exploration
- Consider adding `expansion_factor` config parameter for tuning
- Statistics tracking important for monitoring performance
- May need to expose more VectorIndexManager internals via friend class

## Related Issues

- GAP-006 Main tracking issue
- VectorIndexManager enhancements
- GPU acceleration for vector operations
