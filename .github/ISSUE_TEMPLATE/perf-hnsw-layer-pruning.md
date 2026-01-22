---
name: ⚡ Performance: HNSW Vector Index Layer Pruning Implementation
about: Implement HNSW layer pruning for O(log²N) to O(log N) reduction
title: "[PERF] Implement HNSW Layer Pruning for 1B+ Vectors"
labels: priority:P1, type:performance, area:vector-index, effort:x-large, phase:implementation
assignees: ''
---

## 📊 Performance Enhancement - Phase 4 Implementation

**Current Status:** Design complete, implementation pending  
**Priority:** P1 (High)  
**Effort:** 3 weeks  
**Target Version:** v1.5.0  
**Parent PR:** #XXX (Scaling Optimizations to 10B Records)  
**Design Document:** `docs/performance/phase4_hnsw_layer_optimization.md`  
**Related Files:**
- `src/index/vector_index.cpp`
- `include/index/vector_index.h`

---

## 📋 Problem Description

HNSW (Hierarchical Navigable Small World) vector index shows **O(log²N) layer traversal cost** at scale, resulting in:
- **-15% overhead** at 1B vectors (vs smaller datasets)
- Degraded search latency at 10M+ vectors
- Unnecessary deep layer traversals

**Performance Impact:** Missing **+67% efficiency improvement** (reducing overhead from -15% to -5% at 1B items).

**Root Cause:**
- All layers are traversed during search (no early termination)
- No adaptive layer selection based on query patterns
- No cache locality optimization for batch inserts
- No efficiency tracking per layer

---

## 🎯 Requirements

### Must Have (P1)

- [ ] **1. Predictive Layer Pruning**
  - Skip deeper layers if sufficient candidates found
  - Implement threshold: `candidate_count > k * threshold_multiplier`
  - Default threshold multiplier: 5
  - Configurable via `hnsw_optimization.layer_pruning.threshold_multiplier`
  - **Expected Impact:** 20-30% reduction in layer traversals

- [ ] **2. Adaptive Layer Selection**
  - Track layer efficiency per query type
  - Adjust entry layer dynamically based on statistics
  - Cache layer statistics (moving window of 1000 queries)
  - Optimize `ef` parameter based on query patterns
  - **Expected Impact:** 10-15% improvement in search latency

- [ ] **3. Layer Efficiency Tracking**
  - Track metrics per layer: access count, candidates found, search time
  - Calculate efficiency score: `candidates_found / search_time`
  - Report statistics via monitoring API
  - Store statistics for adaptive optimization
  - **Expected Impact:** Enable data-driven tuning

- [ ] **4. Configuration Support**
  - Load from `config/scaling_optimizations.yaml`
  - Runtime enable/disable per feature
  - Expose tuning parameters (threshold, window size, etc.)
  - Default: disabled for backward compatibility

### Should Have (P2)

- [ ] **5. Batch Insert Optimization**
  - Group inserts by estimated target layer
  - Insert vectors for same layer together (cache locality)
  - Implement pending insert buffer with layer grouping
  - Flush when buffer reaches threshold
  - **Expected Impact:** 15-20% improvement in bulk insert throughput

- [ ] **6. Advanced Pruning**
  - Distance-based pruning (skip layers if distance threshold met)
  - Diversity-based pruning (skip if candidates diverse enough)
  - Query-specific pruning thresholds

### Nice to Have (P3)

- [ ] **7. Per-Collection Tuning**
  - Different pruning thresholds per collection
  - Collection-specific layer efficiency tracking
  - Auto-tuning based on workload patterns

---

## 🔧 Implementation Plan

### Week 1: Layer Pruning Foundation
**Day 1-2: Design & Infrastructure**
- [ ] Create `HnswLayerOptimizer` class
- [ ] Add layer statistics structure
- [ ] Implement statistics collection framework

