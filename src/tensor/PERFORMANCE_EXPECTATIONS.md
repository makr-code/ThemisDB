# PERFORMANCE_EXPECTATIONS — src/tensor

## Scope

- Modul: src/tensor
- Diese Datei dokumentiert modulspezifische Performance-Erwartungen fuer TensorFingerprintGraph und Tensor-Index-Pfade.
- Primarquelle fuer Benchmark-Zuordnung: benchmarks/benchmark_target_mapping.json.

## Benchmark-Bezug

- Relevante Benchmark-Dateien:
  - benchmarks/bench_tensor_fingerprint_graph.cpp
  - benchmarks/bench_tensor_fingerprint.cpp

## Spezifische Erwartungswerte

| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| TFG-1 Insert-Latenz | <= 10 ms pro Tensor (bis 100K Nodes) | BM_TFG_Insert_SingleNode |
| TFG-2 Similarity Query | <= 50 ms (findSimilar, bis 100K Nodes) | BM_TFG_FindSimilar |
| TFG-3 Neighbour Lookup | <= 5 ms (neighbours) | BM_TFG_Neighbours |
| TFG-4 Persisted Export | Full-Graph-Export ohne unkontrolliertes Wachstum, p95 <= 500 ms bei 10K Nodes | BM_TFG_ExportPersistedGraph |
| TNS-1 Insert Throughput | >= 1,000 Inserts/s im Referenzprofil | BM_TFG_Insert_Throughput |
| TNS-2 Concurrent Reads | Skalierungsfaktor >= 1.8x zwischen 1 und 4 Threads | BM_TFG_ConcurrentReads |

## Integritaetsziele (aktuell nicht als dedizierter Performance-Benchmark messbar)

| Ziel-ID | Erwartungswert | Nachweis |
|---|---|---|
| TDM-1 Snapshot/Restore Integritaet | Konsistente Wiederherstellung von Topologie und Records | TensorDeduplicationManagerSnapshotTest (TDM-12..TDM-24) |
| TDM-2 Journal-Replay/Kompaktion | Replizierbarer Replay nach Post-Snapshot Mutationen | TensorDeduplicationManagerSnapshotTest (TDM-18/19/22/23/24) |

## Validierung

- TFG-1..TFG-4 und TNS-1..TNS-2 gelten als erfuellt, wenn die Benchmarks reproduzierbar laufen und die Schwellwerte erreichen.
- TDM-1/TDM-2 sind Integritaetsziele; dedizierte Performance-Benchmarks bleiben Folgeaufgabe.
