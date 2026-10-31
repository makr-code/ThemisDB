---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Updates Module Operations

This document defines deployment-time checks and observability expectations for
operators and platform maintainers. It supplements the incident procedures in
[`RUNBOOK.md`](RUNBOOK.md); it does not replace the deployment's own CLI or
authorization documentation.

## Configuration before enabling updates

`UpdatesConfig` groups update checking, auto-update and approval behavior,
hot-reload directories, notifications, canary policy, and hardware telemetry.
The defaults disable those feature toggles. Review the resolved configuration
before enabling any update path; in particular:

- Set install, download, backup, and history locations to deployment-owned
  directories with appropriate capacity and access controls.
- Keep signature verification and backup enabled unless a reviewed deployment
  requirement explicitly changes that policy.
- Configure approval timeout and critical-only behavior deliberately; a zero
  approval timeout has the special meaning documented by the configuration
  type.
- Set canary node identity and ordered stages intentionally. The final stage
  should represent complete rollout; per-node canary control is not cluster
  coordination.
- Keep notifications disabled until the endpoint and secret-handling policy
  are reviewed.
- Hardware telemetry is opt-in. When enabled, its configuration includes CPU
  model/count, RAM, OS, and architecture field switches; the configured send
  interval is clamped to at least one day and HTTP timeout/retry settings
  apply.

YAML loading logs errors and returns a default `UpdatesConfig` on exceptions.
Treat this as a startup configuration failure in production rather than
silently continuing with defaults. The parser is in
[`updates_config.cpp`](updates_config.cpp).

## Observability and routine checks

Capture structured diagnostics with `DiagnosticEmitter` and retain the error
code, severity, root cause, operation, phase, node ID, version, and
`extra_context` needed for triage. The module reserves codes 7400–7499.
`OperatorDiagnostics` can provide alert rules, log-pattern suggestions, and
metric names for each of its failure scenarios; validate generated rules in
the target observability system before relying on them.

Routine checks should verify:

1. No update remains unexpectedly in `DOWNLOADING`, `VERIFYING`, `APPLYING`,
   or `ROLLING_BACK`.
2. An in-flight-history indication has an owner and a reconciled recovery
   decision.
3. Rollback storage and update staging locations have sufficient space and
   expected permissions.
4. Canary and coordinated controllers are not being treated as equivalent:
   canary membership is local/deterministic, while cluster managers own
   coordination.
5. Audit/history and diagnostic data are retained and access-controlled
   according to deployment policy.

## Rollback and change control

Preserve the rollback ID returned by a successful hot reload and ensure the
corresponding backup remains available for the intended retention window.
Checkpoint state in `UpdateStateMachine` records control state; it is not a
substitute for a file backup. Stop progression if the active version, file
integrity, migration state, or cluster quorum cannot be established.

For a release, update operational documentation when public contracts,
configuration defaults, diagnostics, or recovery behavior changes. Verify
focused tests for the changed component and benchmark expectations in
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md). See
[`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md) for existing module
requirements; its claims remain subject to the source-of-truth order in
[`DOCUMENTATION_GOVERNANCE.md`](../../DOCUMENTATION_GOVERNANCE.md).

> Exact model backend not verifiable from final document text alone.
