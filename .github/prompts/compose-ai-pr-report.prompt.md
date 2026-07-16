---
name: "Compose AI PR Report"
description: "Generate a structured PR report for ThemisDB from implemented changes and review findings, including residual risks and release-readiness notes."
argument-hint: "Paste scope: changed files, key fixes, and findings summary"
agent: "themisdb-reviewer"
---

Compose a concise, evidence-based PR report block that can be pasted into the PR description.

## Inputs

- Report scope and notes: ${input}

## Required Content

1. Change summary (what and why)
2. Findings disposition (resolved, accepted, deferred)
3. Validation evidence (build and tests run)
4. Residual risks and follow-up actions
5. Release-readiness note for release-scoped changes

## Output Format

Return markdown with these headings:

- AI Findings Summary
- Changes Implemented
- Validation Evidence
- Residual Risks
- Follow-up Actions
- Release Readiness (if applicable)

## Constraints

- Use concrete evidence only.
- Keep text actionable and auditable.
- Do not include speculative claims.
