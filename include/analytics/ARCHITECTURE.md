> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-08-19 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/analytics/ARCHITECTURE.md -->

# Analytics Module — Public Header Architecture

**Module Path:** `include/analytics/`
**Implementation:** `../../src/analytics/`
**Canonical architecture doc:** [`../../src/analytics/ARCHITECTURE.md`](../../src/analytics/ARCHITECTURE.md)

---

## 1. Overview

`include/analytics/` defines the **public OLAP, streaming, ML-serving, and process-mining API contract** for ThemisDB. The 24 headers cover columnar query execution, CEP, streaming joins/windows, forecasting, anomaly detection, AutoML, model serving, NLP text analysis, knowledge-base access, diff computation, incremental views, and LLM-based process analysis.

For runtime composition — execution pipelines, ML-model lifecycle, and stream-join internals — see:
→ [`../../src/analytics/ARCHITECTURE.md`](../../src/analytics/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 OLAP and Columnar Execution

| Header | Public Type | Purpose |
|--------|------------|---------|
| `olap.h` | `OLAPEngine` | Multi-dimensional OLAP query execution |
| `columnar_execution.h` | `ColumnarExecutionEngine` | Vectorised columnar scan and aggregation |
| `jit_aggregation.h` | `JITAggregationEngine` | JIT-compiled aggregation kernels |
| `incremental_view.h` | `IncrementalView` | Incrementally maintained materialised views |

### 2.2 Streaming and CEP

| Header | Public Type | Purpose |
|--------|------------|---------|
| `cep_engine.h` | `CEPEngine` | Complex-event processing pattern matching |
| `streaming_window.h` | `StreamingWindowAggregator` | Tumbling/sliding window aggregations |
| `streaming_join.h` | `StreamingJoin` | Low-latency stream-to-stream join |

### 2.3 Forecasting and Anomaly Detection

| Header | Public Type | Purpose |
|--------|------------|---------|
| `forecasting.h` | `ForecastingEngine` | Time-series forecasting (ARIMA, Prophet variants) |
| `anomaly_detection.h` | `AnomalyDetector` | Statistical and ML-based anomaly detection |
| `diff_engine.h` | `DiffEngine` | Snapshot-to-snapshot difference computation |

### 2.4 ML Serving and AutoML

| Header | Public Type | Purpose |
|--------|------------|---------|
| `automl.h` | `AutoMLPipeline` | Automated feature selection and model search |
| `ml_serving.h` | `MLServingEndpoint` | Embedded ML inference endpoint |
| `model_serving.h` | `ModelServingManager` | Multi-model lifecycle and routing |

### 2.5 NLP and Text Analytics

| Header | Public Type | Purpose |
|--------|------------|---------|
| `nlp_text_analyzer.h` | `NLPTextAnalyzer` | Named-entity recognition and sentiment analysis |
| `knowledge_base.h` | `KnowledgeBase` | Structured knowledge-base access and query |
| `expert_system_engine.h` | `ExpertSystemEngine` | Rule-based inference over domain knowledge |

### 2.6 Process Mining and LLM Analysis

| Header | Public Type | Purpose |
|--------|------------|---------|
| `process_mining.h` | `ProcessMiner` | Event-log process discovery and conformance |
| `process_pattern_matcher.h` | `ProcessPatternMatcher` | Pattern-based process variant detection |
| `llm_process_analyzer.h` | `LLMProcessAnalyzer` | LLM-assisted process narrative generation |
| `lora_pattern_classifier.h` | `LoRAPatternClassifier` | LoRA-tuned classifier for analytics patterns |

### 2.7 Export and Distribution

| Header | Public Type | Purpose |
|--------|------------|---------|
| `analytics_export.h` | — | DLL/visibility macros for analytics exports |
| `arrow_export.h` | `ArrowExporter` | Apache Arrow record-batch export |
| `arrow_flight.h` | `ArrowFlightServer` | Arrow Flight RPC endpoint for bulk analytics transfer |
| `distributed_analytics.h` | `DistributedAnalyticsCoordinator` | Multi-shard analytics coordination |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::analytics` | Core OLAP, streaming, forecasting, and ML types |
| `themis::analytics::nlp` | NLP and knowledge-base types |
| `themis::analytics::process` | Process-mining and pattern types |

---

## 4. Public Contract Notes

- Columnar and JIT headers define the vectorised execution contract; callers must not assume arena layout or SIMD specifics.
- Streaming headers model bounded-latency semantics; window and join state management is internal.
- ML-serving headers expose model-lifecycle and inference APIs; model binaries are managed via `ModelServingManager`.
- `model_serving.h` includes integrity-aware load paths (SHA-256 expected hash) and optional fail-closed enforcement controls.
- `ml_serving.h` exposes secure-by-default TF transport policy with explicit plaintext opt-in.
- Arrow export headers provide stable zero-copy transfer contracts for downstream analytics consumers.
- LLM-integrated headers (`llm_process_analyzer.h`, `lora_pattern_classifier.h`) depend on `include/llm/` for model context.