**Day 3-4: Predictive Pruning**
- [ ] Implement pruning logic in `searchKnn`
- [ ] Add candidate count tracking per layer
- [ ] Implement early termination
- [ ] Unit tests for pruning

**Day 5: Configuration & Testing**
- [ ] Load configuration from YAML
- [ ] Integration tests
- [ ] Performance benchmarks (baseline vs pruning)

### Week 2: Adaptive Layer Selection
**Day 1-2: Statistics Collection**
- [ ] Track layer access patterns
- [ ] Calculate efficiency scores
- [ ] Implement moving window statistics

**Day 3-4: Adaptive Selection**
- [ ] Implement optimal entry layer selection
- [ ] Implement optimal `ef` calculation
- [ ] Wire up to search logic

**Day 5: Testing**
- [ ] Unit tests for adaptive selection
- [ ] Integration tests with various workloads
- [ ] Performance validation

### Week 3: Batch Insert & Integration
**Day 1-2: Batch Insert**
- [ ] Implement insert buffering
- [ ] Layer-based grouping
- [ ] Flush logic

**Day 3: Monitoring & Metrics**
- [ ] Export layer statistics
- [ ] Add performance metrics
- [ ] Grafana dashboard updates

**Day 4-5: Final Testing & Documentation**
- [ ] End-to-end testing with 1B vectors
- [ ] Performance validation against goals
- [ ] Documentation updates
- [ ] Code review & refinement

---

## 📝 Implementation Notes

### Code Structure

**New File: `include/index/hnsw_layer_optimizer.h`**
```cpp
class HnswLayerOptimizer {
public:
    struct LayerStats {
        int layer;
        int64_t access_count = 0;
        int64_t candidates_found = 0;
        double avg_search_time_ms = 0.0;
        double efficiency_score = 0.0;
    };
    
    void recordLayerAccess(int layer, int candidates_found, double search_time_ms);
    int getOptimalEntryLayer() const;
    int getOptimalEf(size_t k) const;
    bool shouldPruneLayer(int current_layer, size_t candidate_count, size_t k) const;
    
private:
    std::map<int, LayerStats> layer_stats_;
    Config config_;
};
```

**Modified: `src/index/vector_index.cpp`**
```cpp
// searchKnn with layer pruning
std::vector<Result> VectorIndexManager::searchKnn(...) {
    #ifdef THEMIS_HNSW_ENABLED
    if (useHnsw_ && hnsw_optimizer_ && hnsw_optimizer_->isEnabled()) {
        auto* appr = static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
        
        // Get optimal entry layer
        int entry_layer = hnsw_optimizer_->getOptimalEntryLayer();
        int ef = hnsw_optimizer_->getOptimalEf(k);
        
        // Search with pruning
        for (int layer = entry_layer; layer >= 0; --layer) {
            auto start = std::chrono::steady_clock::now();
            
            // Search current layer
            auto candidates = searchLayer(layer, ...);
            
            auto duration = std::chrono::steady_clock::now() - start;
            hnsw_optimizer_->recordLayerAccess(layer, candidates.size(), 
                                               duration.count());
            
            // Pruning decision
            if (hnsw_optimizer_->shouldPruneLayer(layer, candidates.size(), k)) {
                break;  // Skip deeper layers
            }
        }
    }
    #endif
}
```

**Configuration:**
```yaml
hnsw_optimization:
  enabled: true
  
  layer_pruning:
    enabled: true
    threshold_multiplier: 5  # candidate_count > k * 5
  
  adaptive_layer_selection:
    enabled: true
    stats_window_size: 1000  # Moving window
  
  batch_insert:
    enabled: true
    batch_size: 100
```

---

## ✅ Testing Requirements

### Unit Tests
- [ ] Layer pruning logic
- [ ] Adaptive layer selection
- [ ] Statistics collection
- [ ] Configuration loading
- [ ] Batch insert buffering

### Integration Tests
- [ ] End-to-end search with pruning
- [ ] Batch insert with optimization
- [ ] Configuration changes
- [ ] Monitoring metrics

