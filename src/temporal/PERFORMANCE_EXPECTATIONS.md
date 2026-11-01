# Temporal performance evidence and benchmark mapping

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

## Runtime benchmark cases

`benchmarks/temporal/bench_temporal_queries.cpp` benchmarks the
`themisdb::temporal::BiTemporalTable` implementation:

| Area | Benchmark cases | Input dimensions in source |
|---|---|---|
| Insert | `BM_BiTemporalTable_Insert` | 1, 100 rows per key |
| Update | `BM_BiTemporalTable_Update` | 1, 100 versions |
| Delete | `BM_BiTemporalTable_Delete` | Single-key repeated delete/reinsert |
| Bitemporal read | `BM_BiTemporalTable_QueryBiTemporal` | 10, 100, 1,000 versions |
| Current valid-time read | `BM_BiTemporalTable_QueryCurrentByValidTime` | 10, 100 keys |
| History | `BM_BiTemporalTable_GetHistory` | See benchmark source for setup |

`benchmarks/temporal/bench_interval_tree_erase.cpp` exercises
`IntervalTreeIndex` erase/rebuild and mixed read/erase workloads.

## Registration and thresholds

`benchmarks/temporal/CMakeLists.txt` glob-registers `bench_*.cpp` with
`themis_add_standard_benchmark` when `THEMIS_BUILD_BENCHMARKS` is enabled. The
top-level CMake default is `OFF`.

The other two sources are not runtime performance evidence:

- `bench_temporal_dedicated_gates.cpp` times local stub functions.
- `bench_temporal_release_gates.cpp` measures mock vector operations and helper
  functions; it does not call the temporal runtime APIs.

Their embedded thresholds are source comments, not evidence that release gates
are wired to or have passed on a configured runtime benchmark. The inspected
module sources do not provide benchmark results, a release baseline, a p99
threshold configuration, or manifest-completeness validation.

**TODO:** before treating a threshold as an active release gate, identify its
canonical release configuration, runtime workload, baseline artifact, and
recorded run result. No numeric acceptance target is asserted by this page.
