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

# Training Operations

## Operating boundary

The training module performs data preparation, adapter training, checkpoint
management, provenance recording, and a serving handoff. It does not define
deployment-specific scheduling, backup policy, or production authorization.
Those are responsibilities of the integrating service and deployment. The
production constraints in
[`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md) remain normative.

## Pre-run checks

- Confirm the deployment is authorized to read the selected source data and
  write training samples, checkpoints, and audit/provenance records.
- Validate the base-model path, adapter version/mode, rank, batch/sequence
  bounds, device and quantization settings, and collection names.
- Configure checkpoint location, retention, integrity validation and I/O
  timeout for the deployment. Ensure available disk can accommodate a new
  checkpoint without removing the last usable version.
- Wire database/query and graph dependencies intended for production. For
  semantic similarity, inject an initialized vector index with vectors keyed
  by document ID.
- Set stage flags intentionally and arrange a trusted consumer for sanitized
  progress callbacks and audit outputs.

## Runtime observation

Capture the stage and overall elapsed times, documents/samples processed,
selection input/output/filter counts, quality/drift findings, training loss
and accuracy, adapter version, and provenance write/rejection counts from
`PipelineStats` and trainer metrics. For dataset selection, retain the
selection audit entry and its input/output counts. For checkpoint issues,
record manifest validation and recovery diagnostics.

The mapped benchmark cases and regression thresholds are documented in
[`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md). They are
release-validation expectations, not runtime service-level guarantees.

## Adapter release and rollback

1. Train and validate an adapter without changing the serving traffic state.
2. Confirm its checkpoint is present and integrity validation succeeds.
3. Deploy through `deployVersionEx()` with a traffic split in `[0, 1]`; inspect
   `DeployResult` and confirm router availability/update.
4. Observe the deployed version and workload-specific quality/latency metrics
   in the integrating inference service.
5. If rollback is required, use `rollbackVersionEx()` with a verified target
   version and confirm the router and local registry state after the call.

Do not infer production readiness from a successful training result alone.
Serving success depends on the injected router and the integrating runtime.

## Recovery and escalation

Use [`RUNBOOK.md`](./RUNBOOK.md) for checkpoint, training, and handoff
incidents. Preserve manifest and checkpoint evidence before any cleanup.
Escalate authorization, data exposure, or integrity concerns using the
project-level security incident process; include sanitized identifiers and
diagnostic codes rather than raw training content or credentials.

## Sources

- [`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md)
- [`SECURITY.md`](./SECURITY.md)
- [`ARCHITECTURE.md`](./ARCHITECTURE.md)
- [`include/training/TRAINING_CONTRACT.md`](../../include/training/TRAINING_CONTRACT.md)
- [`RUNBOOK.md`](./RUNBOOK.md)
