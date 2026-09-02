# Wave B Option B1: Benchmark Protocol Specification for LLM Wiki Phase B
## Representative-Hardware Validation & Benchmark Protocol Design

**Document ID:** WAVE_B1_BENCHMARK_PROTOCOL_2026_09_02  
**Phase:** Phase 1 (Baseline Protocol Design)  
**Milestone:** Sept 16 Readiness  
**Date:** 2026-09-02  
**Owner:** Platform Performance & LLM Wiki Integration  
**Status:** Draft Specification

---

## Executive Summary

This document specifies three reproducible benchmark scenarios for LLM Wiki Phase B validation (Sept 16–30, 2026). Each scenario is fully defined with:
- Dataset size & composition
- Retrieval chain depth (2-layer to 4-layer)
- Concurrency profile (100 to 10K concurrent queries)
- Performance targets (p95/p99 latency, throughput)
- Measurement methodology (warmup, steady-state, cooldown)
- Reproducibility locks (seed, configuration, data distribution)

### Success Criteria (Gate: Sept 5)
- ✅ **3 benchmark scenarios fully specified** — Small, Medium, Large (all p95/p99 targets locked)
- ✅ **Concurrency profiles defined** — 100 / 1K / 10K concurrent queries with load distribution
- ✅ **Measurement protocol locked** — 5-min warmup, 20-min steady-state, p50/p95/p99 collection
- ✅ **Reproducibility checklist validated** — Seed, data, configuration versioning in spec

### Primary Deliverables by Sept 16
1. Scenario configuration files (JSON) + data generation scripts
2. Reproducibility checksum validation
3. Load driver integration (Apache JMeter / custom cpp harness)
4. Baseline execution on representative hardware (dry-run)

---

## 1. Benchmark Scenario Definitions

### Scenario P0: Small (10K Vectors)
**Purpose:** Fast feedback loop; daily development regression detection

#### Dataset Configuration
| Parameter | Value | Rationale |
|-----------|-------|-----------|
| **Total Vectors** | 10,000 | Fits in-memory even on modest hardware |
| **Embedding Dim** | 1536 (text-embedding-3-large) | Production dimension |
| **Vector Type** | float32 | Standard precision |
| **Document Source** | Wikipedia subset (10K random articles) | Realistic text corpus |
| **Index Type** | HNSW (M=12, ef_construction=200) + BM25+ | Phase B configuration |
| **RocksDB Cache** | 2GB page cache | Typical production setting |
| **Seed** | 42 | Deterministic data generation |

#### Retrieval Chain Configuration
```
Layer 1 (Text Search):     BM25+ scorer on embeddings index
Layer 2 (Vector Search):   HNSW exact-match with EF=50
                           ↓
Output:                    Top-K results (K=10 default)
                           Reranked by RRF (Reciprocal Rank Fusion)
```

#### Query Workload Profile
| Metric | Value | Notes |
|--------|-------|-------|
| **Concurrent Queries** | 100 | 100 parallel client threads |
| **Query Distribution** | Uniform random from 10K Wikipedia articles | Realistic access pattern |
| **Query Type** | Mixed text + embedding distance | 50% text-only, 50% hybrid |
| **Query per Client** | 200 queries per connection | Sustained 100*200 = 20K total queries |
| **Total Query Count** | 20,000 | Ensures p95/p99 validity (n ≥ 30 required) |

#### Performance Targets (Hard Gates)
| Metric | Target | Threshold Type | Justification |
|--------|--------|---|---|
| **p50 latency** | ≤ 50ms | Tracking | Median end-to-end retrieval time |
| **p95 latency** | ≤ 150ms | Hard gate | SLA compliance; production tail-latency |
| **p99 latency** | ≤ 250ms | Hard gate | Extreme tail; outlier detection |
| **Throughput** | ≥ 1000 qps | Hard gate | Minimum sustained retrieval rate |
| **Memory Peak** | ≤ 4GB | Soft gate | HNSW + RocksDB cache bounded |

