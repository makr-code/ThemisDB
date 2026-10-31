---
Author: "ThemisDB Team"
Created: "2026-10-29"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# Vector Search Module

## Überblick

`src/vector_search/` ist die Dokumentations- und Governance-Grenze des Moduls.
Die Laufzeitimplementierung befindet sich im gemeinsamen Index-Subsystem unter
`src/index/` und `include/index/`; dieses Verzeichnis enthält keinen eigenen
Vector-Search-Runtime-Code.

## Kernquellen

- `include/index/vector_index.h` und `src/index/vector_index.cpp` —
  `VectorIndexManager`, RocksDB-Namespace, Such- und Lifecycle-APIs
- `include/index/multi_vector_search.h` und
  `src/index/multi_vector_search.cpp` — Mehrvektor- und Fusionssuche
- `include/index/distributed_vector_index.h` —
  In-Process-Sharding; keine Zusicherung für persistente Replikation oder einen
  Multi-Node-Produktionsdienst

## Validierung

Der High-Cardinality-Test und der Integration-Soak-Test verwenden lokale
Flat-Index-Fixtures. Auch der dedizierte Gate-Benchmark misst einen synthetischen
Flat-Index. Diese Artefakte sind kein Nachweis für Laufzeitperformance oder
Produktionsreife des tatsächlichen Index-Subsystems.

Ein fokussierter CTest-Lauf muss mit einem testfähigen, konfigurierten und im
Repository verfügbaren CMake-Preset erfolgen; `windows-release` deaktiviert
Tests. Siehe [Roadmap](../../../src/vector_search/ROADMAP.md),
[Performance-Erwartungen](../../../src/vector_search/PERFORMANCE_EXPECTATIONS.md)
und [Lückenregister](../../../src/vector_search/MODULE_GAPS.md).
