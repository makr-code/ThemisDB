---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Retrieval Module Guide

## Module purpose

`src/retrieval` contains the LoRA artifact metadata/hash library and the source for a separate, currently stubbed dual-read metrics helper. It is not a finished retrieval product surface.

## Primary files

### Documentation

- `README.md` — module overview and context
- `ARCHITECTURE.md` — module boundaries and integration assumptions
- `ROADMAP.md` — verified current boundaries plus an unverified historical plan
- `FUTURE_ENHANCEMENTS.md` — non-normative open questions
- `PRODUCTION_REQUIREMENTS.md` — implementation limits; not a production-readiness contract
- `SECURITY.md` — source-level security boundaries

### Source

- `include/lora_package.h` — artifact declarations exposed by the library target
- `src/lora_package.cc` — serialization, manifest-hash helpers, and in-memory store
- `../../include/retrieval/dual_read_validator.h` and `dual_read_validator.cpp` — separate validator API and implementation; query execution is a placeholder

## How to reason about the module

Use the module in this order:

1. Read the roadmap to understand maturity and what is actually in scope.
2. Check the API file for the public contract and invariants.
3. Read the implementation file to confirm behavior and failure handling.
4. Treat the validation and runbook docs as explanatory; they are not production-readiness approval.

## Change expectations

When modifying the module, keep these rules in mind:

- preserve deterministic hashing and serialization,
- preserve source-backed error behavior,
- do not imply that status fields enforce transitions or that the in-memory store is persistent,
- avoid broad retrieval claims without a matching roadmap update,
- update the runbook when operational steps or failure signals change.

## Recommended workflow for a change

### For a package artifact-data change

- confirm the behavior in `lora_package.h` and `lora_package.cc`,
- validate JSON/hash changes; statuses are represented but transitions are not enforced by this store,
- ensure the security and trust boundaries still hold,
- update `ROADMAP.md` and `RUNBOOK.md` if maturity or operator actions change.

### For a retrieval comparison change

- inspect `dual_read_validator.cpp` and the associated metric definitions,
- confirm how empty or degraded ground truth is handled,
- verify metric helper behavior; `ExecuteQuery()` currently returns empty results and `ValidateQuery()` does not return deltas,
- do not present local thresholds as production configuration.

## Known boundaries

The module is not a blanket replacement for the broader search stack. The manifest store is in-memory and does not enforce trust or lifecycle policy; the dual-read query path does not execute retrieval. Do not infer production readiness from historical roadmap claims.

## Conclusion

Treat this module as a narrow artifact metadata/hash library plus a separate metrics helper. It does not provide a complete retrieval engine, durable registry, lifecycle policy, or production dual-read validation.
