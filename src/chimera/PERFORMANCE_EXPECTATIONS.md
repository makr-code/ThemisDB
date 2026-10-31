---
Author: unknown (original author not verified)
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: AI documentation assistant
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

# Chimera benchmark coverage and performance expectations

**Purpose / audience:** identify benchmarked code and distinguish it from enforced performance criteria. No benchmark was run for issue #7336.

## Checked-in benchmark cases

| Source | Coverage | Important limitation |
|---|---|---|
| `benchmarks/chimera/bench_chimera_adapter.cpp` | CHM-01..06: connect, query, disconnect, 100 sequential query calls, transaction commit, error handling | Fixture default-constructs `ThemisDBAdapter`; it does not establish live-engine or connection-pool performance. Source comments mention thresholds, but no threshold enforcement is claimed here. |
| `benchmarks/chimera/bench_chimera_dedicated_gates.cpp` | CH-BM-01..04: routing counter, fallback counter, route decision, 100-value average | Uses local `RouterStub` and `stubMerge`, not production adapter/router code. Not a production release gate. |

`benchmarks/CMakeLists.txt` declares standard benchmark targets for both sources. `benchmarks/chimera/CMakeLists.txt` also registers globbed `bench_*.cpp` files when benchmark support is enabled. Actual inclusion depends on the enclosing benchmark configuration.

## Expectations

There is no source-backed basis here for numerical release thresholds, p95/p99 guarantees, or a claim that a benchmark run passed. To establish a release gate, connect a real adapter workload to the relevant CMake/CI gate, define the measurement environment and baseline, and record executed results.

## References

`benchmarks/chimera/bench_chimera_adapter.cpp`, `benchmarks/chimera/bench_chimera_dedicated_gates.cpp`, `benchmarks/CMakeLists.txt`, `benchmarks/chimera/CMakeLists.txt`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
