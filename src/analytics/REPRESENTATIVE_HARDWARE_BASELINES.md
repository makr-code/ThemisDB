# Analytics Representative Hardware Baselines

<!-- Status: current | validated: 2026-09-16 -->
<!-- Links: ROADMAP.md · PERFORMANCE_EXPECTATIONS.md · ../../benchmarks/analytics/INDEX.md -->

## Scope

This document defines the canonical hardware matrix and artifact contract for
analytics p95/p99 and throughput baseline capture.

## Required Benchmark Suites

- `benchmarks/analytics/bench_analytics_release_gates.cpp`
- `benchmarks/analytics/bench_analytics_distributed_coordinator.cpp`
- `benchmarks/analytics/bench_streaming_window.cpp`
- `benchmarks/analytics/bench_analytics_operability_paths.cpp`

## Representative Hardware Matrix

| Profile ID | Class | Required Focus |
|---|---|---|
| `analytics-cpu-balanced` | 8+ vCPU general-purpose node | OLAP, export, streaming, serving fail-closed paths |
| `analytics-cpu-memory` | memory-optimized node | high-cardinality streaming and distributed merge pressure |
| `analytics-gpu-optional` | optional accelerator-equipped node | validate that analytics-adjacent accelerated paths remain fail-closed when unavailable or degraded |

## Artifact Contract

The canonical manifest for required outputs is:

- `benchmarks/baselines/analytics/representative_hardware_manifest.json`

Each profile must publish:

- raw benchmark JSON
- summarized p95/p99 report
- baseline comparison output
- deviation decision (`pass`, `warn`, `fail`)

## Current Source-Validated Status

- benchmark wiring for all required analytics operability suites is present in source.
- benchmark/result artifact locations are defined.
- representative-hardware execution evidence is still pending and must be attached
  before roadmap items that require authoritative p95/p99 numbers are closed.

## Closure Rule

The representative-hardware roadmap items may only move to `[x]` after the
manifested artifacts exist for every required profile and are linked from the
module evidence bundle.
