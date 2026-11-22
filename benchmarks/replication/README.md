---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - CMakeLists.txt
  - bench_geo_placement_wal_shipping.cpp
  - bench_replication_dedicated_gates.cpp
  - bench_replication_multi_dc_multi_writer.cpp
  - bench_replication_release_gates.cpp
  - bench_replication_throughput.cpp
  - ../../benchmarks/CMakeLists.txt
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication benchmarks

This folder contains replication benchmark sources; it is not a mirror of
`src/replication/`. `CMakeLists.txt` adds `bench_*.cpp` targets only when
`THEMIS_BUILD_BENCHMARKS` is enabled. The root `benchmarks/CMakeLists.txt` also has
explicit registrations for some of these files.

| Source | Measured areas |
|---|---|
| `bench_geo_placement_wal_shipping.cpp` | Candidate selection, placement validation, WAL shipping throughput, lag-alert latency, stats, and metrics export. |
| `bench_replication_dedicated_gates.cpp` | Standalone diagnostic of local queue/counter stubs. It is not a production-path benchmark and is intentionally not registered as a Google Benchmark CTest smoke. |
| `bench_replication_multi_dc_multi_writer.cpp` | Geographic leader/failover selection, placement validation, WAL enqueue/lag, multi-writer conflict rate, multi-DC write throughput, and alert latency. |
| `bench_replication_release_gates.cpp` | CRDT merge, LWW resolution, event serialization, WAL apply, partition-offset lookup, and lag-check cases. |
| `bench_replication_throughput.cpp` | `WALEntry` serialization/deserialization, HLC conflict detection, CRDT merge, manager initialization, and promotion. |

These are benchmark definitions, not evidence that a benchmark was executed or that
any threshold was met. See `src/replication/PERFORMANCE_EXPECTATIONS.md` for the
documented mapping and its validation caveat.

## Dedicated-gates diagnostic and correctness tests

The `bench_replication_dedicated_gates` executable prints the legacy REP-BM-01..04
threshold comparisons for local stubs. The existing values are unchanged, but
exceedances are warnings rather than enforced test assertions; the output is not
Google Benchmark JSON and is not production performance evidence. Run it manually
only when examining that diagnostic:

```powershell
cmake --build --preset windows-bench-release --target bench_replication_dedicated_gates
.\build-msvc-windows-bench\bin\bench_replication_dedicated_gates.exe
```

CTest retains the diagnostic as an ordinary, explicitly labeled executable
process-completion check (not a benchmark smoke); it does not turn the printed
threshold warnings into pass/fail assertions:

```powershell
ctest --test-dir build-msvc-windows-bench --tests-regex "^ReplicationDedicatedGateDiagnostic$" --output-on-failure
```

For replication correctness coverage, use the actual module test runner as well:

```powershell
ctest --preset replication-tests-release --tests-regex "^ReplicationHAFocusedTests$" --output-on-failure
```
