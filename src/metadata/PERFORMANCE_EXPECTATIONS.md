# PERFORMANCE_EXPECTATIONS - src/metadata

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
**Canonical source references:** [metadata roadmap](./ROADMAP.md), [metadata cache benchmark source](../../benchmarks/metadata/bench_metadata_cache.cpp)
**Related issue / milestone:** [#7133](https://github.com/makr-code/ThemisDB/issues/7133) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

## Scope

- Module: src/metadata
- This file defines measurable metadata module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/metadata/bench_metadata_cache.cpp
- The mapped `BM_MetadataCache_*` cases below are present in the referenced
  benchmark source. Their presence does not establish that a current benchmark
  run was executed or that any expectation or gate was met.

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| METP-1 | metadata cold discovery scans remain bounded across table-count scaling | BM_MetadataCache_ColdScan, BM_MetadataCache_RocksDBScan_Direct |
| METP-2 | warm metadata cache retrieval paths remain bounded | BM_MetadataCache_WarmHit, BM_MetadataCache_HitRate_Hit, BM_MetadataCache_HitRate_Miss |
| METP-3 | table-level and database-level metadata lookup paths remain bounded | BM_MetadataCache_GetTable_Hit, BM_MetadataCache_GetTable_Miss, BM_MetadataCache_GetDatabaseMetadata |
| METP-4 | metadata cache maintenance and adaptation paths remain bounded under operational load | BM_MetadataCache_RefreshCache, BM_MetadataCache_TTLVariants, BM_MetadataCache_AdaptiveTTL, BM_MetadataCache_ConcurrentReads |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| METG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| METG-2 | metadata hot-path p99 <= release threshold | p99 from mapped metadata benchmark cases |
| METG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional metadata benchmark scenarios are introduced.

## Source Verification (Module: metadata/performance)

- Verified benchmark source:
  - benchmarks/metadata/bench_metadata_cache.cpp
- The mapped cases are present in that source. The expectations and gates above
  remain targets; this source reference is not evidence of a current run,
  measured results, or a passing release gate. Release claims require recorded,
  reproducible measurements and baseline comparisons.