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

# Analytics runtime architecture

The analytics module is a set of related runtime components, not one mandatory
pipeline. Public declarations live in [`../../include/analytics/`](../../include/analytics/),
implementations are in this directory, and focused tests are in
[`../../tests/analytics/`](../../tests/analytics/).

## Runtime areas

| Area | Main interfaces and implementations |
|---|---|
| Aggregation, OLAP, and columnar execution | `aggregation.*`, `olap.*`, `columnar_execution.*`, `jit_aggregation.*`, `incremental_view.*` |
| Event streams | `streaming_window.*`, `streaming_join.*`, `cep_engine.*` |
| Predictive analysis and model integration | `forecasting.*`, `anomaly_detection.*`, `automl.*`, `model_serving.*`, `ml_serving.*` |
| Process, text, and knowledge analysis | `process_mining.*`, `process_pattern_matcher.*`, `nlp_text_analyzer.*`, `knowledge_base.*`, `expert_system_engine.*`, `llm_process_analyzer.*`, `lora_pattern_classifier.*` |
| Export and distributed coordination | `analytics_export.*`, `arrow_export.*`, `arrow_flight.*`, `distributed_analytics.*` |

These are source-file groupings, not guarantees that every optional backend is
compiled or available at runtime.

## Data flow and module boundaries

- Callers construct component-specific requests using the public header
  declarations. The shared `analytics_api_contract.h` defines cross-cutting
  errors, limits, and selected runtime semantics.
- Each component validates its inputs and performs its own operation; there is
  no requirement that a request pass through every runtime area listed above.
- Export and serving paths return component-specific status and result metadata.
  Optional capabilities must be checked rather than inferred from the presence
  of a header.
- Distributed execution fans requests out to shard executors and aggregates
  their outcomes. A partial result is distinct from a result covering all
  shards.
- Query parsing, storage internals, server authentication, and authorization
  remain owned by their respective modules and integration boundaries.

## Failure and resource behavior

- Use the component's declared result/status and the shared contract where
  applicable. Unsupported optional capabilities and invalid input are explicit
  failure conditions; do not reinterpret them as successful output.
- Streaming callers must handle configured window/resource limits,
  backpressure, and late-event outcomes.
- Serving and export callers must honor configured integrity, transport, and
  bounded-execution policies. A zero policy limit means unconstrained where the
  shared policy defines it that way.
- Distributed callers must inspect shard outcomes before treating an aggregate
  as complete.

For exact declarations and integration notes, see [API.md](./API.md) and
[IMPLEMENTATION.md](./IMPLEMENTATION.md). For focused build/test instructions,
see [MODULE_GUIDE.md](./MODULE_GUIDE.md).
