---
Author: ThemisDB Contributors
Created: 2026-10-16
Last Updated: 2026-10-16
Status: active
---

# ThemisDB Release Governance (Role-Scoped Human Sign-Off)

## Purpose

Define a traceable and auditable release governance model for ThemisDB so every human sign-off is explicit, scoped, and machine-verifiable across `commit -> tag -> release -> deploy`.

## Scope

- Repository: `makr-code/ThemisDB`
- Branch model: `develop`, `community`, `minimal`, `enterprise`, `hyperscaler`, `military`
- Approval domains:
  - Code ownership approval (pull request scope)
  - Release approval (tag/release scope)
  - Deployment approval (environment scope)

## Role Model

### 1) Code Owner

- Authority scope: changed paths in a PR (as defined by `/.github/CODEOWNERS`).
- Decision scope: "this code change is acceptable for merge."
- Evidence scope label: `code`.
- Not sufficient alone for release publication or production deployment.

### 2) Release Owner

- Authority scope: release candidate/tag/release package set.
- Decision scope: "this version is approved for release publication."
- Evidence scope label: `release`.
- Must be represented in release approval workflows and evidence records.

### 3) Deploy Owner

- Authority scope: concrete target environment (`staging`, `production`, distro/registry endpoints).
- Decision scope: "this approved release is authorized for deployment to this environment."
- Evidence scope label: `deploy`.
- Must be captured in environment-protected workflows and deployment evidence records.

## Human Sign Rule (Normative)

A human sign is valid only when all conditions are true:

1. **Role-bound**: approver identity is mapped to an explicit role (`code_owner`, `release_owner`, `deploy_owner`).
2. **Scope-bound**: approval is tied to one scope (`code`, `release`, `deploy`) and one object (PR, tag, environment deployment).
3. **Evidence-bound**: approval is persisted in the release evidence ledger with timestamp, workflow context, and signature.

A generic "reviewed" or comment-only approval without role + scope + evidence recording is **not** a valid release-chain sign-off.

## Governance Control Points in GitHub

### CODEOWNERS

- File: `/.github/CODEOWNERS`
- Function: path-level code ownership and PR review assignment.
- Expectation: branch protection requires code owner review for protected branches.

### GitHub Environments

- Used for release/deploy control points (for example `docker-production`, `winget-production`, `linux-distro-production`, `windows-distro-production`).
- Expectation:
  - required reviewers enabled
  - deployment approvals recorded by GitHub
  - environment referenced in evidence ledger records

### Branch Protection

- Reference: `/.github/branch-protection-rules.json`
- Expectation:
  - required status checks per release lane
  - required PR reviews
  - code owner review enforcement
  - conversation resolution and no force-push on protected release branches

### Status Checks

- Examples from policy and workflows:
  - `Security: CodeQL Gate`
  - `validate-roadmap`
  - `validate-ai-guardrails`
  - build/test checks required by target branch rules
- Expectation: release approval is blocked if required checks are not green.

## Release & Deployment Flow with Approval + Evidence

1. **Commit / PR**
   - Commit lands via protected-branch PR flow.
   - Code owner approval is captured in GitHub review artifacts.
   - Evidence event written: `code_approved`.

2. **Tag**
   - Release owner approves creation/use of canonical tag (`vX.Y.Z`, edition-prefixed tags where applicable).
   - Evidence event written: `tag_approved`.

3. **Release**
   - Release workflow packages artifacts, checksums, SBOM, and release metadata.
   - Release owner approval gate finalizes publication.
   - Evidence event written: `release_published`.

4. **Deploy**
   - Deployment workflow targets environment protected by required approvers.
   - Deploy owner approves environment gate; deployment executes.
   - Evidence event written: `deploy_executed`.

Every event links to the previous event hash, producing a continuous evidence chain.

## Required Evidence Linkage

Each approval/deployment step must reference:

- `commit_sha`
- `tag` / `version`
- `artifact_digest_sha256`
- `sbom_digest_sha256`
- `previous_hash`
- `approver_identity`
- `approver_role`
- `approval_scope`
- `timestamp_utc`
- `environment`
- workflow metadata (`workflow`, `run_id`, `run_attempt`, `job`, `url`)
- `signature`

Detailed record and hashing rules are defined in:
- [../security/release-evidence-chain.md](../security/release-evidence-chain.md)
- [../ai/release-audit.md](../ai/release-audit.md)

## Acceptance Criteria

- [ ] Role model is documented with distinct responsibilities for code owner, release owner, deploy owner.
- [ ] Human-sign validity is defined as role-scoped and scope-bound, not generic reviewer approval.
- [ ] Commit -> tag -> release -> deploy approval flow is explicitly documented.
- [ ] GitHub integration points are documented: CODEOWNERS, environments, required approvers, status checks, branch protection.
- [ ] Required evidence fields are defined and linked to signed hash-chain records.
- [ ] Documentation is understandable by human maintainers and directly consumable by AI audit workflows.

## Practical Implementation Plan (ThemisDB)

### Phase 1 — Governance Baseline

1. Define role assignment matrix (accounts/teams mapped to `code_owner`, `release_owner`, `deploy_owner`).
2. Confirm `/.github/CODEOWNERS` coverage for release-relevant paths.
3. Align protected branch settings with `/.github/branch-protection-rules.json`.

### Phase 2 — Workflow Capture

1. Ensure release/deploy workflows include environment gates and stable run metadata.
2. Add/confirm evidence write step after each approval/deployment transition.
3. Persist evidence artifacts with release assets and repository history.

### Phase 3 — Verification & Auditability

1. Enforce hash-chain continuity checks before release and before production deploy.
2. Enforce signature verification for evidence records and artifacts.
3. Add AI audit checks that fail on missing role/scope/evidence fields.

### Phase 4 — Operationalization

1. Publish runbook for incident handling (chain break, signature mismatch, missing approval).
2. Train maintainers on role-scoped approvals and evidence expectations.
3. Add periodic governance review cadence (quarterly minimum).
