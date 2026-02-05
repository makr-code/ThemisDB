---
name: GAP-006 Multi-Vector Search Implementation
about: Implement multi-vector search with fusion strategies for complex similarity queries
title: '[GAP-006] Implement MultiVectorSearch Algorithm'
labels: ['type:feature', 'area:vector', 'gap:006', 'priority:P2', 'status:ready']
assignees: ''
---

## Implementation Task
Implement the MultiVectorSearch algorithm to support complex similarity queries involving multiple vectors with various fusion strategies.

**Current Status:** Stub implementation (returns NOT_IMPLEMENTED)  
**Target Release:** Q4 2026  
**Priority:** P2 - High  
**Effort:** Large (3-4 weeks)

## Background

MultiVectorSearch enables advanced retrieval scenarios:
- Multiple query vectors (e.g., multiple query reformulations)
- Multiple vector fields per document (e.g., title + content embeddings)
- Fusion strategies to combine similarity scores
- Hybrid search (vector + keyword)

The stub interface is defined in:
- `include/index/multi_vector_search.h` - Complete interface definition
- `src/index/multi_vector_search.cpp` - Stub implementation (needs real algorithm)

## Use Cases

1. **Query Expansion**: Search with multiple query reformulations
2. **Multi-Modal Search**: Combine text + image embeddings
3. **Multi-Field Search**: Search across title, content, metadata vectors
4. **Hybrid Search**: Combine semantic (vector) + lexical (BM25) search
5. **Ensemble Retrieval**: Combine multiple retrieval methods

## Fusion Strategies to Implement

### 1. Linear Combination
```
fused_score = w1 * score1 + w2 * score2 + ... + wn * scoren
where Σwi = 1.0 (normalized weights)
```

### 2. Reciprocal Rank Fusion (RRF)
```
fused_score = Σ [1 / (k + rank_i)]
where k = 60 (configurable constant)
```

### 3. Rank-Based Fusion (Borda Count)
```
fused_score = Σ (N - rank_i)
where N = total documents
```

### 4. Max/Min/Avg Score
```
fused_score = max(score1, score2, ..., scoren)
fused_score = min(score1, score2, ..., scoren)
fused_score = avg(score1, score2, ..., scoren)
```

## Required Implementation

### 1. Core Multi-Vector Search

**File:** `src/index/multi_vector_search.cpp`

```cpp
Result<MultiVectorSearch::MultiSearchResult> 
MultiVectorSearch::search(
    const MultiQuery& query,
    const SearchConfig& config) {
    
    // IMPLEMENTATION STEPS:
    
    // 1. Validate inputs
    //    - Check query vectors are non-empty
    //    - Validate weights (if provided) sum to 1.0
    //    - Validate dimensions match
    
    // 2. Perform individual vector searches
    std::vector<VectorSearchResults> individual_results;
    for (size_t i = 0; i < query.vectors.size(); ++i) {
        // Query VectorIndexManager for each vector
        auto result = vector_manager_.search(
            query.vectors[i], 
            config.top_k * 2  // Get more for better fusion
        );
        individual_results.push_back(result);
    }
    
    // 3. Apply fusion strategy
    auto fused = applyFusion(
        individual_results,
        config.fusion,
        config.weights.empty() ? query.weights : config.weights
    );
    
    // 4. Post-process
    //    - Sort by fused_score
    //    - Take top_k results
    //    - Calculate statistics
    
    // 5. Return MultiSearchResult
}
```

### 2. Fusion Strategy Implementation

