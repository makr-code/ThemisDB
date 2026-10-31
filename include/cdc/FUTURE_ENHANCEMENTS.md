---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source level: "level1"
SOT domain: "api-contract"
Canonical source references: "src/cdc/FUTURE_ENHANCEMENTS.md; src/cdc/ROADMAP.md; include/cdc/cdc_delivery_contract.h; include/cdc/README.md"
Related milestone: "DOC-WEEKLY-2026-44"
---

# CDC public API future enhancements

## Scope

This page covers public-header contracts only. It does not define a separate
feature backlog; implementation and benchmark work is tracked in
[`src/cdc/FUTURE_ENHANCEMENTS.md`](../../src/cdc/FUTURE_ENHANCEMENTS.md) and
[`src/cdc/ROADMAP.md`](../../src/cdc/ROADMAP.md).

## Design constraints

- Preserve the sequence, delivery, replay, and failure invariants in
  [`cdc_delivery_contract.h`](./cdc_delivery_contract.h).
- Review public signature or compatibility changes against the repository's
  versioning policy before implementation.
- Describe build-conditional transports with their actual feature guards;
  declaration availability alone does not establish runtime support.

## Required interfaces

The current public declarations are indexed in
[`README.md`](./README.md). Consult the corresponding header for exact
signatures, ownership, configuration, and failure behavior rather than
duplicating those details here.

## Implementation notes

Keep API changes source-compatible unless a breaking change is explicitly
approved and documented. Keep build guards and ownership/failure behavior
consistent with the implementation and delivery contract.

## Test strategy

Validate changes with the focused CDC tests under
[`tests/cdc/`](../../tests/cdc/) and update API-facing documentation in the
same change. Do not treat a proposed enhancement or an existing header
declaration as evidence that a runtime capability is complete.

## Performance targets

No separate public-header performance target is defined here. Performance
changes and benchmark evidence are tracked in
[`src/cdc/PERFORMANCE_EXPECTATIONS.md`](../../src/cdc/PERFORMANCE_EXPECTATIONS.md)
and the canonical implementation roadmap.

## Security and reliability

Preserve the contract's acknowledgement and fail-closed rules. Authentication
and authorization remain the responsibility of the hosting service unless a
specific CDC interface documents otherwise.

**Exact model backend not verifiable from final document text alone.**
