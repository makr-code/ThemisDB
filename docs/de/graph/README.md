# Graph-Modul

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/graph/README.md -->

**Stand:** 13. Mai 2026
**Version:** aktuell
**Kategorie:** Graph-Datenbank
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Graph-Modul implementiert kostenbasierte Query-Optimierung, eingeschränkte
Pfadfindung, verteilte Traversierung, parallele Ausführung und GPU-gestützte
Traversal-Pfade (mit CPU-Fallback). Das Modul ist mit AQL, HTTP-Endpoints und
dem GraphIndex-Subsystem integriert.

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
| [`src/graph/README.md`](../../../src/graph/README.md) | Modulübersicht, API, Laufzeitverhalten, Fehlerfälle, Grenzen |
| [`include/graph/README.md`](../../../include/graph/README.md) | Public Header/Entry-Points und Konfigurationsflächen |
| [`src/graph/ARCHITECTURE.md`](../../../src/graph/ARCHITECTURE.md) | Komponentenfluss und Integrationsdesign |
| [`src/graph/ROADMAP.md`](../../../src/graph/ROADMAP.md) | Umsetzungsphasen, Readiness-Checklist, bekannte Lücken |
| [`src/graph/FUTURE_ENHANCEMENTS.md`](../../../src/graph/FUTURE_ENHANCEMENTS.md) | Geplante Erweiterungen mit Constraints/Teststrategie |
| [`src/graph/PERFORMANCE_EXPECTATIONS.md`](../../../src/graph/PERFORMANCE_EXPECTATIONS.md) | Performance-Ziele und Benchmark-Mapping |
| [`src/graph/SECURITY.md`](../../../src/graph/SECURITY.md) | Modulbezogene Security-/Reliability-Hinweise |
| [`docs/de/roadmap/graph_roadmap.md`](../roadmap/graph_roadmap.md) | Deutsche Roadmap-Spiegelung und Referenzen |

---

## Usage

### Beispiel: Shortest-Path-Optimierung

```cpp
#include "graph/graph_query_optimizer.h"

GraphQueryOptimizer optimizer(graph_manager);
GraphQueryOptimizer::QueryConstraints constraints;
constraints.max_depth = 5;
constraints.unique_vertices = true;

auto plan = optimizer.optimizeShortestPath("A", "B", constraints);
```

### Beispiel: Constrained Path Finding

```cpp
#include "graph/path_constraints.h"

PathConstraints constraints(&graph_manager);
constraints.addMinLength(2);
constraints.addMaxLength(6);
constraints.addForbiddenNode("blocked");
auto result = constraints.findConstrainedPaths("start", "target", 10);
```

### Konfigurationsoptionen (Kurzüberblick)

- `GraphQueryOptimizer::QueryConstraints`: `max_depth`, `max_results`, `edge_type`, `graph_id`, `forbidden_vertices`, `required_vertices`
- `ParallelTraversal::Config`: `num_threads`, `fan_out_threshold`, `timeout_ms`
- `GPUGraphTraversal::Config`: `gpu_device`, `min_vertices_for_gpu`, `max_depth`
- `RefreshPolicy`: `refresh_interval`, `relevance_threshold`, `add_threshold`, `max_removal_fraction`, `ann_min_vertices`

---

## Troubleshooting

| Symptom | Ursache | Vorgehen |
|---------|---------|----------|
| `INVALID_STATE` bei Optimizer/Constraints | GraphManager nicht gesetzt/inkonsistente Lebensdauer | `GraphIndexManager` vor Nutzung initialisieren und Lebensdauer koppeln |
| Leere Ergebnismengen trotz erwarteter Pfade | Zu restriktive Constraints oder zu kleines `max_depth` | Constraints schrittweise lockern, `describeConstraints()` prüfen |
| GPU-Traversal läuft auf CPU | Schwellwert nicht erreicht oder keine GPU-Unterstützung | `min_vertices_for_gpu` prüfen, CPU-Fallback als regulären Betriebspfad einplanen |
| Edge-Refresh-Zyklen werden abgebrochen | Safety-Gates (`max_removal_fraction`, Add/Remove-Limits) zu strikt | `RefreshPolicy` schrittweise nachjustieren und `RefreshStats` auswerten |

---

## Installation

Das Modul ist Bestandteil von ThemisDB-Builds. Header werden über den
globalen Include-Pfad eingebunden:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
