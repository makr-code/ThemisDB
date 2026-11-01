---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source level: 1
SOT domain: api-contract
Canonical references: include/scheduler/*.h; include/scheduler/README.md; src/scheduler/FUTURE_ENHANCEMENTS.md
Related issue: #7376; DOC-WEEKLY-2026-44
---

Exact model backend not verifiable from final document text alone.

# Scheduler public-header future enhancements

These are proposals for API-contract review, not committed releases or claims
about current behavior. Runtime and algorithm proposals belong in
[`src/scheduler/FUTURE_ENHANCEMENTS.md`](../../src/scheduler/FUTURE_ENHANCEMENTS.md).

## Candidate work

- Review public declarations and Doxygen comments against implementation
  behavior before marking a contract as guaranteed.
- Consider targeted `noexcept` annotations only after exception behavior is
  established by implementation and tests.
- Add API examples only when they compile against current public headers and
  use valid configuration requirements.
- Define compatibility/ABI policy with the API owner before documenting
  stability markers or version-bump rules.
- Consider module-interface or header-only entry points only after a concrete
  consumer requirement and build support are established.

## Constraints

- Prefer additive changes when appropriate, but do not claim source or ABI
  compatibility without an approved project policy.
- Keep implementation detail and performance targets in the canonical
  `src/scheduler/` documents.
- Do not infer a layer association, dependency guarantee, or stability promise
  from the header directory alone.

## Follow-up

**TODO:** API owners should prioritize these proposals and record any
accepted compatibility policy in the project-level canonical source.