#### Measurement Phases
```
Phase 1: Setup (1 min)
  - Initialize RocksDB store
  - Load 10K embeddings into HNSW index
  - Warm up BM25+ scorer (trigger compilation/JIT)
  - Start profiling (NVML, perf, jemalloc)

Phase 2: Warmup (5 min)
  - 100 concurrent queries × 200 queries/client
  - Goal: Warm L3 cache, JIT compilation, kernel optimization
  - NO MEASUREMENT (results discarded)
  - Detect thermal throttling; pause if T > 80°C

Phase 3: Steady-State (20 min)  ← MEASUREMENT WINDOW
  - Same workload as Phase 2
  - Collect latency histograms at 1ms resolution
  - Record p50, p95, p99, p99.9, max
  - Monitor GPU/CPU utilization, memory, thermal

Phase 4: Cooldown (1 min)
  - Graceful shutdown; capture cleanup overhead
  - Save final memory snapshot
  - Stop profiling
```

---

### Scenario P1: Medium (1M Vectors)
**Purpose:** Production-scale performance validation; representative data volume

#### Dataset Configuration
| Parameter | Value | Rationale |
|-----------|-------|-----------|
| **Total Vectors** | 1,000,000 | ~1GB embedding data; typical prod workload |
| **Embedding Dim** | 1536 | Consistent with P0 |
| **Vector Type** | float32 | Standard precision |
| **Document Source** | Wikipedia corpus (1M articles) | Realistic scale |
| **Index Type** | HNSW (M=16, ef_construction=400) | Optimized for 1M scale |
| **RocksDB Cache** | 8GB page cache | Larger production setting |
| **Seed** | 42 | Deterministic |

#### Retrieval Chain Configuration
```
Layer 1 (Text Search):     BM25+ pre-filter (top-100 candidates)
         ↓
Layer 2 (Vector ANN):      HNSW (EF=100) on filtered candidates
         ↓
Layer 3 (Reranking):       RRF + LLM judge integration (mock integration)
                           ↓
Output:                    Top-K results (K=5)
```

#### Query Workload Profile
| Metric | Value | Notes |
|--------|-------|-------|
| **Concurrent Queries** | 1,000 | 1K parallel client threads (increased load) |
| **Query Distribution** | Zipfian (θ=1.2) | Realistic with hot queries |
| **Query Type** | Hybrid text + embedding | 70% hybrid, 30% text-only |
| **Query per Client** | 100 queries per connection | Sustained 1K*100 = 100K total queries |
| **Total Query Count** | 100,000 | Robust p95/p99 estimation |

#### Performance Targets (Hard Gates)
| Metric | Target | Threshold Type | Justification |
|--------|--------|---|---|
| **p50 latency** | ≤ 100ms | Tracking | Median with larger dataset |
| **p95 latency** | ≤ 300ms | Hard gate | Production SLA at 1M scale |
| **p99 latency** | ≤ 500ms | Hard gate | Tail-latency under load |
| **Throughput** | ≥ 500 qps | Hard gate | Realistic throughput at 1M scale |
| **Memory Peak** | ≤ 12GB | Soft gate | HNSW (larger M) + expanded cache |

#### Measurement Phases
```
Phase 1: Setup (3 min)
  - Initialize RocksDB store
  - Load 1M embeddings into HNSW index with ef_construction=400
  - Trigger Zipfian query distribution bias
  - Start profiling

Phase 2: Warmup (5 min)
  - 1K concurrent queries × 100 queries/client
  - Goal: Cache warm-up, index page fault amortization
  - NO MEASUREMENT

Phase 3: Steady-State (20 min)  ← MEASUREMENT WINDOW
  - Same workload
  - Measure latency histograms with 2ms resolution
  - Record memory/thermal/GPU metrics

Phase 4: Cooldown (1 min)
  - Shutdown; save snapshots
```

---

### Scenario P2: Large (10M Vectors, Sustained)
**Purpose:** Hyperscale sustained load; extreme tail validation

