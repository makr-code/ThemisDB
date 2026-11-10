---
Author: "ThemisDB Team"
Created: "2026-11-09"
Last Updated: "2026-11-09"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# Benchmark Performance Evaluation (2026-11-09)

## Executive Summary

The serial Windows Release benchmark run executed 416 CTest cases: 400 passed and 16 failed (96.2% pass rate). Total test time was 8,662.86 seconds (about 2 h 24 min). The failures comprise 10 timeouts, three segmentation faults, and three Windows process exits with code `0xc0000409`.

This run does not support a suite-wide performance pass/fail conclusion. The postprocessing step could not parse the only persisted benchmark JSON because it was truncated after a benchmark process failure. It loaded zero result entries, leaving all 163 performance expectation targets as `NOT_RUN`.

Seven complete records could still be recovered from the partial latency JSON. They show low latency for warmed, single-threaded RocksDB reads and higher latency for writes, but they are a single run without a comparable raw baseline. Treat these numbers as diagnostic observations, not release evidence.

## Run Context

| Field | Value |
|---|---|
| Date | 2026-11-09 |
| Build | Windows x64, Release, MSVC |
| Benchmark library | Google Benchmark v1.9.0 |
| Host | `DESKTOP-712S8LO`, 20 logical CPUs, reported 3,696 MHz |
| CTest command | `ctest --test-dir build/windows-bench-release --output-on-failure -j 1` |
| Test result | 416 total; 400 passed; 16 failed |
| Total elapsed | 8,662.86 s |

The latency JSON identifies its measurement context as `2026-11-09T17:59:58+01:00`. The full `bench_latency_comprehensive` CTest case failed, while the isolated `LatencyBenchFixture/RandomReadLatency` invocation completed successfully. The full-case failure therefore remains unlocalized by that focused run.

## Recovered Latency Measurements

The values below are from seven complete records in the partial `bench_latency_comprehensive.json`. Means are in microseconds; throughput is the benchmark-reported operations per second. For small-value writes, the mean is the Google Benchmark iteration time because the fixture does not export its `LatencyTracker` mean. A dash means that the counter was not recorded.

| Benchmark | Mean | P50 | P95 | P99 | P99.9 | Maximum | Throughput |
|---|---:|---:|---:|---:|---:|---:|---:|
| Small-value read | 1.06 | 0.9 | 1.6 | 2.0 | 20.5 | 75.6 | 819,200 ops/s |
| Large-value read | 1.67 | 1.5 | 2.5 | 3.3 | 20.64 | 110.8 | 521,309 ops/s |
| Small-value write | 8.36 | - | - | - | - | - | 119,464 ops/s |
| Large-value write | 11.62 | 10.0 | 19.9 | 32.7 | 50.72 | 87.0 | 14,336 ops/s |
| 70% read / 30% write | 3.55 | 1.7 | 8.1 | 12.3 | 29.2 | 48.0 | 273,070 ops/s |
| Sequential read | 0.99 | 0.9 | 1.5 | 1.8 | 19.1 | 40.5 | 819,200 ops/s |
| Random read | 0.93 | 0.8 | 1.5 | 1.9 | 19.9 | 42.7 | 914,286 ops/s |

### Interpretation

- The measured read means are 0.93-1.67 us; read P99 is 1.8-3.3 us. Sequential and random reads are close in this run, with no evident random-access penalty in the reported percentiles.
- Tail latency is materially higher than the median: read P99.9 is about 19-21 us, with observed maxima of 40.5-110.8 us. A single run cannot establish whether this is stable system noise or a repeatable storage tail.
- Large-value writes have a 32.7 us P99 and 50.72 us P99.9. The reported throughput is much lower than for reads, as expected for a 10 KB value workload, but the result is not a durable-write measurement.
- The 70/30 mix has a 12.3 us P99 and 29.2 us P99.9. It combines reads and writes into one distribution; it does not provide separate read-versus-write tail latency.
- Small-value write percentiles are absent because that benchmark does not publish its `LatencyTracker` counters.

