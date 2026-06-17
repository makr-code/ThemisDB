# Cross-Layer Observability Specification (2026-06-17)

## Purpose
This specification defines mandatory observability behavior for the layered retrieval
path ANN -> Tensor -> Graph -> Final Layer.

It operationalizes:
- CCG-431 (per-layer correlation IDs)
- CCG-432 (routing-reason telemetry)
- CCG-433 (handoff SLOs and dashboards)

## Scope
Applies to request processing in:
- ANN frontdoor routing
- Tensor mid-layer routing and retrieval trigger logic
- Graph truth validation
- Final-layer package/model/adapter resolution

## Normative Requirements

### 1. Correlation and identity fields
Every request chain MUST carry these fields end-to-end:
- `correlation_id` (stable across all layers)
- `request_id` (ingress-level request id)
- `session_id` (if available)
- `trace_parent` (if external tracing is enabled)

Layer-specific fields MUST include:
- `layer_name` in {`ann`, `tensor`, `graph`, `final_layer`}
- `layer_step_index` (monotonic within request)
- `layer_decision_id` (unique per layer decision)

### 2. Routing-reason telemetry fields
Each layer MUST emit structured decision reasons:
- `routing_reason_code`
- `routing_reason_text`
- `routing_confidence`
- `confidence_policy_version`
- `confidence_threshold_key`
- `fallback_mode`
- `fallback_reason_code`
- `escalation_source_layer` (if escalation occurred)

### 3. Final-layer governance fields
Final-layer telemetry MUST include:
- `package_id`
- `target_model_id`
- `primary_adapter_id`
- `compatibility_result` in {`accepted`, `rejected`}
- `compatibility_reason_code`

## Structured Log Schema
Minimum JSON shape for each layer decision log:

```json
{
  "event": "layer_handoff_decision",
  "timestamp": "2026-06-17T12:00:00Z",
  "correlation_id": "...",
  "request_id": "...",
  "session_id": "...",
  "layer_name": "tensor",
  "layer_step_index": 2,
  "layer_decision_id": "...",
  "routing_reason_code": "TENSOR_CONFIDENCE_LOW",
  "routing_reason_text": "confidence below threshold",
  "routing_confidence": 0.37,
  "confidence_policy_version": "2026-06-17",
  "confidence_threshold_key": "tensor.default.low",
  "fallback_mode": "degraded_continue",
  "fallback_reason_code": "ANN_EMPTY",
  "escalation_source_layer": "ann",
  "latency_ms": 4.2,
  "result": "retrieve"
}
```

## Metrics Taxonomy

### Counter metrics
- `themis_layer_handoff_total{from_layer,to_layer,result}`
- `themis_layer_fallback_total{layer,fallback_mode,reason_code}`
- `themis_layer_fail_closed_total{layer,reason_code}`
- `themis_final_layer_compatibility_rejection_total{reason_code}`

### Histogram metrics
- `themis_layer_handoff_latency_ms{from_layer,to_layer}`
- `themis_layer_decision_confidence{layer,threshold_key}`

### Gauge metrics
- `themis_layer_active_policy_version{layer,policy_version}`

Cardinality constraints:
- `reason_code` values must come from a fixed registry.
- Free-text fields are not allowed as metric labels.

## SLO Definitions

### SLO-1: Layer handoff latency
- Metric: `P95(themis_layer_handoff_latency_ms{from_layer,to_layer})`
- Target: <= 25 ms per handoff in normal profile
- Alert window: 10 minutes burn-rate

### SLO-2: Fallback rate
- Metric: `themis_layer_fallback_total / themis_layer_handoff_total`
- Target: <= 5% per layer (steady-state)

### SLO-3: Confidence escalation rate
- Metric: escalations / handoffs per layer
- Target: stable baseline with <= 20% drift week-over-week

### SLO-4: Compatibility rejection rate
- Metric: `themis_final_layer_compatibility_rejection_total / final_layer_decisions`
- Target: <= 1% for production-approved packages

## Dashboard Requirements
Required panels:
1. End-to-end request volume with correlation continuity rate
2. Handoff latency heatmap ANN->Tensor->Graph->Final Layer
3. Fallback mode distribution by layer and reason code
4. Confidence distribution vs threshold keys
5. Final-layer compatibility rejections by reason code

## Validation Strategy

### Tests
1. Correlation continuity integration test
- Asserts unchanged `correlation_id` through all four layers.

2. Routing-reason schema test
- Verifies all mandatory reason fields are present and non-empty.

3. Metrics emission smoke test
- Verifies counters/histograms are emitted for a representative request set.

4. Fail-closed telemetry test
- Verifies fail-closed paths emit `fallback_mode=fail_closed` with reason code.

### Operational verification
- Synthetic canary traffic run confirms dashboard panel population.
- Alert simulation validates threshold and burn-rate rules.

## Rollout Plan
Phase 1:
- Add structured fields and reason registry.
- Emit logs and metrics without hard enforcement.

Phase 2:
- Enable SLO alerting and production dashboards.
- Enforce missing-field checks in integration tests.

Phase 3:
- Gate release readiness on observability conformance checks.

## Ownership
- Architecture: defines schema and policy versions.
- Platform Observability: dashboards, alerts, metric retention.
- Retrieval owners: field emission and test maintenance.