#### Dataset Configuration
| Parameter | Value | Rationale |
|-----------|-------|-----------|
| **Total Vectors** | 10,000,000 | ~10GB embedding data |
| **Embedding Dim** | 1536 | Consistent |
| **Vector Type** | float32 | Standard precision |
| **Document Source** | Wikipedia + Common Crawl excerpt | Large-scale corpus |
| **Index Type** | HNSW (M=24, ef_construction=800) | Optimized for 10M scale |
| **RocksDB Cache** | 16GB page cache | Large production cache |
| **Seed** | 42 | Deterministic |

#### Retrieval Chain Configuration
```
Layer 1 (Text Search):     BM25+ pre-filter (top-200 candidates)
         ↓
Layer 2 (Vector ANN):      HNSW (EF=150) on pre-filter
         ↓
Layer 3 (Graph Traversal): Optional graph-based re-rank (if available)
         ↓
Layer 4 (LLM Judge):       Real LLM integration (or mock with fixed latency)
                           ↓
Output:                    Top-K results (K=3)
```

#### Query Workload Profile
| Metric | Value | Notes |
|--------|-------|-------|
| **Concurrent Queries** | 10,000 | Maximum concurrency stress |
| **Query Distribution** | Power-law (Zipfian θ=2.0) | Extreme skew; realistic |
| **Query Type** | Hybrid + LLM judge | 100% full chain (all 4 layers) |
| **Query per Client** | 50 queries per connection | Sustained 10K*50 = 500K total queries |
| **Total Query Count** | 500,000 | Robust p99.9 estimation |
| **Duration** | 30 minutes (sustained) | Thermal/memory stability test |

#### Performance Targets (Hard Gates)
| Metric | Target | Threshold Type | Justification |
|--------|--------|---|---|
| **p50 latency** | ≤ 200ms | Tracking | Larger dataset + 4-layer chain |
| **p95 latency** | ≤ 600ms | Hard gate | Production SLA at 10M scale |
| **p99 latency** | ≤ 1000ms | Hard gate | Extreme tail; acceptable for large scale |
| **Throughput** | ≥ 200 qps | Hard gate | Realistic under hyperscale load |
| **Memory Peak** | ≤ 30GB | Hard gate | MUST not exceed system RAM at 32GB |
| **Thermal** | <80°C sustained | Hard gate | No throttling for 30-min sustained load |

#### Measurement Phases
```
Phase 1: Setup (5 min)
  - Initialize RocksDB store
  - Load 10M embeddings into HNSW index (ef_construction=800)
  - Trigger power-law query distribution
  - Start profiling (aggressive monitoring due to duration)

Phase 2: Warmup (5 min)
  - 10K concurrent queries × 50 queries/client (partial)
  - Goal: Cache warm-up, thermal stabilization
  - NO MEASUREMENT

Phase 3: Sustained Load (30 min)  ← MEASUREMENT WINDOW (EXTENDED)
  - 10K concurrent queries × 50 queries/client (complete)
  - Record latency histograms every 5 minutes (6 snapshots)
  - Continuous monitoring: memory growth rate, thermal drift
  - Detect memory leaks (memory must plateau, not grow)
  - Detect thermal throttling (adaptive load reduction if T > 80°C)

Phase 4: Cooldown (2 min)
  - Graceful shutdown
  - Final memory snapshot
  - Thermal recovery baseline
```

---

## 2. Reproducibility Specification

### 2.1 Deterministic Seeds & Configuration

**All scenarios must be reproducible to ±1% latency variance:**

```json
{
  "benchmark_version": "WAVE_B1_20260902",
  "scenario": "small_10k",
  "random_seed": 42,
  "wikipedia_snapshot_date": "2026-08-01",
  "wikipedia_snapshot_checksum": "sha256:abcdef1234567890...",
  "embeddings_model": "text-embedding-3-large",
  "embeddings_version": "1.0.0",
  "rocksdb_version": "8.6.7",
  "hnsw_version": "0.8.0",
  "bm25_version": "1.2.3",
  "configuration": {
    "hnsw_m": 12,
    "hnsw_ef_construction": 200,
    "hnsw_ef_search": 50,
    "rocksdb_cache_mb": 2048,
    "rocksdb_block_size": 4096,
    "rocksdb_compression": "lz4"
  },
  "query_distribution": {
    "scenario_p0": "uniform",
    "scenario_p1": "zipfian_theta_1_2",
    "scenario_p2": "zipfian_theta_2_0"
  },
  "expected_variance_pct": 1.0,
  "reproducibility_checksum": "sha256:xyz123..."
}
```

