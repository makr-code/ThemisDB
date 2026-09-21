---
name: GA Promotion Sign-Off Request
about: Open a human-maintained GA promotion approval request that can be promoted into a PR with machine-validated sign-off evidence.
title: "[ga-signoff] "
labels: ["release_critical", "status/needs-approval", "type:documentation"]
assignees: []
---

## Summary

Describe which GA / release promotion needs final human authorization.

## Promotion Target

- **GA Name:** <!-- e.g. ThemisDB v2.4.0 GA -->
- **Target Version / Tag:** <!-- e.g. v2.4.0 -->
- **Source Branch / Ref:** <!-- e.g. develop -->
- **Requested By:** <!-- GitHub handle or release owner -->

## Required Payload

- [ ] Create or update a request payload under `docs/governance/ga_signoff_requests/`
- [ ] Use `docs/governance/templates/GA_PROMOTION_SIGN_OFF_REQUEST.example.yaml` as the schema baseline
- [ ] Keep CI-owned placeholders (`__GITHUB_ACTOR__`, `__GITHUB_REVIEW_SUBMITTED_AT__`, `__GITHUB_REF__`, `__GITHUB_SHA__`) intact unless a maintainer will finalize via `workflow_dispatch`
- **Payload Path:** <!-- repo-relative path to the JSON/YAML payload -->

## Governance Context

- **Sponsor / Owner / Team Lead:** <!-- required -->
- **Intended Approver / Signer:** <!-- authorized maintainer GitHub handle -->
- **Purpose / Rationale:** <!-- why the GA promotion should be approved -->
- **Source Document:** `docs/governance/GA_PROMOTION_SIGN_OFF.md`
- **Evidence / PR Reference:** <!-- issue, PR, runbook, or evidence bundle links -->

## Handoff to PR

- [ ] A human maintainer will move this request into a PR before final sign-off
- [ ] The PR will contain or reference the payload file above
- [ ] Final approval must come from an authorized maintainer review or manual `workflow_dispatch`

## Acceptance Criteria

- [ ] The request contains all GA promotion metadata required by `scripts/ga_signoff.py`
- [ ] The referenced approver is present in `.github/ga-signoff-authorized-maintainers.json`
- [ ] The linked PR/payload is ready for `.github/workflows/ga-promotion-signoff.yml`
- [ ] Final GA approval remains pending until the machine-generated manifest artifact and SHA-256 are produced
