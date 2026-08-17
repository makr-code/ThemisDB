# Tensor Mid-Layer Abstractions: Phase 4 Integration Guide

## Overview

Phase 4 completes the tensor mid-layer abstraction implementation by integrating all components into a cohesive production-ready system. This guide describes the architecture, integration points, performance baselines, and operational considerations.

## Architecture & Components

### 1. Core Tensor Mid-Layer (`TensorMidLayer`)

**Responsibility**: Orchestrate tensor-layer routing decisions based on context classification.

**Key Methods**:
- `plan(context)` — Classify scope and produce routing plan
- `summarize(context)` — Retrieve similar adapters and produce summary
- `summarizeFederatedShards(context)` — Merge results from multiple shard scopes

**Scope Classification**:
- `Adapter` — Single-model adapter scope (default)
- `Package` — Multi-model package scope (prefix: `pkg:` or `package:`)
- `ShardSummary` — Cross-shard federated scope (prefix: `shard:` or `shard_aware=true`)
- `FingerprintSummary` — Fingerprint-graph-optimized scope

### 2. Adapter Repository (`AdapterRepository`)

**Responsibility**: Store and retrieve tensor train adapters with tenant isolation.

**Key Methods**:
- `store(tenant_id, domain, adapter_key, core)` — Store adapter under tenant scope
- `loadAdapter(tenant_id, domain, adapter_key)` — Load adapter by key
- `findSimilarAdapters(domain, base_model_id, top_k)` — Find similar adapters
- `listDomains()` — List registered domains

**Guarantees**:
- Tenant isolation: adapters stored under tenant_id are not visible to other tenants
- Deduplication: identical adapter keys overwrite previous versions
- Indexing: efficient retrieval with O(log n) lookup

### 3. Fingerprint Graph (`TensorFingerprintGraph`)

**Responsibility**: Build and query a similarity graph for tensor train fingerprints.

**Key Methods**:
- `addAdapter(key, core, tenant_id, domain)` — Register adapter in graph
- `findSimilar(query_key, top_k)` — Find top-k similar adapters
- `findSimilarByFingerprint(fingerprint, tenant_id, top_k)` — Query by fingerprint
- `removeAdapter(key)` — Remove adapter from graph
- `entry(key)` — Retrieve fingerprint metadata

**Similarity Metrics**:
- Cosine distance between fingerprints (default: Euclidean)
- Customizable via `setExactSimilarityFn()`
- Excludes query adapter from results

### 4. Error Handling (`TensorErrorHandler`, Guards, Monitor)

**Responsibility**: Centralized error management, recovery strategies, and resilience monitoring.

**Key Components**:
- `TensorErrorHandler` — Central error dispatch and recovery callback registry
- `CompressionGuard` — RAII wrapper for safe compression with fallback
- `RoutingGuard` — RAII wrapper for safe routing decisions
- `ResilienceMonitor` — Track health metrics, recovery statistics, and thresholds

**Recovery Strategy**:
1. Detect error condition
2. Invoke registered recovery callback
3. Log event to audit trail
4. Update health metrics
5. Fallback to degraded mode if available

### 5. Compression Strategies

**Available Algorithms**:
- `TT_SVD` — Tensor Train Singular Value Decomposition (primary)
- `QUANTIZATION` — INT8/INT16 quantization (fallback 1)
- `SAMPLING` — Stochastic sampling (fallback 2)
- `HASHING` — Fingerprint-based (fallback 3)

**Fallback Chain**:
```
TT-SVD → Quantization → Sampling → Hashing → NoOp
```

### 6. Routing Strategies

**Available Strategies**:
- `QualityBasedRoutingStrategy` — Sort by similarity score (default)
- `ShardAwareRoutingStrategy` — Distribute across shard scopes
- `AdaptiveRoutingStrategy` — Select based on shard load and latency

**Context-Driven Selection**:
```cpp
RoutingContext ctx;
ctx.shard_aware = true;              // Triggers ShardAware strategy
ctx.optimize_latency = true;         // Triggers Adaptive strategy
ctx.top_k = 50;                      // Candidate limit
```

### 7. Redundancy Detection

**Detectors**:
- `SimilarityBasedRedundancyDetector` — Compare fingerprint scores
- `ContentHashRedundancyDetector` — Hash-based content deduplication
- `EmbeddingBasedRedundancyDetector` — Embedding space clustering
- `MetadataRedundancyDetector` — Metadata-level deduplication
- `CompositeRedundancyDetector` — Combine multiple detectors

