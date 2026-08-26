---
name: Documentation Audit Finding
about: Report a gap, inaccuracy, or missing documentation discovered during a docs audit
title: "[docs] "
labels: ["type:documentation", "status:open", "priority:medium"]
assignees: []
---

## Summary

A clear and concise description of the documentation gap or inaccuracy found during the audit.

## Audit Reference

- **Audit Version:** <!-- e.g. v1.4.1, v1.5.0 -->
- **Finding ID:** <!-- e.g. DOCS-001 -->
- **Severity:** <!-- Critical | High | Medium | Low -->
- **Discovered:** <!-- Date or sprint -->
- **Documentation Tier:** <!-- Tier 1 (Primary) | Tier 2 (Secondary) | Tier 3 (Tertiary) -->
- **SOT Domain:** <!-- module-behavior | api-contract | build-test | release-versioning | security | architecture-governance -->
- **Update Interval:** <!-- Event-driven | Weekly | Monthly -->
- **Target Milestone:** <!-- DOC-WEEKLY-YYYY-WW | DOC-MONTHLY-YYYY-MM | DOC-RELEASE-vX.Y.Z -->
- **Gate Evidence:** <!-- Required for build-test or release-versioning findings: link Wave/CI/security/recovery/SLA artefacts -->

## Canonical Source References

List the canonical source documents/files that define truth for this finding.

- `ROADMAP.md` / `CHANGELOG.md` / `SECURITY.md` / code paths in `src/` / tests in `tests/`
- Additional references:
	- `...`

## Affected Files

List all documentation files and/or source files impacted by this finding:

- `docs/...` – <!-- brief description of issue -->
- `src/...` – <!-- related source file if applicable -->

## Current State

Describe the current (incorrect or incomplete) state of the documentation.

## Expected State

Describe what the documentation should say or cover after fixing this issue.

## Gap Type

<!-- Check all that apply -->
- [ ] **Inaccurate** – Documentation describes something that no longer exists or works differently
- [ ] **Missing** – Feature or module has no documentation at all
- [ ] **Stale** – Documentation references outdated versions, paths, or APIs
- [ ] **Broken Link** – One or more hyperlinks in the document are dead
- [ ] **Incomplete** – Documentation exists but is missing important sections
- [ ] **Translation** – Documentation is missing in one or more supported languages (DE/EN/FR/ES/JA)
- [ ] **Tier Violation** – Secondary/Tertiary document conflicts with Primary source

## Acceptance Criteria

- [ ] Documentation accurately reflects the current implementation
- [ ] All internal links are valid
- [ ] Code examples are tested and runnable
- [ ] Content is reviewed by at least one other contributor
- [ ] If applicable: German and English versions are in sync
- [ ] Tier precedence check passed (`Primary > Secondary > Tertiary`)
- [ ] Issue is assigned to the correct docs milestone
- [ ] Release-gate evidence is linked when the SOT domain is `build-test` or `release-versioning`
- [ ] Private-plugin findings do not leak confidential repository paths, URLs, or implementation details into public docs
- [ ] If AI coding context is affected: `AI_WIKI_INTEGRATION_PLAYBOOK.md` and `ai_context/developer_llm_wiki/*` references are reviewed for sync and drift

## Additional Context

Add any relevant context, screenshots, or references here.
