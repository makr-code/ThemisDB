> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-05-10 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Analytics Module

All notable changes to the Analytics module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
### Fixed
- **Analytics reliability hardening (rest block)** (`src/analytics/**/*.cpp`)
  - Replaced all remaining `catch (...)` handlers in Analytics implementation files with typed `catch (const std::exception&)`.
  - Targeted delta in this block: **55 → 0** across 11 files (`cep_engine`: 20→0, `streaming_window`: 11→0, `anomaly_detection`: 5→0, plus 8 additional Analytics files).

- Federated analytics query dispatch across multiple ThemisDB clusters (Target: Q3 2026)
- SARIMA and Prophet-style forecasting models (Target: Q4 2026)
- AutoML ONNX export and deployment pipeline (Target: Q4 2026)
- GPU-accelerated OLAP aggregations (CUDA) — PR open (Issue #1469)
- Zero-copy Arrow data transfer optimizations — PR open (Issue #1471)

## [2.0.0] — 2026-04-12
### Added
- **Multi-stream join engine** — `IStreamingJoin` / `HashJoin` / `IntervalJoin` (`analytics/streaming_join.cpp`):
  - `HashJoin`: composite-key hash table, inner/left-outer join, multi-batch build, configurable `max_build_rows`
  - `IntervalJoin`: `before_ms`/`after_ms`/`slack_ms` time-window join with LRU probe-side pruning; late-arrival handling via watermark
  - Public API: `build(batch)` / `probe(batch)` / `reset()` on `IStreamingJoin`; factory function `makeHashJoin()` / `makeIntervalJoin()`
  - 15 focused tests (SJ-01…SJ-15) in `tests/analytics/test_streaming_join.cpp`
  - Error paths: `std::invalid_argument` on empty key column or mismatched schema; late-arriving rows outside the join window are silently dropped

## [1.9.0] — 2026-03-28
### Added
- **Forecasting: Batch prediction, streaming update, parallel auto-tune, fit-result cache** (Issue #4054):
  - `ForecastModel::predictBatch(const std::vector<TimeSeries>& batch, int steps)` — amortises model-state copies across N independent series; returns `std::vector<std::vector<ForecastPoint>>`; throws `std::invalid_argument` for `steps ≤ 0` or empty batch (returns empty vector).
  - `ForecastModel::update(double new_value)` — O(1) incremental state absorption of one new observation without full `fit()` rerun; updates ETS level/trend/seasonal components, ARIMA MA error term, and LR running sums.
  - Parallel auto-tune grid search for Holt–Winters (`auto_tune=true`): 9 α tasks dispatched via `std::async`; wall-time ≤ 5 ms on a 500-sample series (down from ≤ 50 ms single-threaded).
  - FNV-1a 64-bit fit-result cache: repeated `fit()` calls on unchanged data with the same config are O(1) hash lookups; cache keyed on `(xxHash(training_data), config_hash)`.
  - 14 new tests in `ForecastingBatchStreamingTests` (`tests/analytics/test_forecasting.cpp`): shape/consistency, single-series parity, invalid-steps, empty batch, all methods, update no-op on unfitted, update ETS state absorption, update LR state, update ARIMA state, parallel auto-tune correctness, cache hit on repeat fit, cache miss on data change, cache miss on config change, predictBatch thread safety.
  - CI workflow: `forecasting-batch-streaming-ci.yml`.

## [1.7.0] — 2026-03-09
### Added
- AutoML integration: automated model selection for Logistic Regression, Decision Tree, Random Forest, Gradient Boosting, KNN, Linear Regression with hyperparameter search, feature engineering, ensemble generation, and SHAP-based explanations (`analytics/automl.cpp`)
- Integration with external ML tools: ONNX Runtime (local inference) and TensorFlow Serving (REST API) via unified `MLServingClient` (`analytics/ml_serving.cpp`)
- Model serving and online inference pipeline: named+versioned model registry, online/batch inference, class-probability output, per-model health metrics (`analytics/model_serving.cpp`)
- Predictive analytics and time-series forecasting: LINEAR_REGRESSION, EXP_SMOOTHING, Holt-Winters triple exponential, ARIMA (Yule-Walker), ENSEMBLE with weighted combination; confidence intervals, seasonal decomposition, accuracy metrics (MAE, RMSE, MAPE, sMAPE) (`analytics/forecasting.cpp`)
- Multi-language NLP support and full morphological lemmatization (Issue #1478, #1479)
- Advanced graph analytics: betweenness centrality, Louvain community detection (Issue #1475)
- Arrow Flight RPC support for remote analytics: in-process + optional native gRPC transport (`analytics/arrow_flight.cpp`)

## [1.6.0] — 2026-02-15
### Added
- CEP stateful pattern matching with checkpointing: `PatternMatcher::serializeState()`/`restoreState()`, `RuleEngine::serializeMatcherStates()`/`restoreMatcherStates()` — full NFA partial-match persistence across restarts
- CEP engine backpressure: engine queue depth limit, drop policy, backpressure signal at configurable threshold, Prometheus metrics
- Streaming aggregation windows: `TumblingWindow`, `SlidingWindow`, `SessionWindow`, `HoppingWindow` with watermark support (`analytics/streaming_window.cpp`)
- Incremental materialized views with delta-maintenance for all 10 aggregation functions; Welford STDDEV/VARIANCE; COUNT_DISTINCT ref-counting (`analytics/incremental_view.cpp`)
- Real-time anomaly detection: Z-Score, Modified Z-Score (MAD), IQR, Isolation Forest, LOF, Ensemble with adaptive learning (`analytics/anomaly_detection.cpp`)

## [1.5.0] — 2026-01-10
### Added
- CEP full engine: NFA pattern matching, EPL parser (`CREATE RULE … AS`), SELECT aggregations with GROUP BY, WINDOW specs, PATTERN WITHIN, ACTION dispatch (alert/webhook/db_write/log/slack/kafka/email) (`analytics/cep_engine.cpp`)
- SIMD-accelerated aggregations (AVX2) for GROUP BY queries
- Process mining: Alpha Miner, Heuristic Miner, Inductive Miner; conformance checking (token replay and alignment-based)
- NLP text analyzer: tokenization, TF-IDF, NER, sentiment, keyword extraction (`analytics/nlp_text_analyzer.cpp`)
- LLM process analyzer: OpenAI, Anthropic, Azure OpenAI, llama.cpp integration (`analytics/llm_process_analyzer.cpp`)
- Diff engine: changefeed-backed git-like document diffs (`analytics/diff_engine.cpp`)
- Distributed analytics: `DistributedOLAPEngine` for scatter-gather across shards (`analytics/distributed_analytics.cpp`)

## [1.0.0] — 2024-01-01
### Added
- OLAP engine with GROUP BY, CUBE, ROLLUP, and GROUPING SETS
- Window functions: ROW_NUMBER, SUM OVER, AVG OVER with frame specs
- Statistical aggregations: COUNT, SUM, AVG, MIN, MAX, STDDEV, VARIANCE, MEDIAN, PERCENTILE
- Hash-based aggregation with result caching
- Columnar (Arrow) RecordBatch storage
- JSON and CSV export
- Optional Apache Arrow IPC, Parquet (with compression), and Feather export
- Thread-safe `OLAPEngine` for concurrent queries