## Integration Points

### 1. With ANN Frontdoor

The tensor mid-layer integrates with `index::AnnFrontdoor` for candidate retrieval:

```cpp
mid_layer_->setAnnFrontdoor(ann_frontdoor);
// During planning/summarization:
ann_frontdoor_->registerScopeKind(scope_key, ann_scope_kind);
```

### 2. With RAG Pipeline

The mid-layer coordinates with RAG-layer routing:

```cpp
TensorLayerSummary summary = mid_layer_->summarize(context);
// Summary provides:
// - routing_reason (explainability)
// - similar_adapters (ranked candidates)
// - candidate_count (for resource allocation)
```

### 3. With Query Optimizer

Query rewrite leverages tensor layer metadata:

```cpp
// TensorAwareQueryOptimizer inspects TensorMidLayer summaries
// to tag TENSOR_SIMILARITY nodes for specialized execution
```

## Performance Baselines (Phase 4)

### BENCH-001: Mid-Layer Planning
| Scope Type | Latency | Adapters | Notes |
|-----------|---------|----------|-------|
| Adapter | <1 ms | 1,000 | Payload: classification + ANN preload |
| Package | <0.5 ms | 1,000 | Simpler classification |
| ShardSummary | <2 ms | 3 shards | Includes federation overhead |

### BENCH-002: Fingerprint Operations
| Operation | Latency | Dataset Size | Notes |
|-----------|---------|--------------|-------|
| addAdapter | <0.1 ms | 1,000+ | Insertion cost |
| findSimilar (top-10) | <0.5 ms | 5,000 | KNN search overhead |
| findSimilar (top-100) | <5 ms | 5,000 | Batch retrieval budget |

### BENCH-003: Adapter Repository
| Operation | Latency | Count | Notes |
|-----------|---------|-------|-------|
| store | <0.1 ms | 1,000+ | Hash table insertion |
| findSimilarAdapters | <3 ms | 100 candidates | Index-based lookup |

### BENCH-004: Compression
| Algorithm | Tensor Size | Latency | Compression Ratio |
|-----------|-------------|---------|-------------------|
| TT-SVD | 64×64×64 | ~50 ms | 8:1 typical |
| Quantization | 256×256×256 | ~10 ms | 4:1 typical |
| Sampling | 1000×1000 | ~2 ms | 10:1 typical |

### BENCH-005: Redundancy Detection
| Detector | Batch Size | Latency | False Negative Rate |
|----------|-----------|---------|-------------------|
| SimilarityBased | 100 | <1 ms | <2% |
| ContentHash | 500 | <2 ms | <1% |
| Composite | 50 | <5 ms | <0.1% |

## Testing Strategy

### Unit Tests (existing)
- Individual component tests for all 8 core modules
- ~325 test cases with 95%+ coverage

### Integration Tests (Phase 4)
- **TML-INT-01** through **TML-INT-08**: Full pipeline validation
- **TML-PERF-01** through **TML-PERF-03**: Performance baseline assertion
- 8 integration test classes + 3 performance test classes

### Benchmark Suite (Phase 4)
- **BENCH-001 through BENCH-008**: Production baseline measurement
- 30+ benchmark operations covering all major paths
- Baseline assertions embedded (< 1ms, < 5ms, etc.)

### Test Execution
```bash
# Unit tests (tensor module)
ctest -L tensor --output-on-failure

# Integration tests
ctest -R "TensorMidLayerIntegration" --output-on-failure

# Benchmark suite
./bench_tensor_integration_baseline --benchmark_min_time=1 --benchmark_repetitions=5
```

## Build Configuration

### CMake Integration

**Library Target**:
```cmake
add_library(themis_tensor STATIC <20 sources>)
target_link_libraries(themis_tensor PUBLIC Threads::Threads)
target_link_libraries(themis_tensor PRIVATE themis_core themis_index)
target_compile_features(themis_tensor PUBLIC cxx_std_17)
```

**Usage in Downstream Modules**:
```cmake
target_link_libraries(my_module PRIVATE themis_tensor)
```

### Preset Configuration

Tensor module is auto-included in standard presets:
- `community-release` ✓
- `linux-release` ✓
- `windows-release` ✓
- `enterprise-release` ✓

## Operational Considerations

