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

# Updates Module Incident Runbook

Use this runbook to triage update incidents using the module's actual
diagnostic and rollback surfaces. It is not a command-line manual: the module
headers do not define a universal `themis_cli` or `themis-ctl` interface.
Operators must use the control plane supported by their deployment.

## First response and safety

1. Stop or pause further rollout through the deployment's supported control
   plane. Do not start a second update while the first is applying or rolling
   back.
2. Record the update version, node, operation, phase, timestamp, error code,
   and affected file or migration if known. Preserve structured diagnostics
   before restarting a process.
3. Determine whether the incident is local or multi-node. On a partition or
   quorum-loss signal, prevent isolated nodes from independently advancing
   deployment state.
4. Before rollback, verify the target rollback ID/checkpoint exists and the
   affected node is isolated from traffic when required. Never force promotion
   or rollback based only on a scenario label.

## Diagnosis matrix

| Signal | Inspect | Safe next action |
|---|---|---|
| Invalid transition, `STATE_FAILED_LOCKED`, or in-flight history | `currentState()`, `hasInFlightUpdate()`, `inFlightVersion()`, and transaction-log entries from `UpdateStateMachine` | Identify the last accepted transition. `FAILED` requires explicit reset; do not reset until the underlying operation and installed files are reconciled. |
| Patch apply, checksum, base mismatch, or decode error | `DeltaApplyResult`, failed path, expected/actual hash context, and the base version | Stop apply; verify the authoritative manifest and base file. Re-fetch or use a full artifact only through the supported updater. |
| Missing rollback target/checkpoint or rollback failure | Returned rollback ID/checkpoint, rollback callbacks, and `OperatorDiagnostics` recovery procedure | Keep the node isolated and preserve logs/artifacts; escalate if state cannot be proven consistent. |
| Coordination timeout, peer failure, quorum loss, or partition | Error code, node IDs, operation context, and peer/quorum state from the owning coordinator | Halt rollout progression and follow the deployment's quorum/partition recovery policy. Do not let minority partitions proceed. |
| Migration or resource exhaustion | Migration phase, failed nodes, resource exhaustion context, and migration result | Do not blindly retry. Establish whether the migration supports rollback and fix the resource or compatibility cause first. |
| Diagnostic scenario is unknown | Original `ErrorContext` and `getRecoveryProcedure()` output | Treat the default classification as a hint only; page the module/on-call owner when state integrity is uncertain. |

`OperatorDiagnostics` exposes scenario matching, recovery descriptions, alert
rules, metric recommendations, and JSON exports. Its classification is based
on error code and operation/phase text; it is not an independent health probe.
Use [`updates_operator_diagnostics.h`](../../include/updates/updates_operator_diagnostics.h)
and [`updates_diagnostics.h`](../../include/updates/updates_diagnostics.h).

## Recovery verification

Before resuming traffic or rollout:

- Confirm the state machine is at the expected terminal state and no
  unhandled in-flight update remains.
- Verify the active version and required release files against the deployment's
  authoritative manifest/checks.
- Confirm each affected node reports a consistent migration/rollout result.
- Confirm rollback callbacks or diagnostics indicate completion; a recommended
  recovery action is not evidence that recovery succeeded.
- Resume one controlled step at a time and watch structured update diagnostics.

Escalate immediately for data-loss risk, ambiguous partial migration, signature
or integrity failures, unresolved quorum loss, or a rollback that cannot be
verified. Include the redacted `ErrorContext`, transaction history, version,
affected nodes, and actions already taken. Do not attach API tokens, webhook
secrets, or unredacted configuration.

For longer-term operating limits and observability, see
[`OPERATIONS.md`](OPERATIONS.md) and
[`docs/operability/RUNBOOK_UPDATES_ENGINE.md`](../../docs/operability/RUNBOOK_UPDATES_ENGINE.md).

> Exact model backend not verifiable from final document text alone.
