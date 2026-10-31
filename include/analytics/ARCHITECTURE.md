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

# Analytics public-header architecture

`include/analytics/` defines the contracts consumed by analytics callers.
Runtime implementation is under [`../../src/analytics/`](../../src/analytics/);
this document describes the header boundary, not a separate execution engine.

## Contract areas

| Area | Public headers |
|---|---|
| Aggregation, shared errors, and execution limits | `aggregation.h`, `analytics_api_contract.h` |
| OLAP, columnar execution, and incremental results | `olap.h`, `columnar_execution.h`, `jit_aggregation.h`, `incremental_view.h`, `result_aggregator.h`, `diff_engine.h`, `analytics_engine.h`, `time_series.h` |
| Event processing | `cep_engine.h`, `streaming_window.h`, `streaming_join.h` |
| Forecasting, anomaly detection, and model integration | `forecasting.h`, `anomaly_detection.h`, `automl.h`, `model_serving.h`, `ml_serving.h` |
| Process, text, and knowledge analysis | `process_mining.h`, `process_pattern_matcher.h`, `llm_process_analyzer.h`, `lora_pattern_classifier.h`, `nlp_text_analyzer.h`, `knowledge_base.h`, `expert_system_engine.h` |
| Export and shard coordination | `analytics_export.h`, `arrow_export.h`, `arrow_flight.h`, `distributed_analytics.h` |
| Resource helpers | `connection_guard.h`, `thread_guard.h` |

These are groupings by header responsibility, not promises that every backend
or optional dependency is available in every build.

## Namespace and ownership

Headers use more than one namespace family (`themis::analytics`,
`themisdb::analytics`, and other declarations). Include the owning header and
use its declared namespace; do not infer a namespace from the directory name.
The `detail/` headers are implementation support, not public entry points.

## Failure and capability contracts

- Use the shared definitions in `analytics_api_contract.h` where the component
  contract refers to canonical analytics errors or bounded execution.
- Check component result/status values and capability methods before consuming
  outputs or selecting an export format.
- Optional integrations can report unavailable or unsupported results. Their
  presence in this header directory does not establish that the dependency was
  enabled at build time.
- Distributed results can represent partial shard completion; callers that need
  complete results must inspect the shard outcomes.

For exact declarations and current integration behavior, see the headers and
[`../../src/analytics/API.md`](../../src/analytics/API.md).
