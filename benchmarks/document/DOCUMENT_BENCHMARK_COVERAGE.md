# Document Module Benchmark Coverage

<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (c) 2026 ThemisDB Contributors -->

This document catalogs all 24 benchmark IDs and 6 release gates in the
Document Module benchmark suite (`benchmarks/document/`).

Measurement methodology: `benchmarks/MEASUREMENT_HYGIENE.md`  
Release gate thresholds: `benchmarks/document/RELEASE_GATES.md`

---

## DDM: Document Diff / Merge (DDM-BM-01..08)

Source: `bench_document_diff_merge.cpp`  
Namespace: `themis::bench::document`  
Fixture: `DocumentDiffMergeFixture`  
Interfaces under test: `InMemoryDocumentDiffMerge`, `InMemoryDocumentStore`

| ID         | Name                           | Payload description                                         | Strategy              | Iterations |
|------------|--------------------------------|-------------------------------------------------------------|-----------------------|------------|
| DDM-BM-01  | DiffSmallDocument              | 5-field JSON objects, 1 field differs                       | N/A (diff)            | 50 000     |
| DDM-BM-02  | DiffLargeDocument              | 100-field JSON objects, 50 fields differ                    | N/A (diff)            | 5 000      |
| DDM-BM-03  | MergeCleanNoConflict           | 20-field base; ours modifies 0-9, theirs modifies 10-19    | FAIL (no conflicts)   | 10 000     |
| DDM-BM-04  | MergeAllConflicts_OursWins     | 50-field documents, all 50 fields conflict                  | OURS_WINS             | 5 000      |
| DDM-BM-05  | MergeAllConflicts_TheirsWins   | 50-field documents, all 50 fields conflict                  | THEIRS_WINS           | 5 000      |
| DDM-BM-06  | MergeAllConflicts_Fail         | 50-field documents, all 50 fields conflict (error path)    | FAIL → ERR_CONFLICT   | 5 000      |
| DDM-BM-07  | DiffEmptyDocuments             | Two empty JSON objects                                      | N/A (diff)            | 50 000     |
| DDM-BM-08  | MergeIdenticalDocuments        | base = ours = theirs (5-field identical)                   | FAIL (clean)          | 20 000     |

### DDM Gate Benchmarks

| Gate ID     | Benchmark name                         | Threshold          |
|-------------|----------------------------------------|--------------------|
| GATE-DOC-01 | GATE_DOC_01_LargeDocDiff_p99_500us     | p99 ≤ 500 µs       |
| GATE-DOC-02 | GATE_DOC_02_CleanMerge_p99_200us       | p99 ≤ 200 µs       |

---

## RTP: Round-Trip Editor (RTP-BM-01..08)

Source: `bench_document_round_trip.cpp`  
Namespace: `themis::bench::document`  
Fixture: `RoundTripFixture`  
Interfaces under test: `StoreBackedRoundTripEditor`, `InMemoryDocumentStore`

| ID         | Name                       | Description                                                     | Setup pattern         | Iterations  |
|------------|----------------------------|-----------------------------------------------------------------|-----------------------|-------------|
| RTP-BM-01  | BeginRelayThroughput       | beginRelay() for sequential relay IDs (atomic counter)          | Inline; unique ID/iter | 10 000     |
| RTP-BM-02  | SaveInteractionThroughput  | 10 sequential saveInteraction() calls per iteration             | PauseTiming for relay  | 2 000      |
| RTP-BM-03  | LoadInteractionLatency     | loadInteraction() at index 5 from pre-populated relay           | SetUp preload          | 50 000     |
| RTP-BM-04  | CountSnapshotsLatency      | countSnapshots() on relay with 11 snapshots                     | SetUp preload          | 20 000     |
| RTP-BM-05  | FullRelayWorkflow          | begin + 5× save + 5× load end-to-end                           | PauseTiming for ID     | 2 000      |
| RTP-BM-06  | SnapshotIdGeneration       | makeSnapshotId-equivalent: ostringstream zero-padded construction | Inline                | 1 000 000  |
| RTP-BM-07  | LargeDocumentSnapshot      | saveInteraction() with a 10 KB document string                  | PauseTiming for relay  | 1 000      |
| RTP-BM-08  | ConcurrentRelayLoad        | Two concurrent relay workloads via std::thread per iteration    | Atomic counter for IDs | 500        |

### RTP Gate Benchmarks

| Gate ID     | Benchmark name                          | Threshold          |
|-------------|-----------------------------------------|--------------------|
| GATE-DOC-03 | GATE_DOC_03_BeginRelay_p99_1ms          | p99 ≤ 1 ms         |
| GATE-DOC-04 | GATE_DOC_04_LoadInteraction_p99_200us   | p99 ≤ 200 µs       |

---

## DST: Document Store (DST-BM-01..08)

