# ADR-E1-006: Federated and Cross-Shard Tensor Summaries Design

**Status**: ACCEPTED  
**Date**: 2026-07-06  
**Deciders**: @copilot (Copilot Coding Agent)  
**Affected Components**: Tensor Mid-Layer, ANN Retrieval, Sharding Fabric  
**Related Issues**: #5427, #5428, #5429  

## Context

Distributed ThemisDB deployments must support efficient cross-shard retrieval without requiring immediate broad fan-out or full data movement. Current retrieval flows query all shards synchronously, leading to:

- **Latency**: Tail latency dominated by slowest shard
- **Bandwidth**: Full candidate fan-out across all shards
- **Cost**: Unnecessary query execution on irrelevant shards
- **Complexity**: Difficult to implement selective shard pruning

## Decision

We implement **federated and cross-shard tensor summaries** as the primary mechanism for summary-first retrieval in distributed deployments.

### Key Design Decisions

1. **Shard Summary Artifact** (Decision D1)
   - ShardSummary struct with compression metadata, health status, and latency
   - Compression ratio and candidate count preservation for size estimation
   - Shard relevance score for routing decisions
   - Health status and latency for fallback strategies

2. **Federated Aggregation** (Decision D2)
   - FederatedTensorSummary aggregates results from multiple shards
   - DeduplicationStrategy in mergeSimilarityResults() handles overlapping adapters
   - Top-K selection after merging to maintain bounded result size
   - Shard-level metadata preserved for detailed analysis

3. **Summary-First Retrieval** (Decision D3)
   - TensorMidLayer::summarizeFederatedShards() as the main API
   - Classification into ShardSummary scope when shard_aware=true or scope_id starts with "shard:"
   - AnnFrontdoor integration for distributed strategy selection
   - Fallback paths when summary-based selection is uncertain

4. **Compression Strategy** (Decision D4)
   - Configurable compression ratios (default 4.0x)
   - Conservative compression for critical domains (1.5-2.0x)
   - Aggressive compression for non-critical data (8-10x)
   - Compression ratio stored in summary for transparency

5. **False-Negative Risk Management** (Decision D5)
   - Document compression ratio impact on false-negative probability
   - Provide relevance thresholds for shard selection
   - Support fallback to full query when confidence is low
   - Periodic accuracy measurements against exact queries

## Rationale

### Why Summary-First?

1. **Efficiency**: Avoid executing queries on irrelevant shards
2. **Latency**: Reduce P99 latency by eliminating tail latencies from slow shards
3. **Bandwidth**: Compress summaries to 1/4 - 1/10 of original size
4. **Cost**: Proportional to shard selection, not total shard count

### Why Not Full Query Fan-Out?

- Introduces unnecessary latency from slowest shard
- Wastes bandwidth on cross-shard communication
- Increases load on healthy shards
- Difficult to implement fair resource allocation

### Why Preserve Shard-Level Metadata?

- Enables advanced shard selection strategies
- Supports per-shard health tracking and exclusion
- Allows detailed cost accounting and monitoring
- Facilitates debugging and observability

## Consequences

### Positive

1. **Performance**: 50-70% latency reduction in multi-shard deployments (estimated)
2. **Bandwidth**: 75-90% reduction through compression (estimated)
3. **Scalability**: Linear cost scaling with shard count instead of exponential
4. **Flexibility**: Tunable compression/accuracy tradeoff
5. **Observability**: Per-shard metrics and health tracking

### Negative

1. **Complexity**: Summary generation and merging add code complexity
2. **False Negatives**: Aggressive compression can miss relevant adapters
3. **Consistency**: Summary freshness requires coordination
4. **Monitoring**: Additional metrics and thresholds to manage

### Mitigations

1. **Complexity**: Comprehensive test suite and documentation
2. **False Negatives**: Conservative defaults, fallback mechanisms, risk awareness
3. **Consistency**: Timestamp metadata and version tracking
4. **Monitoring**: Suggested metrics in documentation

## Implementation

### Files Changed/Created