### Performance Tests
- [ ] **Baseline:** Search latency without optimization
- [ ] **Pruning:** Search latency with layer pruning
- [ ] **Adaptive:** Search latency with adaptive selection
- [ ] **Batch:** Insert throughput with batch optimization
- [ ] **Scale:** Performance at 10M, 100M, 1B vectors

### Test Datasets
- 10M vectors @ 384 dimensions (baseline)
- 100M vectors @ 384 dimensions (stress test)
- 1B vectors @ 384 dimensions (target scale)

---

## 📚 References

- **Design Document:** `docs/performance/phase4_hnsw_layer_optimization.md`
- **HNSW Paper:** Malkov & Yashunin (2018) - IEEE TPAMI
- **Library:** hnswlib - https://github.com/nmslib/hnswlib
- **Parent PR:** Scaling Optimizations to 10B Records
- **Configuration:** `config/scaling_optimizations.yaml`

---

## ⚠️ Risks & Considerations

1. **Pruning Too Aggressively**
   - Risk: Miss relevant candidates, reduced recall
   - Mitigation: Conservative threshold (k*5), thorough testing
   - Fallback: Configurable threshold, can disable

2. **Adaptive Layer Overhead**
   - Risk: Additional computation per query
   - Mitigation: Lightweight statistics, cached decisions
   - Validation: Benchmark overhead (<1%)

3. **Batch Insert Complexity**
   - Risk: Increased memory usage, delayed inserts
   - Mitigation: Bounded buffer size, automatic flush
   - Monitoring: Track buffer size and flush frequency

4. **Backward Compatibility**
   - Risk: Behavior changes affect existing users
   - Mitigation: All features disabled by default
   - Testing: Validate default behavior unchanged

---

## 🎯 Success Criteria

### Performance Targets

| Metric | Before | After | Target |
|--------|--------|-------|--------|
| Layer Traversal Count @ 1B | 9-10 layers | 6-7 layers | -30% |
| Search Latency @ 1B | ~15ms | ~10ms | -33% |
| Insert Throughput @ 1B | 300k/s | 350k/s | +17% |
| Vector Search Overhead @ 1B | -15% | -5% | **+67% efficiency** |

### Quality Gates
- [ ] Performance targets met on all test datasets
- [ ] Recall@10 ≥ 99% (vs baseline without optimizations)
- [ ] No performance regression on small datasets (<10M)
- [ ] All unit and integration tests pass
- [ ] Code review approved
- [ ] Documentation complete

---

## 📊 Monitoring & Metrics

**New Metrics to Track:**
- `hnsw_layer_traversals_avg` - Average layers traversed per search
- `hnsw_layer_pruning_rate` - % of searches that triggered pruning
- `hnsw_layer_efficiency_score` - Per-layer efficiency scores
- `hnsw_adaptive_entry_layer` - Current optimal entry layer
- `hnsw_batch_insert_buffer_size` - Pending inserts in buffer
- `hnsw_search_latency_p50/p95/p99` - Search latency percentiles

**Dashboard Updates:**
- Add HNSW optimization panel to Grafana
- Track layer statistics over time
- Compare before/after performance

---

## 🔄 Rollout Strategy

### Phase 1: Development Environment (Week 1-2)
- Implement and test all features
- Validate on synthetic datasets
- Performance benchmarks

### Phase 2: Staging (Week 3)
- Deploy with feature flags disabled
- Enable for canary collection
- Monitor for regressions

### Phase 3: Production Rollout (Week 4+)
- Gradual rollout (10% → 50% → 100%)
- Monitor metrics closely
- Ready to disable if issues arise

---

## 💡 Future Enhancements

- [ ] Machine learning-based pruning decisions
- [ ] Per-query-type optimization profiles
- [ ] Dynamic threshold adjustment based on load
- [ ] Integration with query cache (Phase 1)
- [ ] Cross-layer candidate deduplication
