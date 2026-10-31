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
| `bench_replication_dedicated_gates.cpp` | Dedicated release-gate benchmark source; see its registrations/implementation for case names. |
| `bench_replication_multi_dc_multi_writer.cpp` | Geographic leader/failover selection, placement validation, WAL enqueue/lag, multi-writer conflict rate, multi-DC write throughput, and alert latency. |
| `bench_replication_release_gates.cpp` | CRDT merge, LWW resolution, event serialization, WAL apply, partition-offset lookup, and lag-check cases. |
| `bench_replication_throughput.cpp` | `WALEntry` serialization/deserialization, HLC conflict detection, CRDT merge, manager initialization, and promotion. |

These are benchmark definitions, not evidence that a benchmark was executed or that
any threshold was met. See `src/replication/PERFORMANCE_EXPECTATIONS.md` for the
documented mapping and its validation caveat.
