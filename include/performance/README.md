---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_Sources:
  - "include/performance/*.h"
  - "CMakeLists.txt"
  - "cmake/CMakeLists.txt"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Optimization Headers

## Scope and dataflow

`include/performance/` declares public interfaces for measurement, feature
controls, memory/cache management, workload adaptation, hardware dispatch, and
phase-specific algorithms. Implementations are in `src/performance/`; callers
consume these declarations from other modules. The headers are the authority
for signatures, ownership, exceptions, error results, and thread-safety.

The measurement path is compile-time gated: cycle macros in
[`cycle_metrics_config.h`](cycle_metrics_config.h) call the collector only when
the corresponding definitions are enabled; exporter implementations are
separate runtime components. Feature-flag classes expose runtime state, but a
runtime flag alone does not establish that a feature is compiled, supported by
the host, or actually used by a caller. Check the implementation and returned
result/capability fields for each path.

## Header groups

| Area | Representative public headers | Contract surface |
|---|---|---|
| Measurement and configuration | `cycle_metrics.h`, `cycle_metrics_config.h`, `expected_cycles.h`, `runtime_config.h` | Cycle and metric macros are compiled according to their respective definitions. |
| Feature controls | `feature_flags.h`, `phase2_feature_flags.h`, `phase3/feature_flags.h`, `phase4/feature_flags.h` | Phase-specific runtime controls; verify the implementation's compile and capability gates. |
| Memory and placement | `allocator.h`, `huge_pages.h`, `numa_memory_manager.h`, `numa_topology.h`, `alignment_helpers.h` | Allocation, alignment, topology and placement contracts; host OS support may limit behavior. |
| Cache and concurrency | `advanced_cache_manager.h`, `lirs_cache.h`, `rcu.h`, `rcu_hash_table.h`, `lockfree_metrics_buffer.h`, `lockfree_histogram.h` | Cache and synchronization interfaces; honor the documented concurrency contract (including SPSC-only buffers). |
| Workload adaptation | `workload_predictor.h`, `workload_adaptive_optimizer.h`, `adaptive_query_compiler.h`, `phase3/per_query_cost_model.h`, `phase3/adaptive_batch_tuner.h` | Forecasts, strategy recommendations and query-cost feedback. Recommendations do not themselves resize caller-owned resources. |
| Algorithms and optional I/O | `rabitq.h`, `phase3/diskann.h`, `phase3/bwtree.h`, `phase3/splinterdb.h`, `phase3/gunrock.h`, `phase4/io_uring_zero_copy.h`, `phase4/pmem_storage.h`, `phase4/pmu_counters.h` | Specialized algorithm and capability-dependent interfaces; experimental or optional behavior must not be inferred from a header name. |

Use [`performance_api_contract.h`](performance_api_contract.h) for its shared
error/result types, but follow each individual header where an API uses a
different error channel such as an exception, boolean status, or errno value.

## Build configuration

The repository declares these measurement/build controls in its CMake files:

- Root `CMakeLists.txt`: `THEMIS_ENABLE_CYCLE_METRICS`,
  `THEMIS_ENABLE_DETAILED_METRICS`, `THEMIS_ENABLE_GPU_CYCLE_METRICS`,
  `THEMIS_ENABLE_PMU_COUNTERS`, `THEMIS_ENABLE_IO_URING`,
  `THEMIS_ENABLE_METRICS_EXPORT`, and `THEMIS_BENCHMARK_MODE`.
- `cmake/CMakeLists.txt`: `THEMIS_ENABLE_MIMALLOC`,
  `THEMIS_ENABLE_HUGE_PAGES`, `THEMIS_ENABLE_RCU_INDEX`, and
  `THEMIS_ENABLE_LIRS_CACHE`.

These are not a complete list of every `THEMIS_ENABLE_*` identifier mentioned
in header comments. Whether a phase-specific flag is a configured CMake option
or is wired into a particular build target is not established by this list;
check the active CMake configuration before relying on it. Platform facilities
such as GPU counters, PMU access, NUMA policies, and io_uring additionally
depend on host support and permissions.

## Limits

- Research citations and comments about expected gains are not local benchmark
  results. No universal throughput, latency, or compression gain is promised
  here.
- Disabled cycle/metric macros expand to no-ops where defined that way; this
  does not establish zero overhead for every performance component.
- Optional paths may report fallback or failure. Inspect actual result fields
  instead of treating requested configuration as proof of activation.
- Keep allocation ownership, dimension/range validation, and threading
  requirements specific to each public declaration.

See the implementation overview at
[`../../src/performance/README.md`](../../src/performance/README.md), the caller
contract map at [`../../src/performance/API.md`](../../src/performance/API.md),
and CMake registration in `tests/performance/CMakeLists.txt` and
`benchmarks/performance/CMakeLists.txt`.

Exact model backend not verifiable from final document text alone.