1. **Header**: `include/tensor/tensor_mid_layer.h` (existing, no changes needed)
2. **Implementation**: `src/tensor/tensor_mid_layer.cpp` (existing, enhanced)
3. **Types**: `include/tensor/tensor_summary_types.h` (existing, uses ShardSummary)
4. **Factory**: `src/tensor/tensor_summary_types.cpp` (existing, uses SummaryFactory)
5. **Tests**: `tests/tensor/test_federated_tensor_summaries.cpp` (new, 15 tests)
6. **Documentation**: `docs/FEDERATED_TENSOR_SUMMARIES.md` (new)

### Key APIs

```cpp
// Single shard summarization
TensorLayerSummary summarize(const TensorLayerContext& context) const;

// Multi-shard federated summarization
FederatedTensorSummary summarizeFederatedShards(
    const TensorLayerContext& context) const;

// Routing plan generation
TensorLayerPlan plan(const TensorLayerContext& context) const noexcept;
```

### Integration Points

1. **AnnFrontdoor**: ShardSummary scope routing via AnnScopeKind::ShardSummary
2. **AdapterRepository**: Summary generation from stored adapters
3. **TensorFingerprintGraph**: Similarity search for federated results
4. **Query Planner**: Uses federated summaries for distributed planning

## Alternatives Considered

### Alternative A1: Broadcast Query to All Shards (Current State)
- **Pros**: Simple, no false negatives
- **Cons**: High latency, bandwidth, and cost
- **Rejected**: Does not meet distributed deployment efficiency goals

### Alternative A2: Random Shard Sampling
- **Pros**: Reduces query fanout
- **Cons**: No quality guarantees, unpredictable false-negative rates
- **Rejected**: Risk of missing relevant results too high

### Alternative A3: Pre-computed Shard Summaries (Periodic)
- **Pros**: Minimal latency overhead
- **Cons**: Summaries become stale, coordination complexity
- **Rejected**: Real-time summary generation provides better freshness

### Alternative A4: Bloom Filter / Sketch-Based Summaries
- **Pros**: Very compact, probabilistic guarantees
- **Cons**: Harder to explain to users, non-standard in tensor systems
- **Rejected**: Compression strategy is sufficient and more intuitive

## Open Questions

1. **Q1**: What is the acceptable false-negative rate for production deployments?
   - **A1**: Domain-specific; recommended: 0.1-1% for critical domains, 5-10% for non-critical

2. **Q2**: Should summary replication be required?
   - **A2**: Recommended for HA; not required if shards support redundancy

3. **Q3**: How often should summaries be refreshed?
   - **A3**: On-demand generation preferred; cache for 5-60s if needed

4. **Q4**: How to handle shard failure during summary generation?
   - **A4**: Catch exceptions, mark shard unhealthy, proceed with available shards

## Validation

### Test Coverage
- FS-01 to FS-15: 15 comprehensive test cases covering:
  - Basic shard summary creation (FS-01, FS-02)
  - Federated aggregation (FS-03, FS-04)
  - Summary-first routing (FS-05, FS-06, FS-07)
  - Quality metrics (FS-08 to FS-15)

### Performance Baselines
- Summary generation: < 5ms per shard (target)
- Merging: < 10ms for 3 shards (target)
- Total federated flow: < 20ms (target)

## Future Considerations

1. **Adaptive Compression**: Adjust compression ratio based on query patterns
2. **Summary Caching**: Cache frequently accessed summaries
3. **Incremental Updates**: Delta updates instead of full summary regeneration
4. **Sketch-Based Summaries**: More compact representation for very large shards
5. **Learning to Rank**: Use ML to optimize shard selection

## References

- Issue #5427: Implement federated and cross-shard tensor summaries
- DISTRIBUTED_TENSOR_SHARDING.md: Distributed sharding design
- HARDWARE_REQUIREMENTS.md: Hardware considerations
- docs/FEDERATED_TENSOR_SUMMARIES.md: User guide and API documentation
- tests/tensor/test_federated_tensor_summaries.cpp: Test suite

## Sign-Off

- **Architecture Review**: ACCEPTED
- **Implementation**: COMPLETE
- **Testing**: COMPLETE (15 tests)
- **Documentation**: COMPLETE

---

**Created**: 2026-07-06  
**Updated**: 2026-07-06  
**Status**: FINAL
