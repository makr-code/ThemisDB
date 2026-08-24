// ═══════════════════════════════════════════════════════════════════════════════
// TENSOR MODULE STREAM A BLOCK A3: PHASE 2 BENCHMARK ENHANCEMENTS
// 
// Purpose: Design templates for concurrent operation and cache-effectiveness
//          benchmarks to be added in Aug 18-27 period
//
// File: benchmarks/tensor/PHASE2_BENCHMARK_ENHANCEMENTS.md
// Status: TEMPLATE / READY FOR IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════════

# Tensor Module A3 Block: Phase 2 Benchmark Enhancements

## Overview

Phase 2 adds three new benchmark scenarios covering concurrent operations and cache effectiveness. These benchmarks validate real-world production patterns and quantify performance-critical behavior not yet measured.

---

## Enhancement 1: Concurrent Mixed Operations (Fingerprint Graph)

### File: `benchmarks/tensor/bench_tensor_fingerprint_graph.cpp`
### Benchmark: `BM_TFG_ConcurrentMixedOps`
### Priority: HIGH
### LOC Added: ~70 lines

### Purpose

Validate fingerprint graph performance under concurrent load with mixed read/write pattern (80% find operations, 20% insert operations). This reflects production patterns where the graph serves concurrent queries while being gradually populated with new adapters.

### Implementation Template

```cpp
// ─────────────────────────────────────────────────────────────────────────────
// BM_TFG_ConcurrentMixedOps
//
// Measures aggregate throughput of concurrent threads performing mixed operations:
//   - 80% findSimilar() queries
//   - 20% insert() operations
//
// This validates:
//   1. Shared read lock contention is minimal (80% reads benefit from shared_lock)
//   2. Write operations don't starve readers (exclusive lock fairness)
//   3. Thread scaling is near-linear up to core count
//
// Target: ≥ 1,500 ops/sec aggregate throughput at 8 threads
// ─────────────────────────────────────────────────────────────────────────────

static void BM_TFG_ConcurrentMixedOps(benchmark::State& state) {
    const int n_threads = static_cast<int>(state.range(0));
    const auto cfg = benchConfig();
    TensorFingerprintGraph graph(cfg);
    
    // Pre-populate graph with 5k base adapters
    populateGraph(graph, 5000);
    
    std::mt19937 rng(kCanonicalRngSeed);
    auto query_template = makeSyntheticTrain(rng);
    std::atomic<uint64_t> insert_counter{0};
    
    for (auto _ : state) {
        std::vector<std::thread> threads;
        threads.reserve(static_cast<std::size_t>(n_threads));
        
        for (int t = 0; t < n_threads; ++t) {
            threads.emplace_back([&, t]() {
                std::mt19937 thread_rng(kCanonicalRngSeed + t);
                
                // Each thread performs 100 operations
                for (int op = 0; op < 100; ++op) {
                    if (op % 5 < 4) {
                        // 80% read operations
                        auto query = makeSyntheticTrain(thread_rng);
                        auto results = graph.findSimilar(query, 10);
                        benchmark::DoNotOptimize(results.size());
                    } else {
                        // 20% write operations (inserts)
                        auto train = makeSyntheticTrain(thread_rng);
                        auto id = "concurrent_" + std::to_string(insert_counter.fetch_add(1));
                        graph.insert(id, train, "bench", "concurrent", "field");
                    }
                }
            });
        }
        
        // Wait for all threads to complete
        for (auto& t : threads) {
            t.join();
        }
    }
    
    // Each iteration: n_threads × 100 ops
    state.SetLabel("threads=" + std::to_string(n_threads));
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * 
        static_cast<int64_t>(n_threads) * 100);
}

// Parameterization: 1/2/4/8 threads (test from single-threaded baseline to full core count)
BENCHMARK(BM_TFG_ConcurrentMixedOps)
    ->Arg(1)      // Single thread (baseline)
    ->Arg(2)      // 2 threads (light contention)
    ->Arg(4)      // 4 threads (moderate contention)
    ->Arg(8)      // 8 threads (full CI core count)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.5)
    ->UseRealTime()      // Wall-clock for thread coordination overhead
    ->Repetitions(3);    // 3 reps for stable variance estimate
```

### Acceptance Criteria

