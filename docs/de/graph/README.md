# Graph-Modul

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/graph/README.md -->

**Stand:** 6. April 2026  
**Version:** aktuell  
**Kategorie:** Graph-Datenbank  
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Graph-Modul implementiert ThemisDBs fortgeschrittene Graph-Datenbankfunktionalität mit kostenbasierter Query-Optimierung, eingeschränkter Pfadfindung und GPU-beschleunigter Traversierung.

**Primäre Quelle:** [`src/graph/`](../../../src/graph/) · [`include/graph/`](../../../include/graph/)

---

## Kernkomponenten

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| GraphQueryOptimizer | `graph_query_optimizer.h` | `graph_query_optimizer.cpp` | Kostenbasierte Algorithmus-Auswahl und Plan-Generierung |
| DistributedGraph | `distributed_graph.h` | `distributed_graph.cpp` | Verteilte Graph-Partitionierung und Cross-Partition-Traversierung |
| GpuTraversal | `gpu_traversal.h` | `gpu_traversal.cpp` | GPU-beschleunigte Graph-Traversierung |
| ParallelTraversal | `parallel_traversal.h` | `parallel_traversal.cpp` | Multi-Thread-Traversierung (BFS/DFS/Dijkstra/A*) |
| PathConstraints | `path_constraints.h` | `path_constraints.cpp` | Eingeschränkte Pfadfindung (min/max Länge, erforderliche/verbotene Knoten) |
| ScheduledEdgeRefresh | `scheduled_edge_refresh.h` | `scheduled_edge_refresh.cpp` | Geplante Kanten-Aktualisierung für zeitreihenbasierte Graphen |

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/graph/README.md`](../../../src/graph/README.md) | Modulübersicht und Scope |
