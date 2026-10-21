---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Retrieval Module Guide

## Module purpose

`src/retrieval` is the code and documentation boundary for retrieval-oriented trust, artifact lifecycle, and evaluation logic in ThemisDB. The source currently reflects a staged implementation rather than a finished retrieval product surface.

## Primary files

### Documentation

- `README.md` — module overview and context
- `ARCHITECTURE.md` — module boundaries and integration assumptions
- `ROADMAP.md` — implementation phases and maturity status
- `FUTURE_ENHANCEMENTS.md` — planned work and phase targets
- `PRODUCTION_REQUIREMENTS.md` — mandatory deployment requirements
- `SECURITY.md` — security posture and controls

### Source

- `include/lora_package.h` — public artifact contract
- `src/lora_package.cc` — implementation of hashing, serialization, and verification
- `dual_read_validator.cpp` — metric-driven retrieval regression checks

## How to reason about the module

Use the module in this order:

1. Read the roadmap to understand maturity and what is actually in scope.
2. Check the API file for the public contract and invariants.
3. Read the implementation file to confirm behavior and failure handling.
4. Use the validation and runbook docs to decide whether a change is safe for production-facing use.

## Change expectations

When modifying the module, keep these rules in mind:

- preserve deterministic hashing and serialization,
- maintain explicit failure behavior,
- keep status transitions observable and auditable,
- avoid broad retrieval claims without a matching roadmap update,
- update the runbook when operational steps or failure signals change.

## Recommended workflow for a change

### For a package lifecycle change

- confirm the behavior in `lora_package.h` and `lora_package.cc`,
- validate any JSON or status transition changes,
- ensure the security and trust boundaries still hold,
- update `ROADMAP.md` and `RUNBOOK.md` if maturity or operator actions change.

### For a retrieval comparison change

- inspect `dual_read_validator.cpp` and the associated metric definitions,
- confirm how empty or degraded ground truth is handled,
- verify that regression detection remains transparent and testable,
- document any new operational threshold in `OPERATIONS.md` or `RUNBOOK.md`.

## Known boundaries

The module is not a blanket replacement for the broader search stack. It is a controlled set of retrieval trust and validation features layered under the active roadmap. That means a reviewer should be skeptical of any change that claims generalized production retrieval readiness without corresponding roadmap evidence.

## Conclusion

Treat this module as a disciplined contract and quality layer: source-of-truth artifact metadata, integrity enforcement, and explicit retrieval quality assessment. It is most valuable when it stays narrow, auditable, and honest about maturity.
