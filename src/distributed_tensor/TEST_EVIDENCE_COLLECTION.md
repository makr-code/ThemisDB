# Distributed Tensor Test Evidence Collection

**Last Updated:** 2026-11-24
**Source level:** Level 1 — module implementation
**SOT domain:** build-test
**Canonical references:** `tests/CMakeLists.txt`, `tests/distributed_tensor/CMakeLists.txt`, `tests/epic3_distributed_tensor/CMakeLists.txt`, `tests/integration/CMakeLists.txt`, `tests/sharding/CMakeLists.txt`, `tests/sharding/test_shard_transfer_grpc.cpp`, `benchmarks/CMakeLists.txt`, `benchmarks/epic3_distributed_tensor/CMakeLists.txt`
**Related:** Issue #7341

<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot Coding Agent
Human_Review: false
Human_Author: ThemisDB Team
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

`DistributedPlannerContractTests` now includes source regressions for
projection encoding/decoding, canonical ordering, malformed and unsupported
payload rejection, deterministic multi-fragment merge, graph-backed planner
validation, and unsupported fragment payload failure. These test cases are
registered through the existing EPIC 3 planner target; none was built or run
for this change.

The root `tests/CMakeLists.txt` adds `tests/integration/` outside the legacy
root-test guard in both modular and non-modular configurations. No
distributed-tensor soak is registered there: the former
`test_distributed_tensor_soak.cpp` used only in-process queue, sync, and
routing stubs and has been removed. A Linux/gRPC-conditional
`ShardTransferGrpcTests` target in `tests/sharding/CMakeLists.txt` includes the
real mTLS fragment stream and manifest adapter with a test storage reader. An
additional case publishes a fixture summary through the local publisher, reopens
the database, fetches it via mTLS and the planner, and checks rejection of
an incorrect manifest digest and an unbound certificate. Its graph-validation
assertions are compiled only when the query and LLM targets exist. This is
fixture-driven publication, not an automatically wired production producer.
`ShardTransferStoreTests` also contains RocksDB-backed regressions for the
principal/shard/artifact mapping, immutable replay/conflict, size and digest
checks, and restart persistence. On 2026-11-24 the target passed 1/1 in CTest;
the complete `test_shard_transfer_grpc` executable passed 8/8 and its CTest
selection passed 1/1 on retry. A separate local minimal-profile `themis_server`
startup smoke published a projection, and a helper reopened RocksDB after
process exit to read its manifest and nonempty fragment. That smoke did not
test production configuration, graceful shutdown, power-loss durability, or a
client RPC against the running host.

## Benchmark registration

- `benchmarks/distributed_tensor/CMakeLists.txt` globs `bench_*.cpp`; its
  current local source is `bench_distributed_tensor_dedicated_gates.cpp`,
  which uses in-process stubs.
- `benchmarks/epic3_distributed_tensor/CMakeLists.txt` configures four
  standard benchmark sources (placement, integrity, recovery, distributed
  retrieval), plus partial-refit and summary-first targets, when
  `THEMIS_BUILD_BENCHMARKS` is enabled.

These registrations do not indicate that any executable was built or run.

## Nightly coverage gate

The nightly coverage workflow runs all CTest cases carrying the
`distributed_tensor` label and uploads the module LCOV/JSON report and a
per-path evidence file. The release-critical gate requires at least 70% line
and 60% branch coverage for `src/distributed_tensor/src/manifest_store.cpp`
and `src/distributed_tensor/src/artifact_manifest.cc`. A failed threshold is
not waived automatically; no path-specific exception is currently approved.
Workflow configuration is not evidence that a particular run passed. Use the
uploaded evidence from the exact commit when recording measured results.

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

## Focused validation evidence (2026-11-24)

The Linux WSL Debug build passed `ShardTransferStoreTests` (1/1) and
`ShardTransferGrpcTests` (1/1 on retry); direct execution of
`test_shard_transfer_grpc` passed 8/8. A separate manual minimal-profile host
smoke published artifact
`tspj1-524ac927fe0140255f04d6d3d9de9ec54f0b60d06ef605190e8e7e11a1c7e677`;
after the server process exited, a helper reopened the database and read the
manifest and a nonempty fragment. The host smoke used an HTTP-disabled build
with HSM unavailable and explicitly opted into the HSM stub. It is not
production-profile, graceful-shutdown, power-loss, or RPC-client evidence.
Trusted discovery, freshness/invalidation, continuous updates, benchmarks, and
soak coverage remain open.

## Validation attempt (2026-10-31)

- Configure command: `cmake -S . -B /tmp/themis-distributed-tensor-build
  -DCMAKE_TOOLCHAIN_FILE= -DTHEMIS_ALLOW_MISSING_ROCKSDB=ON
  -DTHEMIS_DIAGNOSTIC_MODE=ON -DTHEMIS_BUILD_TESTS=ON
  -DTHEMIS_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Debug`.
- Environment: CMake 3.31.6, GNU 13.3.0, Linux x86_64.
- Result: configuration stopped at `cmake/Dependencies.cmake:727` because the
  required `fmt` library was not found. The build tree was not generated, so no
  module or EPIC 3 test/benchmark target could be built or run.
- A standalone syntax-only compile of
  `include/distributed_tensor/tensor_storage_strategy.h` passed with C++20; the
  same header was rejected with C++17 at `std::span`, confirming its language
  requirement. This does not substitute for building the CMake target.
- `python3 tests/test_ai_dev_llm_wiki_sync.py` passed all 3 wiki-sync regression
  tests; these do not test the distributed-tensor module.

## Current evidence gaps

- Distributed-tensor test pass status remains unverified because configuration
  stopped before targets were generated.
- No benchmark was built or run; no representative-hardware measurements or
  production sign-off are recorded here.
- The configured sources do not, on their own, establish coverage adequacy for
  every production integration path.
