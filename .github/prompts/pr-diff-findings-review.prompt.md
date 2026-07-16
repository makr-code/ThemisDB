---
name: "PR Diff Findings Review"
description: "Review the current branch diff against the default branch and produce findings-first results for ThemisDB pull requests."
argument-hint: "Optional scope hint: module, file pattern, or risk focus"
agent: "themisdb-reviewer"
---

Review the current branch diff against the default branch and return a findings-first assessment.

## Inputs

- Optional review focus: ${input}

## Required Steps

1. Determine current branch and default branch baseline.
2. Inspect changed files and the effective diff.
3. Prioritize correctness, security, reliability, and regression risks.
4. Identify missing or weak tests for changed behavior.
5. Identify documentation drift for changed public behavior or APIs.

## Findings Format (Mandatory)

1. Findings (Critical -> High -> Medium -> Low)
2. For each finding: evidence, impact, minimal remediation
3. Open questions or assumptions
4. Residual risk and suggested validation additions

## Constraints

- Do not over-index on style or formatting nits.
- Prefer high-confidence findings with concrete evidence.
- Keep recommendations minimal and actionable.
