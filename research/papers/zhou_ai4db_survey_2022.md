# AI Meets Database: AI4DB and DB4AI

**Metadaten:**
- Author(en): Xuanhe Zhou, Chengliang Chai, Guoliang Li, Ji Sun
- Konferenz/Journal: ACM SIGMOD International Conference on Management of Data, 2022
- Jahr: 2022
- Link: [ACM DL](https://dl.acm.org/doi/10.1145/3514221.3522563) · [arXiv:2203.06618](https://arxiv.org/abs/2203.06618)
- Zitierweise: `zhou2022ai4db`
- Tags: `ai4db`, `db4ai`, `learned-optimizer`, `knob-tuning`, `cardinality-estimation`, `index-advisor`, `self-driving-database`, `survey`
- ThemisDB-Versionen: v2.0.0+ (methodischer Rahmen für alle AI-in-DB-Features)
- Status: [~] In Progress (AI4DB-Kategorisierung als Entwicklungsrahmen übernommen; einzelne Techniken gezielt implementiert)

## 📋 Executive Summary

Dieses SIGMOD-2022-Paper ist das maßgebliche Übersichtswerk zur Integration von KI in Datenbankmanagement-Systeme. Es etabliert zwei komplementäre Forschungsrichtungen: **AI4DB** (KI verbessert Datenbank-Internals wie Optimizer, Index-Advisor, Konfigurationstuning) und **DB4AI** (Datenbanken als Infrastruktur für KI-Trainings-Pipelines). Das Paper systematisiert fünf Kernbereiche von AI4DB — Query Optimization, Cardinality Estimation, Index/View Advisor, Knob Tuning und Self-Driving Databases — und liefert damit den wissenschaftlichen Rahmen, innerhalb dessen ThemisDB's KI-gestützte Query-Optimierung, adaptiver Cache und IndexAnalyzer einzuordnen sind.

Direkt relevant für `src/query/FUTURE_ENHANCEMENTS.md` (Phase 9), `src/storage/index_analyzer.cpp` (IIndexAnalysisAdvisor), `src/cache/adaptive_query_cache.cpp` und die geplante Self-Driving-Database-Roadmap in `src/ROADMAP.md`.

## 🎯 Key Findings

- **AI4DB — Query Optimization**: Dreiteilung in *Plan-Space-Exploration* (Neo, Bao), *Cardinality Estimation* (MSCN, DeepDB, NeuroCard) und *Cost Estimation* (Zero-Shot-Schätzer). Bao's Hint-basierter Ansatz ist der produktionsreifste.
- **AI4DB — Cardinality Estimation**: ML-Modelle (MSCN, Naru, NeuroCard) übertreffen PostgreSQL-Histogramme um 10–100×; Problem: Modell-Drift bei Datenänderungen erfordert Online-Retraining.
- **AI4DB — Index/View Advisor**: RL-basierte Advisors (DBA Bandit, Smartix) empfehlen Index-Konfigurationen auf Basis von Workload-Sampling; direkter Bezug zu ThemisDB's `IndexAnalyzer` mit `IIndexAnalysisAdvisor`.
- **AI4DB — Knob Tuning**: Systeme wie OtterTune (GP-basiiert), CDBTune (DRL) und Ottertune-v2 (LLM-Empfehlungen) tunen Datenbankparameter (Buffer-Pool-Größe, Flush-Intervalle, etc.) automatisch.
- **AI4DB — Self-Driving Databases**: Carnegie Mellon's NoisePage-Projekt implementiert ein vollautomatisches DBMS ohne manuellen DBA-Eingriff. Peloton ist der Vorläufer.
- **DB4AI — In-Database ML**: Beschleunigte Feature Engineering, Model Training (MLSQL) und Inferencing direkt im Datenbankkernel ohne ETL-Overhead. Relevant für ThemisDB's `LLM INFER`-AQL-Direktive.
- **Herausforderungen**: Distribution Shift, Modell-Interpretierbarkeit, Sicherheitsprobleme bei ML-Modellen in kritischen Optimizer-Entscheidungen.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [~] Query Optimizer → `src/query/adaptive_optimizer.cpp` (AI4DB: Plan-Space-Exploration via Bao-TreeConv)
- [~] Query Optimizer → `src/query/query_optimizer.cpp` (AI4DB: Cardinality Estimation via StatisticsCollector + geplanter MSCN-Integrationspunkt)
- [x] Index Analyzer → `src/storage/index_analyzer.cpp` (AI4DB: Index/View Advisor via `IIndexAnalysisAdvisor`-Hook)
- [x] Adaptive Cache → `src/cache/adaptive_query_cache.cpp` (AI4DB: Workload-Aware Caching via Bayesian Optimization)
- [x] RAG / LLM → `src/rag/`, `src/llm/` (DB4AI: In-Database LLM Inferencing via AQL `LLM INFER` Direktive)
- [ ] Knob Tuning → `src/config/` (AI4DB: Automated Knob Tuning — geplant als eigenständiges Modul, Target Q2 2027)

### What Was Adopted?

1. **Konzeptueller AI4DB-Rahmen**: ThemisDB strukturiert seine KI-Integrationen gemäß dem AI4DB-Paradigma: drei Implementierungsebenen — (a) statistisches Lernen im Optimizer (`StatisticsCollector`), (b) ML-gestütztes Feedback (`AdaptiveOptimizer`, `RuntimeReoptimizer`, `BayesianOptimizer` in RAG), (c) LLM-gestützte Self-Driving-Features (`IndexAnalyzer::IIndexAnalysisAdvisor`).
2. **IIndexAnalysisAdvisor-Interface**: Das in `src/storage/index_analyzer.h` definierte `IIndexAnalysisAdvisor`-Interface wurde direkt von der AI4DB-Index-Advisor-Kategorie inspiriert. Implementierungen können RL-Modelle oder LLM-Empfehlungsservices koppeln.
3. **DB4AI-Direktiven in AQL**: Die `LLM INFER`, `RAG`, `EMBED`-Direktiven in `aql_parser.cpp` realisieren das DB4AI-Konzept der In-Database-ML-Inferenz ohne ETL-Overhead.
4. **Workload-basiertes Cache-Tuning**: `workload_cache_strategy.cpp` implementiert das AI4DB-Konzept workload-adaptiver Speicherzuweisung über Frequenz-Histogramme und Promotion-Schwellwerte.

### How Was It Adapted?

| AI4DB Konzept | ThemisDB Adaptation | Rationale |
|---|---|---|
| PostgreSQL/MySQL als Baseline-DBMS | AQL Multi-Model Engine | ThemisDB hat eigene Plandarstellung; kein direkter PG-Plan-Baum-Import möglich |
| Separater ML-Service für Optimizer | Eingebettetes ONNX-Modell im Optimizer-Pfad | Zero-latency-Anforderung (< 5 ms) verhindert Remote-Service-Calls |
| OtterTune-Style Knob Tuning | Konfigurierbare YAML-Profile + manuelle Expertenregeln (kurzfristig) | Automatisches Knob-Tuning erfordert Sicherheitsbewertung für Produktionsumgebungen |
| NoisePage Self-Driving | Graduelle AI-Erweiterung bestehender Optimizer-Infrastruktur | Inkrementeller Ansatz reduziert Regressions-Risiko gegenüber vollständigem Neuentwurf |

### Performance Impact

| Metric | Paper-Claim (Survey) | ThemisDB Target | Status |
|--------|----------------------|-----------------|--------|
| Cardinality Estimation Fehler (MSCN vs. Histogramme) | 10–100× geringerer Q-Error | 5× geringerer Q-Error (konservativ) | ⏳ Planned (Bao-Phase liefert Basis) |
| Index-Advisor Recall (empfohlene Indexe tatsächlich hilfreich) | 70–90% (DBA Bandit) | ≥ 75% | ⏳ Planned via IIndexAnalysisAdvisor |
| Knob-Tuning-Verbesserung gegenüber Default-Konfiguration | 30–60% Durchsatzgewinn (OtterTune) | 20% (konservativ) | ⏳ Planned Q2 2027 |

## ⚠️ Limitations & Open Questions

- ML-Modelle im Optimizer-Pfad stellen ein Sicherheitsrisiko dar: adversarielle Queries können Modell-Bias ausnutzen.
  - ThemisDB-Lösung: Signaturvalidierung für ML-Modelle via `src/llm/lora_security_validator.cpp`-Pattern; Anomalie-Erkennung für atypische Optimizer-Entscheidungen.
- Cardinality-Estimation-Modelle leiden unter Distribution Shift nach großen Batch-Importen.
  - ThemisDB-Lösung: `IndexAnalyzer`-Trigger bei signifikanter Datenmengenzunahme löst Modell-Retraining aus.
- Self-Driving-Features erfordern Produktions-Observability (Profiler-Daten für ML-Modell-Training).
  - ThemisDB-Lösung: `QueryProfiler` (`src/query/query_profiler.cpp`) liefert `OperatorProfile`-Daten als Trainings-Features.

## 🔬 Validation

- [ ] Code reviewed against paper
- [ ] Konzeptuelles Framework-Mapping mit ThemisDB-Modulen dokumentiert
- [ ] Benchmark: Vergleich AI4DB-Ansätze vs. klassische Optimizer-Metriken (Join-Order-Benchmark, AQL-Workload)
- [ ] Module README linked (`src/query/README.md`, `src/storage/README.md`)
- [ ] implementation_influence index updated

## 📚 Related Work

- [Bao: Learned Query Optimization — Marcus et al. (2021)](marcus_bao_learned_query_opt_2021.md)
- [DuckDB — Raasveldt & Mühleisen (2019)](duckdb_olap_2019.md)
- [Best Practice: AI-Driven Query Optimization](../best_practices/ai_driven_query_optimization.md)
- [IndexAnalyzer AI/ML Hook](../../../src/storage/index_analyzer.h)
- [NoisePage / Peloton Self-Driving DBMS (CMU)](https://db.cs.cmu.edu/projects/noisepage/)
- [OtterTune — Van Aken et al. (2017)](https://dl.acm.org/doi/10.1145/3035918.3064029)
- [MSCN: Learned Cardinality Estimation — Kipf et al. (2019)](https://arxiv.org/abs/1809.00677)

---
**Last Updated:** 2026-04-27  
**Next Review:** 2026-09-30
