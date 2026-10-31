# Document Module Benchmark Coverage

<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (c) 2026 ThemisDB Contributors -->
<!-- Source inventory validated: 2026-10-31; benchmark execution not performed -->

This inventory covers 34 benchmark registrations across four C++ sources: 24 scenario cases over in-memory document interfaces, six additional runtime gate checks, and four synthetic `DOC-BM` cases. Source registration is in `CMakeLists.txt`; targets are conditional on benchmarks being enabled and Google Benchmark being available.

## In-memory interface suites

| IDs | Source | Cases |
|---|---|---|
| DDM-BM-01..08 | `bench_document_diff_merge.cpp` | `DiffSmallDocument`, `DiffLargeDocument`, `MergeCleanNoConflict`, `MergeAllConflicts_OursWins`, `MergeAllConflicts_TheirsWins`, `MergeAllConflicts_Fail`, `DiffEmptyDocuments`, `MergeIdenticalDocuments` |
| RTP-BM-01..08 | `bench_document_round_trip.cpp` | `BeginRelayThroughput`, `SaveInteractionThroughput`, `LoadInteractionLatency`, `CountSnapshotsLatency`, `FullRelayWorkflow`, `SnapshotIdGeneration`, `LargeDocumentSnapshot`, `ConcurrentRelayLoad` |
| DST-BM-01..08 | `bench_document_store.cpp` | `PutThroughput`, `GetLatency`, `ListThroughput`, `CountLatency`, `UpdateThroughput`, `RemoveThroughput`, `LargeBodyPut`, `SchemaValidationThroughput` |

The diff/merge and store suites use `InMemoryDocumentStore` and `InMemoryDocumentDiffMerge` / `InMemoryDocumentSchemaEvolution`. The round-trip suite uses `StoreBackedRoundTripEditor` over `InMemoryDocumentStore`. These cases characterize in-memory implementations, not persistent backend performance.

## Declared runtime checks

| Gate | Source case | Check in benchmark source |
|---|---|---|
| GATE-DOC-01 | `GATE_DOC_01_LargeDocDiff_p99_500us` | Mean per-iteration time ≤ 500 µs |
| GATE-DOC-02 | `GATE_DOC_02_CleanMerge_p99_200us` | Mean per-iteration time ≤ 200 µs |
| GATE-DOC-03 | `GATE_DOC_03_BeginRelay_p99_1ms` | Mean per-iteration time ≤ 1,000 µs |
| GATE-DOC-04 | `GATE_DOC_04_LoadInteraction_p99_200us` | Mean per-iteration time ≤ 200 µs |
| GATE-DOC-05 | `GATE_DOC_05_PutThroughput_100k_ops_s` | Throughput ≥ 100,000 ops/s |
| GATE-DOC-06 | `GATE_DOC_06_GetLatency_p99_100us` | Mean per-iteration time ≤ 100 µs |

Despite `p99` in several benchmark names, the latency checks calculate a mean (`elapsed_time / iterations`) and compare it to the limit. The benchmark code does not calculate p99. See [RELEASE_GATES.md](RELEASE_GATES.md).

## Synthetic serialization-gate cases

`bench_document_serialization_gates.cpp` registers `BM_DOCBM01_SerializationP95`, `BM_DOCBM02_ListReadP95`, `BM_DOCBM03_DiffP95`, and `BM_DOCBM04_MergeP95`. It measures local string/vector helper functions, emits threshold counters, and does not call the document store, serializer, diff/merge implementation, or validate the thresholds. These cases are proxies only; their output is not production-path performance evidence.

## Measurement and validation status

The sources use Google Benchmark registrations; no run output, representative-hardware result, variance analysis, or release CI consumption was checked for this inventory. Thresholds and source code are not evidence that a gate passed. For module-level mapping, see [`../../src/document/PERFORMANCE_EXPECTATIONS.md`](../../src/document/PERFORMANCE_EXPECTATIONS.md).
