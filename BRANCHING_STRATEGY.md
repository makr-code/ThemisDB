# ThemisDB Branching Strategy

> Status: Active
> Last Updated: 2026-08-28

This file is the canonical branch model reference used by root onboarding and release governance documents.

## Canonical Permanent Branches

- `develop`
- `minimal`
- `community`
- `enterprise`
- `hyperscaler`
- `military`

## Legacy Names (Do Not Use For New Work)

- `main` -> replaced by `community`
- `millitary` -> replaced by `military`

## Rules

1. Default implementation work targets `develop`.
2. Community release work targets `community`.
3. Military release work targets `military`.
4. New PRs, workflows, and docs must not introduce legacy branch names.
5. Branching, release, and versioning updates must stay synchronized across:
   - `BRANCHING_STRATEGY.md`
   - `RELEASE_STRATEGY.md`
   - `VERSIONING.md`
   - `CHANGELOG.md`
   - `ROADMAP.md`

## Supporting Context

- Historical branch-strategy documents are preserved in `docs/ci-cd/branching-release-history/`.
- Root governance references should point to this file.
