---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "active"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "src/performance/*.cpp"
  - "include/performance/*.h"
  - "tests/performance/CMakeLists.txt"
  - "benchmarks/performance/CMakeLists.txt"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

## Module Purpose

The performance module provides measurement/export, optimization and workload
feedback, memory/cache support, selected algorithms, and optional hardware/I/O
facilities. Components are exposed through public declarations in
[`../../include/performance`](../../include/performance); callers compose them
and retain responsibility for applying recommendations or selecting fallbacks.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| Measurement/export | `cycle_metrics.cpp`, `async_metrics_exporter.cpp`, `prometheus_exporter.cpp`, `chimera_exporter.cpp` | Compile-gated collection, asynchronous drain, and serializers; exporter behavior depends on configuration. |
| Workload feedback and optimization | `workload_predictor.cpp`, `workload_adaptive_optimizer.cpp`, `adaptive_query_compiler.cpp` | Return forecasts, strategies, or compilation outcomes; recommendations are not resource changes. |
| Memory, cache, and placement | `advanced_cache_manager.cpp`, `numa_memory_manager.cpp`, `numa_topology.cpp`, `intelligent_prefetcher.cpp` | Cache, NUMA, and prefetch components subject to their API and host constraints. |
| Hardware controls and phase algorithms | `phase2_feature_flags.cpp`, `hardware_accelerator.cpp`, `wisckey.cpp`, `dostoevsky.cpp`, `cicada.cpp`, `ligra.cpp`, `rabitq.cpp` | Optional or specialized code paths; verify each compile gate, runtime gate, and result. |
| Phase 3 and 4 | `phase3/*.cpp`, `phase4/*.cpp` | Query-cost, algorithm, memory-pressure, zero-copy I/O, PMEM, and PMU components. |

## Scope

In scope:
- runtime optimization and tuning behavior
- performance metrics/profiling/export surfaces
- hardware-aware and workload-adaptive performance controls

Out of scope:
- business-domain query/storage semantics outside performance boundaries
- external benchmark orchestration ownership outside module interfaces
- non-performance operational domains

## Runtime Behavior and Limits

- behavior depends on enabled compile/runtime feature flags and host capabilities.
- unsupported hardware/acceleration paths use the fallback or failure behavior
  documented by the specific interface; do not assume one module-wide policy.
- requested feature state is not proof that the implementation path was used.

## Sourcecode Verification (Module: performance/readme)

- Verified files:
  - src/performance/cycle_metrics.cpp
  - src/performance/prometheus_exporter.cpp
  - src/performance/async_metrics_exporter.cpp
  - src/performance/chimera_exporter.cpp
  - src/performance/numa_topology.cpp
  - src/performance/numa_memory_manager.cpp
  - src/performance/workload_predictor.cpp
  - src/performance/workload_adaptive_optimizer.cpp
  - src/performance/advanced_cache_manager.cpp
  - src/performance/hardware_accelerator.cpp
  - src/performance/intelligent_prefetcher.cpp
  - src/performance/phase2_feature_flags.cpp
  - src/performance/wisckey.cpp
  - src/performance/dostoevsky.cpp
  - src/performance/cicada.cpp
  - src/performance/rabitq.cpp
  - src/performance/ligra.cpp
- Phase-specific implementations also present:
  - `src/performance/phase3/*.cpp`
  - `src/performance/phase4/*.cpp`
- Verified behavior surfaces:
  - optimization, profiling, and hardware-aware adaptation paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md

Exact model backend not verifiable from final document text alone.