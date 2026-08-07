# Search Module Release Gate Benchmarks

**Version:** 2.0.0  
**Status:** Phase 5 Complete (2026-08-06)  
**Benchmark Suite:** bench_search_release_gates.cpp

---

## Overview

This directory contains the performance benchmarks and release gates for ThemisDB Search Module v2.0.0 production readiness. All gates are defined with baseline values and 10% regression tolerance thresholds.

## Release Gate Definitions

### SRCP-1: Hybrid Search Dispatch
- **Metric:** p99 latency for hybrid dispatch (BM25 + vector + RRF)
- **Baseline:** ≤ 15 ms
- **Gate Threshold:** p99 ≤ 16.5 ms (10% tolerance)
- **Fixture:** 100 shards, 10K candidates, seed=42

### SRCP-2: Distributed Merge Throughput
- **Metric:** Merge throughput (results/sec)
- **Baseline:** ≥ 50K results/sec
- **Gate Threshold:** ≥ 45K results/sec (10% tolerance)
- **Fixture:** 64 shards, 1K results each

### SRCP-3: Reranking Overhead
- **Metric:** p99 latency for LLM fallback
- **Baseline:** ≤ 5 ms overhead
- **Gate Threshold:** ≤ 5.5 ms
- **Fixture:** 1K candidate set, LLM unavailable

### SRCP-4: Multi-Device GPU Acceleration
- **Metric:** p99 latency GPU vs CPU fallback
- **Baseline GPU:** ≤ 8 ms
- **Baseline CPU:** ≤ 10 ms
- **Gate Threshold GPU:** ≤ 8.8 ms
- **Gate Threshold CPU:** ≤ 11 ms

### SRCP-5: Stream Buffer Flush
- **Metric:** Flush latency per 1K-result batch
- **Baseline:** ≤ 10 ms
- **Gate Threshold:** ≤ 11 ms
- **Fixture:** 64-shard streaming

### SRCP-6: Query Expansion Throughput
- **Metric:** Expansion time for 1K queries
- **Baseline:** ≤ 50 ms
- **Gate Threshold:** ≤ 55 ms
- **Fixture:** expansion_limit=5

## Advanced Benchmarks (Q1 2027)

- **SRCP-ADV-1:** Multimodal search (text + image, ≤ 20 ms, nDCG@10 ≥ 0.85)
- **SRCP-ADV-2:** Learning-to-Rank (500 candidates depth=100, ≤ 25 ms)
- **SRCP-ADV-3:** Concurrent indexing (latency increase ≤ 10%)

## Baseline Hardware

**Reference Platform:**
- CPU: Intel Xeon Gold 6348 (28 cores)
- RAM: 256 GB
- GPU: NVIDIA H100 (optional)
- Network: 10 Gbps

**Minimum Production:**
- CPU: Intel Xeon Silver 4310 (12 cores)
- RAM: 64 GB
- Network: 1 Gbps

## Regression Thresholds

- **≤ 5% slower:** Monitor
- **5–10% slower:** Investigate
- **> 10% slower:** FAIL (block merge)

## Running Benchmarks

```bash
# Build
cmake --preset release
cmake --build --preset release --target bench_search_release_gates

# Run all gates
./build/benchmarks/bin/bench_search_release_gates

# Run single gate
./build/benchmarks/bin/bench_search_release_gates \
  --benchmark_filter=SRCP_1_HybridSearchDispatch
```

## Documentation

- **Runbook:** docs/operations/search_runbook.md
- **Performance Tuning:** docs/operations/search_tuning.md
- **Troubleshooting:** docs/operations/search_troubleshooting.md

**Last Updated:** 2026-08-06  
**Next Review:** 2026-11-06 (Q4 2026 re-baseline)