Source: `bench_document_store.cpp`  
Namespace: `themis::bench::document`  
Fixture: `DocumentStoreFixture`  
Interfaces under test: `InMemoryDocumentStore`, `InMemoryDocumentSchemaEvolution`

| ID         | Name                         | Description                                                      | Store state              | Iterations |
|------------|------------------------------|------------------------------------------------------------------|--------------------------|------------|
| DST-BM-01  | PutThroughput                | put() for 1000 sequential documents per iteration                | Unique IDs via counter   | 100        |
| DST-BM-02  | GetLatency                   | get() random access into 1000-document working set               | 1000 docs pre-loaded     | 50 000     |
| DST-BM-03  | ListThroughput               | list() enumerates full 1000-document collection                  | 1000 docs pre-loaded     | 5 000      |
| DST-BM-04  | CountLatency                 | count() on 1000-document collection                              | 1000 docs pre-loaded     | 20 000     |
| DST-BM-05  | UpdateThroughput             | update() body replacement for all 1000 pre-loaded documents      | 1000 docs pre-loaded     | 100        |
| DST-BM-06  | RemoveThroughput             | remove() for 1000 docs per iteration (PauseTiming re-inserts)    | PauseTiming re-insert    | 50         |
| DST-BM-07  | LargeBodyPut                 | put() with a 10 KB JSON body                                     | Unique IDs via counter   | 2 000      |
| DST-BM-08  | SchemaValidationThroughput   | validate() for 1000 docs against 10-field sealed schema          | Pre-built bodies in fixture | 50      |

### DST Gate Benchmarks

| Gate ID     | Benchmark name                            | Threshold                  |
|-------------|-------------------------------------------|----------------------------|
| GATE-DOC-05 | GATE_DOC_05_PutThroughput_100k_ops_s      | Throughput ≥ 100 000 ops/s |
| GATE-DOC-06 | GATE_DOC_06_GetLatency_p99_100us          | p99 ≤ 100 µs               |

---

## GATE-DOC Summary Table

| Gate ID     | Source file                       | constexpr name           | Threshold              | Milestone       |
|-------------|-----------------------------------|--------------------------|------------------------|-----------------|
| GATE-DOC-01 | bench_document_diff_merge.cpp     | kGateDoc01UsP99 = 500.0  | p99 ≤ 500 µs           | Q3 2026         |
| GATE-DOC-02 | bench_document_diff_merge.cpp     | kGateDoc02UsP99 = 200.0  | p99 ≤ 200 µs           | Q3 2026         |
| GATE-DOC-03 | bench_document_round_trip.cpp     | kGateDoc03UsP99 = 1000.0 | p99 ≤ 1 ms             | Q3 2026         |
| GATE-DOC-04 | bench_document_round_trip.cpp     | kGateDoc04UsP99 = 200.0  | p99 ≤ 200 µs           | Q3 2026 / Q1 2027 |
| GATE-DOC-05 | bench_document_store.cpp          | kGateDoc05OpsPerSec = 100000.0 | ≥ 100 000 ops/s  | Q3 2026         |
| GATE-DOC-06 | bench_document_store.cpp          | kGateDoc06UsP99 = 100.0  | p99 ≤ 100 µs           | Q3 2026         |

---

## Q1 2027 Milestone Notes

The following benchmarks are flagged for the Q1 2027 diff/merge/round-trip
milestone (`src/document/ROADMAP.md`, mid-term item):

- **DDM-BM-01..08**: Re-baseline p95/p99 envelopes after Q4 2026 merge
  hardening.  Conflict-heavy benchmarks (DDM-BM-04..06) are most likely to
  shift with MERGE_CONCURRENT improvements.
- **RTP-BM-01..08**: Expand to cover multi-relay concurrent stress (≥ 8
  threads) once concurrency primitives are promoted beyond the current
  mutex-per-store design.
- **RTP-BM-06** (SnapshotIdGeneration): Candidate for re-implementation using
  `fmt::format` or `std::format` (C++23) if zero-alloc ID construction is
  targeted.
- **GATE-DOC-03**: Re-evaluate the 1 ms bound once `beginRelay()` is profiled
  under a persistent store backend.

---

## Measurement Methodology Reference

All benchmarks comply with `benchmarks/MEASUREMENT_HYGIENE.md`:

- **RNG seed**: `kDocCanonicalSeed = 42` for all `std::mt19937` instances.
- **Fixture isolation**: `SetUp()` / `TearDown()` per benchmark function.
- **Real-time mode**: `UseRealTime()` on every registration.
- **DoNotOptimize**: Applied to every result expression to prevent elision.
- **Warmup**: Brief in-fixture warmup (30–50 iterations) primes branch
  predictor before the measurement window.
- **Iteration counts**: Conservative minimum counts chosen to produce stable
  mean latency estimates (see MEASUREMENT_HYGIENE.md §5).
- **CV gate**: ≤ 5% across 5 repeated runs on the same machine.
