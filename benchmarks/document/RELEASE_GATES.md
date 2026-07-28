# Document Module Benchmark — Release Gates

<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (c) 2026 ThemisDB Contributors -->

This file defines the hard release gates for the Document Module benchmark
suite.  Each gate maps to a constexpr threshold in the corresponding benchmark
source file and is evaluated by the gate-verification benchmark registered
under `DocDiffMerge/GATE-DOC-*`, `DocRoundTrip/GATE-DOC-*`, and
`DocStore/GATE-DOC-*`.

Gate results are published as `gate_p99_limit_us`, `gate_min_ops_per_s`,
`mean_us`, and `ops_per_s` benchmark counters and feed the release gate manifest
(`release_gate_manifest.json`).

---

## GATE-DOC-01 — Large Document Diff, p99 ≤ 500 µs

| Attribute             | Value                                              |
|-----------------------|----------------------------------------------------|
| **Benchmark ID**      | DDM-BM-02 / GATE_DOC_01_LargeDocDiff_p99_500us    |
| **Target metric**     | p99 per-iteration latency ≤ 500 µs                 |
| **Measurement method**| mean latency (`elapsed_time / iterations`) as p99 proxy; formal p99 via `--benchmark_repetitions=10` post-processed by `report_variance.py` |
| **Baseline condition**| 100-field JSON document (50 of 100 fields modified); in-memory store; single-threaded |
| **constexpr gate**    | `kGateDoc01UsP99 = 500.0` in `bench_document_diff_merge.cpp` |
| **Wave 7 alignment**  | Mirrors W7-A RCS-03 (range scan, p99 ≤ 500 µs) methodology |
| **Milestone**         | Q3 2026 stabilization                              |

---

## GATE-DOC-02 — Clean Merge, p99 ≤ 200 µs

| Attribute             | Value                                              |
|-----------------------|----------------------------------------------------|
| **Benchmark ID**      | DDM-BM-03 / GATE_DOC_02_CleanMerge_p99_200us      |
| **Target metric**     | p99 per-iteration latency ≤ 200 µs                 |
| **Measurement method**| mean latency as p99 proxy; formal p99 via repetitions + post-processing |
| **Baseline condition**| 20-field base document; ours modifies 0-9, theirs modifies 10-19 (no conflict); `MergeStrategy::FAIL` confirms clean path |
| **constexpr gate**    | `kGateDoc02UsP99 = 200.0` in `bench_document_diff_merge.cpp` |
| **Wave 7 alignment**  | Mirrors W7-A RCS-01 (point read, p99 ≤ 200 µs) methodology |
| **Milestone**         | Q3 2026 stabilization                              |

---

## GATE-DOC-03 — beginRelay(), p99 ≤ 1 ms

| Attribute             | Value                                              |
|-----------------------|----------------------------------------------------|
| **Benchmark ID**      | RTP-BM-01 / GATE_DOC_03_BeginRelay_p99_1ms        |
| **Target metric**     | p99 per-iteration latency ≤ 1 ms (1000 µs)         |
| **Measurement method**| mean latency as p99 proxy (`elapsed_time / iterations × 1 000 000`) |
| **Baseline condition**| Fresh InMemoryDocumentStore; sequential unique relay IDs; single-threaded |
| **constexpr gate**    | `kGateDoc03UsP99 = 1000.0` in `bench_document_round_trip.cpp` |
| **Wave 7 alignment**  | Higher bound than DDM gates due to JSON body assembly + system_clock overhead |
| **Milestone**         | Q3 2026 stabilization                              |

---

## GATE-DOC-04 — loadInteraction(), p99 ≤ 200 µs

| Attribute             | Value                                              |
|-----------------------|----------------------------------------------------|
| **Benchmark ID**      | RTP-BM-03 / GATE_DOC_04_LoadInteraction_p99_200us |
| **Target metric**     | p99 per-iteration latency ≤ 200 µs                 |
| **Measurement method**| mean latency as p99 proxy                          |
| **Baseline condition**| Pre-populated relay with 11 snapshots; point load at index 5 |
| **constexpr gate**    | `kGateDoc04UsP99 = 200.0` in `bench_document_round_trip.cpp` |
| **Wave 7 alignment**  | Mirrors W7-A RCS-01/RCS-08 (point lookup p99 ≤ 200 µs) |
| **Milestone**         | Q3 2026 stabilization / Q1 2027 round-trip milestone |

---

## GATE-DOC-05 — put() Throughput ≥ 100 000 ops/s

| Attribute             | Value                                              |
|-----------------------|----------------------------------------------------|
| **Benchmark ID**      | DST-BM-01 / GATE_DOC_05_PutThroughput_100k_ops_s  |
| **Target metric**     | Throughput ≥ 100 000 ops/s                         |
| **Measurement method**| `items_processed / elapsed_time`; gate checked as `ops_per_s >= 100000` |
| **Baseline condition**| Single-field JSON body; unique sequential IDs; InMemoryDocumentStore |
| **constexpr gate**    | `kGateDoc05OpsPerSec = 100000.0` in `bench_document_store.cpp` |
| **Wave 7 alignment**  | Mirrors W7-A RCS-02 throughput gate pattern (≥ 80 000 ops/s for RocksDB) |
| **Milestone**         | Q3 2026 serialization hot-path stabilization       |

---

## GATE-DOC-06 — get() p99 ≤ 100 µs

| Attribute             | Value                                              |
|-----------------------|----------------------------------------------------|
| **Benchmark ID**      | DST-BM-02 / GATE_DOC_06_GetLatency_p99_100us      |
| **Target metric**     | p99 per-iteration latency ≤ 100 µs                 |
| **Measurement method**| mean latency as p99 proxy; random access across 1000-document working set |
| **Baseline condition**| 1000 pre-loaded 5-field documents; `std::mt19937(kDocCanonicalSeed)` access pattern |
| **constexpr gate**    | `kGateDoc06UsP99 = 100.0` in `bench_document_store.cpp` |
| **Wave 7 alignment**  | Tighter than W7-A RCS-01 (200 µs) because InMemoryDocumentStore has no I/O overhead |
| **Milestone**         | Q3 2026 list/read hot-path stabilization           |

---

## Measurement Protocol

1. Build with the `linux-release` preset (or equivalent `-O3 -march=native`).
2. Pin to a single CPU socket: `numactl --cpunodebind=0 --membind=0`.
3. Set the CPU frequency governor to `performance`.
4. Run with JSON output:

```bash
./bench_document_diff_merge \
    --benchmark_out=bench_document_diff_merge.json \
    --benchmark_out_format=json \
    --benchmark_repetitions=10

./bench_document_round_trip \
    --benchmark_out=bench_document_round_trip.json \
    --benchmark_out_format=json \
    --benchmark_repetitions=10

./bench_document_store \
    --benchmark_out=bench_document_store.json \
    --benchmark_out_format=json \
    --benchmark_repetitions=10
```

5. Compare gate counters (`gate_p99_limit_us`, `gate_min_ops_per_s`) against
   `mean_us` and `ops_per_s` in the JSON output.
6. Coefficient of variation (CV) must be ≤ 5% across 5 repeated runs
   (see `benchmarks/MEASUREMENT_HYGIENE.md` §7).

---

## Wave 7 Alignment Summary

All gates follow the same constexpr + `SkipWithError()` verification pattern
established by `benchmarks/wave7/bench_w7a_release_critical_signoff.cpp` and
the methodology documented in `benchmarks/MEASUREMENT_HYGIENE.md`.  Gate values
are intentionally conservative relative to expected in-memory performance to
provide headroom for slower CI runners.
