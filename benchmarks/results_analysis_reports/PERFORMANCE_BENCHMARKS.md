> ⚠️ **Historische Messdaten** – Die in diesem Dokument enthaltenen Zahlen entstammen einem bestimmten Messzeitpunkt und sind nicht mehr reproduzierbar ohne die ursprüngliche Testumgebung.
> Für reproduzierbare Ergebnisse: Benchmark-Kommandos und aktuelle CMake-Presets unter [`benchmarks/README.md`](../README.md) verwenden.

# Real Google Benchmark Performance Tests

## Overview

This document describes the real Google Benchmark-based performance tests for ThemisDB, implementing comprehensive benchmarks for storage, OLAP queries, embedding cache, and LLM operations.

## Benchmark Suites

### 1. Storage Performance (`bench_storage_performance`)

Tests storage layer performance with real implementations:

#### Memory Allocator Benchmarks
- **Baseline**: System allocator (malloc/free)
- **Optimized**: ThemisDB allocator (mimalloc when available)
- **Test Scenarios**:
  - Small allocations (128 bytes): Target <100ns per op
  - Large allocations (1MB): Target <1μs per op
  - Mixed allocation patterns (realistic workload)

#### Huge Pages Benchmarks
- **Baseline**: Regular 4KB pages
- **Optimized**: Huge pages (2MB/1GB)
- **Test Scenarios**:
  - Sequential memory access: Target 5-10% improvement
  - Random access patterns: Target 10-15% improvement
  - TLB miss rate reduction

#### RCU Index Benchmarks
- **Single-threaded reads**: Target <50ns per read
- **Multi-threaded reads**: Target linear scalability (1, 2, 4, 8 threads)
- **Write with synchronization**: Target <1ms per write
- **Read-write contention**: Measure lock-free performance

**Target Metrics**:
- Allocation throughput: >1M ops/sec (small), >100K ops/sec (large)
- Memory efficiency: <5% overhead
- RCU read scalability: >95% efficiency up to 8 threads

### 2. OLAP Query Performance (`bench_olap_performance`)

Tests analytical query performance with representative data scales:

#### Aggregation Operations
- **COUNT**: Target >1M rows/sec
- **SUM**: Target >1M rows/sec
- **AVG**: Target >1M rows/sec
- **MIN/MAX**: Target >1M rows/sec

#### GROUP BY Operations
- **Single dimension**: Target >500K rows/sec
- **Two dimensions**: Target >400K rows/sec
- **Three dimensions**: Target >300K rows/sec

#### Filter Operations
- **Equality filter**: Target >2M rows/sec
- **Range filter**: Target >2M rows/sec
- **Complex filter** (multi-condition): Target >1M rows/sec

#### Data Scales
- 1K rows (small)
- 10K rows (medium)
- 100K rows (large)
- 1M rows (very large)

**Target Metrics**:
- Query throughput: Rows processed per second
- Query latency: Time per query at each scale
- Memory per operation: <10MB for 1M rows

### 3. Embedding Cache Performance (`bench_embedding_cache_performance`)

Tests semantic similarity cache with vector embeddings:

#### Cache Operations
- **Store without index**: Target >10K stores/sec
- **Store with HNSW**: Target >5K stores/sec (acceptable trade-off)
- **Query without index**: Target >1K queries/sec (10K entries)
- **Query with HNSW**: Target >100K queries/sec (100x improvement)

#### Batch Operations
- Batch sizes: 1, 10, 100, 1000
- Target: Linear scalability with batch size

#### Cache Hit/Miss Performance
- **Hit rate**: 50%, 70%, 90%
- **Cost savings**: Track estimated API cost reduction
- **Similarity thresholds**: 0.90, 0.95, 0.99

#### Memory Usage
- Per-entry overhead: ~6KB (1536-dim embedding + metadata)
- Maximum cache size: 100K entries (~600MB)

**Target Metrics**:
- Cache hit latency: <1ms
- Cache miss latency: <10ms
- Memory efficiency: <10% overhead
- Cost savings: 70-90% API cost reduction at 90% hit rate

### 4. LLM Inference Performance (`bench_llm_inference_performance`)

Tests LLM model inference and adapter operations:

#### LoRA Adapter Operations
- **Load**: Target <100ms per adapter
- **Apply**: Target <10ms per application
- **Remove**: Target <5ms per removal
- **Context switch**: Target <15ms per switch
- **Reuse**: Target <1ms (intelligent reuse)

#### Multi-LoRA Batch Processing
- Batch sizes: 1, 4, 8, 16 adapters
- Target: Linear scalability

#### Inference Simulation
- **Token throughput**: Target >1K tokens/sec (single stream)
- **Prompt latency**: Target <50ms for 512-token prompt
- **End-to-end inference**: Complete pipeline overhead

#### Concurrent Operations
- Thread counts: 1, 2, 4, 8
- Target: Thread-safe with minimal contention

#### Memory Usage
- Per-adapter overhead: ~50MB (typical for 7B model)
- Maximum adapters: 16 (with 8GB VRAM)

**Target Metrics**:
- Adapter switch time: <15ms
- Throughput: >1000 tokens/sec
- Memory per adapter: ~50MB
- Concurrent scalability: >90% efficiency

## Building Benchmarks

```bash
# Build specific benchmark
cmake --build build --target bench_storage_performance
cmake --build build --target bench_olap_performance
cmake --build build --target bench_embedding_cache_performance
cmake --build build --target bench_llm_inference_performance

# Build all new performance benchmarks
cmake --build build --target bench_storage_performance bench_olap_performance bench_embedding_cache_performance bench_llm_inference_performance
```