### 2.2 Data Generation & Validation

**Wikipedia Snapshot:**
1. Download fixed snapshot (date: 2026-08-01)
2. Extract embeddings for P0 (10K), P1 (1M), P2 (10M) subsets
3. Validate checksums before benchmark execution
4. Generate data artifacts in `benchmarks/wave_b_data/`

**Data Generation Script:**
```bash
# Generate all 3 scenarios
./benchmarks/scripts/generate_wave_b_data.sh \
  --snapshot-date 2026-08-01 \
  --seed 42 \
  --output-dir benchmarks/wave_b_data \
  --scenarios p0,p1,p2
```

**Checksum Validation:**
```bash
# Verify data integrity before benchmark run
./benchmarks/scripts/validate_wave_b_checksums.sh \
  --data-dir benchmarks/wave_b_data \
  --manifest benchmarks/wave_b_data/MANIFEST.json
```

### 2.3 Reproducibility Checklist

Before benchmark execution, verify:
- [ ] **Seed locked:** All random sources use seed=42
- [ ] **Data checksums match:** All P0/P1/P2 datasets validated
- [ ] **Configuration checksum matches:** Stored in CI artifact
- [ ] **Hardware profile captured:** GPU model, driver version, CUDA version
- [ ] **OS/compiler captured:** Ubuntu 22.04, clang-17 (-O3)
- [ ] **No background processes:** Confirm clean system state (no heavy I/O)
- [ ] **Thermal baseline:** GPU temp < 60°C before warmup starts

---

## 3. Measurement Protocol (All Scenarios)

### 3.1 Collection Methodology

#### Latency Histogram Collection
```cpp
// Pseudocode for benchmark harness
for (size_t query_idx = 0; query_idx < total_queries; ++query_idx) {
  // Steady-state phase only (skip warmup)
  if (elapsed_time < warmup_duration) continue;
  
  auto start = std::chrono::high_resolution_clock::now();
  auto results = execute_query(query_workload[query_idx]);
  auto end = std::chrono::high_resolution_clock::now();
  
  auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
    end - start
  ).count();
  
  latency_histogram.record(latency_us);
  
  if (query_idx % 100 == 0) {
    // Periodic memory/thermal snapshot
    capture_system_metrics();
  }
}
```

#### Resolution & Binning
```
Bin Width:
  - 0–1000µs:    1µs bins (1000 bins)
  - 1–100ms:     10µs bins (9900 bins)
  - 100ms–10s:   100µs bins (9900 bins)
  - >10s:        1ms bins (5000 bins)

Percentiles Extracted:
  - p50, p75, p90, p95, p99, p99.5, p99.9, max
  - Outlier flagging: any value > 3σ (separately tracked)
```

### 3.2 Profiling Integration

#### GPU Profiling (NVML)
```python
import pynvml
pynvml.nvmlInit()

# Query every 100ms (10 Hz)
metrics = {
  "gpu_util_pct": nvmlDeviceGetUtilizationRates(device).gpu,
  "memory_used_gb": nvmlDeviceGetMemoryInfo(device).used / 1e9,
  "power_w": nvmlDeviceGetPowerUsage(device) / 1e3,  # mW → W
  "temp_c": nvmlDeviceGetTemperature(device, 0),
  "throttle_reason": nvmlDeviceGetCurrentThrottleReasons(device)
}
```

#### CPU Profiling (perf)
```bash
# During benchmark execution
perf record -e cycles,cache-misses,branch-misses,context-switches \
  -c 1000 --call-graph=dwarf -F 99 \
  -- ./benchmark_harness --scenario p0
```

#### Memory Profiling (jemalloc)
```bash
# Link benchmark with jemalloc; enable profiling
MALLOC_CONF="prof:true,prof_prefix:jemalloc_prof_,lg_prof_interval:30" \
  ./benchmark_harness --scenario p0

# Analyze heap dumps
jeprof --pdf ./benchmark_harness jemalloc_prof_*.heap > heap_profile.pdf
```

