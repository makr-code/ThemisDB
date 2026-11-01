# benchmarks/metadata

**Author:** ThemisDB Contributors  
**Created:** 2026-10-31  
**Last Updated:** 2026-10-31  
**Status:** review  
**AI_Used:** true  
**AI_Models:** unknown  
**AI_Contributor:** GitHub Copilot coding agent / ThemisDB documentation orchestrator  
**Human_Review:** false  
**Human_Author:** ThemisDB Contributors  
**Source level:** level1 — module-local  
**SOT domain:** module-behavior  
**Canonical source references:** [benchmark registration](./CMakeLists.txt), [benchmark sources](./), [public metadata contracts](../../include/metadata/)  
**Related issue / milestone:** [#7362](https://github.com/makr-code/ThemisDB/issues/7362) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

This directory contains Google Benchmark cases. The benchmark sources do not
themselves compare measured values with the listed targets or fail a run when a
target is missed; a release runner must perform that comparison separately.

## Gate Table

| ID          | Benchmark                                     | Gate          | File                                         |
|-------------|-----------------------------------------------|---------------|----------------------------------------------|
| GATE-MET-01 | `MetaError` cast throughput                     | Historical target unverified; no threshold assertion in source | bench_metadata_release_gates.cpp |
| GATE-MET-02 | `MetaError` switch dispatch                     | Historical target unverified; no threshold assertion in source | bench_metadata_release_gates.cpp |
| GATE-MET-03 | `MetaError` range check                         | Historical target unverified; no threshold assertion in source | bench_metadata_release_gates.cpp |
| GATE-MET-04 | `MetaError` batch cast                          | Historical target unverified; no threshold assertion in source | bench_metadata_release_gates.cpp |
| GATE-MCL-01 | `ConsistencyIssue::toJSON()` throughput         | ≥ 10M ops/s target; not asserted by benchmark | bench_metadata_consistency_lineage_gates.cpp |
| GATE-MCL-02 | `ColumnRef::toString()` throughput              | ≥ 50M ops/s target; not asserted by benchmark | bench_metadata_consistency_lineage_gates.cpp |
| GATE-MCL-03 | `ColumnRef` JSON round-trip        | ≥ 5M ops/s target; not asserted by benchmark | bench_metadata_consistency_lineage_gates.cpp |
| GATE-MCL-04 | `TransformationType` string conversion          | ≥ 20M ops/s target; not asserted by benchmark | bench_metadata_consistency_lineage_gates.cpp |

`GATE-MET-*` benchmarks exercise `MetaError` from
[`metadata_api_contract.h`](../../include/metadata/metadata_api_contract.h), a
frozen contract artifact. They do not measure the current exporter/runtime error
types. See [`src/metadata/API.md`](../../src/metadata/API.md) for the contract
boundary.

## Cache benchmarks

`bench_metadata_cache.cpp` covers the cache-centric SchemaManager hot paths
(cold scan, warm hit, concurrent reads, adaptive TTL).

## Registration and evidence

The three executables are registered in this directory's `CMakeLists.txt`
through `themis_add_standard_benchmark()`. The parent benchmark configuration
requires `THEMIS_BUILD_BENCHMARKS=ON` and Google Benchmark to be available.
Source registration and benchmark-case presence do not establish that a run was
performed, a threshold was met, or a release gate passed. No benchmark was run
for this documentation update.
