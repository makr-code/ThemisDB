---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# LLM-L-Wiki developer guide

## Scope

Use this guide when inspecting or changing `src/llm_l_wiki/`. In the current
checkout this path has documentation and a zero-byte `.cpp`, but no module build
target, public header, or module-owned tests. It is not a quickstart for a
runnable service.

## Safe onboarding sequence

1. Read [module features/status](FEATURES.md),
   [architecture](ARCHITECTURE.md), and [implementation inventory](IMPLEMENTATION.md)
   before treating the path as executable.
2. Verify the source inventory and build registration in the current branch.
   The documented inventory can become stale as the repository changes.
3. Confirm intended ownership with maintainers. `llm_l_wiki` is distinct from
   [`src/llm_wiki/`](../llm_wiki/README.md) and
   `plugins/themisdb_llm_wiki/`.
4. If implementation is approved, establish API, input/output invariants,
   failures, and tests before advertising behavior.
5. Link code and tests to the module's direct contract; update these docs when
   that contract changes.

## Build and test

There is no module-specific CMake target or test command established for this
path. Do not report a successful `llm_l_wiki` build based on a build of
`themis_llm_wiki`, `llm_wiki`, or the whole repository. After a target and tests
are added, document the exact configure/build/test commands and their
dependency requirements here.

## Decision references

- [API boundary](API.md)
- [Modeling](MODELING.md) and [embedding](EMBEDDING.md) scope
- [Runbook](RUNBOOK.md) and [operations gates](OPERATIONS.md)
- [Test-density matrix](../../tests/TEST_DENSITY_MATRIX.md)

> AI-assisted documentation. Exact model backend not verifiable from final document text alone.
