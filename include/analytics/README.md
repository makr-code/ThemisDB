---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Analytics public headers

This directory declares analytics interfaces. Implementations and module
behavior are documented under [`../../src/analytics/`](../../src/analytics/).
The declaration in each header is authoritative; analytics types do not all
share one namespace.

## Header map

| Area | Headers |
|---|---|
| Shared contracts and aggregation | [`aggregation.h`](aggregation.h), [`analytics_api_contract.h`](analytics_api_contract.h) |
| OLAP and query execution | [`analytics_engine.h`](analytics_engine.h), [`columnar_execution.h`](columnar_execution.h), [`diff_engine.h`](diff_engine.h), [`incremental_view.h`](incremental_view.h), [`jit_aggregation.h`](jit_aggregation.h), [`olap.h`](olap.h), [`result_aggregator.h`](result_aggregator.h), [`time_series.h`](time_series.h) |
| Streaming and event processing | [`cep_engine.h`](cep_engine.h), [`streaming_join.h`](streaming_join.h), [`streaming_window.h`](streaming_window.h) |
| Predictive, model, and process analysis | [`anomaly_detection.h`](anomaly_detection.h), [`automl.h`](automl.h), [`expert_system_engine.h`](expert_system_engine.h), [`forecasting.h`](forecasting.h), [`knowledge_base.h`](knowledge_base.h), [`llm_process_analyzer.h`](llm_process_analyzer.h), [`lora_pattern_classifier.h`](lora_pattern_classifier.h), [`ml_serving.h`](ml_serving.h), [`model_serving.h`](model_serving.h), [`nlp_text_analyzer.h`](nlp_text_analyzer.h), [`process_mining.h`](process_mining.h), [`process_pattern_matcher.h`](process_pattern_matcher.h) |
| Export and distributed coordination | [`analytics_export.h`](analytics_export.h), [`arrow_export.h`](arrow_export.h), [`arrow_flight.h`](arrow_flight.h), [`distributed_analytics.h`](distributed_analytics.h) |
| Resource and thread helpers | [`connection_guard.h`](connection_guard.h), [`thread_guard.h`](thread_guard.h) |

`detail/` contains implementation-support headers and is not part of this
top-level header map.

## Developer references

- [API map and integration notes](../../src/analytics/API.md)
- [Runtime architecture](../../src/analytics/ARCHITECTURE.md)
- [Implementation contracts and focused validation map](../../src/analytics/IMPLEMENTATION.md)
- [Analytics test build and registration](../../tests/analytics/README.md)

## Installation

These headers are part of the ThemisDB source/build tree; this module does not
provide a separate package installation procedure.

## Usage

Include the component header you need, use the namespace declared there, and
check the component's status/capability contract before consuming results.
See the [API map](../../src/analytics/API.md) for examples and integration rules.

Build options and optional-backend availability are defined by the repository's
CMake configuration. Do not infer a feature flag or runtime capability from a
header filename alone; inspect the owning header and implementation.
