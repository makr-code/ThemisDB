# Distributed Tensor Test Evidence Collection

**Last Updated:** 2026-10-31
**Source level:** Level 1 — module implementation
**SOT domain:** build-test
**Canonical references:** `tests/CMakeLists.txt`, `tests/distributed_tensor/CMakeLists.txt`, `tests/epic3_distributed_tensor/CMakeLists.txt`, `tests/integration/CMakeLists.txt`, `benchmarks/CMakeLists.txt`, `benchmarks/epic3_distributed_tensor/CMakeLists.txt`
**Related:** Issue #7341

<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot Coding Agent
Human_Review: false
Human_Author: unknown
Exact model backend not verifiable from final document text alone.
-->

## Current configured test inventory

`tests/distributed_tensor/CMakeLists.txt` registers the following focused tests
from local sources:

| Source | CMake executable target | Registered CTest name |
|---|---|---|
| `test_tensor_allocation.cpp` | `module_distributed_tensor_test_tensor_allocation_focused` | `test_tensor_allocation_distributed_tensor_FocusedTests` |
| `test_tensor_operations.cpp` | `module_distributed_tensor_test_tensor_operations_focused` | `test_tensor_operations_distributed_tensor_FocusedTests` |
| `test_tensor_performance.cpp` | `module_distributed_tensor_test_tensor_performance_focused` | `test_tensor_performance_distributed_tensor_FocusedTests` |

The registration applies the `distributed_tensor` label and a 120-second
timeout. This is a source/CMake inventory only: no configure, build, CTest
execution, pass count, coverage result, or CI result is asserted here.

`tests/CMakeLists.txt` also conditionally adds
`tests/epic3_distributed_tensor/`. Its CMake file registers the EPIC 3
phase/contract sources, including `distributed_planner_test.cc`,
`test_manifest_store_phase_a.cpp`,
`test_lifecycle_staleness_management.cpp`, `test_tensor_delta_log.cpp`,
`test_tensor_rebuild_fallback.cpp`, `test_tensor_shard_summary.cpp`,
`test_phase3_failure_semantics.cpp`, `test_phase4_contract_coverage.cpp`,
`test_tensor_storage_strategy.cpp`, `test_tensor_training_coordinator.cpp`,
`test_tensor_update_worker.cpp`, `integrity_verification_test.cc`, and
`test_phase_b_edge_cases.cpp`. It conditionally adds the separate
`integrity_verification_bench.cc` target when the benchmark dependency is
available. See that CMake file for exact guards, target names, labels, and
timeouts.

`tests/integration/CMakeLists.txt` includes `test_distributed_tensor_soak.cpp`
when present and assigns that soak test a 120-second timeout.

## Benchmark registration

- `benchmarks/distributed_tensor/CMakeLists.txt` globs `bench_*.cpp`; its
  current local source is `bench_distributed_tensor_dedicated_gates.cpp`,
  which uses in-process stubs.
- `benchmarks/epic3_distributed_tensor/CMakeLists.txt` configures four
  standard benchmark sources (placement, integrity, recovery, distributed
  retrieval), plus partial-refit and summary-first targets, when
  `THEMIS_BUILD_BENCHMARKS` is enabled.

These registrations do not indicate that any executable was built or run.

## Collecting evidence

For a future validation run, use the repository's configured build preset and
record:

1. The exact commit, preset, compiler, platform, and CMake options.
2. The configure/build result for all applicable target names above.
3. CTest output and the exact registered test names, including failures and
   skipped tests, from the local, EPIC 3, and integration groups as applicable.
4. The benchmark binary/output separately from test results. Distinguish the
   local stub benchmark from EPIC 3 workload benchmarks; see
   [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).
5. Any representative-hardware results with hardware and topology disclosed.

The EPIC 3 paths and registrations are present in the current CMake
configuration. Historical test counts and pass-status statements still require
their original execution artifacts; source registration alone does not confirm
that they passed.

## Current evidence gaps

- Test execution and pass status have not been checked in this documentation
  task.
- No benchmark results or representative-hardware measurements are recorded
  here.
- The configured sources do not, on their own, establish coverage adequacy for
  every production integration path.
