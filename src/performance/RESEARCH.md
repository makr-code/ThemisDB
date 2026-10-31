---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "include/performance/workload_predictor.h"
  - "include/performance/workload_adaptive_optimizer.h"
  - "include/performance/rabitq.h"
  - "include/performance/phase3/diskann.h"
  - "benchmarks/performance/CMakeLists.txt"
  - "benchmarks/performance/bench_performance_release_gates.cpp"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Research

## Purpose and scope

This page records the engineering methods represented in `src/performance` and
how to evaluate them. It is for developers deciding whether a performance
technique is supported by this implementation and what evidence is still
needed. It does not claim that paper-reported improvements have been reproduced
on ThemisDB workloads.

## Methods in the source

- **Workload trend estimation:** `WorkloadPredictor` retains a bounded window
  of `WorkloadSnapshot` observations. Its documented methods are exponential
  moving-average smoothing, ordinary least-squares trend extrapolation, and a
  coefficient-of-variation confidence score. Prediction is requested by the
  caller; the predictor does not start a background worker.
- **Adaptive execution policy:** `WorkloadAdaptiveOptimizer` classifies recent
  query observations into OLTP, OLAP, mixed, graph, vector, time-series, or
  unknown workloads, then selects an `OptimizationStrategy`. Its optional
  adaptation loop is a separate facility from `WorkloadPredictor`.
- **Cost calibration:** `PerQueryCostModel` records CPU-cycle and wall-clock
  measurements with query type, rows, pages, and estimated cost. Its rolling
  history can produce calibration factors for `OptimizerCostModel`.
- **Hardware- and data-layout-aware methods:** Phase 2 contains WiscKey,
  Dostoevsky, Cicada, Ligra, and RaBitQ implementations. Their headers cite
  research motivations; those citations are not evidence of local performance
  or production suitability by themselves.
- **Vector retrieval:** Phase 3 `DiskANNIndex` persists graph nodes and searches
  through a bounded beam; the RaBitQ implementation uses 2-bit quantized
  vectors and supports quantized distance evaluation. These have different
  memory, search, and accuracy trade-offs.

## Evaluation protocol

1. Define the workload, input distribution, hardware, feature-flag state, and
   baseline implementation. Use the same data and seed for each comparison.
2. Measure the relevant behavior, not only total throughput: latency percentiles,
   throughput, CPU and memory consumption, allocation locality, cache and disk
   activity, and result quality where approximation is used.
3. Use focused tests for correctness and edge cases, then run mapped
   release-profile benchmarks. The current module benchmark registration builds
   the contract microbenchmarks in
   [`bench_performance_release_gates.cpp`](../../benchmarks/performance/bench_performance_release_gates.cpp);
   its six named cases do not encode baseline or pass/fail thresholds. The
   sibling `bench_random_access_prefetch.cpp` is present but is not registered
   by `benchmarks/performance/CMakeLists.txt`.
4. Compare against an explicitly selected and documented release baseline and
   retain the run manifest. The source reviewed here does not define a
   module-wide regression percentage or p99 threshold, so do not claim one is
   enforced by these benchmark targets. See the current inventory in
   [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md).

The roadmap identifies baseline refresh, mixed/distributed benchmark depth,
and sustained workload-shift testing as Q1 2027 work. Current mappings do not
establish broad distributed-workload or universal hardware results.

## Limits and decision guidance

Predictions and calibration factors are inputs to policy decisions, not a
guarantee that a workload will improve. Confidence and observed performance
must be checked before enabling a strategy for a deployment. Compare exact
search and quantized/graph search on recall as well as latency and memory.
Prefer a simpler CPU or non-adaptive path when a representative baseline is
unavailable or the measured benefit does not cover its operational cost.

See [MODELING.md](MODELING.md) for predictor assumptions,
[EMBEDDING.md](EMBEDDING.md) for vector-search evaluation, and
[OPERATIONS.md](OPERATIONS.md) for release measurements.

---

Exact model backend not verifiable from final document text alone.
