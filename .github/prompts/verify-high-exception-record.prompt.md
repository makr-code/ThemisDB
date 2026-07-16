---
name: "Verify High Exception Record"
description: "Validate that accepted High findings in a ThemisDB PR contain all required exception fields and evidence."
argument-hint: "Paste PR description text and findings summary"
agent: "themisdb-reviewer"
---

Validate the PR text for completeness of High-finding exception records.

## Inputs

- PR text and findings summary: ${input}

## Validation Target

If any High finding is accepted (not resolved), verify all required fields are present:

1. Finding reference
2. Maintainer approver
3. Mitigation in current release
4. Target fix milestone
5. Tracking issue

Also verify:

- Severity policy reference is present.
- Validation evidence is provided for mitigation.

## Output Requirements

Return markdown with:

1. Verdict: PASS or FAIL
2. Missing fields list (if any)
3. Consistency issues (severity mismatch, unclear disposition)
4. Minimal remediation text that can be pasted into the PR

## Constraints

- Be strict and deterministic.
- Do not infer missing values.
- Use only evidence from the provided input text.
