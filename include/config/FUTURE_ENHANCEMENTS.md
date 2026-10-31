---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Config Public API — Candidate Work

## Scope

This page records candidate work at the `include/config/` API boundary.
Nothing here is a committed implementation or compatibility promise. Runtime
behavior and current verification gaps are tracked in
[`../../src/config/ROADMAP.md`](../../src/config/ROADMAP.md).

## Candidate API documentation tasks

- [ ] Audit public declarations for useful Doxygen contracts: inputs, results,
  exceptions, ownership/lifetime and thread-safety.
- [ ] Publish a per-header compile and platform-support matrix after checking
  the actual CI/build configuration.
- [ ] Decide and document the intended public stability/ABI policy against
  [`../../VERSIONING.md`](../../VERSIONING.md); do not infer ABI stability from
  header placement alone.
- [ ] Keep `config_contract.h` claims synchronized with implementation and
  focused tests.

## Constraints

- Avoid adding convenience APIs or C++ language-level constraints without a
  demonstrated caller need and compatibility review.
- Preserve the separation between public declarations and internal
  implementation.
- Distinguish the `cms` migration-scanner helper from the runtime API.

## Validation references

- Header inventory: [`README.md`](README.md)
- Runtime behavior: [`../../src/config/API.md`](../../src/config/API.md)
- Contract gaps: [`../../src/config/MODULE_GAPS.md`](../../src/config/MODULE_GAPS.md)
- Focused test registrations: [`../../tests/config/README.md`](../../tests/config/README.md)
