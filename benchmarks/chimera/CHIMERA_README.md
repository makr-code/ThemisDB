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

# Chimera benchmark sources and limits

**Purpose / audience:** distinguish the checked-in Python synthetic suite from the C++ adapter benchmarks. This page is not benchmark output or a release-performance claim.

## Python CI benchmark runner

`external/chimera/run_ci_benchmarks.py` runs four synthetic in-process workloads: integer sort, 128-element dot product, key lookup in a 500-entry dictionary, and BFS over a 200-node chain. Defaults in the runner are 50 warm-ups and 500 measured iterations; output defaults to `benchmark_results/chimera_results.json`. These operations do not invoke ThemisDB adapters, storage, indexes, or external database services.

The runner's workload IDs are mapped as CHI-1 through CHI-4 in `benchmarks/benchmark_target_mapping.json`. The checked-in `benchmarks/baselines/chimera/baseline.json` is dated 2026-03-01 and contains an all-zero commit identifier; it is not evidence of a current reproducible run.

## Schema and methodology drift

`benchmarks/chimera/benchmark_config_schema.yaml` describes different workload sizes/shapes (including 64-dimensional vectors and alternate graph/document descriptions) and different warm-up/iteration defaults than the current Python runner. Treat it as a separate/stale schema artifact until reconciled. This README reports the runner source, not the YAML as an executed configuration.

The Python runner computes timing/percentile reports and writes version/branch/commit metadata. No benchmark was run for issue #7336; no baseline comparison or enforced SLO is asserted here. The referenced comparison/demo CSV is illustrative and not current ThemisDB performance evidence.

## C++ benchmark targets

`bench_chimera_adapter.cpp` registers CHM-01..06 workloads using a default-constructed `ThemisDBAdapter`; it measures the adapter's in-process path and does not establish connection-pool or live-server performance. `bench_chimera_dedicated_gates.cpp` measures local `RouterStub` counters and a local average calculation; its cases do not call production router/merge code.

`benchmarks/CMakeLists.txt` declares `bench_chimera_adapter` and `bench_chimera_dedicated_gates`; the local `CMakeLists.txt` glob-registers `bench_*.cpp` when benchmark support is enabled. These sources are separate from the Python runner. Benchmark target inclusion and execution depend on the configured build; no target was built or run here.

## References

`external/chimera/run_ci_benchmarks.py`, `benchmarks/chimera/benchmark_config_schema.yaml`, `benchmarks/chimera.py`, `benchmarks/benchmark_target_mapping.json`, `benchmarks/baselines/chimera/baseline.json`, local C++ sources, `benchmarks/CMakeLists.txt`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
