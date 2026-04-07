# Index-Modul

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/index/README.md -->

**Stand:** 6. April 2026  
**Version:** aktuell  
**Kategorie:** Indexierung / Vektorsuche  
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Index-Modul implementiert ThemisDBs umfassende Indexierungsinfrastruktur: HNSW-Vektorindizes, GPU-beschleunigte ANN-Suche, Graph-Indizes und adaptives Index-Management.

**Primäre Quelle:** [`src/index/`](../../../src/index/) · [`include/index/`](../../../include/index/)

---

## Kernkomponenten (Auswahl)

| Komponente | Header | Beschreibung |
|------------|--------|--------------|
| AdvancedVectorIndex | `advanced_vector_index.h` | HNSW-basierter Vektorindex mit Product-Quantization |
| GpuVectorIndex | `gpu_vector_index.h` | GPU-beschleunigter Vektorindex (CUDA) |
| ANNIndex | `ann_index.h` | Approximate-Nearest-Neighbor-Indexschnittstelle |
| CudaHnswGraphTraversal | `cuda_hnsw_graph_traversal.h` | CUDA-HNSW-Graph-Traversierung |
| DistributedVectorIndex | `distributed_vector_index.h` | Verteilter Vektorindex (Sharding) |
| GraphIndex | `graph_index.h` | Graphdatenbank-Index |
| GraphAnalytics | `graph_analytics.h` | Graph-Analytik auf Index-Ebene |
| AdaptiveIndex | `adaptive_index.h` | Adaptives Index-Management |
| BinaryQuantizer | `binary_quantizer.h` | Binäre Vektorkomprimierung |
| ApproximateRadiusSearch | `approximate_radius_search.h` | Näherungsweise Radius-Suche |
| GnnEmbeddings | `gnn_embeddings.h` | GNN-Embedding-Generierung |
| HnswLayerOptimizer | `hnsw_layer_optimizer.h` | HNSW-Schicht-Optimierung |
| GpuMemoryOversubscription | `gpu_memory_oversubscription.h` | GPU-Speicher-Überprovisioning |

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/index/README.md`](../../../src/index/README.md) | Modulübersicht |
