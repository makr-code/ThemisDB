# PERFORMANCE_EXPECTATIONS - src/importers

## Scope

- Module: src/importers
- This file defines measurable importers module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_importer_throughput.cpp
  - benchmarks/bench_process_import_retrieval.cpp

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

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| IMPG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| IMPG-2 | importer hot-path p99 <= release threshold | p99 from mapped importer benchmark cases |
| IMPG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional importer benchmark scenarios are introduced.

## Sourcecode Verification (Module: importers/performance)

- Verified benchmark sources:
  - benchmarks/bench_importer_throughput.cpp
  - benchmarks/bench_process_import_retrieval.cpp
- Verified mapping surfaces:
  - PostgreSQL, SQLite, Mongo, MySQL, Kafka, and conflict-resolution throughput scenarios
  - process import/export/list helper benchmark scenarios
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.