---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Retrieval Runbook

## Purpose

This page is guidance for an application integrating the module, not an operator runbook for a production retrieval service. Artifact records are persistent in explicitly configured RocksDB, and deployment has a fail-closed quality/authorization entry point, but there is no current deployment-service wiring or live index query path. `DualReadValidator::ExecuteQuery()` returns an empty result vector, and `ValidateQuery()` does not return a comparison delta.

## Common symptoms

### 1. Artifact integrity mismatch

Symptoms:

- `verifyPackageIntegrity()` or `verifyProductIntegrity()` returns false,
- manifest hash does not match the canonical JSON representation,
- the artifact lacks a matching manifest hash, valid Ed25519 signature, or active
  configured signer pin,
- a caller's import or verification handling reports an issue.

Action:

1. Recompute the artifact manifest using the canonical JSON serialization path.
2. Recompute and compare the manifest hash. This API does not read or verify the referenced weight file.
3. Confirm whether the package was imported from an untrusted source or tampered with.
4. The registry rejects unsigned/invalid artifacts during write/import and
   rejects invalid persisted records during startup; quarantine the source and
   investigate the signing key if encountered.

Escalation:

- if the artifact is in a production environment, treat the event as a trust incident and suspend package deployment until the source is confirmed.

### 2. Retrieval regression detected

Symptoms:

- a caller-supplied pair of `RetrievalMetrics` makes `HasRegression()` return true.

`ValidateQuery()` does not calculate this comparison; its current/previous query result vectors are empty because the executor is a placeholder.

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
- metrics are zero or surprising,
- warning logs note that no ground truth is cached for the query.

Action:

1. Load the judgments into the validator via `SetGroundTruth()`.
2. Treat empty ground truth as a failed evaluation path rather than a valid release signal.
3. Do not enable changes based on ungrounded retrieval metrics.

## Operational checks

Use the following checklist before rollout or promotion:

- [ ] the configured RocksDB path is dedicated, persistent, and protected by the owning service,
- [ ] artifact hashes and signatures pass verification with the configured pinned-key verifier,
- [ ] the owning deployment service supplies and audits its authorization callback,
- [ ] quality inputs are non-empty and the RAG quality controller returns `kAllow`,
- [ ] all critical queries have ground truth loaded,
- [ ] regression deltas are reviewed before promoting a new retrieval build,
- [ ] package or product records are not silently reclassified without evidence.

## Recovery and rollback

### Recovery from bad package state

- remove the bad package from the persistent store if appropriate; quarantine requires an external system,
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

These are application-side response suggestions, not implemented module controls. The RocksDB registry is durable artifact state, not an audit-event system; rollback, quarantine, and alerting remain external.
