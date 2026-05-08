# AI-Driven Query Optimization in ThemisDB

**Metadaten:**
- Source: Marcus et al. (2021) — Bao (SIGMOD); Zhou et al. (2022) — AI4DB (SIGMOD); Ryan Marcus et al. (2019) — Neo (VLDB); Leis et al. (2015) — How Good Are Query Optimizers, Really? (VLDB)
- URL: [arXiv:2004.03814](https://arxiv.org/abs/2004.03814) · [arXiv:2203.06618](https://arxiv.org/abs/2203.06618)
- Tags: `query-optimization`, `learned-optimizer`, `ai4db`, `treeconv`, `bandit`, `cardinality-estimation`, `adaptive-optimizer`, `runtime-feedback`
- ThemisDB-Versionen: v2.0.0+ (`src/query/adaptive_optimizer.cpp`, `src/query/runtime_reoptimizer.cpp`, `src/query/query_optimizer.cpp`)
- Status: [~] Partially Adopted (statistische Grundlagen implementiert; ML-Schicht geplant Q3/Q4 2026)

## 📋 Summary

ThemisDB integriert KI auf mehreren Ebenen in seinen Query-Optimizer, um die strukturellen Schwächen klassischer kostenbasierter Optimizer zu überwinden: fehlerhafte Kardinalitätsschätzungen, starre Join-Order-Heuristiken und mangelnde Workload-Adaption. Die Best-Practice-Schicht orientiert sich am **dreistufigen AI4DB-Modell**: statistische Verbesserung (Level 1), ML-gestütztes Feedback-Learning (Level 2) und selbststeuernde Datenbankoptimierung (Level 3).

Die zentrale Erkenntnis aus Leis et al. (2015) ist, dass Kardinalitätsfehler — nicht Kostenfunktionen — die häufigste Ursache für suboptimale Pläne sind. ThemisDB's AI-Schicht adressiert dies primär durch inkrementelles Feedback-Learning statt durch vollständige Optimizer-Ersetzung.

## 🎯 Core Principles

- **Principle 1 — Hint-Based Steering statt Full Replacement**: Der bestehende kostenbasierte Optimizer bleibt als Baseline. KI wählt aus einem konfigurierbaren Hint-Set (Join-Methode, Scan-Strategie, Parallelisierungsgrad), das den Optimizer steuert. Regressionsrisiko ist auf den schlechtesten Hint beschränkt — nicht auf vollständiges Optimizer-Versagen.
- **Principle 2 — Feedback-Loop mit realem Ausführungsmesswert**: Jede Abfrageausführung erzeugt ein Trainings-Triple `(Plan-Embedding, Hint-Index, gemessene_Latenz_ms)`. Das ML-Modell lernt ausschließlich aus Produktionsdaten; keine synthetischen Workloads als primäre Trainingsquelle.
- **Principle 3 — Tenant-Isolation**: Jeder Mandant erhält ein eigenes ML-Modell-Instanz (`AdaptiveQueryStats`-Partition). Lern-Signale eines Mandanten dürfen nicht die Optimizer-Entscheidungen eines anderen Mandanten beeinflussen.
- **Principle 4 — Bounded Regression Guarantee**: Das Thompson-Sampling-Bandit garantiert, dass die gewählte Hint-Auswahl mit hoher Wahrscheinlichkeit ≤ 1.5× schlechter als der Standard-Optimizer ist. Ohne diese Garantie ist ML-gestütztes Optimizer-Steering für Produktionssysteme nicht vertretbar.
- **Principle 5 — Observability First**: Jede AI-Optimizer-Entscheidung wird mit dem ausgewählten Hint, dem vorhergesagten Score und der tatsächlichen Ausführungszeit in OpenTelemetry-Spans protokolliert. Ohne vollständige Observability kann kein sinnvolles Debugging und keine Qualitätsbewertung erfolgen.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/query/adaptive_optimizer.cpp` — Level-2-Feedback-Loop: Plan-Embedding + Hint-Scoring + Online-Modell-Update
- `src/query/runtime_reoptimizer.cpp` — Ausführungszeit-Messung und Feedback-Weiterleitung an AdaptiveOptimizer
- `src/query/query_optimizer.cpp` — Plan-Feature-Extraktion (Operator-Typ, geschätzte Kardinalität, Histogramm-Selektivität) als TreeConv-Input
- `src/query/semantic_cache.cpp` — Level-1: Embedding-basiertes Query-Caching vermeidet redundante Optimizer-Durchläufe
- `src/storage/index_analyzer.cpp` — Level-2: `IIndexAnalysisAdvisor`-Hook für ML-gestützte Index-Empfehlungen
- `src/query/workload_cache_strategy.cpp` — Level-1: Workload-Frequenz-Histogramme als Proxy für Optimizer-Prioritäten

### What Was Adopted?

#### Level 1: Statistische Verbesserung (bereits implementiert)

- **Equi-Height-Histogramme**: `StatisticsCollector::getHistogram(collection, field)` liefert Selektivitätsschätzungen für Join-Kosten-Modell in `optimizer_cost_model.cpp`. Deutlich präziser als flache Kardinalitätsannahmen.
- **Semantic Query Cache**: `semantic_cache.cpp` verhindert wiederholte Optimizer-Läufe für semantisch äquivalente Queries via HNSW-Embedding-Similarity.
- **Adaptive Join-Strategie**: `adaptive_join.cpp` wählt zur Laufzeit zwischen Hash-Join, Sort-Merge und Nested-Loop basierend auf gemessenen Row-Counts — ein rudimentärer Feedback-Loop ohne ML.
- **Plan Cache**: `plan_cache.cpp` speichert kompilierte Pläne für parameter-invariante Query-Wiederholungen.

#### Level 2: ML-Feedback-Loop (geplant Q3/Q4 2026 — Bao-Integration)

- **TreeConv-Plan-Embedding**: Operator-Baum → 64-dim Embedding via 2-schichtiger Tree-CNN.
- **Thompson-Sampling-Bandit**: Hint-Auswahl auf Basis von Posterior-Normalverteilungen über vorhergesagte Latenzen.
- **Online-Modell-Update**: Replay-Buffer + Mini-Batch-SGD nach je 50 neuen Ausführungs-Triples.
- **IIndexAnalysisAdvisor**: Hook für ML-gestützte Index-Empfehlungen in `index_analyzer.cpp`.

#### Level 3: Self-Driving-Features (geplant Q2 2027)

- Automatisches Knob-Tuning (Buffer-Pool-Größe, Flush-Intervalle) via GP-Bayes oder DRL.
- Vollautomatische Index-Erstellung und -Löschung basierend auf Workload-Sampling.

### Deviations & Rationale

- **Kein vollständiger Optimizer-Ersatz (Neo/DQ-Ansatz)**: Neo und Deep Q-Network-basierte Optimizer ersetzen die gesamte Join-Order-Enumeration durch DRL. ThemisDB verwendet hint-basiertes Steering (Bao-Ansatz), da: (a) AQL ist Multi-Model (Graph, Vector, Spatial, Temporal) — nicht nur Join-Order; (b) Regressions-Risiko bei Full-Replacement ist inakzeptabel für Produktionssystem; (c) Bao konvergiert schneller und ist leichter zu debuggen.
- **CPU-only TreeConv (kurzfristig)**: ONNX Runtime statt CUDA für TreeConv-Inferenz, um GPU-Budget dem Vector-Search-Pfad vorzubehalten. Scoring-Latenz < 5 ms auf CPU ist für Optimizer-Overhead akzeptabel.

## ⚠️ Trade-offs & Limitations

- **Kaltstart**: Neuer Tenant hat kein Modell → fällt auf Standard-Optimizer zurück. Bootstrapping durch plan-feature-ähnliche Transfer-Learning-Initialisierung erforderlich.
- **Distribution Shift**: Batch-Import großer Datenmengen kann Histogramme obsolet machen → Trigger-basiertes Modell-Retraining via `IndexAnalyzer`.
- **Adversarielle Queries**: Böswillige Queries könnten gezielt den Bandit-State manipulieren → Anomalie-Erkennung und Signaturvalidierung für ML-Modelle.
- **AQL-Komplexität**: ThemisDB's AQL-Pläne sind komplexer als reine SQL-Join-Bäume (Graph-Traversal, Vector-ANN, GeoSpatial) → TreeConv muss für Multi-Model-Operatoren erweitert werden.

## 🔬 Validation

- [x] Statistische Grundlage (Histogramme, Prometheus-Metriken) in `query_optimizer.cpp` implementiert und getestet
- [x] Adaptive Join-Strategie in `adaptive_join.cpp` implementiert
- [ ] TreeConv-Modell implementiert und gegen Paper-Benchmark validiert
- [ ] Bandit-Aktion-Selektion implementiert (Thompson Sampling)
- [ ] Online-Modell-Update implementiert (Replay-Buffer + SGD)
- [ ] A/B-Test: AI-Optimizer vs. Standard-Optimizer auf AQL-Join-Workload
- [ ] Bounded-Regression-Guarantee experimentell verifiziert (≤ 1.5× Worst-Case)
- [ ] OpenTelemetry-Spans für Hint-Entscheidungen implementiert

## 📚 Related

- [Bao — Marcus et al. (2021)](../papers/marcus_bao_learned_query_opt_2021.md)
- [AI4DB Survey — Zhou et al. (2022)](../papers/zhou_ai4db_survey_2022.md)
- [`src/query/adaptive_optimizer.cpp`](../../../src/query/adaptive_optimizer.cpp)
- [`src/query/runtime_reoptimizer.cpp`](../../../src/query/runtime_reoptimizer.cpp)
- [`src/storage/index_analyzer.cpp`](../../../src/storage/index_analyzer.cpp)
- [How Good Are Query Optimizers, Really? — Leis et al. (2015)](https://dl.acm.org/doi/10.14778/2850583.2850594)

---
**Last Updated:** 2026-04-27
