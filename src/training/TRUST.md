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

# Training Trust Boundaries

## Purpose

This page identifies where training inputs, external dependencies, and
generated artifacts cross trust boundaries. It supplements, but does not
replace, the threat model and controls in [`SECURITY.md`](./SECURITY.md) or the
mandatory deployment requirements in
[`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md).

## Boundaries and controls

| Boundary | Risk | Module-level control or required check |
|---|---|---|
| Source collections and submitted samples → labeler/selection | Untrusted, malformed, sensitive, or prompt-injection content can influence labels or enter training. | Extraction and selection use shared prompt-safety checks; selection can reject PII and low-quality inputs. These checks do not replace data authorization, privacy review, or human label review. |
| Graph context and vector index → enrichment | Stale, unrelated, or unavailable context can affect a sample. | Configure graph scope and similarity threshold; production similarity calls require an initialized injected vector index. Validate missing-vector and search-failure outcomes. |
| Trainer → checkpoint directory | Corruption, path abuse, partial writes, or unauthorized artifact replacement can affect recovery/deployment. | Use path validation and SHA-256 manifest validation; keep the directory access-controlled and preserve a known-good checkpoint. A digest detects content mismatch but is not a signer identity or authenticity proof. |
| Training result → serving router | A bad version or partial routing update can expose an unintended adapter. | Deployment validates the adapter and split, returns explicit errors, and reverts local registry state on router update failure. The caller must verify external router state. |
| Progress callbacks and diagnostics → logs/consumers | Raw document or control-token content can leak through messages. | Pipeline callbacks are sanitized before emission. Treat other log fields and exception diagnostics as potentially sensitive; apply access controls and retention limits at the sink. |
| Provenance/audit writes → graph or log store | Lineage may disclose source identifiers or be incomplete when writes fail. | Track written/rejected counts, configure a finite write timeout where required, restrict access to provenance storage, and monitor failures. A zero timeout is unbounded. |

## Identity, authorization, and secrets

The module-level headers accept collection names, model paths, connection
information, and external component pointers; they do not establish deployment
identity or authorization by themselves. Integrators must authorize reads and
writes, validate configuration at startup, protect checkpoint and audit
storage, and keep credentials out of configuration snapshots and diagnostics.
Never disable production authorization checks to make an offline/test path work.

## Incident handling

For suspected data exposure, unauthorized training, checkpoint tampering, or
unexpected adapter deployment:

1. Stop further training/deployment through the owning service's approved
   control, without deleting checkpoints or audit evidence.
2. Preserve sanitized run IDs, adapter versions, checkpoint manifest and
   integrity results, provenance/audit events, and router outcome.
3. Restrict access to affected artifacts and escalate through the project
   security incident process.
4. Resume or redeploy only from a verified checkpoint/version after the
   responsible operator approves recovery.

See [`RUNBOOK.md`](./RUNBOOK.md) for module diagnosis and
[`OPERATIONS.md`](./OPERATIONS.md) for deployment checks.

## Source evidence

- [`SECURITY.md`](./SECURITY.md) and
  [`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md) — current module
  security posture and required production controls.
- [`training_pipeline.cpp`](./training_pipeline.cpp) — pipeline input guards
  and callback sanitization.
- [`lora_checkpoint_manager.cpp`](./lora_checkpoint_manager.cpp) —
  checkpoint integrity and recovery.
- [`incremental_lora_trainer.cpp`](./incremental_lora_trainer.cpp) — serving
  registry/router coordination.
- [`provenance_tracker.h`](../../include/training/provenance_tracker.h) —
  provenance fields, write timeout, and audit contract.