## Measurement Limits

1. **Incomplete artifact:** The raw JSON is syntactically incomplete. The postprocessor reported a parse error at line 186 and loaded zero benchmark result entries. The generated `benchmark_comparison_report.md` consequently reports all 163 expectation targets as `NOT_RUN`.
2. **No repeated trials:** Each JSON record has one benchmark repetition. Iterations within a repetition do not replace independent process-level repetitions for a stable comparison.
3. **In-memory write path:** The fixture disables RocksDB WAL, uses a 256 MB memtable and 512 MB block cache, and warms the database before the read cases. The results do not represent WAL-enabled durable writes, transaction latency, remote storage, or a cold-cache workload.
4. **Timer choice:** The benchmark source uses `high_resolution_clock`; this clock is not guaranteed by the C++ standard to be monotonic. `steady_clock` is the safer basis for elapsed-time measurement.
5. **No comparable baseline:** The historical [benchmark baseline report](BENCHMARK_BASELINE_REPORT.md) states expected improvements but does not contain matching raw measurements from this host and workload. It is not used as a numeric comparator here.
6. **Suite failures are not all regressions:** Timeouts may indicate workloads exceeding the current 120-second CTest limit; process crashes invalidate their corresponding measurements. Neither category, by itself, demonstrates a performance regression.

## CTest Failures

| Test | Result |
|---|---|
| `bench_latency_comprehensive` | Segmentation fault |
| `bench_multithreading_comprehensive` | Timeout |
| `bench_residual_quantization` | Timeout |
| `bench_scalability_comprehensive` | Segmentation fault |
| `bench_tpcc` | Timeout |
| `bench_docker_raid_comprehensive` | Timeout |
| `bench_edge_cases_comprehensive` | Timeout |
| `bench_fts_phase_b` | Timeout |
| `bench_gpu_phase_c_gates` | Timeout |
| `bench_multi_gpu_lora_advanced` | Timeout |
| `bench_graph_query_optimizer` | Exit `0xc0000409` |
| `bench_process_import_retrieval` | Segmentation fault |
| `bench_product_quantization` | Timeout |
| `bench_task_scheduler` | Exit `0xc0000409` |
| `bench_voice_assistant` | Exit `0xc0000409` |
| `bench_voice_stt_tts_latency` | Timeout |

## Recommendations

1. Fix or isolate the 16 failing benchmark cases before treating their timings as usable evidence. For timeouts, verify intended workload duration before changing CTest limits; do not raise timeouts solely to make the suite green.
2. Write benchmark output to a per-case temporary file and publish it only after successful process completion, so a crash cannot leave a partial file that is mistaken for a valid result set.
3. Add percentile output for small-value writes, use a monotonic timer, and distinguish WAL-disabled in-memory measurements from WAL-enabled durable-write measurements.
4. Re-run representative benchmarks with at least five independent repetitions on the same host, build, and configuration. Compare against a baseline generated by the same benchmark definitions.
5. Re-run the expectation postprocessor only after valid result files are present; require nonzero parsed-result coverage before accepting its report as a performance gate.

## Evidence

- CTest final summary: terminal output from the completed run. In this environment, `build/windows-bench-release/Testing/Temporary/LastTest.log` contains the postprocessing result, not the full per-test output.
- Postprocessor evidence: `LastTest.log` records the JSON parse error and that zero benchmark result entries were loaded.
- Partial latency data: [bench_latency_comprehensive.json](../../build/windows-bench-release/bench_results/current/bench_latency_comprehensive.json).
- Generated expectation report: [benchmark_comparison_report.md](../../build/windows-bench-release/bench_results/current/benchmark_comparison_report.md).
- Benchmark methodology: [bench_latency_comprehensive.cpp](../../benchmarks/base/bench_latency_comprehensive.cpp).
- Historical context: [BENCHMARK_BASELINE_REPORT.md](BENCHMARK_BASELINE_REPORT.md).