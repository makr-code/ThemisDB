<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · AUDIT.md · SECURITY.md -->

# Analytics Module — Public Header Architecture

**Version:** 1.7.0
**Last Updated:** 2026-04-06
**Module Path:** `include/analytics/`
**Implementation:** `../../src/analytics/`

---

## 1. Overview

The `include/analytics/` directory exposes public C++ headers for ThemisDB's analytics engine,
covering Complex Event Processing (CEP), OLAP columnar execution, ML model serving, time-series
forecasting, anomaly detection, distributed analytics, and NLP/LLM-based process analysis.

---

## 2. Design Principles

- **Stream-First** – CEP and streaming window headers (`cep_engine.h`, `streaming_window.h`)
  are the primary real-time analytics interfaces; batch OLAP builds on the same types.
- **OLAP Columnar Execution** – `columnar_execution.h` and `jit_aggregation.h` expose
  column-oriented evaluation interfaces for high-throughput aggregation.
- **ML/AI Integration** – `ml_serving.h`, `model_serving.h`, and `automl.h` decouple model
  lifecycle from query execution.
- **Arrow Interoperability** – `arrow_export.h` and `arrow_flight.h` provide zero-copy data
  exchange with Apache Arrow Flight endpoints.
- **Extensibility via Diff/Incremental** – `diff_engine.h` and `incremental_view.h` support
  incremental materialised view maintenance.

---

## 3. Interface Inventory

| Header | Classes / Interfaces | Purpose |
|--------|----------------------|---------|
| `cep_engine.h` | `ICEPEngine`, `CEPRule`, `CEPEvent` | Complex event processing: rule registration and stream subscription |
| `streaming_window.h` | `IStreamingWindow`, `WindowSpec` | Tumbling, sliding, session window abstractions |
| `olap.h` | `IOLAPEngine`, `OLAPQuery`, `OLAPResult` | OLAP query execution interface |
| `columnar_execution.h` | `IColumnarExecutor`, `ColumnBatch` | Vectorised columnar execution |
| `jit_aggregation.h` | `IJITAggregator` | JIT-compiled aggregation pipelines |
| `forecasting.h` | `IForecaster`, `ForecastRequest`, `ForecastResult` | Time-series forecasting interface |
| `anomaly_detection.h` | `IAnomalyDetector`, `AnomalyEvent` | Statistical and ML-based anomaly detection |
| `ml_serving.h` | `IMLServingEngine`, `ModelInferRequest` | Real-time ML model inference |
| `model_serving.h` | `IModelServingBackend`, `ModelSpec` | Model lifecycle and backend abstraction |
| `automl.h` | `IAutoMLEngine`, `AutoMLConfig` | Automated model selection and training |
| `distributed_analytics.h` | `IDistributedAnalytics`, `AnalyticsShard` | Distributed analytics across shards |
| `arrow_export.h` | `IArrowExporter`, `ArrowSchema` | Arrow IPC export from analytics results |
| `arrow_flight.h` | `IArrowFlightServer`, `FlightDescriptor` | Arrow Flight streaming endpoint |
| `diff_engine.h` | `IDiffEngine`, `DiffResult` | Result diff computation for change detection |
| `incremental_view.h` | `IIncrementalView`, `ViewSpec` | Incremental materialised view maintenance |
| `nlp_text_analyzer.h` | `INLPTextAnalyzer`, `TextAnalysisResult` | NLP annotation and entity extraction |
| `llm_process_analyzer.h` | `ILLMProcessAnalyzer` | LLM-backed process mining analysis |
| `process_mining.h` | `IProcessMiner`, `ProcessTrace` | Process mining from event logs |
| `process_pattern_matcher.h` | `IProcessPatternMatcher`, `PatternSpec` | Pattern matching over process traces |
| `analytics_export.h` | Export macros (`ANALYTICS_API`) | DLL export/import symbol visibility |

> **Implementation details:** `../../src/analytics/`
