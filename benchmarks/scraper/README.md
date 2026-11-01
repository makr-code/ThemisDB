# Scraper Benchmarks

**Author:** ThemisDB Contributors<br>
**Created:** 2026-10-31<br>
**Last Updated:** 2026-10-31<br>
**Status:** review

The module contains microbenchmarks for scraper helper/type paths. They are not end-to-end scrape, HTTP, HTML extraction, evaluator-quality, or durable-write benchmarks. Thresholds below are source-declared labels; no measured result or baseline is implied.

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot; themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** unknown
Exact model backend not verifiable from final document text alone.

## Release-Gate Benchmarks (`bench_scraper_release_gates`)

| Gate ID | Benchmark case | Source-declared threshold label |
|---|---|---|
| GATE-SCR-01 | `BM_SCR01_ErrorEnumCast` | p99 ≤ 5 ns |
| GATE-SCR-02 | `BM_SCR02_SwitchDispatch` | p99 ≤ 10 ns |
| GATE-SCR-03 | `BM_SCR03_StructAlloc` | p99 ≤ 500 ns |
| GATE-SCR-04 | `BM_SCR04_BatchCast` | p99 ≤ 5 µs/batch |
| GATE-SCR-05 | `BM_SCR05_BurstAcquire` | p99 ≤ 100 ns |
| GATE-SCR-06 | `BM_SCR06_SummaryRead` | p99 ≤ 1 µs |

## Pipeline-Depth Benchmarks (bench_scraper_pipeline_depth)

These cases exercise diagnostic event emission, summary aggregation, request allocation, and fault-classification helpers—not the full scraper pipeline.

| Gate ID | Benchmark case | Source-declared threshold label |
|---|---|---|
| PIPE-01 | `BM_PIPE01_BatchEmit1000` | p99 ≤ 500 µs/batch |
| PIPE-02 | `BM_PIPE02_SummaryAggregation10k` | total ≤ 5 ms |
| PIPE-03 | `BM_PIPE03_ScrapeRequestBatchAlloc` | p99 ≤ 200 µs/batch |
| PIPE-04 | `BM_PIPE04_FaultClassLoop10k` | p99 ≤ 10 µs/batch |

## CMake Registration and Gate Semantics

When `THEMIS_BUILD_BENCHMARKS` is enabled, `benchmarks/scraper/CMakeLists.txt` calls `themis_add_standard_benchmark` to register `bench_scraper_release_gates` and `bench_scraper_pipeline_depth` from their corresponding source files. When the option is off, the directory returns without registering either target.

The thresholds above are labels/comments in the benchmark sources; the benchmark code does not compare measurements to them or fail on a threshold breach. `SetLabel`, repetitions, and aggregate reporting are not automated pass/fail gates, and the p99 labels do not establish that p99 percentiles are measured. Treat them as source-declared targets only; no result or baseline is implied.
