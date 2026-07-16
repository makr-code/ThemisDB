---
name: "Release Readiness Check"
description: "Run a release readiness review for ThemisDB branch transitions with governance, test, and documentation checks."
argument-hint: "Source and target branch, plus release scope (for example: develop -> community)"
agent: "themisdb-reviewer"
---

Assess release readiness for the requested branch transition and scope.

## Inputs

- Release transition and scope: ${input}

## Required Checks

1. Branch governance alignment with BRANCHING_STRATEGY.md and RELEASE_STRATEGY.md.
2. Breaking-change signaling and versioning consistency with VERSIONING.md and CHANGELOG.md.
3. Test evidence quality: focused tests, integration coverage, and known-risk areas.
4. Documentation sync: README, ROADMAP, FUTURE_ENHANCEMENTS, and relevant API docs.
5. Security and hardening readiness for affected modules.

## Output Requirements

Return:

1. Readiness verdict: Ready, Conditionally Ready, or Not Ready
2. Blocking findings with severity and evidence
3. Required pre-release actions
4. Suggested post-release validation watchlist
