---
Author: ThemisDB Contributors
Created: 2026-10-21
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
  - ../../benchmarks/replication/CMakeLists.txt
  - ../../benchmarks/replication/bench_geo_placement_wal_shipping.cpp
  - ../../benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp
  - ../../benchmarks/replication/bench_replication_release_gates.cpp
  - ../../benchmarks/replication/bench_replication_throughput.cpp
  - ../../benchmarks/replication/bench_replication_dedicated_gates.cpp
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication performance expectations

## Benchmark inventory

The current benchmark sources are under `benchmarks/replication/` (not
`benchmarks/bench_replication_throughput.cpp` or `benchmarks/bench_changefeed_throughput.cpp`):

| Source | Current benchmark coverage |
|---|---|
| `bench_replication_throughput.cpp` | WAL-entry serialization/deserialization, HLC conflict detection, CRDT merge, manager initialization, promotion. |
| `bench_geo_placement_wal_shipping.cpp` | Leader/failover candidate selection, placement validation, WAL shipping, lag-alert latency, stats, and metrics export. |
| `bench_replication_multi_dc_multi_writer.cpp` | Geographic selection/validation, WAL enqueue/lag, conflict rate, multi-DC write throughput, and alert latency. |
| `bench_replication_release_gates.cpp` | CRDT merge, LWW resolution, event serialization, WAL apply, partition-offset lookup, and lag checks. |
| `bench_replication_dedicated_gates.cpp` | A source file exists; the reviewed file did not expose a conventional `BENCHMARK(...)` registration. **TODO:** Determine its actual invocation and gate semantics. |

`benchmarks/replication/CMakeLists.txt` enumerates `bench_*.cpp` files when
`THEMIS_BUILD_BENCHMARKS` is enabled. The root benchmark CMake also registers named
replication benchmark targets.

## Expectations and limits

- Compare measurements only on the same supported build profile and representative
  hardware; record the exact target, arguments, and baseline.
- Maintain regression thresholds and p95/p99 limits in a separately reviewed release
  gate/baseline artifact. This file does not establish a current numeric threshold.
- A benchmark source or CMake target proves registration/definition only; it does not
  prove a run succeeded or a target was met.

**TODO:** Replace historical mappings and thresholds only after a reproducible current
benchmark run and an identified baseline owner are available. No benchmark was run for
this documentation update.
