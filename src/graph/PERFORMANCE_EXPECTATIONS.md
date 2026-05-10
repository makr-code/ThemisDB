# PERFORMANCE_EXPECTATIONS — src/graph

## Scope
- Modul: `src/graph`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_graph_traversal.cpp`
  - `benchmarks/bench_tensor_fingerprint_graph.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| GR-SparseEdge | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `GraphTraversalBenchmarkFixture_SparseEdgeAddition` |
| GR-DenseNeighbor | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `GraphTraversalBenchmarkFixture_DenseNeighborQuery` |
| GR-BFS | Siehe Zielbeschreibung: Graph BFS Traversal (Depth-3) | `GraphTraversalBenchmarkFixture_BFSTraversal` |
| GR-TFG-Insert | <= 10 ms pro Tensor bei Graphgroesse bis 100K Nodes | `BM_TFG_Insert_SingleNode` |
| GR-TFG-Similarity | <= 50 ms fuer `findSimilar` bei Graphgroesse bis 100K Nodes | `BM_TFG_FindSimilar` |
| GR-TFG-Neighbours | <= 5 ms fuer `neighbours` | `BM_TFG_Neighbours` |
| GR-TFG-Export | Persisted Graph Export ohne unkontrolliertes Wachstum | `BM_TFG_ExportPersistedGraph` |
| GR-TDM-SnapshotRestore | Konsistente Wiederherstellung von Graph+Records nach Snapshot/Restore | `TensorDeduplicationManagerSnapshotTest` (TDM-12..TDM-24) |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
