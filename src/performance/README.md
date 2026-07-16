# ThemisDB Performance Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The performance module provides optimization, profiling, hardware-aware acceleration, and runtime performance-tuning infrastructure for ThemisDB.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| cycle_metrics.cpp | cycle-based measurement and low-level timing surfaces |
| prometheus_exporter.cpp | performance metric export behavior |
| async_metrics_exporter.cpp | asynchronous performance metric export |
| chimera_exporter.cpp | benchmark/result export integration |
| numa_topology.cpp | NUMA topology discovery and affinity support |
| numa_memory_manager.cpp | NUMA-aware allocation and memory behavior |
| workload_predictor.cpp | workload prediction for performance adaptation |
| workload_adaptive_optimizer.cpp | adaptive strategy selection and application |
| advanced_cache_manager.cpp | cache policy and memory/caching optimizations |
| hardware_accelerator.cpp | accelerator-dispatch behavior |
| intelligent_prefetcher.cpp | prefetch optimization behavior |
| phase2_feature_flags.cpp | runtime feature-flag control |
| wisckey.cpp | WiscKey optimization implementation |
| dostoevsky.cpp | compaction policy optimization implementation |
| cicada.cpp | OCC optimization implementation |
| rabitq.cpp | quantization optimization implementation |
| ligra.cpp | graph-processing optimization implementation |
| phase3/ | phase-3 optimization components |
| phase4/ | phase-4 optimization components |

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
- unsupported hardware/acceleration paths degrade deterministically with explicit outcomes.
- optimization behavior remains bounded by module-local policies and safety checks.

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
- Verified behavior surfaces:
  - optimization, profiling, and hardware-aware adaptation paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md