```cpp
std::vector<SearchResult> applyFusion(
    const std::vector<VectorSearchResults>& individual_results,
    FusionStrategy strategy,
    const std::vector<float>& weights) {
    
    // 1. Collect all unique document IDs
    std::unordered_set<std::string> all_docs;
    for (const auto& results : individual_results) {
        for (const auto& result : results) {
            all_docs.insert(result.id);
        }
    }
    
    // 2. For each document, calculate fused score
    std::vector<SearchResult> fused_results;
    for (const auto& doc_id : all_docs) {
        SearchResult result;
        result.id = doc_id;
        
        // Collect scores and ranks from each query
        std::vector<float> scores;
        std::vector<int> ranks;
        
        for (size_t i = 0; i < individual_results.size(); ++i) {
            auto it = findInResults(individual_results[i], doc_id);
            if (it != individual_results[i].end()) {
                scores.push_back(it->score);
                ranks.push_back(it->rank);
            } else {
                scores.push_back(0.0f);  // Not found
                ranks.push_back(INT_MAX);  // Worst rank
            }
        }
        
        // Apply fusion strategy
        switch (strategy) {
            case FusionStrategy::LINEAR_COMBINATION:
                result.fused_score = linearCombination(scores, weights);
                break;
            case FusionStrategy::RECIPROCAL_RANK:
                result.fused_score = reciprocalRankFusion(ranks, config.rrf_k);
                break;
            case FusionStrategy::RANK_FUSION:
                result.fused_score = rankFusion(ranks);
                break;
            case FusionStrategy::MAX_SCORE:
                result.fused_score = *std::max_element(scores.begin(), scores.end());
                break;
            // ... other strategies
        }
        
        result.individual_scores = scores;
        result.individual_ranks = ranks;
        fused_results.push_back(result);
    }
    
    // 3. Sort by fused_score (descending)
    std::sort(fused_results.begin(), fused_results.end(),
              [](const auto& a, const auto& b) {
                  return a.fused_score > b.fused_score;
              });
    
    return fused_results;
}
```

### 3. Individual Fusion Functions

```cpp
// Linear combination: weighted sum of scores
float linearCombination(const std::vector<float>& scores,
                       const std::vector<float>& weights) {
    float sum = 0.0f;
    for (size_t i = 0; i < scores.size(); ++i) {
        sum += scores[i] * weights[i];
    }
    return sum;
}

// RRF: Reciprocal rank fusion
float reciprocalRankFusion(const std::vector<int>& ranks, float k) {
    float score = 0.0f;
    for (int rank : ranks) {
        if (rank != INT_MAX) {
            score += 1.0f / (k + rank);
        }
    }
    return score;
}

// Borda count: rank-based voting
float rankFusion(const std::vector<int>& ranks) {
    int max_rank = *std::max_element(ranks.begin(), ranks.end());
    float score = 0.0f;
    for (int rank : ranks) {
        if (rank != INT_MAX) {
            score += (max_rank - rank);
        }
    }
    return score;
}
```

### 4. Score Normalization

```cpp
// Normalize scores to [0, 1] before fusion
std::vector<float> normalizeScores(const std::vector<float>& scores) {
    if (scores.empty()) return {};
    
    float min_score = *std::min_element(scores.begin(), scores.end());
    float max_score = *std::max_element(scores.begin(), scores.end());
    float range = max_score - min_score;
    
    if (range < 1e-6) return scores;  // All same
    
    std::vector<float> normalized;
    for (float score : scores) {
        normalized.push_back((score - min_score) / range);
    }
    return normalized;
}
```

### 5. Additional Search Methods

```cpp
// Multi-field search
Result<MultiSearchResult> searchMultiField(
    const std::vector<float>& query_vector,
    const std::vector<std::string>& field_names,
    const SearchConfig& config) {
    
    // 1. For each field:
    //    - Query that field's vector index
    //    - Collect results
    // 2. Fuse results across fields
    // 3. Return combined results
}

// Hybrid search (vector + keyword)
Result<MultiSearchResult> hybridSearch(
    const std::vector<float>& query_vector,
    const std::unordered_map<std::string, float>& keyword_scores,
    const SearchConfig& config) {
    
    // 1. Perform vector similarity search
    // 2. Get keyword/BM25 scores (provided)
    // 3. Fuse vector and keyword scores
    //    - Treat keyword_scores as additional "query"
    //    - Apply fusion strategy
}

// Batch multi-vector search
Result<std::vector<MultiSearchResult>> batchSearch(
    const std::vector<MultiQuery>& queries,
    const SearchConfig& config) {
    
    // Process multiple multi-vector queries
    // Optional: Parallelize for performance
}

// Learn optimal weights from training data
Result<std::vector<float>> optimizeWeights(
    const std::vector<MultiQuery>& queries,
    const std::vector<std::vector<std::string>>& relevance_judgments) {
    
    // 1. For each query, get results with various weights
    // 2. Calculate metrics (NDCG, MAP, MRR)
    // 3. Use grid search or gradient descent to find optimal weights
    // 4. Return learned weights
}
```

## Testing Requirements

### Unit Tests

**File:** `tests/test_multi_vector_search.cpp`

