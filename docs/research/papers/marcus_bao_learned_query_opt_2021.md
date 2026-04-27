# Bao: Making Learned Query Optimization Practical

**Metadaten:**
- Author(en): Ryan Marcus, Parimarjan Negi, Hongzi Mao, Nesime Tatbul, Mohammad Alizadeh, Tim Kraska
- Konferenz/Journal: ACM SIGMOD International Conference on Management of Data, 2021
- Jahr: 2021
- Link: [arXiv:2004.03814](https://arxiv.org/abs/2004.03814) · [ACM DL](https://dl.acm.org/doi/10.1145/3448016.3452838)
- Zitierweise: `marcus2021bao`
- Tags: `learned-query-optimizer`, `query-optimization`, `tree-conv`, `bandit`, `reinforcement-learning`, `ai4db`, `adaptive-optimizer`
- ThemisDB-Versionen: v2.0.0+ (`src/query/adaptive_optimizer.cpp`, `src/query/runtime_reoptimizer.cpp`)
- Status: [~] In Progress (TreeConv embedding planned Q3 2026; bandit hint selection planned Q4 2026)

## 📋 Executive Summary

Bao (Bandit-over-operators) ist ein **lernender Query-Optimizer**, der die Wissenslücken klassischer kostenbasierter Optimizer mit einem Tree-Convolutional Neural Network (TreeConv) + Multi-Armed-Bandit-Framework schließt. Anstatt den gesamten Optimizer zu ersetzen, selektiert Bao aus einer konfigurierbaren Menge von **Query-Hints** (z.B. Join-Methode, Scan-Strategie, Parallelisierungsgrad), bewertet alternative Pläne durch Vorhersage der Ausführungszeit und lernt aus realen Ausführungsdaten inkrementell weiter. Dieses pragmatische Design macht Bao direkt kompatibel mit bestehenden Optimizer-Infrastrukturen — ideal für ThemisDB's `AdaptiveOptimizer` + `RuntimeReoptimizer`-Architektur.

Direkt referenziert in `src/query/FUTURE_ENHANCEMENTS.md` (Phase 9: Learned Query Optimization, Target Q3/Q4 2026).

## 🎯 Key Findings

- **TreeConv-Embedding**: Query-Pläne werden als Bäume mit Operatornamen, geschätzter Kardinalität und Kostenschätzung als Features kodiert. Eine Tree-CNN-Architektur erzeugt einen festen Embedding-Vektor pro Plan, der für Regressionsvorhersage der Ausführungszeit verwendet wird.
- **Hint-Sets als Aktionsraum**: Statt alle Plan-Permutationen zu betrachten, wählt Bao aus einem kleinen Set von Optimizer-Hints (typisch 5–20). Jeder Hint-Satz erzeugt einen alternativen Plan; das Modell bewertet alle Pläne und wählt den mit niedrigster vorhergesagter Latenz.
- **Thompson-Sampling-Bandit**: Bao verwendet einen Bayes'schen Bandit (Thompson Sampling) statt Policy Gradient / Q-Learning — keine Exploration-Exploitation-Dilemma-Probleme, keine Belohnungsverzögerung.
- **Inkrementelles Lernen**: Nach jeder Abfrageausführung wird das Modell mit dem Trainings-Triple `(Plan-Embedding, Hint-Index, gemessene_Latenz)` online aktualisiert. Konvergenz nach ≈200 Abfragen auf JOB-Benchmark.
- **Robustheit vs. Regression**: Bao garantiert durch den Bandit-Mechanismus eine gebundene Regression gegenüber dem klassischen Optimizer; das Worst-Case-Overhead-Risiko ist kontrollierbar.
- **Generalisierung**: Ein auf Workload A trainiertes Bao-Modell übertrifft einen Cold-Start-Optimizer auf Workload B noch vor vollständiger Konvergenz — wertvoller Warmstart für neue Tenant-Workloads.
- **Latenz-Overhead**: Bao-Scoring für einen Plan: < 1 ms (GPU) / < 5 ms (CPU-only); vernachlässigbar gegenüber Optimizer-Gesamtlatenz (typisch 5–50 ms).

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [~] Query Optimizer → `src/query/adaptive_optimizer.cpp` (Hint-Set-Auswahl via TreeConv-Score)
- [~] Runtime Reoptimizer → `src/query/runtime_reoptimizer.cpp` (Online-Feedback-Loop: gemessene Latenz → Modell-Update)
- [~] Query Optimizer → `src/query/query_optimizer.cpp` (Plan-Annotation mit Operator-Features für TreeConv-Input)
- [ ] Training Module → `src/training/` (Speicherung und periodisches Retraining des Bao-Modells)
- [ ] LLM/ML-Plugin → `src/llm/` (ONNX-Export des TreeConv-Modells für ONNX Runtime Inferenz)

### What Was Adopted?

1. **TreeConv-Plan-Embedding**: `AdaptiveOptimizer` annotiert jeden generierten Query-Plan mit Operator-Typ, geschätzter Eingabezeilen-Anzahl (aus `StatisticsCollector::getHistogram()`) und bisheriger Ausführungshistorie. Ein leichtgewichtiger TreeConv-Encoder (2 Faltungsschichten, Hidden-Dim 128) erzeugt ein 64-dim Plan-Embedding.
2. **Hint-Set-Definition**: ThemisDB-Hints umfassen: `{join_method: [hash, merge, nested_loop], scan_type: [index, full, bitmap], parallelism: [1, 4, 16], materialization: [eager, lazy]}` — 18 kombinierte Hint-Vektoren pro Query.
3. **Thompson-Sampling-Bandit**: Jeder Hint-Index hat eine Normalverteilung über die vorhergesagte Latenz (Mittelwert + Varianz aus dem TreeConv-Regressor). Thompson Sampling zieht einen Sample pro Hint-Vektor und wählt den mit dem niedrigsten Sample.
4. **Online-Feedback via RuntimeReoptimizer**: `RuntimeReoptimizer::ExecutionGuard` misst nach Abschluss die reale Latenz und schreibt `(embedding, hint_idx, actual_latency_ms)` in einen Replay-Buffer. Alle 50 neuen Samples wird das TreeConv-Modell mit SGD/Adam inkrementell aktualisiert.

### How Was It Adapted?

| Bao Konzept | ThemisDB Adaptation | Rationale |
|---|---|---|
| PostgreSQL-Hint-Injection | AQL `OPTIMIZER_HINT` Direktive in `aql_parser.cpp` | ThemisDB nutzt eigene Hint-Infrastruktur statt PostgreSQL-spezifischer Hints |
| JOB-Benchmark-Workload | Tenant-spezifische Workload-Profile (Multi-Tenant-Isolation) | Jeder Tenant erhält ein eigenes Bao-Modell im `AdaptiveQueryStats`-Store |
| Einzel-GPU-Training | CPU-only TreeConv mit ONNX Runtime | Optimizer-GPU-Budget soll dem Vector-Suche-Pfad vorbehalten bleiben |
| Festes Hint-Set | Dynamisch erweiterbares Hint-Set via YAML-Konfiguration | ThemisDB-Operator-Typen wachsen mit jeder Version |
| Vollständiges Modell-Retraining | Inkrementelles Online-Training (Replay-Buffer, SGD) | Kein Downtime-Fenster für periodisches Retraining erforderlich |

### Performance Impact

| Metric | Bao-Paper-Claim | ThemisDB Target | Status |
|--------|-----------------|-----------------|--------|
| Median-Latenz-Verbesserung gegenüber PostgreSQL-Standard-Optimizer | -40% auf JOB-Benchmark | -20% (konservativ) auf AQL-Join-Workload | ⏳ Planned Q3/Q4 2026 |
| Worst-Case-Regression gegenüber Standard-Optimizer | < 2× | < 1.5× | ⏳ Planned |
| Konvergenz nach N Abfragen | ~200 Queries (JOB) | ~500 Queries (AQL Multi-Model) | ⏳ Planned |
| Bao-Scoring-Overhead | < 1 ms | < 5 ms (CPU-only TreeConv) | ⏳ Planned |
| Kaltstart-Verhalten (neuer Tenant) | Standard-Optimizer bis Konvergenz | Standard-Optimizer + Thompson-Exploration | ⏳ Planned |

## ⚠️ Limitations & Open Questions

- Bao setzt voraus, dass alle Pläne tatsächlich ausgeführt werden, um Feedback zu erhalten; seltene Queries konvergieren langsam.
  - ThemisDB-Lösung: Warmstart durch Plan-Feature-Ähnlichkeit zu bekannten Queries; K-NN-basiertes Similarity-Bootstrapping.
- TreeConv-Embedding ist nicht aus dem Nichts lernbar; benötigt einen Mindest-Trainingsdatensatz (≈100 Queries).
  - ThemisDB-Lösung: Bootstrapping mit synthetisch generierten Workload-Queries zur initialen Modell-Initialisierung.
- Bao arbeitet mit hint-based Optimizer-Steering; wenn der Basisoptimizierer suboptimale Pläne generiert (falsche Kardinalitätsschätzung), können Hints das nicht vollständig kompensieren.
  - ThemisDB-Lösung: Parallelisierung mit verbesserter Kardinalitätsschätzung (MSCN — Multi-Set Convolutional Network; siehe `CARDINALITY_ESTIMATION.md`).
- Modell-Drift bei stark verändernden Datenverteilungen (z.B. Batch-Import großer Datenmengen).
  - ThemisDB-Lösung: Index-Analyzer-Trigger (`src/storage/index_analyzer.cpp`) löst Modell-Reset aus bei `data_volume_change > threshold`.
- Mehrere konkurrierende Abfragen können den Bandit-State gleichzeitig aktualisieren — Race-Condition möglich.
  - ThemisDB-Lösung: Replay-Buffer-Schreibzugriff via `std::mutex`; Batch-SGD auf Snapshot.

## 🔬 Validation

- [ ] Code reviewed against paper
- [ ] Unit tests written (TreeConv forward pass, bandit action selection)
- [ ] Integration test: Bao vs. baseline optimizer on synthetic AQL join workload
- [ ] Benchmark executed (median latency on 100-query workload)
- [ ] Documentation updated (`src/query/FUTURE_ENHANCEMENTS.md` Phase 9)
- [ ] Module README linked (`src/query/README.md`)
- [ ] implementation_influence index updated

## 📚 Related Work

- [AI Meets Database: AI4DB Survey — Zhou et al. (2022)](zhou_ai4db_survey_2022.md)
- [DuckDB — Raasveldt & Mühleisen (2019)](duckdb_olap_2019.md)
- [CQL — Arasu, Babu & Widom (2006)](arasu_cql_2006.md) (Streaming Query Optimization)
- [Best Practice: AI-Driven Query Optimization](../best_practices/ai_driven_query_optimization.md)
- [`src/query/adaptive_optimizer.cpp`](../../../src/query/adaptive_optimizer.cpp)
- [`src/query/runtime_reoptimizer.cpp`](../../../src/query/runtime_reoptimizer.cpp)
- [Neo: A Learned Query Optimizer — Marcus et al. (2019)](https://arxiv.org/abs/1904.03711)
- [MSCN: Learned Cardinality Estimation — Kipf et al. (2019)](https://arxiv.org/abs/1809.00677)

---
**Last Updated:** 2026-04-27  
**Next Review:** 2026-09-30
