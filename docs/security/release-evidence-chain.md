---
Author: ThemisDB Contributors
Created: 2026-10-16
Last Updated: 2026-10-16
Status: active
---

# ThemisDB Release Evidence Chain (Signed Hash Ledger)

## Purpose

Define a tamper-evident, append-only release/deployment evidence ledger for ThemisDB that can be validated by humans and AI systems.

## Ledger Model

- Format: JSON Lines (`.jsonl`) or one JSON record per event.
- Property: append-only.
- Integrity: each record stores `previous_hash` and its own `record_hash`.
- Authenticity: each record is signed (`signature`) by the workflow signing identity.

## Required Record Fields

Each release/deploy evidence record must contain:

- `schema_version`
- `index` (monotonic integer)
- `event_type` (for example `code_approved`, `tag_approved`, `release_published`, `deploy_executed`, `rollback`)
- `commit_sha`
- `tag`
- `version`
- `artifact_digest_sha256`
- `sbom_digest_sha256`
- `previous_hash`
- `approver_identity`
- `approver_role` (`code_owner` | `release_owner` | `deploy_owner`)
- `approval_scope` (`code` | `release` | `deploy`)
- `timestamp_utc` (RFC3339 UTC)
- `environment`
- `workflow` (name/file)
- `workflow_run_id`
- `workflow_run_attempt`
- `job_name`
- `repository`
- `signature`

## Canonical Hashing Rules

To prevent inconsistent hashes across tools:

1. Serialize the payload with deterministic key ordering.
2. Exclude `signature` from hash input.
3. Compute:
   - `payload_hash = SHA256(canonical_payload_json)`
   - `record_hash = SHA256(previous_hash + ":" + payload_hash)`
4. Sign `record_hash` with the configured signing key.

If any protected field changes, `payload_hash` and `record_hash` change and chain verification fails.

## Example Record (AI-Readable)

```json
{
  "schema_version": "1.0",
  "index": 42,
  "event_type": "release_published",
  "commit_sha": "a91f3c0d68a8ee12345...",
  "tag": "v2.6.0",
  "version": "2.6.0",
  "artifact_digest_sha256": "f1a789...d82a",
  "sbom_digest_sha256": "0f1d2a...ab71",
  "previous_hash": "2b3f...e9d1",
  "approver_identity": "alice",
  "approver_role": "release_owner",
  "approval_scope": "release",
  "timestamp_utc": "2026-10-16T12:40:22Z",
  "environment": "community-release",
  "workflow": ".github/workflows/release-mainline.yml",
  "workflow_run_id": 18421,
  "workflow_run_attempt": 1,
  "job_name": "publish-community",
  "repository": "makr-code/ThemisDB",
  "payload_hash": "5d44...aaab",
  "record_hash": "90ee...c1f8",
  "signature": "base64-signature"
}
```

## Chain Validation Rules

For each record `n > 0`:

1. `record[n].previous_hash == record[n-1].record_hash`
2. recomputed `payload_hash` equals stored `payload_hash`
3. recomputed `record_hash` equals stored `record_hash`
4. signature verification of `record_hash` succeeds
5. required fields are present and non-empty
6. role/scope pair is valid for `event_type`

Any mismatch is a release/deploy blocker.

## Release/Deploy Capture Points

Evidence must be recorded at:

1. PR/code approval accepted (`code_approved`)
2. Release tag authorization (`tag_approved`)
3. Release publication (`release_published`)
4. Deployment approval and execution per environment (`deploy_executed`)
5. Rollback events (`rollback`) with reason and target version

## GitHub Actions Capture Contract

Workflows must capture and persist:

- Git refs (`github.sha`, tag/ref name)
- workflow metadata (`github.workflow`, `github.run_id`, `github.run_attempt`, job name)
- approver identity (`github.actor` and/or environment reviewer identity)
- environment name
- artifact and SBOM digests
- signature output

Evidence artifacts should be stored:

- as release assets for release tags
- in repository evidence history for long-term audit continuity

## Security Requirements

- Use short-lived/OIDC-backed signing identity where possible.
- Never store private signing keys in repository content.
- Protect ledger write path from non-release workflows.
- Reject deploy if release evidence signature or chain continuity is invalid.

## Acceptance Criteria

- [ ] Evidence ledger includes all required record fields.
- [ ] Chain linkage with `previous_hash` and deterministic hashing is specified.
- [ ] Signature rules and verification steps are specified.
- [ ] Release and deployment capture points are explicitly defined.
- [ ] GitHub Actions metadata and approver role/scope are included in records.
- [ ] Validation failures are documented as release/deploy blockers.