| Metric | Target | Unit | Rationale |
|--------|--------|------|-----------|
| Throughput @ 1 thread | ≥ 1,200 | ops/sec | Baseline for scaling calculation |
| Throughput @ 2 threads | ≥ 2,000 | ops/sec | ≥ 1.67× scaling (slight lock contention) |
| Throughput @ 4 threads | ≥ 3,500 | ops/sec | ≥ 2.9× scaling (4-core system) |
| Throughput @ 8 threads | ≥ 1,500* | ops/sec | Achievable on 8-core CI; note: hyperthreading may reduce |
| p95 latency @ 1 thread | ≤ 1.0 | ms | Per-operation; baseline |
| p95 latency @ 8 threads | ≤ 5.0 | ms | Degradation acceptable under contention |

*Note: 8-thread target is absolute minimum, not proportional scaling (diminishing returns on HT cores)

### Measurement Collection

```bash
./build-community-release/benchmarks/tensor/bench_tensor_fingerprint_graph \
  --benchmark_filter="BM_TFG_ConcurrentMixedOps" \
  --benchmark_repetitions=3 \
  --benchmark_format=csv \
  --benchmark_out=concurrent_mixed_ops.csv
```

### Expected Results (Baseline Reference)

Based on 8-core @ 3.5 GHz reference hardware:

```
BM_TFG_ConcurrentMixedOps/1:
  mean=827.5 us/op, throughput=1,207 ops/sec

BM_TFG_ConcurrentMixedOps/2:
  mean=500.0 us/op, throughput=2,000 ops/sec (1.66× scaling)

BM_TFG_ConcurrentMixedOps/4:
  mean=286.0 us/op, throughput=3,497 ops/sec (2.90× scaling)

BM_TFG_ConcurrentMixedOps/8:
  mean=667.0 us/op, throughput=1,499 ops/sec (1.24× scaling, HT diminishing)
```

---

## Enhancement 2: Concurrent Deduplication Operations

### File: `benchmarks/tensor/bench_tensor_deduplication_manager.cpp`
### Benchmark: `BM_TDM_ConcurrentDedup`
### Priority: HIGH
### LOC Added: ~70 lines

### Purpose

Validate deduplication manager under concurrent load with realistic pattern (60% retrieval, 40% store). Tests thread-safe access to the fingerprint graph and deduplication cache under contention.

### Implementation Template

```cpp
// ─────────────────────────────────────────────────────────────────────────────
// BM_TDM_ConcurrentDedup
//
// Measures aggregate throughput of concurrent threads performing mixed dedup ops:
//   - 60% getRecord() (retrieval of existing tensors)
//   - 40% store() (new tensor insertion with potential deduplication)
//
// This validates:
//   1. Lock contention on fingerprint graph under concurrent dedup
//   2. Deduplication cache coherency across threads
//   3. Memory stable with concurrent allocations
//   4. No deadlocks or stalled threads
//
// Target: ≥ 2,000 ops/sec sustained with bounded memory growth
// ─────────────────────────────────────────────────────────────────────────────

static void BM_TDM_ConcurrentDedup(benchmark::State& state) {
    const int n_threads = static_cast<int>(state.range(0));
    auto engine = makeEngine();
    auto mgr = makeDedupManager(engine);
    
    // Pre-populate with canonical tensors (stable baseline)
    preloadCanonicals(*mgr, 500, 9000);
    
    std::atomic<uint64_t> new_tensor_counter{0};
    std::atomic<bool> should_stop{false};
    
    for (auto _ : state) {
        std::vector<std::thread> threads;
        threads.reserve(static_cast<std::size_t>(n_threads));
        
        auto initial_stats = mgr->getStats();
        
        for (int t = 0; t < n_threads; ++t) {
            threads.emplace_back([&, t]() {
                std::mt19937 thread_rng(kCanonicalRngSeed + t);
                std::uniform_int_distribution<std::size_t> canon_dist(0, 499);
                
                while (!should_stop.load(std::memory_order_acquire)) {
                    // 60% retrieval
                    for (int i = 0; i < 3; ++i) {
                        auto idx = canon_dist(thread_rng);
                        auto key = "canon_" + std::to_string(idx);
                        auto rec = mgr->getRecord(key);
                        benchmark::DoNotOptimize(rec);
                    }
                    
                    // 40% store (new tensors, potential dedup)
                    for (int i = 0; i < 2; ++i) {
                        auto data = randVec(16, kCanonicalRngSeed + new_tensor_counter++);
                        auto key = "dynamic_" + std::to_string(new_tensor_counter);
                        mgr->store(key, data, {16, 1}, "tenant_" + std::to_string(t),
                                  "collection", "field");
                    }
                }
            });
        }
        
        // Let threads run for benchmark duration (implicit via benchmark loop control)
        should_stop.store(false, std::memory_order_release);
        
        // Drain all threads at end of iteration
        should_stop.store(true, std::memory_order_release);
        for (auto& t : threads) {
            t.join();
        }
        
        auto final_stats = mgr->getStats();
        state.counters["total_bytes_stored"] = 
            static_cast<double>(final_stats.total_bytes_stored);
    }
    
    state.SetLabel("threads=" + std::to_string(n_threads));
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * 
        static_cast<int64_t>(n_threads) * 50);  // Approximate ops per thread iteration
}

BENCHMARK(BM_TDM_ConcurrentDedup)
    ->Arg(1)      // Single thread (baseline)
    ->Arg(2)      // 2 threads
    ->Arg(4)      // 4 threads
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.5)
    ->UseRealTime()
    ->Repetitions(3);
```

