# ThemisDB Analytics Module

<!-- Status: current | validated: 2026-07-19 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The analytics module provides analytical runtime capabilities for OLAP execution, streaming analytics, CEP, forecasting, anomaly detection, process mining, ML serving, and analytics export paths.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| olap.cpp | OLAP query execution and aggregation paths |
| cep_engine.cpp | event pattern processing and rule execution |
| streaming_window.cpp | tumbling, sliding, session, and hopping windows |
| streaming_join.cpp | stream join execution paths |
| forecasting.cpp | time-series fit/predict/evaluate flows |
| anomaly_detection.cpp | statistical and model-based anomaly scoring |
| model_serving.cpp | in-process model registry and inference |
| ml_serving.cpp | external model serving integration |
| analytics_export.cpp | analytics export orchestration |
| arrow_export.cpp | columnar export conversion paths |
| distributed_analytics.cpp | shard fan-out and aggregation coordination |

## Scope

In scope:
- OLAP and analytics runtime execution surfaces
- streaming and CEP processing paths
- forecasting, anomaly detection, and model serving integration
- analytics export and distributed analytics coordination

Out of scope:
- raw query language parsing
- model lifecycle ownership outside analytics integration boundaries
- storage engine internals beyond consumed interfaces

## Runtime Behavior and Limits

- runtime behavior depends on feature flags and available optional dependencies.
- analytics execution paths include both local and distributed orchestration surfaces.
- export and serving integrations can return structured not-supported or dependency errors when optional backends are unavailable.

**Production Readiness Status (Batch 3 verified 2026-08-14):**
- **Ready for production:** OLAP execution, tumbling/sliding/hopping/session windows, CEP rule matching, basic forecasting, anomaly detection
- **Production-ready with limits:** Streaming join (backpressure), model serving (circuit breaker), distributed analytics (fault tolerance)
- **Not yet production-ready:** Cross-cluster federated analytics (Wave B target Q4 2026), high-volume export with SLA guarantees (Wave B target Q4 2026)

---

## Sourcecode Verification (Module: analytics/readme)

- Verified files:
  - src/analytics/olap.cpp
  - src/analytics/cep_engine.cpp
  - src/analytics/streaming_window.cpp
  - src/analytics/streaming_join.cpp
  - src/analytics/forecasting.cpp
  - src/analytics/anomaly_detection.cpp
  - src/analytics/model_serving.cpp
  - src/analytics/ml_serving.cpp
  - src/analytics/analytics_export.cpp
  - src/analytics/arrow_export.cpp
  - src/analytics/distributed_analytics.cpp
- Verified behavior surfaces:
  - analytical execution and streaming paths
  - forecasting/anomaly/model-serving integration paths
  - export and distributed orchestration surfaces
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical completion remains in CHANGELOG.md