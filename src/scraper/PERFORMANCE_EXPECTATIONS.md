# Scraper Performance Expectations

<!-- Source: benchmarks/scraper/; scope is the benchmark cases currently registered for this module. -->

**Author:** ThemisDB Contributors<br>
**Created:** 2026-10-31<br>
**Last Updated:** 2026-10-31<br>
**Status:** review

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot; themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** unknown
Exact model backend not verifiable from final document text alone.

## Benchmark Coverage

`benchmarks/scraper/CMakeLists.txt` registers two benchmark targets when `THEMIS_BUILD_BENCHMARKS` is enabled. The current cases measure diagnostic primitives, small value-type allocation, and run-summary helpers; they do not measure end-to-end scraping, network fetch, HTML extraction, evaluator quality, or durable writes.

| Target | Cases | Source-declared label |
|---|---|---|
| `bench_scraper_release_gates` | `BM_SCR01_ErrorEnumCast`, `BM_SCR02_SwitchDispatch`, `BM_SCR03_StructAlloc`, `BM_SCR04_BatchCast`, `BM_SCR05_BurstAcquire`, `BM_SCR06_SummaryRead` | GATE-SCR-01..06; target labels include p99 bounds (5 ns, 10 ns, 500 ns, 5 µs/batch, 100 ns, 1 µs respectively) |
| `bench_scraper_pipeline_depth` | `BM_PIPE01_BatchEmit1000`, `BM_PIPE02_SummaryAggregation10k`, `BM_PIPE03_ScrapeRequestBatchAlloc`, `BM_PIPE04_FaultClassLoop10k` | PIPE-01..04; target labels include 500 µs/batch, 5 ms total, 200 µs/batch, and 10 µs/batch respectively |

These are source-declared label strings, not measured results or demonstrated release baselines; the labels do not establish that the benchmark calculates p99 percentiles. A benchmark run, suitable measurement method, hardware profile, and baseline evidence are required before claiming that a threshold is met.

## Limits and Follow-up

- The current benchmark set does not establish scraper fetch/parse/evaluate/write throughput or p95/p99 for an end-to-end scrape.
- Do not map the adjacent generic extraction/ingestion benchmarks to scraper performance without a documented workload and an explicit mapping.
- TODO: add deterministic scraper-pipeline workloads if release decisions require end-to-end performance evidence.

## Build Registration

The benchmark subdirectory returns without adding targets unless `THEMIS_BUILD_BENCHMARKS` is enabled. The two module targets are registered via `themis_add_standard_benchmark`; see [the benchmark CMake file](../../benchmarks/scraper/CMakeLists.txt).
