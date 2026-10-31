---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Module Implementation

## Purpose and ownership

This is the implementation map for developers changing `src/performance`.
Public contracts are declared under [`include/performance`](../../include/performance);
implementation and module-local policy live under `src/performance`. The module
provides measurement/export, optimization controls, memory/cache behavior,
hardware-aware dispatch, and selected storage and vector-search algorithms. It
does not own query semantics or benchmark orchestration.

## Code organization

| Area | Primary implementation |
|---|---|
| Cycle measurement and export | `cycle_metrics.cpp`, `prometheus_exporter.cpp`, `async_metrics_exporter.cpp`, `chimera_exporter.cpp` |
| Adaptive workload and optimization | `workload_predictor.cpp`, `workload_adaptive_optimizer.cpp`, `adaptive_query_compiler.cpp` |
| Cache, memory, and placement | `advanced_cache_manager.cpp`, `numa_memory_manager.cpp`, `numa_topology.cpp`, `intelligent_prefetcher.cpp` |
| Hardware feature detection and dispatch | `phase2_feature_flags.cpp`, `hardware_accelerator.cpp` |
| Phase 2 algorithms | `wisckey.cpp`, `dostoevsky.cpp`, `cicada.cpp`, `ligra.cpp`, `rabitq.cpp` |
| Phase 3 algorithms and feedback | `phase3/`, including `per_query_cost_model.cpp`, `diskann.cpp`, and `adaptive_batch_tuner.cpp` |
| Phase 4 I/O and counters | `phase4/`, including `io_uring_zero_copy.cpp`, `pmem_storage.cpp`, and `pmu_counters.cpp` |

## Implementation invariants

- Honor both compile-time support and runtime feature controls. A compile-time
  disabled path must not be reported as available because a runtime flag is set.
- Check hardware and operating-system capability before using an optional
  instruction, device, memory policy, or I/O facility. Preserve the documented
  fallback or explicit error behavior when a prerequisite is absent.
- Validate dimensions, configuration ranges, buffer indexes, and resource
  bounds at the owning API boundary. Do not silently clamp invalid settings
  unless that API explicitly defines clamping.
- Keep timing and metrics overhead bounded. Respect collection sampling and
  per-thread buffer capacity; dropped measurements are reported by the
  collector rather than blocking a hot path.
- Use the interface-specific synchronization contract. For example, the
  Phase 4 io_uring object is not safe for concurrent send/receive calls on one
  instance; callers must serialize access or use one instance per thread.
- Do not infer that a device is truly accelerated from a configured device
  enum. Check execution results and `used_hw_path`; the accelerator header
  describes some device routes as placeholders and GPU dispatch as gated.

## Failure handling

The shared error vocabulary is declared in
[`performance_api_contract.h`](../../include/performance/performance_api_contract.h).
Optional capability failures should remain explicit and observable; preserve
the CPU or standard-I/O path where the API specifies fallback. Configuration
errors, invalid dimensions, allocation failures, stale plan state, unavailable
statistics, and exporter failures have distinct caller-visible behavior.
Avoid converting a real failure into a successful-looking metric or result.

## Test and benchmark evidence

Focused C++ tests are under [`tests/performance`](../../tests/performance).
The module CMake file registers those test sources and derives focused target
names from their source filenames. Tests relevant to the changed implementation
should be run alongside the affected release benchmark cases in
[`benchmarks/performance/bench_performance_release_gates.cpp`](../../benchmarks/performance/bench_performance_release_gates.cpp).
The repository's docs validation command is:

```bash
python -m unittest discover -s tests -p "test_doc_metadata_gate.py"
```

For performance changes, compare a reproducible release-profile run against
the mapped baseline and retain the manifest; see
[PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md).

Exact model backend not verifiable from final document text alone.
