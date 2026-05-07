> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Analytics Module Roadmap

**Version:** 2.0.0
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-04-19
**Module Path:** `src/analytics/`

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
Production-ready for core OLAP, data export, process mining, text analytics, LLM integration, CEP engine, streaming aggregation windows, incremental materialized views, real-time anomaly detection, model serving / online inference pipeline, and predictive analytics / time-series forecasting.

## Completed ✅
- [x] OLAP engine with GROUP BY, CUBE, ROLLUP, and GROUPING SETS
- [x] Window functions (ROW_NUMBER, SUM OVER, AVG OVER with frame specs)
- [x] Statistical aggregations (COUNT, SUM, AVG, MIN, MAX, STDDEV, VARIANCE, MEDIAN, PERCENTILE)
- [x] Hash-based aggregation with result caching
- [x] Columnar (Arrow) RecordBatch storage always available
- [x] JSON and CSV export (no external dependencies)
- [x] Optional Apache Arrow IPC, Parquet (with compression), and Feather export
- [x] Process mining: Alpha Miner, Heuristic Miner, Inductive Miner
- [x] Conformance checking (token replay and alignment-based)
- [x] Process pattern matcher (graph, vector, behavioral, hybrid similarity)
- [x] NLP text analyzer (tokenization, TF-IDF, NER, sentiment, keyword extraction)
- [x] LLM process analyzer (OpenAI, Anthropic, Azure OpenAI, llama.cpp)
- [x] Diff engine (changefeed-backed git-like diffs)
- [x] SIMD-accelerated aggregations (AVX2)
- [x] Thread-safe OLAPEngine for concurrent queries
- [x] CEP full engine (NFA pattern matching, EPL parser, window+aggregation pipeline, alert dispatch, CDC integration) (`analytics/cep_engine.cpp`)
- [x] CEP: EPL (Event Processing Language) parser: `CREATE RULE … AS`, SELECT aggregations (COUNT/SUM/AVG/MIN/MAX/FIRST/LAST/STDDEV/VARIANCE/PERCENTILE/DISTINCT_COUNT/COLLECT/TOPN with AS alias), GROUP BY, parenthesized WINDOW specs with human-readable time units (ms/s/minutes/hours/days), PATTERN WITHIN with time units, ACTION dispatch (alert/webhook/db_write/log/slack/kafka/email), multi-line EPL normalization (`analytics/cep_engine.cpp`)
- [x] CEP stateful pattern matching with checkpointing: `PatternMatcher::serializeState()`/`restoreState()`, `RuleEngine::serializeMatcherStates()`/`restoreMatcherStates()`, full NFA partial-match persistence across restarts (`analytics/cep_engine.cpp`)
- [x] Streaming aggregation windows: TumblingWindow, SlidingWindow, SessionWindow, HoppingWindow with watermark support (`analytics/streaming_window.cpp`)
- [x] Incremental materialized views with delta-maintenance for all 10 aggregation functions, Welford STDDEV/VARIANCE, COUNT_DISTINCT ref-counting (`analytics/incremental_view.cpp`)
- [x] Real-time anomaly detection: Z-Score, Modified Z-Score (MAD), IQR, Isolation Forest, LOF, Ensemble with adaptive learning (`analytics/anomaly_detection.cpp`)
- [x] AutoML integration for automated model selection: Logistic Regression, Decision Tree, Random Forest, Gradient Boosting, KNN, Linear Regression with hyperparameter search, feature engineering, ensemble generation, and SHAP-based explanations (`analytics/automl.cpp`)
- [x] CEP engine-level backpressure handling and buffer management: engine queue depth limit, drop policy, backpressure signal at configurable threshold, Prometheus metrics (`analytics/cep_engine.cpp`)
- [x] Integration with external ML tools: ONNX Runtime (local inference) and TensorFlow Serving (REST API) via unified `MLServingClient` abstraction with `DataPoint` integration and graceful degradation when backends are absent (`analytics/ml_serving.cpp`)
- [x] Model serving and online inference pipeline: thread-safe named+versioned model registry, online/batch inference, class-probability output, per-model health metrics, serialization round-trip (`analytics/model_serving.cpp`)
- [x] Predictive analytics and time-series forecasting: LINEAR_REGRESSION, EXP_SMOOTHING, Holt-Winters triple exponential smoothing, ARIMA (AR+I+MA via Yule–Walker), ENSEMBLE with weighted combination; confidence intervals, seasonal decomposition, accuracy metrics (MAE, RMSE, MAPE, sMAPE), model serialization round-trip (`analytics/forecasting.cpp`)
- [x] Predictive analytics and time-series forecasting (Issue: #1473)
- [x] Forecasting v1.9.0 enhancements (Issue: #4054): `predictBatch()` (batch forecasting across N series), `update(double)` (O(1) incremental ETS/ARIMA/LR state absorption), parallel auto-tune grid search via `std::async` (9 α tasks), FNV-1a 64-bit fit-result cache (O(1) repeated-fit bypass) — 17 new tests in `ForecastingBatchStreamingTests`
- [x] AutoML integration for automated model selection (Issue: #1485) ✅
- [x] Advanced graph analytics: betweenness centrality, Louvain community detection (Issue: #1475)
- [x] Integration with external ML tools (ONNX Runtime, TensorFlow Serving) (Issue: #1476) ✅
- [x] Multi-language NLP support (beyond English) (Issue: #1478)
- [x] Full morphological lemmatization (Issue: #1479)
- [x] Arrow Flight RPC support for remote analytics: in-process + optional native gRPC transport (Issue: #1472) (`analytics/arrow_flight.cpp`)
- [x] **Multi-stream join engine** — `IStreamingJoin` / `HashJoin` / `IntervalJoin` (Issue: #4576) (2026-04-12)
  - `include/analytics/streaming_join.h` + `src/analytics/streaming_join.cpp`
  - `HashJoin`: composite-key hash table, inner/left-outer join, multi-batch build, configurable `max_build_rows`
  - `IntervalJoin`: `before_ms`/`after_ms`/`slack_ms` time-window join with LRU probe-side pruning
  - 15 focused tests (SJ-01…SJ-15) in `tests/analytics/test_streaming_join.cpp`

## In Progress 🚧
*(none — all Phase 3 items completed)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [P] GPU-accelerated OLAP aggregations (CUDA) (Issue: #1469)
- [P] Zero-copy Arrow data transfer optimizations (Issue: #1471)

### Long-term (6-12 months)

- [ ] CUDA geospatial distance and containment kernels (Target: Q3 2026)
  - Inputs: WGS84 points/polygons, batch-size up to 1e6
  - Outputs: distance matrix + containment bitset
  - Constraints: deterministic FP tolerance ≤ 1e-6
  - Errors: invalid geometry (NaN/Inf coordinates), polygon self-intersection, overflow during Haversine distance
  - Tests: unit + property-based + GPU/CPU parity
  - Perf: ≥ 8x speedup vs CPU baseline on RTX-class GPU
- [x] Federated analytics query dispatch across multiple ThemisDB clusters (Target: Q3 2026)
  - Affected: `src/analytics/distributed_analytics.cpp`, `include/analytics/distributed_analytics.h`
  - Expected behavior: scatter-gather with partial failure tolerance; partial results returned if <20% shards fail
  - Errors: shard unreachable → skip with warning; tenant isolation violation → reject with PERMISSION_DENIED
  - Tests: FED-01..FED-08 (`tests/analytics/test_distributed_analytics.cpp`)
  - Perf: fan-out latency ≤ 200 ms for 16 shards on LAN
  - Per-tenant data isolation at the `SourceRegistry` boundary
- [x] SARIMA and Prophet-style forecasting models (Target: Q4 2026)
  - Affected: `src/analytics/forecasting.cpp`, `include/analytics/forecasting.h`
  - Note: `ForecastMethod::SARIMA` and `ForecastMethod::PROPHET` enum values are already defined (`include/analytics/forecasting.h:154-155`) but switch-case handlers are not yet implemented
  - Expected behavior: `fit()`/`predict()` API unchanged; extends existing switch branches
  - Errors: insufficient data for seasonal period (< 2 × seasonality), NaN in input series → structured error
  - Tests: unit tests for fit/predict/evaluate/serialize round-trip; parity vs Python statsmodels reference
  - Perf: SARIMA fit ≤ 5 s for series of length 10 000
  - Confidence intervals and decomposition retained
- [x] AutoML ONNX export and deployment pipeline (Target: Q4 2026)
  - Affected: `src/analytics/automl.cpp`, `include/analytics/automl.h`
  - Expected behavior: `AutoMLModel::exportONNX(path)` serializes trained model; loadable by `MLServingClient`
  - Expected behavior: `AutoML::exportONNX(path)` serializes trained model; loadable by `MLServingClient` <!-- TODO: verify exact method signature when implemented -->
  - Errors: unsupported model type → `UNSUPPORTED_OPERATION`; serialization failure → structured error with cause
  - Tests: unit test export → load → infer round-trip; ONNX opset compatibility for all supported algorithms
  - Perf: export time ≤ 500 ms for any model trained on ≤ 1M samples

- [ ] **Expertensystem-Engine** (CEP + Rule-Engine + ML-Inferenz) (Target: Q2 2027)
  - Affected: `include/analytics/expert_system_engine.h` (new), `src/analytics/expert_system_engine.cpp` (new),
    `include/analytics/knowledge_base.h` (new), `src/analytics/knowledge_base.cpp` (new),
    Integration mit `src/analytics/cep_engine.cpp`, `src/analytics/model_serving.cpp`
  - Scope: Das bestehende CEP-NFA-Pattern-Matching (`cep_engine.cpp`) bildet das Regelausführungssubsystem
    (Working Memory + Agenda + NFA-Matcher). `ExpertSystemEngine` erweitert es um:
    (1) eine persistente `KnowledgeBase` für Fakten (RocksDB-backed), (2) einen Inferenz-Controller
    der Vorwärts- und Rückwärtsverkettung unterstützt, (3) eine Erklärungskomponente die
    Entscheidungspfade als geordnete Regelanwendungs-Traces exportiert
  - Expected behavior:
    - `ExpertSystemEngine::assertFact(fact)` → schreibt Fakt in Working Memory; triggert NFA-Evaluierung
    - `ExpertSystemEngine::queryGoal(goal)` → Rückwärtsverkettung; liefert `GoalResult` mit Proof-Trace
    - `ExpertSystemEngine::forwardChain(max_cycles)` → Vorwärtsverkettung bis Fixpunkt oder Limit
    - `ExpertSystemEngine::explain(decision_id)` → exportiert Regelanwendungs-Trace als JSON
    - ML-Augmentierung: `ExpertSystemEngine::setMLScorer(ModelServingEngine*)` → AI/ML-Modell
      bewertet Regelprämissen mit Konfidenzwert; unscharfe Regeln (confidence < threshold) werden
      als Hinweis nicht als harte Entscheidung behandelt
  - Constraints: max 10 000 aktive Fakten im Working Memory (Ring-Eviction); max 100 Regeln;
    Vorwärtsverkettungs-Zyklus ≤ 50 ms; Erklärungsgeneration ≤ 10 ms
  - Errors: Regelwiderspruch → `ConflictError` mit beteiligten Regel-IDs; zirkulärer Beweis →
    Depth-Limit-Fehler; ML-Scorer nicht erreichbar → deterministische Regelauswertung
  - Tests: ES-01..ES-20 in `tests/analytics/test_expert_system_engine.cpp`
    - ES-01..ES-05: Faktenassertierung + Vorwärtsverkettung
    - ES-06..ES-10: Rückwärtsverkettung + Proof-Traces
    - ES-11..ES-14: ML-Scorer-Integration (Mock)
    - ES-15..ES-17: Regelkonflikt-Erkennung
    - ES-18..ES-20: Concurrency (8 Threads)
  - Perf: 1 000 Fakten + 100 Regeln, Vorwärtsverkettung ≤ 50 ms; Rückwärtsverkettung ≤ 20 ms
  - Wissensrepräsentation: `KnowledgeBase` speichert Fakten als `(subject, predicate, object)` Tripel
    (kompatibel mit `KnowledgeGraphReasoner`); Regeln als Horn-Klauseln in YAML-Format
  - Detail: `src/analytics/FUTURE_ENHANCEMENTS.md` → Expert System Engine

- [ ] **AI/ML + LoRA Mustererkennung und Auswertung** (Target: Q3 2027)
  - Affected: `include/analytics/lora_pattern_classifier.h` (new), `src/analytics/lora_pattern_classifier.cpp` (new),
    Integration mit `src/llm/multi_lora_manager.cpp`, `src/analytics/cep_engine.cpp`,
    `src/analytics/model_serving.cpp`
  - Scope: LoRA-fine-tuned LLM-Adapter übernehmen domänenspezifische Mustererkennung in
    Ereignisströmen und Analyseresultaten. `LoRAPatternClassifier` wrapped `MultiLoRAManager`
    und liefert klassenspezifische Labels + Konfidenzwerte für:
    (1) CEP-Ereignismuster (z. B. Betrugssequenzen, Compliance-Verstöße, Anomalie-Cluster)
    (2) Zeitreihen-Muster (Trend, Saisonalität, Ausreißer) — Ergänzung zu `forecasting.cpp`
    (3) Graph-Pfad-Muster (Hub-Spoke, Authority-Chain) via `KnowledgeGraphReasoner`
  - Expected behavior:
    - `LoRAPatternClassifier::classify(events, adapter_id)` → `PatternResult` mit Label + Konfidenz
    - `LoRAPatternClassifier::selectAdapter(context)` → wählt Adapter via Embedding-Ähnlichkeit
    - `LoRAPatternClassifier::batchClassify(event_batch)` → parallele Klassifikation via Thread-Pool
    - Integration in CEP: `CEPEngine::setLoRAPatternClassifier(classifier)` → Adapter-Klassifikation
      wird als zusätzlicher Filtertyp in EPL-Regeln nutzbar: `PATTERN CLASSIFIED_AS "fraud_sequence"`
    - Integration in `ExpertSystemEngine::setMLScorer()` für ML-augmentierte Regelprämissen
  - Constraints: LoRA-Adapter-Inference ≤ 100 ms pro Batch ≤ 64 Events; Guard `THEMIS_ENABLE_LLM`;
    Fallback: AutoML-Klassifikator (`automl.cpp`) wenn LoRA nicht verfügbar
  - Errors: kein passender Adapter → AutoML-Fallback; Adapter-Load-Fehler → Fehlerprotokoll + Fallback
  - Tests: LPC-01..LPC-15 in `tests/analytics/test_lora_pattern_classifier.cpp`
    - LPC-01..LPC-05: Einzelereignis-Klassifikation mit Mock-Adapter
    - LPC-06..LPC-08: Batch-Klassifikation + Thread-Pool
    - LPC-09..LPC-11: Adapter-Selektion via Embedding-Ähnlichkeit
    - LPC-12..LPC-13: CEP-Integration (`PATTERN CLASSIFIED_AS`)
    - LPC-14..LPC-15: AutoML-Fallback-Verhalten
  - Perf: Batch-Klassifikation 64 Events ≤ 100 ms; Adapter-Selektion ≤ 5 ms
  - Mustererkennung-Ziele (messbar):
    - Betrugssequenzen im CEP-Strom: Precision ≥ 0.90, Recall ≥ 0.85
    - Zeitreihen-Anomalie-Klassifikation: F1 ≥ 0.88 auf Benchmark-Datensatz
    - Graph-Pfad-Muster: Adapter-Konfidenz für bekannte Strukturmuster ≥ 0.80
  - Detail: `src/analytics/FUTURE_ENHANCEMENTS.md` → AI/ML + LoRA Pattern Classification

### Phase 1: Core Analytics Engine (Status: Completed ✅)
- [x] OLAP engine with GROUP BY, CUBE, ROLLUP, and GROUPING SETS (`analytics/olap_engine.cpp`)
- [x] Window functions: ROW_NUMBER, SUM OVER, AVG OVER with frame specifications
- [x] Statistical aggregations (COUNT, SUM, AVG, MIN, MAX, STDDEV, VARIANCE, MEDIAN, PERCENTILE)
- [x] Hash-based aggregation with result caching
- [x] Columnar Arrow RecordBatch storage always available
- [x] JSON, CSV, Parquet, and Feather export (`analytics/exporters/`)
- [x] Process mining: Alpha Miner, Heuristic Miner, Inductive Miner (`analytics/process_mining/`)
- [x] Conformance checking (token replay and alignment-based)
- [x] NLP text analyzer: tokenization, TF-IDF, NER, sentiment, keyword extraction (`analytics/nlp_analyzer.cpp`)
- [x] LLM process analyzer with OpenAI, Anthropic, Azure OpenAI, llama.cpp providers
- [x] Diff engine (changefeed-backed git-like diffs, `analytics/diff_engine.cpp`)
- [x] SIMD-accelerated aggregations (AVX2) in `analytics/simd_aggregations.cpp`
- [x] Thread-safe OLAPEngine for concurrent queries

### Phase 2: Streaming & Incremental Analytics (Status: Completed ✅)
- [x] CEP full engine implementation in `analytics/cep_engine.cpp`
- [x] Streaming aggregation windows (tumbling/sliding/session/hopping) in `analytics/streaming_window.cpp`
- [x] Incremental materialized views in `analytics/incremental_view.cpp`

### Phase 3: Distributed & ML-Augmented Analytics (Status: Completed ✅)
- [x] Columnar execution engine with vectorized operator pipeline (`analytics/columnar_execution.cpp`)
- [x] LLVM-JIT compilation for hot aggregation paths (`analytics/jit_aggregation.cpp`): hot-path detection and template-specialised aggregation dispatch; LLVM MCJIT backend reserved behind `THEMIS_HAS_LLVM_JIT` compile flag (Issue: #1482)
- [x] Distributed analytics sharding across cluster nodes (Issue: #1483)
- [x] Predictive analytics and time-series forecasting integration (Issue: #1484)
- [x] AutoML integration for automated model selection
- [x] Model serving and online inference pipeline (`analytics/model_serving.cpp`) (Issue: #1477)

### Phase 4: Expert System Engine (Status: Planned [ ], Target: Q2 2027)
- [ ] `ExpertSystemEngine` — CEP-basiertes Vorwärts-/Rückwärts-Verkettungs-Framework
  (Target: Q2 2027) → `include/analytics/expert_system_engine.h`, `src/analytics/expert_system_engine.cpp`
- [ ] `KnowledgeBase` — persistente Fakten-Wissensbasis (subject, predicate, object Tripel);
  RocksDB-backed (Target: Q2 2027) → `include/analytics/knowledge_base.h`, `src/analytics/knowledge_base.cpp`
- [ ] YAML-Regelformat (Horn-Klauseln), `KnowledgeBase::loadRulesFromYaml()` (Target: Q2 2027)
- [ ] ML-Scorer-Anbindung: `ExpertSystemEngine::setMLScorer(ModelServingEngine*)` (Target: Q3 2027)
- [ ] Erklärungskomponente: `explain(decision_id)` → JSON Proof-Trace (Target: Q3 2027)
- [ ] Integration mit `KnowledgeGraphReasoner` (Graph-Modul) für Triple-Austausch (Target: Q3 2027)
- [ ] Tests: ES-01..ES-20 (`tests/analytics/test_expert_system_engine.cpp`) + KB-01..KB-08 (Target: Q3 2027)

### Phase 5: AI/ML + LoRA Mustererkennung (Status: Planned [ ], Target: Q3 2027)
- [ ] `LoRAPatternClassifier` — LoRA-Adapter-basierte Mustererkennung in CEP-Ereignisströmen,
  Zeitreihen und Graphpfaden (Target: Q3 2027)
  → `include/analytics/lora_pattern_classifier.h`, `src/analytics/lora_pattern_classifier.cpp`
- [ ] EPL-Erweiterung: `PATTERN CLASSIFIED_AS "..."` + `EXPERT_SYSTEM_CONFIRMS(...)` (Target: Q4 2027)
- [ ] `CEPEngine::setLoRAPatternClassifier()` für Adapter-basierte Ereignisfilterung (Target: Q4 2027)
- [ ] AutoML-Fallback wenn `THEMIS_ENABLE_LLM=OFF` (Target: Q3 2027)
- [ ] Tests: LPC-01..LPC-15 (`tests/analytics/test_lora_pattern_classifier.cpp`) (Target: Q4 2027)


- [x] Unit tests (OLAP, Arrow export, process mining, NLP, diff engine, forecasting)
- [x] Unit tests coverage > 80% (test files added for all Phase 2 components; all three Phase 2 test suites active in CI)
- [x] Integration tests (query module, index module, CDC)
- [x] CEP engine integration tests (`tests/analytics/test_cep_engine.cpp`) — including stateful checkpoint lifecycle (`StatefulCheckpointPreservesPartialMatches`, `CheckpointWithNoPartialMatchesIsClean`)
- [x] Forecasting unit tests (`tests/analytics/test_forecasting.cpp`) — TimeSeries, all five algorithms, fit/predict/evaluate/decompose, serialize/deserialize, edge cases
- [x] Anomaly detection unit tests (`tests/analytics/test_anomaly_detection.cpp`) — all 6 algorithms, streaming, serialize round-trip
- [x] AutoML unit tests (`tests/analytics/test_automl.cpp`) — classification, regression, feature engineering, ensemble, SHAP, serialize
- [x] Distributed analytics unit tests (`tests/analytics/test_distributed_analytics.cpp`) — shard management, scatter-gather, partial failure
- [x] Process pattern matcher unit tests (`tests/analytics/test_process_pattern_matcher.cpp`) — graph/vector/behavioral/hybrid similarity, conformance
- [x] Arrow export + analytics_export unit tests (`tests/analytics/test_arrow_export.cpp`) — RecordBatch, JSON/CSV, optional Parquet/Feather/IPC, sanitization
- [x] Process mining LLM integration tests (`tests/analytics/test_process_mining_llm.cpp`) — conformance, compliance rules, fraud detection, activity prediction
- [x] Standalone focused test targets registered in `tests/CMakeLists.txt` for all 14 analytics test files
- [x] All analytics sources registered in `cmake/CMakeLists.txt` and `cmake/ModularBuild.cmake`
- [x] Arrow Flight RPC (`analytics/arrow_flight.cpp`) — in-process + optional native gRPC transport (Issue: #1472)
- [x] Performance benchmarks (OLAP, export, process mining, graph, NLP)
- [x] Security audit (LLM API key handling, data export sanitization)
- [x] Documentation complete (API docs, OLAP guide, process mining guide)
- [x] API stability guaranteed for OLAP, export, and process mining
- [x] Windows platform compatibility: whole-class `_WIN32` stub removed from `olap.cpp`; SIMD guarded per-instruction; Windows CI workflow at `.github/workflows/02-feature-modules_analytics_windows-olap-ci.yml` (Issue: #238)

## Known Issues & Limitations
- NLP text analyzer uses rule-based approaches — not suitable as a replacement for full NLP frameworks
- LLM analyzer requires external API keys; responses are non-deterministic
- Arrow-dependent formats (Parquet, Feather, IPC) require compile-time flag `THEMIS_HAS_ARROW`; when Arrow is absent, `exportToParquet()` / `exportCollectionToParquet()` return `false` with a `spdlog::warn` message
- Graph analytics advanced algorithms (betweenness centrality, Louvain community detection) are now implemented as AQL functions in `include/query/functions/graph_extensions.h`
- Windows: `ProcessMining` is gated behind the opt-in flag `THEMIS_PROCESS_MINING_WINDOWS_STUB`; the flag is off by default so the full implementation is used

## Breaking Changes
- Arrow export format options may expand in v1.7.0 (additive, non-breaking)

## See Also

- **Implementation README**: [`README.md`](./README.md)
- **Architecture**: [`ARCHITECTURE.md`](./ARCHITECTURE.md)
- **Future Enhancements**: [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md)
- **API Documentation**: [`../../include/analytics/README.md`](../../include/analytics/README.md)
- **Secondary Docs (de)**: [`../../docs/de/analytics/README.md`](../../docs/de/analytics/README.md)
