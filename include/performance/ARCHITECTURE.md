---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "active"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_Sources:
  - "include/performance/*.h"
  - "src/performance/*.cpp"
  - "CMakeLists.txt"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Module — Public Header Architecture

## Boundary and dataflow

`include/performance/` is the public declaration layer; implementations are
under `src/performance/`. A caller configures build-time support, checks the
relevant runtime flag and host capability, then invokes a component-specific
interface. Results, exceptions, error codes, or fallback indicators flow back
to the caller. The headers do not create one central optimization pipeline:
callers decide which optional components to compose and whether to apply a
recommendation.

The measurement path is separately gated: cycle macros may record through the
collector, which feeds the module's exporter implementations when configured.
Workload predictors and optimizers accept observations and return forecasts or
strategies; they do not prove that downstream resources changed. Hardware,
NUMA, PMU, persistent-memory, and io_uring interfaces remain conditional on
their implementation and host prerequisites.

## Header groups

| Group | Representative declarations |
|---|---|
| Measurement and configuration | `cycle_metrics.h`, `cycle_metrics_config.h`, `runtime_config.h`, `feature_flags.h` |
| Adaptive behavior and feedback | `workload_predictor.h`, `workload_adaptive_optimizer.h`, `adaptive_query_compiler.h`, `phase3/per_query_cost_model.h`, `phase3/adaptive_batch_tuner.h` |
| Memory, cache, and concurrency | `allocator.h`, `numa_memory_manager.h`, `numa_topology.h`, `advanced_cache_manager.h`, `lirs_cache.h`, `rcu.h`, `lockfree_metrics_buffer.h` |
| Algorithms and optional facilities | `rabitq.h`, `phase3/diskann.h`, `phase3/bwtree.h`, `phase3/splinterdb.h`, `phase3/gunrock.h`, `phase4/io_uring_zero_copy.h`, `phase4/pmem_storage.h`, `phase4/pmu_counters.h` |

Use the declarations and comments in the individual headers for exact
signatures, ownership, synchronization, validation, and error semantics. Shared
result types are in [`performance_api_contract.h`](performance_api_contract.h);
not all interfaces use that common error vocabulary.

## Configuration boundary

The root build declares measurement options including
`THEMIS_ENABLE_CYCLE_METRICS`, `THEMIS_ENABLE_DETAILED_METRICS`,
`THEMIS_ENABLE_GPU_CYCLE_METRICS`, `THEMIS_ENABLE_PMU_COUNTERS`,
`THEMIS_ENABLE_IO_URING`, and `THEMIS_ENABLE_METRICS_EXPORT`. Additional
allocator/cache options are declared in `cmake/CMakeLists.txt`. These do not
establish that every feature-flag name in the headers has a corresponding
configured option or active implementation path; inspect the target build and
implementation before documenting a specific feature as available.

For implementation composition and failure handling, see
[`../../src/performance/ARCHITECTURE.md`](../../src/performance/ARCHITECTURE.md).

Exact model backend not verifiable from final document text alone.
