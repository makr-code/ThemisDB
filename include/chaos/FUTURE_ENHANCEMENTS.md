---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: OpenAI
Human_Review: false
Human_Author: ThemisDB Contributors
---

Exact model backend not verifiable from final document text alone.

# Chaos public-header future enhancements

## Scope

This is a proposal list for possible public-header changes. None of the items
below is evidence of an approved schedule or implemented capability. Runtime
plans belong in [`../../src/chaos/FUTURE_ENHANCEMENTS.md`](../../src/chaos/FUTURE_ENHANCEMENTS.md).

## Design constraints

- `include/chaos/chaos_contract.h` describes v1.x compatibility expectations.
- Its recommended limits are not all enforced by the current runtime; additions
  must distinguish normative contract text from implementation behavior.
- Future entry points should remain additive unless a versioned breaking change
  is explicitly approved.

## Proposals (TODO: confirm priority and approval)

- Consider deprecation annotations and migration notes for legacy entry points.
- Consider explicit `noexcept` and structured diagnostic types where the
  implementation contract supports them.
- Consider convenience factories, ABI markers, or a C++20 module wrapper only
  after compatibility and build support are established.

The former `LLM/Graph` layer association and reference to `FUTURE_PLAN.md` are
not evidenced by these headers; treat that association as **TODO/assumption**,
not as a current module classification.
