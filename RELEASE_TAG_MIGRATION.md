# Historical Release / Tag Migration Policy

> Status: Active
> Purpose: define how historical releases, tags, and legacy release branches are reassigned to the canonical edition model without losing traceability

## 1. Scope

This document applies to:

- historical release tags
- historical GitHub Releases
- historical release branches
- legacy release lanes such as `main` and `millitary`
- release documentation and changelog traceability

It complements:

- `BRANCHING_STRATEGY.md`
- `RELEASE_STRATEGY.md`
- `VERSIONING.md`
- `CHANGELOG.md`

## 2. Core Principle

A Git tag points to a commit, not to a branch.

Therefore, historical release migration should normally:

1. identify the correct edition
2. ensure the tagged commit is reachable from the canonical edition branch
3. correct documentation and governance references
4. avoid rewriting published tags

## 3. Canonical Mapping

| Historical context | Canonical edition branch |
|---|---|
| `main`-based Community release | `community` |
| `millitary`-based Military release | `military` |
| Minimal release | `minimal` |
| Enterprise release | `enterprise` |
| Hyperscaler release | `hyperscaler` |

## 4. Tag Handling Policy

### 4.1 Published tags

Published or externally consumed tags must normally remain unchanged.

Preferred approach:

- keep the original tag
- move the release commit into the correct canonical branch if needed
- document the historical origin and canonical reassignment

### 4.2 Internal or unpublished tags

Internal or clearly unpublished tags may be corrected, but only with explicit human approval and recorded rationale.

### 4.3 Incorrect tags

If a historical tag is materially wrong:

- do not silently rewrite it
- record the problem
- create a corrected follow-up release/tag only if necessary
- note the correction in release notes, changelog, and migration documentation

## 5. Historical Branch Handling Policy

Historical release branches are not the long-term archive.

Preferred long-term archive sources are:

- tags
- release notes
- changelog entries
- canonical branch history

A historical release branch may be deleted after:

- the release tag exists
- the release notes exist
- the canonical target branch contains the released state
- there are no open PRs or active workflows depending on it

## 6. Required Inventory Fields

Each historical release/tag entry should capture:

- release or tag name
- commit SHA
- historical source branch
- intended edition
- canonical edition branch
- published/unpublished status
- current correction state
- required action
- approval status if exceptional action is needed

## 7. Correction States

Use the following states during migration:

- `aligned` — already consistent with canonical model
- `needs-branch-alignment` — tagged commit must be made reachable from correct branch
- `needs-doc-fix` — docs/release notes/changelog need correction
- `needs-human-audit` — unclear or conflicting evidence
- `safe-to-retag` — unpublished/internal only, explicit approval still required

## 8. Operational Order

Recommended sequence:

1. inventory historical releases/tags
2. classify each entry
3. align canonical branches
4. correct release documentation
5. review exceptional retagging cases separately
6. only then delete obsolete historical release branches

## 9. Community and Military Special Cases

### Community

All historical Community release artifacts tied to `main` should be reassigned logically to `community`.

### Military

All historical Military release artifacts tied to `millitary` should be reassigned logically to `military`.

## 10. Deletion Safety Rule

Do not delete a historical branch until:

- the canonical reassignment is documented
- the required commit history is preserved
- tags and release notes remain traceable
- workflows and protections no longer depend on the old branch

---
Zuletzt geprueft: 2026-06-15
