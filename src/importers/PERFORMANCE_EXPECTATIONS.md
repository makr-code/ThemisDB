# PERFORMANCE_EXPECTATIONS - src/importers

## Scope

- Module: src/importers
- This file defines measurable importers module performance expectations for release gating.

## Benchmark Reference

- Phase 5 Release Gates (Phase 5 — Performance and Hardening):
  - benchmarks/importers/bench_importers_release_gates.cpp (IMRG-01..IMRG-06, primary gates)
  
- Legacy/Extended benchmarks (historical reference):
  - benchmarks/bench_importer_throughput.cpp (IMPP-1..IMPP-7 scenarios)
  - benchmarks/bench_process_import_retrieval.cpp (process import helper scenarios)

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| IMPP-1 | PostgreSQL dump import throughput and parse behavior remain bounded across scale and dry-run modes | BM_ImportCopyRows_10k, BM_ImportCopyRows_100k, BM_ImportCopyRows_1M, BM_ImportInsertRows_10k, BM_ImportMixedLoad, BM_ImportDryRun_100k |
| IMPP-2 | SQLite import throughput remains bounded across scale and dry-run modes | BM_SQLiteInsertRows_10k, BM_SQLiteInsertRows_100k, BM_SQLiteMixedLoad, BM_SQLiteDryRun_100k |
| IMPP-3 | Mongo import payload parsing/import throughput remains bounded across formats and dry-run modes | BM_MongoNdjson_10k, BM_MongoNdjson_100k, BM_MongoJsonArray_10k, BM_MongoJsonArray_100k, BM_MongoBsonTypes_10k, BM_MongoDryRun_100k |
| IMPP-4 | MySQL/MariaDB import throughput remains bounded across scale and dry-run modes | BM_MySQLInsertRows_10k, BM_MySQLInsertRows_100k, BM_MySQLInsertRows_1M, BM_MySQLMixedLoad, BM_MySQLDryRun_100k |
| IMPP-5 | Kafka mock import throughput and dry-run behavior remain bounded | BM_KafkaImport_100k, BM_KafkaImport_100k_4k, BM_KafkaDryRun_100k |
| IMPP-6 | conflict-resolution overhead remains bounded against baseline behavior | BM_ConflictBaseline_100k, BM_ConflictOverwrite_100k, BM_ConflictSkip_100k, BM_ConflictOverwrite_100f, BM_ConflictMerge_10f_100k, BM_ConflictMerge_100f_100k |
| IMPP-7 | process-import retrieval helper benchmarks remain bounded for import/export listing surfaces | BM_BpmnImport_NodeCount, BM_BpmnExport, BM_EpkImport_EventCount, ProcessManagerFixture/List_Scan |

## Module Hard Gates (Phase 5 Release Gates)

### Primary Release Gates (IMRG-01..IMRG-06, verified 2026-08-02)

| Gate ID | Expectation | Threshold | Measurement | Benchmark |
|---|---|---|---|---|
| GATE-IMRG-01 | CSV row parse throughput | ≥5M rows/s | throughput | BM_IMRG01_CsvRowParse |
| GATE-IMRG-02 | Schema validation latency | p99 ≤50µs | latency | BM_IMRG02_PerRowSchemaValidation |
| GATE-IMRG-03 | Duplicate key check latency | p99 ≤100µs | latency | BM_IMRG03_DuplicateKeyCheck |
| GATE-IMRG-04 | Row buffer commit latency | p99 ≤5ms RT | latency (RealTime) | BM_IMRG04_RowBufferCommit |
| GATE-IMRG-05 | Import quota check latency | p99 ≤50µs | latency | BM_IMRG05_ImportQuotaCheck |
| GATE-IMRG-06 | Schema evolution check latency | p99 ≤200µs | latency | BM_IMRG06_SchemaEvolutionCheck |

**Configuration:** Seed = 42, Repetitions = 5, WarmupIterations = 200, self-contained (in-memory mocks)

### Legacy Release Gates (v1.0 docs baseline, IMPG-1..IMPG-3)

| Gate ID | Expectation | Measurement |
|---|---|---|
| IMPG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| IMPG-2 | importer hot-path p99 <= release threshold | p99 from mapped importer benchmark cases |
| IMPG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Phase 5 Release Gates (IMRG-01..IMRG-06):
  - Expectations are met when benchmarks run reproducibly in release profile
  - All gates use deterministic PRNG seed = 42 for reproducibility
  - Hot paths validated against thresholds via benchmark configuration
  - Mock data and self-contained runs eliminate external I/O variability
  
- Legacy expectations (IMPP-1..IMPP-7):
  - Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds
  - Mapping should be expanded as additional importer benchmark scenarios are introduced

## Sourcecode Verification (Module: importers/performance)

### Phase 5 Release Gates (verified 2026-08-02)
- Verified benchmark source:
  - benchmarks/importers/bench_importers_release_gates.cpp
- Verified release gate benchmarks: IMRG-01..IMRG-06 (6 gates)
  - CSV parser, schema validator, dedup checker, row commit, quota check, schema evolution check
  - All gates use deterministic PRNG and self-contained in-memory mocks
- Result:
  - Phase 5 release gates are implemented, verified, and reproducible
  - Hard thresholds established (see Module Hard Gates above)
  - Ready for CI/CD integration

### Legacy Benchmark Verification (IMPP-1..IMPP-7)
- Verified benchmark sources:
  - benchmarks/bench_importer_throughput.cpp
  - benchmarks/bench_process_import_retrieval.cpp
- Verified mapping surfaces:
  - PostgreSQL, SQLite, Mongo, MySQL, Kafka, and conflict-resolution throughput scenarios
  - process import/export/list helper benchmark scenarios
- Result:
  - Referenced benchmark cases exist in current benchmark sources
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons