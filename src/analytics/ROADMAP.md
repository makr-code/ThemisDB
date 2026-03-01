# Analytics Module Roadmap

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
- [x] AutoML integration for automated model selection (Issue: #1485) ✅
- [x] Advanced graph analytics: betweenness centrality, Louvain community detection (Issue: #1475)
- [x] Integration with external ML tools (ONNX Runtime, TensorFlow Serving) (Issue: #1476) ✅
- [x] Multi-language NLP support (beyond English) (Issue: #1478)
- [x] Full morphological lemmatization (Issue: #1479)

## In Progress 🚧
*(none — all Phase 3 items completed)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [P] GPU-accelerated OLAP aggregations (CUDA) (Issue: #1469)
- [P] Zero-copy Arrow data transfer optimizations (Issue: #1471)
- [P] Arrow Flight RPC support for remote analytics (Issue: #1472)

### Long-term (6-12 months)

## Implementation Phases

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

## Production Readiness Checklist
- [x] Unit tests (OLAP, Arrow export, process mining, NLP, diff engine, forecasting)
- [x] Unit tests coverage > 80% (test files added for all Phase 2 components; all three Phase 2 test suites active in CI)
- [x] Integration tests (query module, index module, CDC)
- [x] CEP engine integration tests (`tests/analytics/test_cep_engine.cpp`) — including stateful checkpoint lifecycle (`StatefulCheckpointPreservesPartialMatches`, `CheckpointWithNoPartialMatchesIsClean`)
- [x] Forecasting unit tests (`tests/analytics/test_forecasting.cpp`) — TimeSeries, all five algorithms, fit/predict/evaluate/decompose, serialize/deserialize, edge cases
- [x] Performance benchmarks (OLAP, export, process mining, graph, NLP)
- [x] Security audit (LLM API key handling, data export sanitization)
- [x] Documentation complete (API docs, OLAP guide, process mining guide)
- [x] API stability guaranteed for OLAP, export, and process mining

## Known Issues & Limitations
- NLP text analyzer uses rule-based approaches — not suitable as a replacement for full NLP frameworks
- LLM analyzer requires external API keys; responses are non-deterministic
- Arrow-dependent formats (Parquet, Feather, IPC) require compile-time flag `THEMIS_HAS_ARROW`
- Graph analytics advanced algorithms (betweenness centrality, Louvain community detection) are now implemented as AQL functions in `include/query/functions/graph_extensions.h`

## Breaking Changes
- Arrow export format options may expand in v1.7.0 (additive, non-breaking)