## Running Benchmarks

### Basic Execution
```bash
# Run with default settings
./build/benchmarks/bench_storage_performance
./build/benchmarks/bench_olap_performance
./build/benchmarks/bench_embedding_cache_performance
./build/benchmarks/bench_llm_inference_performance
```

### JSON Output for CI
```bash
# Generate JSON output for regression tracking
./build/benchmarks/bench_storage_performance \
    --benchmark_out=storage_results.json \
    --benchmark_out_format=json

./build/benchmarks/bench_olap_performance \
    --benchmark_out=olap_results.json \
    --benchmark_out_format=json

./build/benchmarks/bench_embedding_cache_performance \
    --benchmark_out=embedding_results.json \
    --benchmark_out_format=json

./build/benchmarks/bench_llm_inference_performance \
    --benchmark_out=llm_results.json \
    --benchmark_out_format=json
```

### Filtering Benchmarks
```bash
# Run only specific benchmarks
./build/benchmarks/bench_storage_performance --benchmark_filter="Allocator.*"
./build/benchmarks/bench_olap_performance --benchmark_filter="GroupBy.*"
./build/benchmarks/bench_embedding_cache_performance --benchmark_filter="Query.*"
./build/benchmarks/bench_llm_inference_performance --benchmark_filter="LoRA.*"
```

### Statistical Analysis
```bash
# Run with multiple repetitions for statistics
./build/benchmarks/bench_storage_performance \
    --benchmark_repetitions=10 \
    --benchmark_report_aggregates_only=true

# Set minimum time per benchmark
./build/benchmarks/bench_olap_performance \
    --benchmark_min_time=1.0
```

## CI Integration

### Automated Benchmark Execution

```bash
#!/bin/bash
# run_performance_benchmarks.sh

OUTPUT_DIR="benchmark_results"
mkdir -p "$OUTPUT_DIR"

# Run all benchmarks with JSON output
./build/benchmarks/bench_storage_performance \
    --benchmark_out="$OUTPUT_DIR/storage.json" \
    --benchmark_out_format=json

./build/benchmarks/bench_olap_performance \
    --benchmark_out="$OUTPUT_DIR/olap.json" \
    --benchmark_out_format=json

./build/benchmarks/bench_embedding_cache_performance \
    --benchmark_out="$OUTPUT_DIR/embedding_cache.json" \
    --benchmark_out_format=json

./build/benchmarks/bench_llm_inference_performance \
    --benchmark_out="$OUTPUT_DIR/llm_inference.json" \
    --benchmark_out_format=json

# Combine results
cat "$OUTPUT_DIR"/*.json > "$OUTPUT_DIR/all_results.json"

echo "Benchmark results saved to $OUTPUT_DIR/"
```

### Regression Detection

Compare current results with baseline:
```bash
# Compare with baseline
./tools/compare_bench.py \
    baseline_results.json \
    current_results.json \
    --threshold=0.05  # 5% threshold
```

## Interpreting Results

### Metrics Reported

1. **Time**: Wall-clock time per iteration
2. **CPU Time**: CPU time per iteration
3. **Items Processed**: Number of items processed per second
4. **Bytes Processed**: Throughput in bytes per second
5. **Label**: Descriptive label for the benchmark variant

### Example Output

```
--------------------------------------------------------------------
Benchmark                          Time             CPU   Iterations
--------------------------------------------------------------------
BM_Allocator_System_Small        85 ns         85 ns      8234567
BM_Allocator_Themis_Small        68 ns         68 ns     10234567
BM_OLAP_Count/1000              123 us        123 us         5678
BM_OLAP_Count/1000000        123456 us     123456 us            6
BM_EmbeddingCache_Query_WithIndex  125 ns    125 ns     5600000
BM_LoRA_Apply                   8.5 ms        8.5 ms           82
```

### Performance Baselines

| Benchmark | Baseline | Target | Current |
|-----------|----------|--------|---------|
| Allocator (small) | 85ns | <80ns | TBD |
| OLAP Count (1M rows) | 1.2s | <1s | TBD |
| Cache Query (HNSW) | 125ns | <200ns | TBD |
| LoRA Apply | 9ms | <10ms | TBD |

## Benchmark Validation

### Requirements
- ✅ Uses actual code (no stubs/mocks)
- ✅ Records real metrics (time, memory, throughput)
- ✅ Compares baseline vs optimized variants
- ✅ Outputs JSON for CI regression
- ✅ Includes representative scales and batch sizes
- ✅ Documents target metrics

### Code Quality
- Real implementations from ThemisDB codebase
- Proper setup/teardown for accurate measurements
- Realistic data generation
- Multiple scales for scalability testing
- Thread-safety testing where applicable

## Future Enhancements

1. **GPU Benchmarks**: Add CUDA/HIP kernel benchmarks
2. **Network Benchmarks**: Add gRPC/HTTP protocol benchmarks
3. **Distributed Benchmarks**: Add multi-node coordination benchmarks
4. **Hardware Profiling**: Add CPU cache, TLB, branch prediction metrics
5. **Power Consumption**: Add energy efficiency metrics

## References

- Google Benchmark: https://github.com/google/benchmark
- ThemisDB Documentation: docs/performance/
- Benchmark Best Practices: docs/benchmarks/best_practices.md