```cpp
TEST(MultiVectorSearchTest, LinearCombinationFusion) {
    // Create small vector index
    // Query with 2 vectors
    // Verify linear combination works correctly
}

TEST(MultiVectorSearchTest, ReciprocalRankFusion) {
    // Test RRF formula
    // Verify ranks are used correctly
}

TEST(MultiVectorSearchTest, RankBasedFusion) {
    // Test Borda count
}

TEST(MultiVectorSearchTest, MaxMinAvgFusion) {
    // Test simple aggregation strategies
}

TEST(MultiVectorSearchTest, ScoreNormalization) {
    // Test normalization when enabled
    // Verify scores in [0, 1]
}

TEST(MultiVectorSearchTest, MultiFieldSearch) {
    // Index with multiple vector fields
    // Search across fields
    // Verify fusion
}

TEST(MultiVectorSearchTest, HybridSearch) {
    // Combine vector + keyword scores
    // Verify fusion works
}

TEST(MultiVectorSearchTest, WeightOptimization) {
    // Provide training queries
    // Optimize weights
    // Verify improved metrics
}
```

### Integration Tests

**File:** `tests/integration/test_multi_vector_search_integration.cpp`

```cpp
TEST(MultiVectorIntegrationTest, QueryExpansionScenario) {
    // Simulate query expansion use case
    // Multiple query reformulations
    // Verify RRF improves results
}

TEST(MultiVectorIntegrationTest, MultiModalSearch) {
    // Text + image embeddings
    // Search with both
    // Verify fusion
}

TEST(MultiVectorIntegrationTest, PerformanceBenchmark) {
    // 3 query vectors, 100K documents
    // Measure query time
    // Verify < 100ms
}
```

### Quality Testing

```cpp
TEST(MultiVectorQualityTest, FusionComparisonNDCG) {
    // Compare different fusion strategies
    // Calculate NDCG@10 for each
    // Verify RRF typically outperforms others
}
```

## Algorithm Complexity

**Target Complexity:**
- **Time:** O(m × k × log N) where m = # query vectors, k = top-k, N = index size
  - Individual searches: m × O(k × log N)
  - Fusion: O(k × m × log k)
  
- **Space:** O(m × k) for storing m result sets of size k

**Performance Targets:**
- Query time < 100ms for 3 vectors, 1M documents, top_k=100
- RRF fusion < 10ms for combining results
- Batch processing: 10 queries < 1 second

## Implementation Checklist

### Phase 1: Core Fusion (Week 1)
- [ ] Implement basic multi-vector search
- [ ] Linear combination fusion
- [ ] RRF fusion
- [ ] Max/Min/Avg fusion
- [ ] Basic unit tests

### Phase 2: Extended Fusion (Week 2)
- [ ] Rank-based fusion (Borda count)
- [ ] Score normalization
- [ ] Multi-field search
- [ ] Comprehensive unit tests

### Phase 3: Advanced Features (Week 3)
- [ ] Hybrid search (vector + keyword)
- [ ] Batch processing
- [ ] Weight optimization (learning)
- [ ] Integration tests

### Phase 4: Optimization & Polish (Week 4)
- [ ] Performance optimization
- [ ] Parallel query execution
- [ ] Statistics tracking
- [ ] Documentation and examples
- [ ] Quality/NDCG testing

## Success Criteria

- [ ] All 6 fusion strategies implemented
- [ ] Multi-field search working
- [ ] Hybrid search working
- [ ] Unit test coverage > 90%
- [ ] Integration tests passing
- [ ] Performance targets met (< 100ms)
- [ ] Weight optimization functional
- [ ] Documentation complete with examples
- [ ] NDCG improvements demonstrated

## References

### Academic Papers
1. Cormack, G. V., et al. (2009). "Reciprocal rank fusion outperforms condorcet"
2. Fox, E. A., & Shaw, J. A. (1994). "Combination of multiple searches" (CombSUM, CombMNZ)
3. Montague, M., & Aslam, J. A. (2002). "Condorcet fusion for improved retrieval"
4. Beitzel, S. M., et al. (2009). "On fusion of effective retrieval strategies"

### Existing Code to Study
- `src/index/vector_index.cpp` - VectorIndexManager search
- Fusion strategy patterns from information retrieval

### Similar Implementations
- Elasticsearch: Multi-match queries with various scoring
- Solr: DisMax query parser with field boosts
- Vespa: Tensors and multi-vector ranking

## Notes

- Consider adding learned fusion (ML-based) in future
- Statistics tracking important for A/B testing fusion strategies
- May need configuration profiles for common use cases
- Consider query performance vs quality trade-offs

## Related Issues

- GAP-006 Main tracking issue
- VectorIndexManager multi-field support
- Hybrid search architecture design
