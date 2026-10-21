---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Retrieval Runbook

## Purpose

This runbook covers the operational actions for the currently implemented retrieval module surfaces: LoRA artifact integrity management and dual-read retrieval regression checks.

## Common symptoms

### 1. Artifact integrity mismatch

Symptoms:

- `verifyPackageIntegrity()` or `verifyProductIntegrity()` returns false,
- manifest hash does not match the canonical JSON representation,
- a package is marked as invalid or suspicious during import.

Action:

1. Recompute the artifact manifest using the canonical JSON serialization path.
2. Validate the reported `weights_hash` and `manifest_hash`.
3. Confirm whether the package was imported from an untrusted source or tampered with.
4. Reject or quarantine the artifact before deployment.

Escalation:

- if the artifact is in a production environment, treat the event as a trust incident and suspend package deployment until the source is confirmed.

### 2. Retrieval regression detected

Symptoms:

- `RetrievalMetrics::HasRegression()` indicates a downgrade,
- recall, nDCG, or MRR falls outside the expected tolerance threshold,
- current results show a clear delta against previous index evaluation.

Action:

1. Validate the ground-truth judgments used by `SetGroundTruth()`.
2. Check whether the stale index or current index path was incorrectly configured.
3. Confirm the query list and evaluation window are still consistent.
4. If the regression is real, revert to the previous index or stop rollout until the cause is understood.

Escalation:

- if the regression affects a production retrieval path, open an operational incident and compare the current result set to the previous baseline before enabling traffic.

### 3. Missing or empty ground truth

Symptoms:

- `GetGroundTruth()` returns nothing,
- dual-read validation reports zero or surprising values,
- warning logs note that no ground truth is cached for the query.

Action:

1. Load the judgments into the validator via `SetGroundTruth()`.
2. Treat empty ground truth as a failed evaluation path rather than a valid release signal.
3. Do not enable changes based on ungrounded retrieval metrics.

## Operational checks

Use the following checklist before rollout or promotion:

- [ ] artifact hashes have been recomputed and verified,
- [ ] package status is consistent with the lifecycle policy,
- [ ] signature verification callback is configured for untrusted imports,
- [ ] all critical queries have ground truth loaded,
- [ ] regression deltas are reviewed before promoting a new retrieval build,
- [ ] package or product records are not silently reclassified without evidence.

## Recovery and rollback

### Recovery from bad package state

- remove or quarantine the bad package from the manifest store,
- confirm that no derived product continues to reference the invalid package,
- restore the last valid package version if necessary,
- re-run integrity verification on all restored artifacts.

### Recovery from retrieval regression

- fall back to the previous index path,
- reload the previous ground-truth dataset,
- compare the current metric deltas against the previous baseline,
- back out the rollout if regression exceeds the configured tolerance.

## Escalation

Escalate to module or release ownership when:

- package integrity failure impacts multiple artifacts,
- a regression persists across repeated validation runs,
- the module is used in production traffic despite incomplete ground truth,
- a status transition is being bypassed or altered without review.

## Conclusion

The runbook is intentionally conservative: the module is only safe when artifact integrity and retrieval quality are explicit, measured, and traceable.
