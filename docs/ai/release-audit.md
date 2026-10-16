---
Author: ThemisDB Contributors
Created: 2026-10-16
Last Updated: 2026-10-16
Status: active
---

# ThemisDB AI Release Audit Guide

## Purpose

Provide an AI-readable audit procedure for verifying ThemisDB release and deployment traceability from `commit -> tag -> release -> deploy`, including role-scoped human approvals and signed evidence-chain integrity.

## Required Inputs

1. Governance policy:
   - [../governance/release-governance.md](../governance/release-governance.md)
2. Evidence chain specification:
   - [../security/release-evidence-chain.md](../security/release-evidence-chain.md)
3. GitHub governance artifacts:
   - `/.github/CODEOWNERS`
   - `/.github/branch-protection-rules.json`
   - release/deploy workflow metadata from GitHub Actions runs

## AI Audit Objectives

1. Verify every human sign has valid **role** and **scope**.
2. Verify every transition is represented in the signed evidence ledger.
3. Verify ledger chain continuity and signatures.
4. Verify release/deploy actions correspond to the approved commit/tag/artifacts.
5. Verify required GitHub controls are in place and consistent with governance policy.

## Deterministic Audit Procedure

### Step 1 — Role and Scope Validation

For each approval record:

- Confirm `approver_role` is one of `code_owner`, `release_owner`, `deploy_owner`.
- Confirm `approval_scope` is one of `code`, `release`, `deploy`.
- Confirm role-scope pairing is valid for the `event_type`.

### Step 2 — Chain Continuity Validation

For each record in index order:

- Recompute `payload_hash` from canonical payload.
- Recompute `record_hash`.
- Check `previous_hash` linkage.
- Verify `signature`.

### Step 3 — Transition Coverage Validation

For each released version:

- `code_approved` exists for the released commit/PR scope.
- `tag_approved` exists for the release tag.
- `release_published` exists for published release assets.
- `deploy_executed` exists for every required deployment target.

### Step 4 — Artifact Binding Validation

- `artifact_digest_sha256` matches published artifact checksums.
- `sbom_digest_sha256` matches published SBOM digest.
- `commit_sha`, `tag`, `version` remain consistent across chain events.

### Step 5 — GitHub Control Validation

- CODEOWNERS rules exist for touched paths.
- Branch protection policy requires code-owner review and required status checks.
- Release/deploy workflows run with named environments and approval gates.

## Audit Output Schema (Suggested)

```json
{
  "audit_version": "1.0",
  "release_tag": "v2.6.0",
  "status": "pass|warn|fail",
  "checks": [
    {"id": "role_scope", "status": "pass", "details": "..."},
    {"id": "hash_chain", "status": "pass", "details": "..."},
    {"id": "signature", "status": "pass", "details": "..."},
    {"id": "artifact_binding", "status": "pass", "details": "..."},
    {"id": "github_controls", "status": "pass", "details": "..."}
  ],
  "blocking_findings": [],
  "timestamp_utc": "2026-10-16T00:00:00Z"
}
```

## Severity Rules

- **FAIL (blocking)**:
  - missing role or scope
  - invalid role-scope mapping
  - chain break (`previous_hash` mismatch)
  - signature verification failure
  - artifact/SBOM digest mismatch
  - release/deploy without required approval evidence
- **WARN (non-blocking unless policy escalates)**:
  - incomplete non-critical metadata
  - delayed evidence replication where primary signed record is intact

## Acceptance Criteria

- [ ] AI audit flow verifies role-scoped approvals and forbids generic approvals.
- [ ] AI audit flow verifies `commit -> tag -> release -> deploy` coverage.
- [ ] AI audit flow verifies signed hash-chain integrity and signatures.
- [ ] AI audit flow verifies artifact and SBOM digest binding.
- [ ] AI audit flow checks GitHub integration controls (CODEOWNERS, environments, status checks, branch protection expectations).
- [ ] Audit output format is machine-readable and maintainable by humans.

## Practical Implementation Plan (ThemisDB)

### Phase 1 — Data Availability

1. Ensure workflows emit structured evidence JSON for each release/deploy event.
2. Ensure release assets include checksums, SBOM digest, and evidence manifest.

### Phase 2 — Audit Automation

1. Implement/extend CI audit step to run deterministic chain + signature checks.
2. Emit machine-readable audit result artifact per release/deploy run.

### Phase 3 — Policy Enforcement

1. Gate production deployment on `status=pass` for release audit.
2. Add explicit failure messages for missing role/scope or chain integrity.

### Phase 4 — Continuous Governance

1. Review governance-role mapping quarterly.
2. Reconcile workflow/environment settings with policy documents after governance changes.