### Tenant Isolation

All operations respect tenant boundaries:
```cpp
// Tenant A cannot see Tenant B's adapters
adapter_repo_->store("tenant_a", "domain1", "adapter1", core);
adapter_repo_->store("tenant_b", "domain1", "adapter2", core);
// tenant_a's queries will not retrieve adapter2
```

### Error Recovery

On error, the system automatically:
1. Logs to audit trail (THEMIS_ERROR)
2. Invokes registered recovery callback
3. Falls back to degraded mode
4. Continues operation without propagating error

Example:
```cpp
error_handler_->setRecoveryCallback([](const TensorError& err) {
    THEMIS_WARN("Compression failed, falling back: {}", err.message);
    // System automatically tries next strategy
});
```

### Monitoring & Observability

Health metrics available via:
```cpp
auto monitor = error_handler_->getResilienceMonitor();
std::cout << "Recovery rate: " << monitor->getRecoveryRate() << "\n";
std::cout << "Error count: " << monitor->getTotalErrors() << "\n";
std::cout << "Fallback chain depth: " << monitor->getMaxFallbackDepth() << "\n";
```

### Tuning Parameters

**Performance Tuning**:
```cpp
TensorLayerContext ctx;
ctx.top_k = 50;                    // Candidate limit (default: 10)
ctx.use_fingerprint_summary = true; // Enable fingerprint optimization
ctx.shard_aware = false;            // Disable shard distribution
```

**Resilience Tuning**:
```cpp
ResilienceMonitor::Config config;
config.error_threshold = 0.05;      // Fail if >5% operations error
config.recovery_timeout_ms = 100;   // Max recovery time
config.max_fallback_depth = 4;      // Max fallback chain length
```

## Deployment Checklist

- [ ] CMake configured with `src/tensor/CMakeLists.txt`
- [ ] `themis_tensor` library target builds successfully
- [ ] Unit tests pass (tensor module)
- [ ] Integration tests pass (TML-INT-01 through TML-INT-08)
- [ ] Benchmark baseline recorded (BENCH-001 through BENCH-008)
- [ ] Performance baselines verified (<1ms plan, <5ms search)
- [ ] Downstream modules link successfully
- [ ] Tenant isolation verified
- [ ] Error recovery tested
- [ ] Documentation complete

## Next Steps (Phase 5+)

1. **Phase 5: RocksDB Persistence** — ✅ Implemented via durable fingerprint entry storage + journaled recovery  
   - Runtime wrapper: `include/tensor/persistent_tensor_fingerprint_graph.h`, `src/tensor/persistent_tensor_fingerprint_graph.cpp`
   - Tests: `tests/tensor/test_persistent_tensor_fingerprint_graph.cpp`
2. **Phase 6: Distributed Training** — ✅ Implemented via multi-node coordinator with lifecycle/retry/aggregation  
   - Coordinator: `include/distributed_tensor/tensor_training_coordinator.h`, `src/distributed_tensor/tensor_training_coordinator.cc`
   - Tests: `tests/epic3_distributed_tensor/test_tensor_training_coordinator.cpp`
3. **Phase 7: GPU Acceleration** — ✅ Implemented CUDA kernels for tensor compression/routing with CPU fallback  
   - CUDA kernels: `src/acceleration/cuda/tensor_compression_routing_kernels.cu`
   - Runtime dispatch: `include/tensor/tensor_compression_routing_accelerator.h`, `src/tensor/tensor_compression_routing_accelerator.cpp`
   - CMake integration: `cmake/ModularBuild.cmake`, `cmake/CMakeLists.txt`
   - Tests: `tests/tensor/test_tensor_acceleration_observability.cpp`
4. **Phase 8: Observability** — ✅ Implemented metrics export + SLO evaluator for tensor workflows  
   - Observability surface: `include/tensor/tensor_workflow_observability.h`, `src/tensor/tensor_workflow_observability.cpp`
   - Tests: `tests/tensor/test_tensor_acceleration_observability.cpp`

## References

- **Architecture**: `ARCHITECTURE.md` (Tensor section)
- **Roadmap**: `ROADMAP.md` (Phase 7: Tensor-Native Index)
- **Tests**: `tests/test_tensor_mid_layer_integration.cpp`
- **Benchmarks**: `benchmarks/bench_tensor_integration_baseline.cpp`
- **CMake**: `src/tensor/CMakeLists.txt`