### 3.3 Result Output Format

**JSON Output (Machine-Readable):**
```json
{
  "benchmark_metadata": {
    "scenario": "small_10k",
    "timestamp_utc": "2026-09-16T14:30:00Z",
    "hardware_profile": "A100_40GB",
    "os": "Ubuntu 22.04 LTS",
    "cuda_version": "12.2",
    "nvidia_driver_version": "535.104.05",
    "cmake_preset": "linux-release",
    "compiler": "clang-17",
    "compiler_flags": "-O3 -march=native -DNDEBUG"
  },
  "measurement_window": {
    "warmup_duration_sec": 300,
    "steady_state_duration_sec": 1200,
    "cooldown_duration_sec": 60
  },
  "latency_metrics": {
    "latency_us": {
      "p50": 42300,
      "p75": 89000,
      "p90": 120000,
      "p95": 156700,
      "p99": 203400,
      "p99_5": 225100,
      "p99_9": 248100,
      "max": 312500
    },
    "outliers_above_3sigma": [
      { "latency_us": 312500, "timestamp_sec": 456.8 }
    ]
  },
  "throughput_metrics": {
    "queries_per_second": 5230,
    "total_queries_measured": 20000,
    "total_time_sec": 1200
  },
  "resource_metrics": {
    "gpu": {
      "memory_peak_gb": 18.4,
      "memory_avg_gb": 16.2,
      "utilization_avg_pct": 87.3,
      "power_avg_w": 215,
      "temp_max_c": 74,
      "throttle_events": 0
    },
    "cpu": {
      "utilization_avg_pct": 42.1,
      "l3_cache_misses_million": 234.5,
      "context_switches": 56789
    },
    "memory": {
      "peak_rss_mb": 18400,
      "jemalloc_peak_gb": 18.2
    }
  },
  "validation": {
    "reproducibility_checksum": "sha256:xyz123...",
    "data_checksum_p0": "sha256:abc123...",
    "regression_gates": {
      "p95_latency_ms_gate": "PASS",
      "throughput_qps_gate": "PASS",
      "memory_peak_gate": "PASS"
    }
  }
}
```

**CSV Output (Analysis-Friendly):**
```csv
scenario,timestamp,latency_ms,query_index,gpu_memory_gb,gpu_util_pct,temp_c
small_10k,2026-09-16T14:30:00Z,42.3,0,18.4,85,72
small_10k,2026-09-16T14:30:01Z,156.7,1,18.4,87,73
small_10k,2026-09-16T14:30:02Z,89.1,2,18.3,84,72
...
```

---

## 4. Regression Detection & Gate Evaluation

### 4.1 Gate Comparison Methodology

**Baseline Comparison (vs Wave 7 baseline + current develop):**

```python
def evaluate_regression_gate(current_metric, baseline_metric, gate_config):
    """
    Compare current measurement against baseline + threshold.
    """
    delta_pct = 100 * (current_metric - baseline_metric) / baseline_metric
    
    if delta_pct >= gate_config['red_threshold']:
        return "RED"      # Critical regression
    elif delta_pct >= gate_config['yellow_threshold']:
        return "YELLOW"   # Warning; investigate
    else:
        return "GREEN"    # Acceptable
```

### 4.2 Per-Scenario Gate Configuration

**Scenario P0 (Small 10K):**
| Metric | Baseline | Yellow (+%) | Red (+%) | Current |
|--------|----------|---|---|---|
| p95 latency (ms) | 150 | +5 (157.5) | +10 (165) | [ ] TBD |
| Throughput (qps) | 1000 | -3 (970) | -10 (900) | [ ] TBD |
| Memory peak (GB) | 4 | +10 (4.4) | +25 (5) | [ ] TBD |

**Scenario P1 (Medium 1M):**
| Metric | Baseline | Yellow (+%) | Red (+%) | Current |
|--------|----------|---|---|---|
| p95 latency (ms) | 300 | +5 (315) | +10 (330) | [ ] TBD |
| Throughput (qps) | 500 | -3 (485) | -10 (450) | [ ] TBD |
| Memory peak (GB) | 12 | +10 (13.2) | +25 (15) | [ ] TBD |

