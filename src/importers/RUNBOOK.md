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
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/importers/importer_interface.h; include/importers/importers_api_contract.h; src/importers/wikipedia_pipeline.cpp; src/importers/AUDIT.md"
Related_Reference: "makr-code/ThemisDB#7124"
---

> Exact model backend not verifiable from final document text alone.

# Importers Incident Runbook

For operators and on-call developers diagnosing an active import. Use the
returned `ImportStats` and structured errors as the first evidence; connector
logs and source-system health provide the next layer. This runbook does not
prescribe destructive target cleanup.

## Triage by symptom

| Symptom or code | Diagnosis | Safe next action |
|---|---|---|
| `FILE_NOT_FOUND`, `FILE_OPEN_FAILED`, `FILE_READ_FAILED` | Confirm the selected path, file availability, permissions, and format expected by that importer. | Correct the source/access issue, rerun source validation, then retry only after checking whether earlier batches committed. |
| `IMPORT_CONNECTOR_UNAVAILABLE` | Check connector feature availability, host/network reachability, and source-side limits. | Restore the source path or connector, then retry according to that connector's checkpoint and idempotency behavior. |
| `DEADLINE_EXCEEDED` | Compare configured `import_timeout_ms` with elapsed time, connector latency, and batch size. | Reduce workload or adjust the configured deadline under the service's latency budget; do not classify other error codes as retryable by analogy. |
| `SCHEMA_VALIDATION_FAILED`, `IMPORT_SCHEMA_MISMATCH`, or conversion errors | Inspect the structured error location and compare source values, sampled schema, mappings, and target schema. | Correct the mapping/data or apply the explicit migration required for a breaking schema change; do not disable validation merely to clear the incident. |
| `ROW_TOO_LARGE` or quota error | Inspect configured row/statement bounds and the affected row or import size. | Increase a limit only after confirming memory, disk, and downstream capacity; otherwise split or filter the source. |
| `CONFLICT_ERROR` | Confirm conflict keys and the selected `ConflictStrategy`. | Reconcile source and target identity, then choose a permitted conflict policy intentionally. Avoid truncating the target as an automatic recovery. |
| `PERMISSION_DENIED` | Check the host-supplied `permission_check("import", "write")` decision. | Restore only the required import permission and retry through the authorized caller. |

The exact error enum and spelling are declared in
[`importer_interface.h`](../../include/importers/importer_interface.h).
Connector errors may differ; use the structured code actually returned by the
selected implementation.

## Resume, cancellation, and rollback

1. Capture the source identifier/path, connector configuration version,
   checkpoint path, `ImportStats`, structured errors, and relevant source and
   target health signals.
2. Determine whether the importer committed, wrote a checkpoint, or emitted
   quarantine/dead-letter records. Check the connector implementation because
   checkpoint formats and commit boundaries are not uniform.
3. Resume only against the intended source and compatible mapping/configuration.
   Do not reuse a checkpoint for an unrelated source.
4. Use `cancel()` to request importer cancellation where supported, then allow
   the implementation to report its final counters. Cancellation is not proof
   that a partially processed external operation was rolled back.
5. Roll back only through the owning target transaction/restore procedure and
   only after confirming the commit boundary. Preserve audit and checkpoint
   evidence before any recovery action.

The contract header describes retryable timeout and unavailable-connector
conditions for its `ImporterErrorCode`; it does not make all connector errors
safe to replay. Idempotency and recovery must be verified for the particular
entry point.

## Wikipedia export failure

Retain the source dump and checkpoint, inspect `validateDatabase()` output,
and review the generated manifest/verification report before distributing an
export. Re-run validation before export after correcting source or checkpoint
issues. Preserve the prior artifact until the replacement has been validated.

## Escalation

Escalate with sanitized structured errors, source ID, import options with
secrets removed, checkpoint state, and the smallest reproducible input. Do
not attach raw records or credential-bearing connector configuration to a
public issue. See the [trust model](./TRUST.md) and
[operations guide](./OPERATIONS.md).
