---
Author: "ThemisDB Governance Team"
Created: "2026-10-13"
Last Updated: "2026-10-13"
Status: "active"
---

# PR Governance Tests Checklist

Use this checklist for any Pull Request that adds, modifies, or validates governance, compliance, release-gate, or audit-scope tests.

## Required GitHub metadata

- [ ] Milestone set to `Wave 10 Phase 1 - Parallel D+A`
- [ ] Labels present: `wave-10`, `block-d`
- [ ] Governance or audit labels applied as appropriate: `governance`, `audit`, `quality`
- [ ] Related issue or tracking link included in the PR description

## Scope validation

- [ ] I confirmed whether the change affects `audit/`, governance docs, release gates, or roadmap compliance checks.
- [ ] I checked that the scope does not broaden beyond the Block D governance deliverables.
- [ ] I reviewed `audit/BLOCK_D_GOVERNANCE_IMPLEMENTATION_GUIDE.md` and the parallel execution plan before coding.

## Test documentation

- [ ] A governance test template was used or linked.
- [ ] The test purpose, gate assumption, and expected result are described in the PR.
- [ ] The relevant audit or release evidence is included or linked.
- [ ] Any new documentation is aligned with `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, and the current audit baseline.

## Validation checklist

- [ ] Test names are descriptive and stable.
- [ ] The validation targets the proper release wave (`Wave 7`, `Wave 8`, or `Wave 9`).
- [ ] The expected pass/fail behavior is explicit.
- [ ] The test is scoped to the right branch and artifact set.
- [ ] CI or local validation commands were run and recorded.

## Nightly audit integration

- [ ] The change is covered by the nightly governance audit workflow.
- [ ] The workflow includes the relevant `audit/` scope or governance artifact paths.
- [ ] Any artifact upload or summary output is documented in the PR.

## Review and sign-off

- [ ] Governance owner reviewed the change.
- [ ] Release-readiness reviewer checked the gate criteria.
- [ ] The PR description links to the test template and any supporting evidence.
- [ ] The change is ready to merge without broadening the task scope.

## Copy/paste PR template

```markdown
## Governance Test Update

### Summary
- What governance check is being added or updated?
- Which release gate or audit requirement does it cover?

### Scope
- Files touched:
- Related issue:
- Milestone: Wave 10 Phase 1 - Parallel D+A
- Labels: wave-10, block-d

### Validation
- Command(s) run:
- Result:
- Artifact or evidence link:

### Checklist
- [ ] I used the governance test template.
- [ ] I reviewed the Block D implementation guide.
- [ ] I validated the relevant nightly audit scope.
- [ ] I attached the required GitHub metadata.
```
