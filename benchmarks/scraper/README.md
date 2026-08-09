# benchmarks/scraper

Mirrored benchmark folder for `src/scraper`.

## Pipeline-Depth Benchmarks (bench_scraper_pipeline_depth)

Dedicated pipeline-throughput benchmarks introduced in Q4 2026 wave.

| Gate ID  | Benchmark                    | Threshold               |
|----------|------------------------------|-------------------------|
| PIPE-01  | BatchEmit1000                | p99 ≤ 500 µs / batch    |
| PIPE-02  | SummaryAggregation10k        | total ≤ 5 ms            |
| PIPE-03  | ScrapeRequestBatchAlloc 1000 | p99 ≤ 200 µs / batch    |
| PIPE-04  | FaultClassLoop 10 000×       | p99 ≤ 10 µs / batch     |
