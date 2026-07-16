# Tensor Update Benchmark Design And Coverage Matrix

**Status:** Active (Phase 1)  
**Effective Date:** 2026-07-02  
**Scope:** Benchmark coverage for dynamic tensor-update performance in the Tensor-Graph architecture

---

## 1. Goal

This document defines comprehensive benchmark coverage for dynamic tensor-update performance characteristics.

Design principle:

**Benchmarks measure commit overhead, update worker throughput, snapshot rebuild latency, and CPU/GPU break-even points for bounded tensor maintenance work.**

---

## 2. Benchmark Coverage Matrix

### 2.1 Commit-Path Overhead Dimension

Measures write-path overhead introduced by tensor delta logging and manifest management.

| Benchmark | Scenario | Metrics | Output Format |
|---|---|---|---|
| `bench_tensor_commit_overhead` | baseline RocksDB transaction | ops/sec, latency p50/p95/p99 | CSV + flamegraph |
| `bench_tensor_commit_overhead` | transaction + delta logging | ops/sec, latency p50/p95/p99 | CSV + flamegraph |
| `bench_tensor_commit_overhead` | transaction + delta + manifest invalidation | ops/sec, latency p50/p95/p99 | CSV + flamegraph |

Acceptance criteria:
- Delta logging overhead ≤ 10% write latency increase
- Manifest invalidation overhead ≤ 5% write latency increase

### 2.2 Update Worker Throughput Dimension

Measures worker processing rate for different update path types.

| Benchmark | Scenario | Metrics | Output Format |
|---|---|---|---|
| `bench_tensor_update_worker` | small delta patch path | patches/sec, latency, quality change | CSV + timeseries |
| `bench_tensor_update_worker` | partial refit path | refits/sec, latency, quality change | CSV + timeseries |
| `bench_tensor_update_worker` | full rebuild path | rebuilds/sec, latency, quality change | CSV + timeseries |
| `bench_tensor_update_worker` | rank growth sensitivity | throughput vs rank (R=[1,8,16,32,64]) | CSV + plot |
| `bench_tensor_update_worker` | residual/error tracking | overhead of quality metrics | CSV |

Acceptance criteria:
- Patch throughput ≥ 1000 ops/sec (local artifact, <100KB)
- Refit throughput ≥ 100 ops/sec (partial rebuild on 10% changes)
- Rebuild throughput ≥ 10 ops/sec (full artifact regeneration)

### 2.3 Query Routing Quality Dimension

Measures fan-out reduction and routing quality with fresh vs stale tensor artifacts.

| Benchmark | Scenario | Metrics | Output Format |
|---|---|---|---|
| `bench_tensor_query_routing` | ANN-only baseline | fan-out, recall | CSV |
| `bench_tensor_query_routing` | ANN+Tensor (fresh) | fan-out reduction, recall overhead | CSV |
| `bench_tensor_query_routing` | ANN+Tensor+Graph (fresh) | cumulative fan-out reduction | CSV |
| `bench_tensor_query_routing` | summary-first vs direct fetch | latency, correctness trade-offs | CSV |
| `bench_tensor_query_routing` | stale vs fresh summaries | quality degradation at staleness thresholds | CSV + plot |
| `bench_tensor_query_routing` | exact fallback frequency | fallback rate by staleness | CSV |

Acceptance criteria:
- Fresh tensor summary achieves ≥10% fan-out reduction vs ANN-only
- Quality degradation on stale summary ≤5% at staleness threshold

### 2.4 CPU vs GPU Break-Even Dimension

Measures computational and transfer overhead to determine GPU utility.

| Benchmark | Scenario | Metrics | Output Format |
|---|---|---|---|
| `bench_tensor_cpu_gpu_break_even` | batch-size sweep (N=[1,8,32,128,512]) | latency CPU vs GPU, transfer time | CSV + plot |
| `bench_tensor_cpu_gpu_break_even` | density sweep (nnz=[10%,30%,50%,70%,90%]) | latency CPU vs GPU | CSV + plot |
| `bench_tensor_cpu_gpu_break_even` | rank sweep (R=[1,8,16,32,64]) | latency CPU vs GPU | CSV + plot |
| `bench_tensor_cpu_gpu_break_even` | host↔device transfer overhead | transfer time, bandwidth utilization | CSV |
| `bench_tensor_cpu_gpu_break_even` | bounded tensor update windows | latency for bounded-time updates | CSV |

Acceptance criteria:
- GPU break-even point identified (N × R × density)
- Transfer overhead ≤10% of compute time for break-even workload

### 2.5 Snapshot Rebuild Latency Dimension

Measures snapshot extraction and rebuild performance.

| Benchmark | Scenario | Metrics | Output Format |
|---|---|---|---|
| `bench_tensor_snapshot_rebuild` | snapshot extraction cost | extraction time, I/O throughput | CSV |
| `bench_tensor_snapshot_rebuild` | rebuild latency by graph size (N=[1K,10K,100K,1M]) | latency, throughput | CSV + plot |
| `bench_tensor_snapshot_rebuild` | artifact publish/swap latency | atomic visibility latency | CSV |

Acceptance criteria:
- Snapshot extraction overhead ≤5% graph query latency
- Rebuild latency scales linearly or better with graph size

---

## 3. Benchmark Implementation Conventions

### 3.1 Benchmark Structure

Each benchmark uses Google Benchmark with state initialization:

```cpp
#include <benchmark/benchmark.h>

namespace themis {
namespace distributed_tensor {

class TensorUpdateBenchmark : public benchmark::Fixture {
protected:
    void SetUp(const ::benchmark::State& state) override {
        // Initialize tensor infrastructure
        // Create synthetic workload
    }

    void TearDown(const ::benchmark::State& state) override {
        // Clean up
        // Report results
    }
};

} // namespace distributed_tensor
} // namespace themis
```

