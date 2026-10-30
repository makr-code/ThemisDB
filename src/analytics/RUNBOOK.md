---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Analytics Incident Runbook

## Use and safety

This runbook supports incident triage for analytics callers and operators.
First preserve the operation/correlation ID, failure class, timestamps, and
component status. Do not replay an event or publish a partial result until its
failure semantics are understood. See [OPERATIONS.md](./OPERATIONS.md) for
routine deployment checks. The more compact incident taxonomy maintained for
cross-module troubleshooting is
[`docs/troubleshooting/analytics_operability_runbook.md`](../../docs/troubleshooting/analytics_operability_runbook.md);
keep both descriptions aligned when its incident classes or signals change.

## Triage by symptom

| Symptom | Diagnose | Safe response |
|---|---|---|
| Inference returns `UNAVAILABLE` | Check `activeBackendName()`, backend availability/build options, endpoint reachability, and requested model/version. | Restore the configured backend/model or route the request through an explicitly approved alternate; do not report an unavailable call as a successful prediction. |
| Inference returns `TIMEOUT` or `POLICY_REJECTED` | Inspect configured `BoundedExecutionPolicy`, concurrent load, model latency, and serving diagnostics. | Reduce ingress or concurrency first; adjust limits only after validating the service capacity and deployment policy. |
| TLS/serving connection failure | Verify endpoint scheme, certificate validity, verification setting, timeout, and server logs. | Restore a valid HTTPS endpoint/certificate. Do not enable plaintext transport in production to bypass TLS failure. |
| Model load fails integrity validation | Compare the expected SHA-256 with the artifact obtained from the approved source; inspect the load error and deployment record. | Quarantine the artifact and restore a verified copy. Do not disable mandatory integrity checks or substitute an unverified artifact. |
| Streaming returns `WINDOW_EXPIRED` | Compare event timestamp/watermark and the configured window semantics. | Treat the event as rejected/discarded by the contract; correct upstream ordering or lateness policy before any controlled replay. |
| Backpressure or high-cardinality window saturation | Inspect `partition_keys_rejected`, `windows_evicted`, `max_distinct_partition_keys`, `max_open_windows`, `max_records_per_window`, queue limits, and selected `BackPressureMode`. | Reduce ingress or partition cardinality and allow consumers to drain; re-submit only events known not to have been accepted. |
| Distributed result is partial or a shard circuit is open | Compare `successful_shards` with `total_shards`; inspect `shard_info`, last shard errors, circuit state, failure class, and operator hints. | Withhold completeness-dependent output, restore failed shards, and retry after health/recovery conditions are met. Preserve partial output only when the application explicitly supports it. |
| Export returns `NOT_SUPPORTED` or `POLICY_REJECTED` | Check exporter `supportsFormat()`, optional Arrow capability, selected format, and execution policy. | Use only a format explicitly supported by the configured exporter or enable the required backend through the normal build/deployment process. Do not relabel bytes as another format. |

## Evidence to capture

Capture request configuration without secrets, input identifiers needed to
reproduce the issue, build feature flags, model name/version (not model
contents), backend status, error/status code, and relevant result metadata.
For distributed results, preserve shard-level outcome and timing fields. For
export, record rows/bytes/duration. For in-process serving, inspect
`ModelHealthMetrics` including recent latency and prediction counts.

## Recovery and rollback gates

1. Stop or rate-limit the affected producer/operation if repeated requests can
   amplify load or duplicate side effects.
2. Correct the failing dependency, configuration, or resource envelope.
3. Replay only after confirming the original operation's acceptance state,
   idempotency behavior, and application-level duplicate handling.
4. Roll back the deployment/configuration if a known-good version restores
   complete results without weakening TLS, authorization, integrity checks, or
   bounded-resource controls.
5. Escalate unresolved data loss, integrity mismatch, cross-tenant exposure, or
   repeated shard failures to the service owner and project security channel.

See [TRUST.md](./TRUST.md) for security incidents and
[`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md) for mandatory
production controls.
