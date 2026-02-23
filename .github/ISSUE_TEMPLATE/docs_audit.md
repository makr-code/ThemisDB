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

## Acceptance Criteria

- [ ] Documentation accurately reflects the current implementation
- [ ] All internal links are valid
- [ ] Code examples are tested and runnable
- [ ] Content is reviewed by at least one other contributor
- [ ] If applicable: German and English versions are in sync

## Additional Context

Add any relevant context, screenshots, or references here.