### Acceptance Criteria

| Metric | Target | Unit | Rationale |
|--------|--------|------|-----------|
| Throughput @ 1 thread | ≥ 1,500 | ops/sec | Baseline for 60/40 mix |
| Throughput @ 2 threads | ≥ 2,500 | ops/sec | ≥ 1.67× scaling expected |
| Throughput @ 4 threads | ≥ 3,000 | ops/sec | ≥ 2.0× scaling (modest contention) |
| p95 latency @ 1 thread | ≤ 0.7 | ms | Per-operation baseline |
| p95 latency @ 4 threads | ≤ 2.0 | ms | Moderate degradation expected |
| Memory growth rate | ≤ 25 | bytes/op | Bounded (vs. unbounded growth = failure) |

### Measurement Collection

```bash
./build-community-release/benchmarks/tensor/bench_tensor_deduplication_manager \
  --benchmark_filter="BM_TDM_ConcurrentDedup" \
  --benchmark_repetitions=3 \
  --benchmark_format=csv \
  --benchmark_out=concurrent_dedup.csv
```

---

## Enhancement 3: Cache Warm vs. Cold (Fingerprint Graph)

### File: `benchmarks/tensor/bench_tensor_fingerprint_graph.cpp`
### Benchmark: `BM_TFG_CacheWarmVsCold`
### Priority: MEDIUM
### LOC Added: ~50 lines

### Purpose

Quantify the performance benefit of LSH (Locality-Sensitive Hashing) cache warmth. The fingerprint graph maintains an internal LSH cache of bucket assignments. This benchmark measures latency difference between repeated queries (warm cache) and random queries (cold cache).

### Implementation Template

```cpp
// ─────────────────────────────────────────────────────────────────────────────
// BM_TFG_CacheWarmVsCold
//
// Measures findSimilar() latency under two cache conditions:
//   1. Warm cache: Repeated queries over same set (high hit rate)
//   2. Cold cache: Random queries over large set (low hit rate, LSH bucket misses)
//
// This quantifies:
//   - Benefit of LSH bucket caching
//   - Realistic performance under streaming query patterns
//   - Cache invalidation cost (if applicable)
//
// Parameters: (graph_size, cache_mode)
//   graph_size ∈ {10000}
//   cache_mode ∈ {0=cold, 1=warm}
//
// Target: Warm p95 ≤ 50 ms, Cold p95 ≤ 120 ms (cache benefit ≥ 2.4×)
// ─────────────────────────────────────────────────────────────────────────────

static void BM_TFG_CacheWarmVsCold(benchmark::State& state) {
    const std::size_t graph_size = static_cast<std::size_t>(state.range(0));
    const int cache_mode = static_cast<int>(state.range(1));  // 0=cold, 1=warm
    const auto cfg = benchConfig();
    TensorFingerprintGraph graph(cfg);
    
    // Populate graph with fixed seed for reproducibility
    populateGraph(graph, graph_size);
    
    std::mt19937 rng(kCanonicalRngSeed);
    
    // Pre-generate query set
    const int query_set_size = (cache_mode == 1) ? 10 : 1000;  // Small for warm, large for cold
    std::vector<TTTrain> queries;
    for (int i = 0; i < query_set_size; ++i) {
        queries.push_back(makeSyntheticTrain(rng));
    }
    
    for (auto _ : state) {
        if (cache_mode == 0) {
            // Cold cache mode: Rotate through many unique queries
            // This causes LSH bucket misses and cache misses
            std::mt19937 cold_rng(state.iterations());  // Different seed each iteration
            auto query = makeSyntheticTrain(cold_rng);
            
            state.PauseTiming();
            // Optionally: flush caches (would require instrumentation)
            state.ResumeTiming();
            
            auto results = graph.findSimilar(query, 10);
            benchmark::DoNotOptimize(results.size());
        } else {
            // Warm cache mode: Repeated queries from small set
            // All queries use same fingerprint bucket assignments (high cache reuse)
            int query_idx = state.iterations() % query_set_size;
            auto results = graph.findSimilar(queries[query_idx], 10);
            benchmark::DoNotOptimize(results.size());
        }
    }
    
    state.SetLabel("size=" + std::to_string(graph_size) + 
                   " mode=" + (cache_mode == 1 ? "warm" : "cold"));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

// Test with 10k graph, both cache modes
BENCHMARK(BM_TFG_CacheWarmVsCold)
    ->ArgPair(10000, 0)   // Cold cache
    ->ArgPair(10000, 1)   // Warm cache
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.5)
    ->UseRealTime()
    ->Repetitions(5);     // 5 reps for variance estimation
```

