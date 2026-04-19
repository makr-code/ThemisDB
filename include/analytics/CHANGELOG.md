<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Analytics Module Public Headers

All notable changes to public headers in `include/analytics/`.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.7.0] — 2026-03-09
### Added
- `llm_process_analyzer.h`: `ILLMProcessAnalyzer` for LLM-backed process mining
- `process_pattern_matcher.h`: `IProcessPatternMatcher` for pattern matching over event traces
- `automl.h`: `IAutoMLEngine` and `AutoMLConfig` for automated model selection
- `arrow_flight.h`: `IArrowFlightServer` for Apache Arrow Flight streaming endpoint

### Changed
- `anomaly_detection.h`: `AnomalyEvent` extended with `severity` and `confidence` fields
- `forecasting.h`: `ForecastResult` now includes `confidence_interval` bounds

## [1.6.0] — 2026-02-15
### Added
- `distributed_analytics.h`: `IDistributedAnalytics` and `AnalyticsShard` for cross-shard analytics
- `diff_engine.h`: `IDiffEngine` for result diff and change-detection
- `incremental_view.h`: `IIncrementalView` for incremental materialised view maintenance
- `jit_aggregation.h`: `IJITAggregator` for JIT-compiled aggregation pipelines
- `process_mining.h`: `IProcessMiner` and `ProcessTrace` for event-log process mining

## [1.5.0] — 2026-01-10
### Added
- Initial public header set: `cep_engine.h`, `streaming_window.h`, `olap.h`,
  `columnar_execution.h`, `forecasting.h`, `anomaly_detection.h`
- `ml_serving.h`, `model_serving.h`: ML model inference and lifecycle headers
- `arrow_export.h`: Arrow IPC export interface
- `nlp_text_analyzer.h`: NLP annotation and entity extraction
- `analytics_export.h`: DLL visibility macro
