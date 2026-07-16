# Tensor Mid-Layer Abstractions - Design & Implementation Guide

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Core Abstractions](#core-abstractions)
4. [Compression Strategies](#compression-strategies)
5. [Tensor Summary Types](#tensor-summary-types)
6. [Routing & Prioritization](#routing--prioritization)
7. [Redundancy Detection](#redundancy-detection)
8. [Usage Examples](#usage-examples)
9. [Error Handling](#error-handling)
10. [Performance Considerations](#performance-considerations)

## Overview

The Tensor Mid-Layer provides a unified abstraction layer between ANN retrieval and graph validation in ThemisDB. Its responsibilities include:

- **Candidate Compression**: Reduce candidate sets through TT decomposition, quantization, and sampling
- **Redundancy Reduction**: Detect and remove duplicate candidates using similarity, hashing, and metadata
- **Routing**: Direct results to appropriate downstream layers based on quality metrics
- **Prioritization**: Rank candidates by similarity, freshness, and cost efficiency
- **Summary Generation**: Create compact tensor summaries of retrieved candidates

## Architecture

```
┌─────────────────────────────────────────────────────┐
│           ANN Frontdoor                             │
│  (Returns candidates with distances)               │
└─────────────────┬───────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────┐
│      Tensor Mid-Layer (NEW)                         │
│  ┌──────────────────────────────────────────────┐  │
│  │ 1. Apply Compression Strategy                │  │
│  │    - TT Decomposition                        │  │
│  │    - Quantization                            │  │
│  │    - Sampling                                │  │
│  └──────────────────────────────────────────────┘  │
│                   │                                 │
│  ┌────────────────▼──────────────────────────────┐ │
│  │ 2. Create Summary Objects                     │ │
│  │    - Adapter/Package/Shard/Entity/Chunk      │ │
│  │    - With compression metrics                │ │
│  └──────────────────────────────────────────────┘ │
│                   │                                 │
│  ┌────────────────▼──────────────────────────────┐ │
│  │ 3. Detect & Remove Redundancy                │ │
│  │    - Similarity-based                        │ │
│  │    - Content-hash-based                      │ │
│  │    - Metadata-based                          │ │
│  └──────────────────────────────────────────────┘ │
│                   │                                 │
│  ┌────────────────▼──────────────────────────────┐ │
│  │ 4. Prioritize & Sort Candidates              │ │
│  │    - By similarity score                     │ │
│  │    - By freshness                            │ │
│  │    - By cost efficiency                      │ │
│  └──────────────────────────────────────────────┘ │
│                   │                                 │
│  ┌────────────────▼──────────────────────────────┐ │
│  │ 5. Make Routing Decision                     │ │
│  │    - To graph validation                     │ │
│  │    - To fallback/enrichment                  │ │
│  │    - Shard-aware routing                     │ │
│  └──────────────────────────────────────────────┘ │
└─────────────────┬───────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────┐
│      Graph Validation Layer                         │
│  (Returns graph-validated results)                 │
└─────────────────────────────────────────────────────┘
```

## Core Abstractions

### 1. Compression Strategy (`ICompressionStrategy`)

**Purpose**: Define and apply compression algorithms to reduce candidate size and storage.

**Interface**:
```cpp
class ICompressionStrategy {
    virtual std::string name() const = 0;
    virtual CompressionResult compress(const float* data, size_t dim, 
                                      const std::vector<size_t>& mode_sizes,
                                      const CompressionConfig& config) const = 0;
    virtual CompressionResult compressTTTrain(const storage::TTTrain& train,
                                             const CompressionConfig& config) const = 0;
    virtual float estimateRatio(const float* data, size_t dim,
                               const CompressionConfig& config) const = 0;
};
```

**Concrete Implementations**:

| Strategy | Use Case | Compression Ratio | Error | Notes |
|----------|----------|-------------------|-------|-------|
| TT Decomposition | General-purpose | 2-10x | Configurable (epsilon) | Best for structured data |
| Quantization (INT8) | Fast inference | 4x | ~1% | Suitable for ML models |
| Quantization (INT16) | High precision | 2x | <0.1% | Good balance |
| Sampling | Size reduction | Variable | 0% (exact subset) | Loses candidates |
| Hashing | Fingerprinting | 1000x+ | High (approximation) | For deduplication |

### 2. Tensor Summary Types

All summary types inherit from `BaseTensorSummary` with common fields:
- `id`: Unique identifier
- `tenant_id`: Tenant namespace
- `domain`: Domain tag (legal, medical, etc.)
- `similarity_score`: Score in [0, 1]
- `confidence`: Confidence in score
- `compression_strategy`: Strategy used
- `compression_info`: Compression metrics

**Concrete Types**:

#### AdapterSummary
For LoRA/PEFT adapters:
```cpp
struct AdapterSummary : BaseTensorSummary {
    std::string adapter_key;      // Storage key
    std::string base_model_id;    // llama3-8b, gpt-4-turbo, etc.
    std::size_t param_count;      // Total parameters
    std::size_t avg_tt_rank;      // Average TT rank
    float adapter_norm;           // Frobenius norm
    std::vector<float> fingerprint;
    bool inference_ready;
};
```

#### PackageSummary
For adapter packages:
```cpp
struct PackageSummary : BaseTensorSummary {
    std::string package_id;
    std::size_t adapter_count;
    std::vector<std::string> adapter_keys;
    float internal_similarity;    // Inter-adapter similarity
    std::vector<float> package_fingerprint;
    std::string version;
};
```

#### ShardSummary
For cross-shard results:
```cpp
struct ShardSummary : BaseTensorSummary {
    std::string shard_id;
    std::size_t candidates_before_compression;
    std::size_t candidates_after_compression;
    float shard_relevance;
    float retrieval_latency_ms;
    bool shard_healthy;
};
```

#### EntitySummary
For knowledge graph entities:
```cpp
struct EntitySummary : BaseTensorSummary {
    std::string entity_id;
    std::string entity_type;      // Person, Organization, etc.
    std::string entity_label;
    std::size_t relationship_count;
    std::vector<std::string> related_entity_ids;
    float centrality_score;
};
```

#### ChunkSummary
For document chunks:
```cpp
struct ChunkSummary : BaseTensorSummary {
    std::string chunk_id;
    std::string document_id;
    std::string document_title;
    std::string chunk_content;    // May be truncated
    float bm25_score;
    float tfidf_score;
    bool verified_source;
};
```

#### FingerprintSummary
For LSH fingerprint-based results:
```cpp
struct FingerprintSummary : BaseTensorSummary {
    std::string fingerprint_hash;
    uint8_t fingerprint_bits;     // 64, 128, 256, etc.
    std::string hash_function;    // SHA256, MINHASH, etc.
    std::size_t candidates_matched;
    uint32_t hamming_distance;    // To query fingerprint
    float collision_probability;
};
```

### 3. Routing Strategy (`IRoutingStrategy`)

**Purpose**: Decide where tensor-layer results should be forwarded.

**Output**: `RoutingDecision` with:
- `primary_target`: Main routing destination (GRAPH_VALIDATION, FALLBACK, CACHE, etc.)
- `fallback_target`: Backup destination if primary fails
- `confidence`: Decision confidence (0.0-1.0)
- `priority`: Processing priority (0-100)
- `reason_code`: Machine-readable reason

**Concrete Implementations**:

| Strategy | Selection Criteria | Use Case |
|----------|-------------------|----------|
| QualityBased | Confidence + compression ratio | General-purpose |
| ShardAware | Shard health + latency | Federated queries |
| Adaptive | Learned performance metrics | Long-running services |

### 4. Prioritization Strategy (`IPrioritizationStrategy`)

**Purpose**: Assign priority scores to candidates for ranking.

**Concrete Implementations**:

| Strategy | Factors | Suitable For |
|----------|---------|-------------|
| SimilarityBased | Similarity score + confidence | Standard retrieval |
| RankBased | Rank position + freshness | Time-sensitive queries |
| CostBased | Quality + processing cost | Latency-critical scenarios |

### 5. Redundancy Detection (`IRedundancyDetector`)

**Purpose**: Identify and remove duplicate candidates.

**Output**: `RedundancyMetrics` with:
- `total_candidates`: Input count
- `redundant_count`: Duplicates found
- `unique_count`: Unique after deduplication
- `redundancy_ratio`: Ratio of redundancy

**Concrete Implementations**:

| Strategy | Detection Method | Best For |
|----------|-----------------|----------|
| SimilarityBased | Score difference threshold | Quick filtering |
| ContentHash | Cryptographic hash | Exact duplicates |
| EmbeddingBased | Cosine similarity on embeddings | Semantic duplicates |
| MetadataBased | Field comparison | Structural patterns |
| Composite | Weighted combination | Production systems |

## Compression Strategies

### TT Decomposition

Converts tensors to low-rank Tensor-Train format:

```cpp
TTDecompositionStrategy strategy;
CompressionConfig config;
config.tt_epsilon = 0.01f;        // Reconstruction error tolerance
config.max_tt_rank = 16;           // Maximum rank

auto result = strategy.compress(data, dim, mode_sizes, config);
// Returns: compression_ratio, achieved_error, achieved_rank
```

**When to use**:
- General-purpose compression
- High-dimensional data
- Need to preserve exact semantics with bounded error

**Trade-offs**:
- ✓ High compression (2-10x)
- ✓ Configurable accuracy
- ✗ Slower than quantization
- ✗ Requires tensor structure knowledge

### Quantization

Reduces bit-depth (float32 → INT8/INT16):

```cpp
QuantizationStrategy strategy(8);  // 8-bit quantization
CompressionConfig config;

auto result = strategy.compress(data, dim, mode_sizes, config);
// Returns: compression_ratio (4x for INT8)
```

**When to use**:
- Fast inference on specialized hardware
- Memory-constrained environments
- Acceptable ~1% accuracy loss

**Trade-offs**:
- ✓ Very fast
- ✓ Hardware-accelerated
- ✗ Information loss
- ✗ Not invertible

### Sampling

Probabilistic candidate reduction:

```cpp
SamplingStrategy strategy(0.5f);   // Keep 50% of candidates
CompressionConfig config;

auto result = strategy.compress(data, dim, mode_sizes, config);
// Returns: compression_ratio = 1/sampling_ratio
```

**When to use**:
- Too many candidates from ANN
- Trade-off accuracy for throughput
- Results are representative sample

**Trade-offs**:
- ✓ Simple, no approximation error
- ✓ Exact subset of original data
- ✗ Loses information
- ✗ Results are stochastic

### Hashing

Creates fixed-size fingerprints:

```cpp
HashingStrategy strategy(64);      // 64-bit fingerprints
CompressionConfig config;

auto result = strategy.compress(data, dim, mode_sizes, config);
// Returns: extreme compression for deduplication
```

**When to use**:
- Deduplication
- Quick fingerprinting
- Approximate matching

**Trade-offs**:
- ✓ Extreme compression (1000x+)
- ✓ Fast comparison
- ✗ High false positive rate
- ✗ Not reversible

## Tensor Summary Types

### AdapterSummary Example

```cpp
// Create from compression result
CompressionResult comp_result;
comp_result.compression_ratio = 2.5f;
comp_result.achieved_rank = 8;

auto summary = SummaryFactory::createAdapterSummary(
    "__adapters__:t1:legal:llama3",
    "llama3-8b",
    comp_result);

// Summary contains:
// - Compressed TT representation
// - Metadata (tenant, domain, model)
// - Compression metrics
// - Fingerprint for similarity search
```

### PackageSummary Example

```cpp
std::vector<std::string> adapters = {
    "__adapters__:t1:legal:llama3",
    "__adapters__:t1:legal:gpt4",
    "__adapters__:t1:legal:claude"
};

auto summary = SummaryFactory::createPackageSummary(
    "legal_package_v1",
    adapters);

// Summary aggregates:
// - All adapter keys
// - Cross-adapter similarity metrics
// - Package-level fingerprint
```

### ShardSummary Example

```cpp
CompressionResult shard_compression;
shard_compression.compression_ratio = 1.5f;

auto summary = SummaryFactory::createShardSummary(
    "shard_0",
    5000,  // Candidates before compression
    shard_compression);

// Summary tracks:
// - Shard health and latency
// - Compression effectiveness
// - Routing reason
```

## Routing & Prioritization

### QualityBasedRouting

```cpp
QualityBasedRouting routing;
routing.confidence_threshold = 0.7f;
routing.compression_ratio_threshold = 2.0f;

RoutingDecision decision = routing.route(
    summaries,
    candidate_count,
    compression_ratio,
    query_context);

if (decision.primary_target == "GRAPH_VALIDATION") {
    // High-confidence results → direct to graph
    graph_validator.validate(summaries);
} else if (decision.primary_target == "FALLBACK") {
    // Low-confidence results → fallback enrichment
    enrichment_service.enhance(summaries);
}
```

### SimilarityBasedPrioritization

```cpp
SimilarityBasedPrioritization prioritization;
prioritization.similarity_weight = 0.7f;
prioritization.confidence_weight = 0.3f;

std::vector<float> priorities = prioritization.prioritize(summary_ptrs);

// Sort by priority
prioritization.sort(summaries);

// Top-k candidates are now first
for (size_t i = 0; i < std::min(10UL, summaries.size()); ++i) {
    process(summaries[i]);
}
```

### ShardAwareRouting

```cpp
ShardAwareRouting routing;
routing.latency_threshold_ms = 100.0f;
routing.health_threshold = 0.8f;

auto decision = routing.route(summaries, count, ratio, context);

// Hints for healthy, low-latency shards
for (const auto& shard_id : decision.shard_hints) {
    query_shard(shard_id);
}
```

## Redundancy Detection

### SimilarityBasedDetection

```cpp
SimilarityBasedDetector detector;

// Detect redundancy
auto metrics = detector.detect(summary_ptrs, 0.05f);
std::cout << "Redundant: " << metrics.redundant_count << "\n";
std::cout << "Unique: " << metrics.unique_count << "\n";

// Remove redundant candidates
auto removed_indices = detector.deduplicate(summaries, 0.05f);
```

### CompositeDetection

```cpp
auto detector = RedundancyFactory::createDefaultComposite();

// Combines:
// - 40% Similarity-based (quick)
// - 35% Content-hash-based (exact)
// - 25% Metadata-based (structural)

auto metrics = detector->detect(summary_ptrs, 0.1f);
auto removed = detector->deduplicate(summaries, 0.1f);
```

## Usage Examples

### Complete Tensor Mid-Layer Pipeline

```cpp
#include "tensor/compression_strategy.h"
#include "tensor/tensor_routing_strategy.h"
#include "tensor/tensor_redundancy_detection.h"
#include "tensor/tensor_summary_types.h"

// 1. Get candidates from ANN
index::AnnFrontdoorResult ann_result = ann_frontdoor.search(query, k);

// 2. Compress candidates
auto compression = CompressionFactory::create("TT_DECOMPOSITION");
CompressionConfig config;
config.tt_epsilon = 0.01f;

std::vector<BaseTensorSummary> summaries;
for (const auto& candidate : ann_result.candidates) {
    auto comp_result = compression->compress(
        candidate.vector_data, 
        candidate.dimension, 
        candidate.mode_sizes, 
        config);
    
    auto summary = SummaryFactory::createAdapterSummary(
        candidate.id, 
        candidate.base_model_id, 
        comp_result);
    summaries.push_back(summary);
}

// 3. Detect redundancy
auto redundancy_detector = RedundancyFactory::createDefaultComposite();
auto metrics = redundancy_detector->detect(
    reinterpret_cast<std::vector<const BaseTensorSummary*>>(summaries), 
    0.1f);
std::cout << "Redundancy ratio: " << metrics.redundancy_ratio << "\n";

// Remove duplicates
redundancy_detector->deduplicate(summaries, 0.1f);

// 4. Prioritize candidates
auto prioritizer = RoutingFactory::createPrioritization("SIMILARITY_BASED");
prioritizer->sort(summaries);

// 5. Make routing decision
auto router = RoutingFactory::createRouting("QUALITY_BASED");
auto routing_decision = router->route(
    summaries, 
    ann_result.candidates.size(),
    compression->estimateRatio(nullptr, 0, config),
    ann_result);

std::cout << "Route to: " << routing_decision.primary_target << "\n";
std::cout << "Confidence: " << routing_decision.confidence << "\n";

// 6. Forward to downstream layer
if (routing_decision.primary_target == "GRAPH_VALIDATION") {
    graph_validator.validate(summaries);
} else {
    fallback_enrichment.enhance(summaries);
}
```

## Error Handling

### Compression Error Handling

```cpp
CompressionResult result = strategy->compress(data, dim, mode_sizes, config);

if (!result.success) {
    THEMIS_ERROR("Compression failed: {}", result.error_message);
    
    // Fallback to simpler strategy
    auto fallback = CompressionFactory::create("SAMPLING");
    result = fallback->compress(data, dim, mode_sizes, config);
    
    if (!result.success) {
        // Last resort: no compression
        return original_data;
    }
}
```

### Routing Error Handling

```cpp
RoutingDecision decision = router->route(
    summaries, count, ratio, context);

try {
    if (decision.primary_target == "GRAPH_VALIDATION") {
        graph_validator.validate(summaries);
    } else {
        fallback_service.process(summaries);
    }
} catch (const std::exception& e) {
    THEMIS_WARN("Primary routing failed: {}", e.what());
    
    if (router->shouldRetryOnFailure(e.what(), 0, 3)) {
        // Retry with fallback target
        fallback_service.process(summaries);
    } else {
        throw;
    }
}
```

## Performance Considerations

### Compression Performance

| Strategy | Throughput | Latency | Memory | CPU |
|----------|-----------|---------|--------|-----|
| Sampling | Very high | <1ms | Low | Low |
| Quantization | High | 1-5ms | Low | Medium |
| Hashing | High | 2-10ms | Very low | Medium |
| TT Decomposition | Medium | 10-100ms | Medium | High |

### Redundancy Detection Performance

| Strategy | Throughput | Precision | Recall |
|----------|-----------|-----------|--------|
| Similarity-based | Very high | Medium | Low |
| Content-hash | High | Very high | High |
| Embedding-based | Medium | High | Medium |
| Composite | Medium | Very high | Very high |

### Recommendations

1. **For speed-critical paths**: Use Sampling + Similarity-based detection
2. **For accuracy-critical paths**: Use TT + Composite detection
3. **For balanced scenarios**: Use Quantization + Content-hash detection
4. **For long-running services**: Use Adaptive routing with learned metrics

### Tuning Guidelines

```cpp
// Speed optimization
CompressionConfig fast_config;
fast_config.tt_epsilon = 0.05f;      // Higher error tolerance
fast_config.max_tt_rank = 4;         // Lower rank

// Accuracy optimization
CompressionConfig accurate_config;
accurate_config.tt_epsilon = 0.001f; // Lower error tolerance
accurate_config.max_tt_rank = 32;    // Higher rank

// Balanced
CompressionConfig balanced_config;
balanced_config.tt_epsilon = 0.01f;
balanced_config.max_tt_rank = 8;
```

---

**Document Version**: 1.0  
**Last Updated**: 2026-07-01  
**Status**: Production Ready