**Scenario P2 (Large 10M):**
| Metric | Baseline | Yellow (+%) | Red (+%) | Current |
|--------|----------|---|---|---|
| p95 latency (ms) | 600 | +5 (630) | +10 (660) | [ ] TBD |
| Throughput (qps) | 200 | -3 (194) | -10 (180) | [ ] TBD |
| Memory peak (GB) | 30 | +10 (33) | +25 (37.5) | [ ] TBD |
| Sustained (no mem leak) | Plateau | Growth detected | OOM | [ ] TBD |

---

## 5. Execution Timeline & Milestones

### Phase 1: Specification & Preparation (Sept 2–5)
- [x] Benchmark protocol specification finalized (this document)
- [ ] Data generation scripts created & tested
- [ ] Checksum validation procedures locked
- [ ] Load driver harness ready (JMeter or custom C++)

### Phase 2: Dry-Run & Calibration (Sept 9–12)
- [ ] Scenario P0 dry-run on representative hardware
- [ ] Latency histogram collection validated
- [ ] Regression gate thresholds calibrated
- [ ] Memory profiling operational

### Phase 3: Full Execution (Sept 16–30)
- [ ] Scenario P0 baseline captured (Sept 16–17)
- [ ] Scenario P1 baseline captured (Sept 18–20)
- [ ] Scenario P2 sustained-load baseline (Sept 21–23)
- [ ] Regression gate evaluation (Sept 24–30)
- [ ] Results compiled into evidence bundle

---

## 6. Success Criteria & Sign-Off

**Benchmark Protocol Complete When:**
✅ All 3 scenarios fully specified (P0/P1/P2)  
✅ Concurrency profiles locked (100/1K/10K concurrent queries)  
✅ p50/p95/p99 latency targets set  
✅ Measurement window protocol defined (5-min warmup, 20-min steady-state)  
✅ Reproducibility checksums validated  
✅ Ready for Phase 2 dry-run (Sept 9–12)

**Sign-Off Authority:** Performance Lead + LLM Wiki Module Owner

**Current Status:** Draft for Sept 2 Review

---

## Appendix A: Query Distribution Formulas

### Uniform Distribution (Scenario P0)
```python
def uniform_query_distribution(num_queries, num_documents):
    """All documents equally likely."""
    return [random.randint(0, num_documents-1) for _ in range(num_queries)]
```

### Zipfian Distribution (Scenarios P1 & P2)
```python
def zipfian_distribution(num_queries, num_documents, theta=1.2):
    """
    Power-law distribution; common in real workloads.
    theta=1.2: Light skew (P1)
    theta=2.0: Heavy skew (P2, realistic)
    """
    ranks = np.arange(1, num_documents + 1)
    probabilities = (1.0 / ranks) ** theta
    probabilities /= probabilities.sum()  # Normalize
    return np.random.choice(num_documents, size=num_queries, p=probabilities)
```

---

## Appendix B: Load Driver Integration

### Option 1: Custom C++ Harness
```cpp
// benchmarks/harness/wave_b_benchmark.cpp
class WaveBBenchmarkDriver {
  void execute_scenario(const ScenarioConfig& config) {
    // Load dataset, warm up, collect metrics
    // Integrated with NVML profiler
    // Output JSON results
  }
};
```

### Option 2: Apache JMeter
```xml
<!-- benchmarks/wave_b_load_profile.jmx -->
<ThreadGroup guiclass="ThreadGroupGui" testname="Wave B Load">
  <elementProp name="ThreadGroup.main_controller">
    <ThreadGroup>
      <stringProp name="ThreadGroup.num_threads">100</stringProp>
      <stringProp name="ThreadGroup.ramp_time">60</stringProp>
      <elementProp name="ThreadGroup.duration_limit">1200</elementProp>
    </ThreadGroup>
  </elementProp>
  <!-- Query samplers; result collectors -->
</ThreadGroup>
```

---

**Document Version:** 1.0  
**Next Review:** Sept 5, 2026  
**Last Updated:** 2026-09-02
