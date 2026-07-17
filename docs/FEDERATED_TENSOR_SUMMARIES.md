# Federated and Cross-Shard Tensor Summaries Guide

## Overview

Federated and cross-shard tensor summaries enable efficient distributed retrieval in ThemisDB deployments by allowing query planning to happen at the summary level before requiring full data movement across shards.

## Concept

### Shard Summary

A **shard summary** is a compressed representation of retrieval results from a single shard, containing:

- **Shard metadata**: identifier, health status, latency
- **Candidate compression**: number of candidates before/after compression
- **Relevance metrics**: shard relevance score, retrieval quality indicators
- **Routing information**: routing reasons and fallback strategies

### Federated Summary

A **federated summary** aggregates shard summaries across multiple shards, enabling:

- **Cross-shard candidate deduplication**: Merge and deduplicate candidates from multiple shards
- **Summary-first routing**: Make routing decisions based on summaries before loading exact data
- **Efficient shard selection**: Select the most relevant shards without querying all shards

## Architecture

### Components

```
┌────────────────────────────────────────────────────────┐
│          Retrieval Query                               │
└────────────────────────────────────────────────────────┘
                        │
                        ▼
┌────────────────────────────────────────────────────────┐
│          TensorMidLayer::plan()                         │
│  - Classify scope (Adapter/Package/ShardSummary)       │
│  - Select routing strategy                             │
└────────────────────────────────────────────────────────┘
                        │
                        ▼
     ┌──────────────────┴──────────────────┐
     │                                     │
     ▼                                     ▼
┌─────────────┐                  ┌──────────────────┐
│Single Shard │                  │ Multiple Shards  │
│Summarize    │                  │ (Federated)      │
│             │                  │                  │
│summarize()  │                  │summarizeFederated│
│             │                  │Shards()          │
└─────────────┘                  └──────────────────┘
                                         │
                                         ▼
                        ┌─────────────────────────────┐
                        │ Cross-Shard Merging         │
                        │ mergeSimilarityResults()    │
                        │ - Deduplication            │
                        │ - Score aggregation        │
                        │ - Top-K selection          │
                        └─────────────────────────────┘
```

### Data Flow

1. **Context Preparation**: Build `TensorLayerContext` with shard scope IDs
2. **Shard Identification**: Classify scope as shard summary if scope_id starts with "shard:" or shard_aware=true
3. **Federated Summarization**: Call `summarizeFederatedShards()` to retrieve summaries from all shards
4. **Result Merging**: Deduplicate and aggregate results across shards
5. **Routing Decision**: Make routing decision based on merged summaries

## API Reference

### TensorLayerContext

```cpp
struct TensorLayerContext {
    std::string tenant_id;
    std::string domain;
    std::string base_model_id;
    std::string scope_id;
    std::vector<std::string> shard_scope_ids;  // For federated queries
    std::size_t top_k = 10;
    bool use_fingerprint_summary = true;
    bool shard_aware = false;                  // Set true for shard-aware routing
};
```

### FederatedTensorSummary

```cpp
struct FederatedTensorSummary {
    std::vector<TensorLayerSummary> shard_summaries;
    std::vector<SimilarityResult> merged_similar_adapters;
    std::string routing_reason;
};
```

### ShardSummary

```cpp
struct ShardSummary : public BaseTensorSummary {
    std::string shard_id;
    std::size_t candidates_before_compression = 0;
    std::size_t candidates_after_compression = 0;
    float shard_relevance = 0.0f;
    float retrieval_latency_ms = 0.0f;
    bool shard_healthy = true;
    std::string shard_routing_reason;
    std::vector<std::string> compressed_candidates;
};
```

### TensorMidLayer Methods

#### plan()
```cpp
[[nodiscard]] TensorLayerPlan plan(const TensorLayerContext& context) const noexcept;
```

Generates a routing plan for the given context.

**Parameters:**
- `context`: TensorLayerContext with scope information

