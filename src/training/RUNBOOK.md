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

# Training Incident Runbook

## Scope and safety

Use this runbook to diagnose a failed training run, checkpoint recovery, or
adapter handoff. It describes module-level diagnostics, not deployment-specific
commands. Preserve the last known-good adapter and checkpoint while
investigating. Do not clear checkpoint storage or delete files as an initial
recovery step.

## Collect evidence first

Record the run/adapter version, configured device and training parameters,
source and target collection names, last completed pipeline stage, returned
error code/message, checkpoint directory and manifest state, and relevant
metrics. Avoid copying raw training records, credentials, or full document
content into incident logs.

Use available structured outputs:

- `PipelineStats` for stage counts, selected/filtered sample counts, training
  success, loss, accuracy, elapsed time, and provenance counts.
- `IncrementalLoRATrainer::getTrainingDiagnostics()` and
  `getRecoveryStatus()` for trainer state and interruption details.
- `LoRACheckpointManager::resumeWithDiagnostics()` and
  `auditCheckpoints()` for candidate validity and recovery attempts.
- Deployment `DeployResult.error` for adapter validation or router failures.

## Triage and response

| Symptom | Diagnose | Safe response |
|---|---|---|
| Pipeline returns with no useful progress | Check required target collection, drift threshold range, required selection language, enabled stages, and result stats. | Correct invalid configuration and rerun only after confirming the intended source/target. The pipeline's critical-input guards fail closed. |
| Labeling or enrichment creates no samples/context | Check collection access, query-engine wiring, source data, and stage counts; distinguish offline/test mode from a live database path. | Repair the owning data/query integration. Do not treat an empty offline result as successful production ingestion. |
| Training fails or stops after interruption | Inspect `TrainingResult.error_message`, trainer diagnostics, device/resource configuration, and `getRecoveryStatus()`. | Resume only from a checkpoint reported valid by the checkpoint manager; otherwise use the deployment's approved backup/retrain procedure. |
| Checkpoint integrity mismatch or truncated file | Run `auditCheckpoints()` and `resumeWithDiagnostics()`; inspect the manifest and preserve all candidate files for evidence. | If configured, resume may select an older valid checkpoint. If none is valid, stop and escalate for restore from an approved backup; do not bypass validation. |
| Deployment or rollback returns an error | Check version existence, checkpoint integrity, traffic split, router availability, and router update result. | Keep the prior serving state; repair the failed dependency or select a verified version. Confirm the router and local registry agree before accepting traffic. |
| Similar-document enrichment throws or returns empty | Check that `VectorIndexManager` is initialized and injected, the document key has a vector, and index search succeeded. | Restore index wiring/data or disable that enrichment step by approved configuration; empty results can also mean missing vectors or failed search. |

## Recovery and rollback boundaries

`CheckpointManagerConfig::auto_rollback` controls whether `resume()` tries
older manifest entries after a candidate fails validation. It does not replace
an external backup. Keep validation enabled for integrity-sensitive use.
`cleanupPartialCheckpoints()` and `clearAll()` are destructive operations; use
them only under an approved maintenance procedure after preserving required
evidence and confirming a valid recovery source.

For serving, `rollbackVersionEx()` validates the target and reports router
failure explicitly. A failed operation is not proof that the external inference
router and local version registry are synchronized; verify both before closing
the incident.

## Escalation

Escalate with sanitized diagnostics, relevant error codes, checkpoint audit
output, configuration with secrets removed, and the smallest reproducible
training test. Route data authorization or suspected exposure issues through
the project security incident process in [`SECURITY.md`](./SECURITY.md).

## Sources

- [`include/training/TRAINING_CONTRACT.md`](../../include/training/TRAINING_CONTRACT.md)
- [`lora_checkpoint_manager.h`](../../include/training/lora_checkpoint_manager.h)
- [`incremental_lora_trainer.h`](../../include/training/incremental_lora_trainer.h)
- [`adapter_serving.h`](../../include/training/adapter_serving.h)
- [`OPERATIONS.md`](./OPERATIONS.md)