### 3.2 Benchmark Scenarios

Benchmarks use realistic workloads:

- **Data size:** 1KB to 100MB artifacts
- **Batch size:** 1 to 512 records
- **Graph scale:** 1K to 1M nodes
- **Tensor rank:** 1 to 64 dimensions
- **Sparsity:** 10% to 90% non-zero

### 3.3 Metrics Output

Each benchmark produces CSV output with:

```csv
time_unit,min,max,mean,median,stddev,throughput
nanoseconds,<ns>,<ns>,<ns>,<ns>,<ns>,ops/sec
```

And latency percentiles:

```
p50,p95,p99,p999
<ns>,<ns>,<ns>,<ns>
```

---

## 4. Benchmark Naming Convention

Benchmark names follow Google Benchmark pattern:

```
BENCHMARK_F(FixtureName, BenchmarkName)
```

Example:

```cpp
BENCHMARK_F(TensorCommitOverheadBench, BaselineRocksDB);
BENCHMARK_F(TensorCommitOverheadBench, WithDeltaLogging);
BENCHMARK_F(TensorUpdateWorkerBench, SmallDeltaPatch);
```

---

## 5. Baseline Management

### 5.1 Baseline Storage

Baseline measurements stored in `benchmarks/baselines/tensor_update/`:

```
benchmarks/
├── baselines/
│   └── tensor_update/
│       ├── bench_tensor_commit_overhead_baseline.csv
│       ├── bench_tensor_update_worker_baseline.csv
│       ├── bench_tensor_query_routing_baseline.csv
│       ├── bench_tensor_cpu_gpu_break_even_baseline.csv
│       └── bench_tensor_snapshot_rebuild_baseline.csv
```

### 5.2 Baseline Comparison

Benchmarks compare against baseline with ±10% regression threshold.

Tool: `tools/compare_benchmarks.py`

```bash
./benchmarks/bench_tensor_commit_overhead --benchmark_format=csv > /tmp/current.csv
python3 tools/compare_benchmarks.py \
    --baseline benchmarks/baselines/tensor_update/bench_tensor_commit_overhead_baseline.csv \
    --current /tmp/current.csv \
    --threshold 0.10
```

---

## 6. Dependencies And Linking

### 6.1 Required Libraries

- `benchmark::benchmark` - Google Benchmark framework
- `benchmark::benchmark_main` - Main entry point
- `themis_core` - Core dependencies
- `themis_distributed_tensor` - Tensor infrastructure
- `RocksDB::rocksdb` - For I/O-intensive benchmarks
- `OpenSSL::SSL/Crypto` - For manifest signing (if applicable)
- `Threads::Threads` - For parallelism

### 6.2 Header Includes

```cpp
#include <benchmark/benchmark.h>
#include "distributed_tensor/include/artifact_manifest.h"
#include "distributed_tensor/include/tensor_artifact_classes.h"
```

---

## 7. Smoke-Test Policy

### 7.1 Smoke-Test Execution

Benchmarks are executed in CI with reduced workload scale:

```bash
ctest --preset benchmarks-smoke -R "tensor_update" --output-on-failure
```

Smoke-test parameters:
- 1-2 iterations per benchmark (not full Google Benchmark runs)
- Small dataset sizes (1K-100K, not full 1M scale)
- Execution time budget: 60s per benchmark

### 7.2 Smoke-Test Acceptance

Smoke-test passes if:
- [ ] All benchmarks complete without crash
- [ ] Output contains required CSV metrics
- [ ] Metrics are within 2σ of baseline (loose threshold)
- [ ] No memory leaks detected (if Valgrind/ASan enabled)

---

## 8. Output And Integration

### 8.1 Benchmark Output Format

Each benchmark produces:

1. **CSV metrics:** `<benchmark>_results.csv`
2. **Flamegraph:** `<benchmark>_flamegraph.svg` (if perf data available)
3. **JSON summary:** `<benchmark>_summary.json`
4. **Markdown report:** `<benchmark>_report.md`

### 8.2 Evaluation Pipeline Integration

Benchmark outputs consumed by:

- **Planner Issue (#XXXXX):** CPU/GPU break-even guidance
- **Lifecycle Issue (#XXXXX):** Rebuild latency for artifact refresh strategy
- **Hardware Issue (#XXXXX):** GPU utility analysis

Example integration in issue:

```markdown
### Tensor Update Performance Baselines

From `bench_tensor_cpu_gpu_break_even`:
- GPU useful when N × R ≥ 256 (batch-size × rank)
- Transfer overhead ~5% at break-even point

See: benchmarks/baselines/tensor_update/
```

---

## 9. Acceptance Criteria

✅ **Benchmark Suite Complete When:**
- [ ] All 5 benchmark binaries implemented
- [ ] Coverage matrix dimensions covered (21 scenarios minimum)
- [ ] Baselines established and committed
- [ ] Smoke-test CI integration working
- [ ] Baseline comparison tool functional
- [ ] Planner/Lifecycle/Hardware issues reference benchmark outputs
- [ ] Code reviewed and merged to develop

---

## 10. References

- `include/benchmark/` - Google Benchmark headers
- `benchmarks/` - Other ThemisDB benchmarks (reference implementations)
- `benchmarks/baselines/` - Baseline storage structure
- `tools/compare_benchmarks.py` - Baseline comparison tool
- `EVALUATION_FRAMEWORK.md` - Benchmark evaluation policy
