---
Author: ThemisDB Contributors
Created: 2026-10-21
Last Updated: 2026-10-21
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: unknown
Human_Review: true
Human_Author: ThemisDB Contributors
---

# Module Guide for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This guide is the entry point for engineers working on the `user_storage_encrypted` module. It is intentionally narrow: it points to the right source artifacts, explains the module boundary, and highlights the operational invariants that matter when making changes.

## Module purpose

The module manages encrypted user storage for ThemisDB using a gocryptfs-backed backend, a key derivation service, a rotation scheduler, and a multi-level storage orchestrator. The source is concentrated in:

- `src/user_storage_encrypted/gocryptfs_backend.cpp`
- `src/user_storage_encrypted/key_derivation_service.cpp`
- `src/user_storage_encrypted/key_rotation_scheduler.cpp`
- `src/user_storage_encrypted/multi_level_storage.cpp`

The public API surface lives under:

- `include/user_storage_encrypted/`

## Recommended reading order

1. `README.md` — overall module purpose and quick boundaries.
2. Existing architecture material in the module directory, if present, for runtime planes and failure domains.
3. `API.md` — public contract and error semantics.
4. `EMBEDDING.md` and `MODELING.md` — module boundaries and runtime design assumptions.
5. `RESEARCH.md` — problem framing, trade-offs, and design rationale.
6. `IMPLEMENTATION.md` — file-by-file implementation logic.
7. `RUNBOOK.md` and `OPERATIONS.md` — diagnosis and normal operations.
8. `TRUST.md` — trust assumptions and risk posture.

## Ownership map

- Public API: `include/user_storage_encrypted/`
- Module implementation: `src/user_storage_encrypted/`
- Build configuration: `src/user_storage_encrypted/CMakeLists.txt`
- Plugin config example: `plugins/user_storage_encrypted/config/storage_config.yaml.example`
- Contract freeze: `include/user_storage_encrypted/user_storage_encrypted_api_contract.h`

## Critical invariants

When changing this module, keep the following invariants in mind:

- fail closed when backend or KDF operations fail
- validate input paths and mount parameters before invoking host commands
- preserve rotation state and scheduling semantics across restarts
- keep stale-mount reconciliation explicit and observable
- avoid silently downgrading from encrypted storage to plaintext storage

## Questions to ask before a change

- Does this change affect backend availability assumptions?
- Does it modify the KDF contract or compatibility assumptions?
- Does it touch rotation intervals or duration checks?
- Does it widen the failure domain for mount or unmount operations?
- Does operator visibility remain intact through logs and metrics?

## Change checklist

Before merging a change in this area, confirm:

- the public API contract still matches the code path
- API or behavior changes are reflected in a relevant document
- metrics or health checks still describe the runtime accurately
- fail-closed semantics still hold
- tests cover the changed lifecycle path

## Known current status

The module is implemented and has production-oriented source evidence, but the roadmap explicitly records continuing hardening work around mount resilience, scheduler diagnostics, and benchmark refreshes. That should be reflected in both code review and maintenance docs. The module is not treated as a fully closed problem space.
