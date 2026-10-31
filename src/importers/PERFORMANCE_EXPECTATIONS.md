---
Author: "ThemisDB Team"
Created: "2026-08-02"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "benchmarks/importers/bench_importers_release_gates.cpp; benchmarks/importers/bench_importer_throughput.cpp; benchmarks/importers/CMakeLists.txt"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Creation-date note: Original creation date is not recorded in the available checkout; the front-matter `Created` date is a metadata placeholder, not independently verified.

> Exact model backend not verifiable from final document text alone.

# PERFORMANCE_EXPECTATIONS - src/importers

> Thresholds below are target labels declared by benchmark sources. They are
> not current measured results, computed p99 values, or enforced release gates.
> No benchmark was run for this documentation update.

## Scope

- Module: src/importers
- This file maps source-declared performance targets and benchmark scenarios; it
  does not itself enforce gates or record run results.

## Benchmark Reference

- Phase 5 Release Gates (Phase 5 — Performance and Hardening):
  - benchmarks/importers/bench_importers_release_gates.cpp (IMRG-01..IMRG-06, primary gates)
  
- Legacy/Extended benchmarks (historical reference):
  - benchmarks/importers/bench_importer_throughput.cpp (mock/synthetic scan and helper scenarios)

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| BM_Importer_Postgres | Synthetic PostgreSQL-like dump scan (counts INSERT statements and COPY declarations, not individual COPY data rows) | 10,000 and 100,000 generated rows |
| BM_Importer_Sqlite | Synthetic SQLite-like dump scan | 10,000 and 100,000 rows |
| BM_Importer_MongoNdjson / BM_Importer_MongoJsonArray | Synthetic payload line scan | 10,000 and 100,000 docs |
| BM_Importer_Mysql | Synthetic MySQL-like dump scan | See source arguments |
| BM_Importer_KafkaJsonParse | JSON parsing of generated payload | See source arguments |
| BM_Importer_ConflictResolve | In-memory parity check over generated row IDs; does not invoke `ConflictResolver` | See source arguments |

These are not connector execution benchmarks: PostgreSQL/SQLite/MySQL and
Mongo cases scan generated text rather than invoking the production importer.

## Source-declared target labels (IMRG-01..IMRG-06)

### Benchmark-source target labels

| Gate ID | Expectation | Threshold | Measurement | Benchmark |
|---|---|---|---|---|
| GATE-IMRG-01 | CSV row parse throughput | ≥5M rows/s | throughput | BM_IMRG01_CsvRowParse |
| GATE-IMRG-02 | Schema validation latency | p99 ≤50µs | latency | BM_IMRG02_PerRowSchemaValidation |
| GATE-IMRG-03 | Duplicate key check latency | p99 ≤100µs | latency | BM_IMRG03_DuplicateKeyCheck |
| GATE-IMRG-04 | Row buffer commit latency | p99 ≤5ms RT | latency (RealTime) | BM_IMRG04_RowBufferCommit |
| GATE-IMRG-05 | Import quota check latency | p99 ≤50µs | latency | BM_IMRG05_ImportQuotaCheck |
| GATE-IMRG-06 | Schema evolution check latency | p99 ≤200µs | latency | BM_IMRG06_SchemaEvolutionCheck |

**Source configuration:** seed constant 42; five benchmark repetitions; 200
explicit warmup calls in each body; in-memory/mock work. The source does not
calculate p99 or contain threshold assertions.

## Validation

- Phase 5 Release Gates (IMRG-01..IMRG-06):
  - A target is evaluated only after running the corresponding binary and retaining
    its output, toolchain/build profile, and machine context.
  - Source labels and repeated runs alone do not define a pass/fail threshold.
  - Only the duplicate-key gate uses the seed constant for generated key selection;
    do not infer a benchmark-wide seeded data generator.
  
- Historical IMPP-1..IMPP-7 references from earlier versions are not treated as
  current cases; use only symbols present in the benchmark sources above.

## Source References (not evidence of a passing run)

### IMRG target cases
- Verified benchmark source:
  - benchmarks/importers/bench_importers_release_gates.cpp
- Source-declared benchmark cases: IMRG-01..IMRG-06 (6 cases)
  - CSV parser, schema validator, dedup checker, row commit, quota check, schema evolution check
  - All six use in-memory/mock work; the seed constant is used for duplicate-key selection
- Status: implemented benchmark cases. No current run result or threshold
  enforcement is established by the source file alone.

### Throughput benchmark cases
- Verified benchmark sources:
  - benchmarks/importers/bench_importer_throughput.cpp
- Verified mapping surfaces:
  - PostgreSQL, SQLite, Mongo, MySQL, Kafka, and conflict-resolution throughput scenarios
  - process import/export/list helper benchmark scenarios are not in this
    importer benchmark source; TODO: identify a current primary source before
    documenting such cases.