# Document Module Benchmark Gate Definitions

<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (c) 2026 ThemisDB Contributors -->
<!-- Source/configuration review: 2026-10-31; benchmark execution not performed -->

This file records the limits and checks present in `bench_document_diff_merge.cpp`, `bench_document_round_trip.cpp`, and `bench_document_store.cpp`. These are source-defined checks, not a report of a passing run. Subdirectory CMake registers the executables only when benchmarks are enabled and Google Benchmark is found.

## Runtime-checked gates

| Gate | Benchmark source case | Limit | Actual comparison |
|---|---|---:|---|
| GATE-DOC-01 | `DocDiffMerge/GATE_DOC_01_LargeDocDiff_p99_500us` | 500 µs | `mean_us > limit` calls `SkipWithError` |
| GATE-DOC-02 | `DocDiffMerge/GATE_DOC_02_CleanMerge_p99_200us` | 200 µs | `mean_us > limit` calls `SkipWithError` |
| GATE-DOC-03 | `DocRoundTrip/GATE_DOC_03_BeginRelay_p99_1ms` | 1,000 µs | `mean_us > limit` calls `SkipWithError` |
| GATE-DOC-04 | `DocRoundTrip/GATE_DOC_04_LoadInteraction_p99_200us` | 200 µs | `mean_us > limit` calls `SkipWithError` |
| GATE-DOC-05 | `DocStore/GATE_DOC_05_PutThroughput_100k_ops_s` | 100,000 ops/s | `ops_per_s < limit` calls `SkipWithError` |
| GATE-DOC-06 | `DocStore/GATE_DOC_06_GetLatency_p99_100us` | 100 µs | `mean_us > limit` calls `SkipWithError` |

The `p99` names are historical labels: the four latency checks compare a mean computed from elapsed time and iteration count. They do not measure p99. Threshold values need a reviewed measurement method and representative-hardware baseline before being described as percentile budgets.

## DOC-BM threshold counters (not enforced gates)

`bench_document_serialization_gates.cpp` emits four `gate_threshold_*` counters: serialization ≤ 500 µs, list-read throughput ≥ 50,000 ops/s, diff ≤ 1,000 µs, and merge ≤ 2,000 µs. Its implementation uses synthetic string/vector helpers and does not call the corresponding document production/interface operations. It does not call `SkipWithError` or otherwise fail the benchmark when the counters are missed. Treat these cases as synthetic instrumentation examples, not release gates or production performance results.

## Execution and release integration

The module CMake file defines four targets and installs them under `bin/benchmarks/document`; it does not by itself establish which release workflow consumes their output. TODO: verify the consuming workflow/manifest and attach a run artifact before asserting CI enforcement or gate passage.

To collect local diagnostic output, run the built executables with Google Benchmark JSON output and retain the command, build configuration, hardware details, and raw result file. No results were generated for this documentation review.