**Returns:**
- TensorLayerPlan with layer kind, ANN scope kind, scope key, and routing reason

**Example:**
```cpp
TensorLayerContext context;
context.shard_scope_ids = {"shard_0", "shard_1", "shard_2"};
context.shard_aware = true;
context.top_k = 10;

auto plan = mid_layer_->plan(context);
assert(plan.layer_kind == TensorLayerKind::ShardSummary);
```

#### summarizeFederatedShards()
```cpp
[[nodiscard]] FederatedTensorSummary summarizeFederatedShards(
    const TensorLayerContext& context) const;
```

Retrieves and merges summaries from multiple shards.

**Parameters:**
- `context`: TensorLayerContext with shard_scope_ids populated

**Returns:**
- FederatedTensorSummary containing:
  - shard_summaries: Vector of TensorLayerSummary for each shard
  - merged_similar_adapters: Deduplicated and sorted results
  - routing_reason: Human-readable explanation

**Example:**
```cpp
TensorLayerContext context;
context.tenant_id = "tenant1";
context.domain = "legal";
context.base_model_id = "llama3";
context.shard_scope_ids = {"shard_eu", "shard_us", "shard_asia"};
context.shard_aware = true;
context.top_k = 20;

auto federated = mid_layer_->summarizeFederatedShards(context);
ASSERT_EQ(federated.shard_summaries.size(), 3);

// Access merged results
for (const auto& adapter : federated.merged_similar_adapters) {
    std::cout << "Adapter: " << adapter.adapter_key 
              << ", Score: " << adapter.score << std::endl;
}
```

## Usage Patterns

### Pattern 1: Single-Shard Query

```cpp
TensorMidLayer mid_layer;
TensorLayerContext context;
context.tenant_id = "tenant1";
context.domain = "domain1";
context.base_model_id = "model1";
context.scope_id = "shard:shard_0";
context.top_k = 10;

auto summary = mid_layer.summarize(context);
// Process single shard results
```

### Pattern 2: Multi-Shard Federated Query

```cpp
TensorMidLayer mid_layer;
TensorLayerContext context;
context.tenant_id = "tenant1";
context.domain = "domain1";
context.base_model_id = "model1";
context.shard_scope_ids = {"shard_0", "shard_1", "shard_2"};
context.shard_aware = true;
context.top_k = 20;

auto federated = mid_layer.summarizeFederatedShards(context);

// Process shard-level information
for (const auto& shard_summary : federated.shard_summaries) {
    if (shard_summary.shard_healthy) {
        std::cout << "Shard " << shard_summary.scope_key 
                  << ": " << shard_summary.candidate_count 
                  << " candidates" << std::endl;
    }
}

// Use merged results for final ranking
for (const auto& adapter : federated.merged_similar_adapters) {
    // Load adapter if score meets threshold
}
```

### Pattern 3: Summary-First Retrieval Flow

```cpp
// Step 1: Plan the retrieval
TensorLayerContext context;
context.shard_scope_ids = {"shard_0", "shard_1", "shard_2"};
context.shard_aware = true;
context.top_k = 10;

auto plan = mid_layer.plan(context);
assert(plan.layer_kind == TensorLayerKind::ShardSummary);

// Step 2: Get federated summary
auto federated = mid_layer.summarizeFederatedShards(context);

// Step 3: Decide which shards to query exactly based on summary
std::vector<std::string> selected_shards;
float relevance_threshold = 0.7f;
for (const auto& shard : federated.shard_summaries) {
    if (shard.shard_healthy && shard.shard_relevance >= relevance_threshold) {
        selected_shards.push_back(shard.scope_key);
    }
}

// Step 4: Query selected shards exactly
for (const auto& shard_id : selected_shards) {
    // Execute exact query against this shard
}
```

## Compression and Size Optimization

### Candidate Compression

Shard summaries support compression to reduce network bandwidth and storage:

```cpp
CompressionResult result;
result.success = true;
result.compression_ratio = 4.0f;  // Compress to 1/4 size

ShardSummary summary = SummaryFactory::createShardSummary(
    "shard_0",
    100,  // Original candidates
    result
);

// After compression: 100 / 4.0 = 25 candidates
assert(summary.candidates_after_compression == 25);
```

### Size Estimation

For planning purposes, estimate federated summary size:

```cpp
size_t estimate_federated_summary_size(
    const TensorLayerContext& context,
    float avg_compression_ratio = 4.0f) {
    // Each shard summary: ~1KB metadata + candidates
    size_t per_shard = 1024 + (context.top_k * sizeof(SimilarityResult));
    return context.shard_scope_ids.size() * per_shard;
}
```

## False Negative Risk Management

### Understanding False Negatives

When using summary-first retrieval with compression:

- **False Negative**: A relevant adapter is missed because it was compressed away from the shard summary
- **Risk Factors**:
  - High compression ratio (more aggressive = higher risk)
  - Small number of candidates in summary
  - Domain-specific relevance that doesn't correlate with score

### Mitigation Strategies

1. **Conservative Compression**
   ```cpp
   // Use lower compression ratio for critical domains
   float compression_ratio = 1.5f;  // Keep more candidates
   ```

2. **Relevance Thresholding**
   ```cpp
   // Only select shards with minimum relevance
   float shard_relevance_threshold = 0.8f;
   ```

3. **Fallback to Full Query**
   ```cpp
   // If summary-based results are uncertain, query all shards
   if (federated.merged_similar_adapters.size() < context.top_k) {
       // Fall back to full distributed query
   }
   ```

## Integration with ANN Frontdoor

Shard summaries integrate with ANN retrieval planning:

```cpp
index::AnnQueryContext ann_context;
ann_context.scope_id = "shard:shard_0";
ann_context.shard_aware = true;

auto plan = ann_frontdoor->planRetrieval(ann_context);
assert(plan.scope_kind == index::AnnScopeKind::ShardSummary);
assert(plan.strategy == index::AnnStrategy::DISTRIBUTED);
```

## Monitoring and Observability

### Key Metrics

- **shard_summary_creation_time_ms**: Time to create shard summary
- **federated_merge_time_ms**: Time to merge summaries across shards
- **compression_ratio**: Achieved compression in summaries
- **false_negative_probability**: Estimated FN risk based on compression
- **shard_availability**: Percentage of healthy shards
- **summary_accuracy**: Correlation between summary ranking and exact ranking

### Example Monitoring

```cpp
struct SummaryMetrics {
    float creation_time_ms;
    float merge_time_ms;
    float compression_ratio;
    float false_negative_risk;  // 0.0 = no risk, 1.0 = high risk
    size_t shards_queried;
    size_t shards_healthy;
};
```

## Performance Characteristics

### Summary-First Query vs. Full Fan-Out

| Aspect | Summary-First | Full Fan-Out |
|--------|--|--|
| Latency | Lower | Higher |
| Bandwidth | Lower | Higher |
| False Negatives | Possible | None |
| Selectivity | Dynamic | Broad |
| Cost | Lower | Higher |

## Best Practices

1. **Use appropriate compression ratios** for your domain and SLA requirements
2. **Monitor false-negative risk** and adjust thresholds accordingly
3. **Fallback to full queries** when summary results are uncertain
4. **Cache shard summaries** to avoid redundant computation
5. **Use shard health checks** to exclude unhealthy replicas
6. **Measure summary accuracy** against exact queries periodically

## References

- `include/tensor/tensor_mid_layer.h`: TensorMidLayer interface
- `include/tensor/tensor_summary_types.h`: Summary type definitions
- `src/tensor/tensor_mid_layer.cpp`: Implementation
- `tests/tensor/test_federated_tensor_summaries.cpp`: Comprehensive test suite
- `DISTRIBUTED_TENSOR_SHARDING.md`: Distributed sharding design
- `HARDWARE_REQUIREMENTS.md`: Hardware considerations for distributed deployments
