# PERFORMANCE_EXPECTATIONS - src/metadata

## Scope

- Module: src/metadata
- This file defines measurable metadata module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_metadata_cache.cpp

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

## Sourcecode Verification (Module: metadata/performance)

- Verified benchmark source:
  - benchmarks/bench_metadata_cache.cpp
- Verified mapping surfaces:
  - cold/warm metadata cache behavior
  - lookup, refresh, TTL adaptation, and concurrent reads
- Result:
  - Referenced benchmark cases exist in current benchmark source.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.