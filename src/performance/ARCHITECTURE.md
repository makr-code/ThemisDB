# Architecture - Performance Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The performance module composes measurement, optimization, cache/memory tuning, and hardware-aware acceleration into a bounded runtime optimization subsystem for ThemisDB.

## Main Execution Planes

1. Measurement and export plane
- cycle metrics, profiling, and performance export behavior
- metrics serialization and asynchronous export paths

2. Optimization and adaptation plane
- workload prediction and adaptive optimization strategies
- phase-gated feature control and optimization policy selection

3. Memory, cache, and hardware plane
- NUMA-aware memory behavior and cache optimization
- accelerator dispatch and prefetch optimization surfaces

## Core Contracts

| Contract | Behavior |
|---|---|
| measurement contract | deterministic cycle/profiling data capture/export semantics |
| optimization contract | bounded and explicit runtime optimization decisions |
| memory/cache contract | deterministic cache and NUMA-aware behavior |
| accelerator contract | explicit capability-aware hardware path behavior |

## Failure Semantics

- invalid configuration or unsupported capability paths fail with explicit outcomes.
- optimization feature fallbacks remain deterministic and observable.
- export/profiling failures are surfaced explicitly.

## Sourcecode Verification (Module: performance/architecture)

- Verified files:
  - src/performance/cycle_metrics.cpp
  - src/performance/workload_predictor.cpp
  - src/performance/workload_adaptive_optimizer.cpp
  - src/performance/advanced_cache_manager.cpp
  - src/performance/numa_memory_manager.cpp
  - src/performance/hardware_accelerator.cpp
- Verified architecture claims:
  - explicit measurement/optimization/memory-hardware planes
  - deterministic failure boundaries across runtime optimization workflows
  - module-local ownership of performance orchestration behavior
---

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| `cache` | `include/performance/allocator.h` | `AlignedVectorAllocator` depends on the performance allocator for SIMD-aligned cache buffer allocation (`include/cache/aligned_vector_allocator.h`) |
| `index` | `include/performance/phase3/diskann.h` | `ANNIndex` uses the Phase 3 DiskANN integration for on-disk approximate neighbour search (`include/index/ann_index.h`) |
| `utils` | `include/performance/alignment_helpers.h` | `UnalignedAccess` utilities use alignment helpers for safe unaligned memory reads (`include/utils/unaligned_access.h`) |
| `llm` | `include/performance/alignment_helpers.h` | GPU LoRA layer kernels use alignment helpers for aligned tensor buffer management (`src/llm/lora_framework/gpu_lora_layers.cpp`) |
| `query` | `include/performance/phase3/per_query_cost_model.h` | Query optimizer applies the per-query cost model for cardinality-aware plan selection (`src/query/query_optimizer.cpp`) |
| `rag` | `include/performance/phase3/bao.h`, `include/performance/workload_adaptive_optimizer.h` | Continuous learning orchestrator uses BAO and workload-adaptive optimizer for RAG execution tuning (`src/rag/continuous_learning_orchestrator.cpp`) |
| `server` | `include/performance/phase3/bao.h`, `include/performance/workload_adaptive_optimizer.h` | HTTP server integrates BAO and workload-adaptive optimizer for query routing decisions (`src/server/http_server.cpp`) |
| `storage` | `include/performance/prefetch_hints.h` | RocksDB wrapper applies prefetch hints for sequential scan paths (`src/storage/rocksdb_wrapper.cpp`) |
