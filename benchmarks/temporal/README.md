# Temporal benchmarks

Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors

Exact model backend not verifiable from final document text alone.

This directory contains four benchmark sources. Its
[`CMakeLists.txt`](CMakeLists.txt) glob-registers `bench_*.cpp` with
`themis_add_standard_benchmark` when `THEMIS_BUILD_BENCHMARKS` is enabled; the
top-level CMake default for that option is `OFF`.

| Source | Workload |
|---|---|
| `bench_temporal_queries.cpp` | Runtime `BiTemporalTable` insert, update, delete, query, and history cases. |
| `bench_interval_tree_erase.cpp` | Runtime `IntervalTreeIndex` erase/rebuild and mixed read/erase cases. |
| `bench_temporal_dedicated_gates.cpp` | Four benchmark names implemented with local stub functions, not temporal runtime classes. |
| `bench_temporal_release_gates.cpp` | TRG-labelled examples using mock vector/helper operations, not the temporal runtime. |

The names and thresholds embedded in the stub/mock sources are not evidence of
runtime release-gate results. See
[`../../src/temporal/PERFORMANCE_EXPECTATIONS.md`](../../src/temporal/PERFORMANCE_EXPECTATIONS.md)
for the source-backed benchmark mapping and evidence gaps. No benchmarks were
run as part of this documentation update.
