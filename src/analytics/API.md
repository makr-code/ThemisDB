---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Analytics API Map

## Audience and source of truth

This page maps public interfaces for developers integrating analytics
capabilities. Header declarations and Doxygen are normative; this summary is
not a replacement for signatures or complete error details.

| Capability | Public contract |
|---|---|
| Aggregation, OLAP, shared errors and limits | [`aggregation.h`](../../include/analytics/aggregation.h), [`olap.h`](../../include/analytics/olap.h), [`analytics_api_contract.h`](../../include/analytics/analytics_api_contract.h) |
| Streaming windows and joins | [`streaming_window.h`](../../include/analytics/streaming_window.h), [`streaming_join.h`](../../include/analytics/streaming_join.h) |
| CEP | [`cep_engine.h`](../../include/analytics/cep_engine.h) |
| Forecasting and anomaly detection | [`forecasting.h`](../../include/analytics/forecasting.h), [`anomaly_detection.h`](../../include/analytics/anomaly_detection.h) |
| In-process and external model serving | [`model_serving.h`](../../include/analytics/model_serving.h), [`ml_serving.h`](../../include/analytics/ml_serving.h) |
| Process mining and pattern matching | [`process_mining.h`](../../include/analytics/process_mining.h), [`process_pattern_matcher.h`](../../include/analytics/process_pattern_matcher.h) |
| Export and distributed coordination | [`analytics_export.h`](../../include/analytics/analytics_export.h), [`arrow_flight.h`](../../include/analytics/arrow_flight.h), [`distributed_analytics.h`](../../include/analytics/distributed_analytics.h) |
| NLP utilities | [`nlp_text_analyzer.h`](../../include/analytics/nlp_text_analyzer.h) |

## Integration rules

1. Construct requests from the component's declared types. For ML serving,
   tensor names, shapes, and row-major `float32` values must match the model;
   `inferFromDataPoint` extracts numeric features in deterministic field-name
   order.
2. Check the returned status before consuming output. Model serving may throw
   for absent named/versioned registry entries; external serving reports
   `UNAVAILABLE`, `INVALID_INPUT`, `BACKEND_ERROR`, `TIMEOUT`, or
   `POLICY_REJECTED`.
3. Set `BoundedExecutionPolicy` limits explicitly when the operation must be
   bounded. Zero means no timeout, no concurrency cap, or no queue cap,
   respectively.
4. Handle streaming late-event and backpressure outcomes at the producer.
   `WINDOW_EXPIRED` means the event was discarded; it is not a successful
   partial update.
5. For distributed queries, inspect `DistributedResult` fields such as
   `successful_shards`, `total_shards`, `shard_info`, `failure_class`, and
   `operator_hints`; a configured partial result must not be treated as a full
   result.
6. Use explicit export status and `supportsFormat()` checks. Optional Arrow
   capability absence is a not-supported condition, not a reason to reinterpret
   the output as another format.

## Namespace and compatibility notes

Analytics headers use both `themis::analytics` and `themisdb::analytics`
namespaces, and some integration headers bridge between them. Include the
component's public header and use its declared namespace/type rather than
assuming every analytics type has one namespace. The shared runtime contract
states stability within the active v1.x line; contract-breaking changes require
the documented major-version and migration process.

## Example: bounded external inference

The example deliberately uses the namespaces declared by the owning headers:
`MLServingRequest` is in `themisdb::analytics`, while
`BoundedExecutionPolicy` is in `themis::analytics`. Analytics headers do not
all share one namespace.

```cpp
themisdb::analytics::MLServingRequest request;
request.model_name = "risk-model";
request.inputs.push_back(input_tensor);  // Populate name, shape, and row-major data.

themis::analytics::BoundedExecutionPolicy policy;
policy.max_latency_ms = 500;
policy.max_concurrent_requests = 8;
policy.queue_depth = 0;

const auto response = client.infer(request, policy);
if (!response.ok()) {
    // Handle status/failure_class; do not consume outputs as a successful result.
}
```

The example's limits are illustrative, not production recommendations. See
[`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md) for deployment
requirements and [TRUST.md](./TRUST.md) for boundary responsibilities.
