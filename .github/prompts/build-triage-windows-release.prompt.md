---
name: "Build Triage Windows Release"
description: "Diagnose and fix ThemisDB build or test failures in windows-release with minimal safe changes."
argument-hint: "Paste failing task name, target, and first error lines"
agent: "themisdb-implementer"
---

Triage a failing windows-release build or focused test target and deliver the smallest safe fix.

## Inputs

- Failure context: ${input}
- Optional scope limit: module or file area

## Required Steps

1. Reproduce using the matching workspace task or equivalent command.
2. Classify failure type: compile, link, CMake generate, runtime test, environment lock.
3. Identify first actionable root cause, not downstream noise.
4. Apply minimal code or CMake fix.
5. Rebuild only affected target first, then rerun failing tests.
6. If fixed, summarize root cause and prevention note.

## Constraints

- Avoid broad refactors.
- Do not modify unrelated modules.
- Keep branch governance and C++ documentation rules intact.

## Output Requirements

Return:

1. Root cause
2. Patch summary
3. Validation results
4. Remaining risk and suggested follow-up
