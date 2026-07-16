---
name: "API Change Impact Review"
description: "Analyze impact and compatibility risk of ThemisDB API changes, including call sites, tests, docs, and migration notes."
argument-hint: "Paste changed API symbol, files, or diff summary"
agent: "themisdb-reviewer"
---

Analyze the impact of the provided API change scope and produce a compatibility-focused review.

## Inputs

- API change scope: ${input}
- Optional compatibility requirement: strict backward compatible or intentional breaking change

## Required Checks

1. Call-site impact across modules and tests
2. Behavioral deltas versus previous contract
3. Backward compatibility and migration implications
4. Public documentation and Doxygen sync status
5. Missing validation for failure and edge-case behavior
6. Build and test fallout risks by target/module

## Output Requirements

Return:

1. Findings by severity with file-level evidence
2. Compatibility assessment: compatible, conditionally compatible, or breaking
3. Required migration or rollout notes
4. Recommended focused validation matrix