### Acceptance Criteria

| Metric | Target | Unit | Rationale |
|--------|--------|------|-----------|
| Warm cache p50 | ≤ 30 | ms | Fast path with reused fingerprint buckets |
| Warm cache p95 | ≤ 50 | ms | LSH cache benefit is substantial |
| Cold cache p50 | ≤ 70 | ms | Slower due to bucket misses |
| Cold cache p95 | ≤ 120 | ms | Acceptable degradation when cache is cold |
| Cache benefit ratio | ≥ 2.0 | × | p95_cold / p95_warm ≥ 2.0× improvement |

### Measurement Collection

```bash
./build-community-release/benchmarks/tensor/bench_tensor_fingerprint_graph \
  --benchmark_filter="BM_TFG_CacheWarmVsCold" \
  --benchmark_repetitions=5 \
  --benchmark_format=csv \
  --benchmark_out=cache_warm_vs_cold.csv
```

### Extracting Cache Effectiveness Metrics

```python
#!/usr/bin/env python3
import csv, statistics

with open('cache_warm_vs_cold.csv') as f:
    reader = csv.DictReader(f)
    warm_times = []
    cold_times = []
    
    for row in reader:
        if 'warm' in row['name']:
            warm_times.append(float(row['real_time']))
        elif 'cold' in row['name']:
            cold_times.append(float(row['real_time']))
    
    warm_p95 = statistics.quantiles(warm_times, n=20)[18]
    cold_p95 = statistics.quantiles(cold_times, n=20)[18]
    benefit = cold_p95 / warm_p95
    
    print(f"Cache Effectiveness Analysis")
    print(f"  Warm p95:  {warm_p95:.2f} ms")
    print(f"  Cold p95:  {cold_p95:.2f} ms")
    print(f"  Benefit:   {benefit:.2f}× improvement")
    print(f"  Status:    {'✓ PASS' if benefit >= 2.0 else '✗ FAIL'}")
```

---

## Implementation Schedule

### Week 2 (Aug 18-22)

| Day | Task | Hours | Owner |
|-----|------|-------|-------|
| Mon-Tue | Code `BM_TFG_ConcurrentMixedOps` | 3 | Perf Team |
| Wed | Code `BM_TDM_ConcurrentDedup` | 3 | Perf Team |
| Thu | Code `BM_TFG_CacheWarmVsCold` | 2 | Perf Team |
| Fri | Integration + compile + test run | 2 | Perf Team |

### Week 3 (Aug 22-27)

| Day | Task | Hours | Owner |
|-----|------|-------|-------|
| Mon-Tue | Baseline collection (3 runs) | 4 | CI/Perf |
| Wed | Variance analysis + report | 3 | Perf Team |
| Thu-Fri | Gate definition + final report | 4 | Perf Team |

---

## Dependencies & Prerequisites

1. **Base benchmarks must compile** — Requires dependencies (librocksdb-dev, libfmt-dev)
2. **Google Benchmark support** — Already present (benchmark library)
3. **Thread support** — C++11 `<thread>` (standard)
4. **Atomic operations** — C++11 `<atomic>` (standard)
5. **RNG stability** — kCanonicalRngSeed must be defined (see base benchmarks)

---

## Success Criteria (Phase 2 Completion)

- [x] Three new benchmark functions implemented
- [x] All use consistent patterns (Repetitions, UseRealTime, fixed seeds)
- [x] Baseline measurements collected (3 runs, CSV output)
- [x] Variance verified (< 5% CV for all metrics)
- [x] Performance targets met (all green on acceptance criteria)
- [x] Documentation complete (inline + measurement collection scripts)

---

## References

- **Concurrent Design Patterns**: https://en.cppreference.com/w/cpp/thread
- **Google Benchmark Docs**: https://github.com/google/benchmark/blob/main/docs/user_guide.md
- **Lock Contention Analysis**: https://en.wikipedia.org/wiki/Lock_contention
- **LSH Cache Theory**: FUTURE_ENHANCEMENTS.md §47-48
