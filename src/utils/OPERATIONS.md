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

# Utils Operations

## Purpose and operating boundary

This guide is for operators and module owners using shared utils services.
The module supplies reusable logging, privacy, key, codec, concurrency, and
runtime helpers; it does not own application-level authorization, deployment
policy, or the recovery decision for each consumer. Keep each helper's
dependencies, capacity, and failure behavior visible to its caller.

## Operational signals

- Use structured logs and `ErrorContext` categories to identify whether a
  failure belongs to audit/logging, privacy, crypto, compression, concurrency,
  serialization, or tracing. The error taxonomy is in
  [`include/utils/error_contracts.h`](../../include/utils/error_contracts.h).
- For a thread pool, inspect `getStatistics()` and compare queued, active,
  executed, and failed work with the caller's submission/rejection rate.
- For a rate limiter, check configured rate/burst and the caller's timeout and
  rejection behavior. Avoid unbounded wait/retry patterns.
- For PII detection, inspect configured/active engine metadata and errors.
  Detection is not a universal proof of absence; track incomplete or failed
  scans distinctly from successful scans with no findings.
- For codecs, record the codec, payload size, and explicit error result.
  Preserve size/ratio guards and LZ4 original-size metadata.
- For key and PKI operations, verify storage availability, certificate and
  trust-store configuration, pin state, and rotation/revocation status without
  logging secret material.

## Capacity and lifecycle controls

Configure queues, input sizes, scan timeouts, compression bounds, connection
pool capacity, and retry/wait timeouts for the workload. Defaults and limits
are component-specific; consult the corresponding public header and
[`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md) before changing
deployment values.

On shutdown, stop owned background workers and pools while their storage,
logger, and key dependencies are still alive. A rejected task, failed flush,
or key operation is an explicit failure that the consuming module must
surface; do not mask it as success.

## Recovery and change safety

Use [`RUNBOOK.md`](RUNBOOK.md) for symptom-by-symptom diagnosis. Recovery must
restore the last known-good configuration and dependency state without
disabling validation, signature checks, or resource bounds. For hot-path
capacity changes, compare the benchmark cases and release thresholds in
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md); local tests alone
do not establish production throughput.

After recovery, confirm:

1. The affected helper returns or records the expected success state.
2. Its failure path remains observable and bounded.
3. Downstream consumers handle rejection, timeout, and incomplete data
   according to their policy.
4. Security-relevant events are audited and no secret or raw PII was added to
   logs or incident artifacts.

Escalate security and integrity concerns through the root
[`SECURITY.md`](../../SECURITY.md); engage the owning module for impact outside
the utils boundary.
