---
Author: unknown (original author not verified)
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: AI documentation assistant
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

# Chimera public API follow-up candidates

**Purpose / audience:** possible header-level work inferred from current declarations, not approved milestones.

## Scope, constraints, and required interfaces

Keep declarations aligned with the actual external-or-fallback base contract, document ownership and error behavior, and avoid asserting ABI stability until the selected upstream dependency is identified.

- Establish which `database_adapter.hpp` contract version is authoritative when the external header is present and the fallback shim is selected.
- Document which public methods are simulated, engine-backed, or unavailable for each build configuration.
- Specify ownership, lifetime, thread-safety, and exception/error semantics for adapter, async, stream, statement, retry, batch, and transaction types from implementations rather than broad class-level claims.
- Define compatibility policy only after the external dependency/version and actual ABI requirements are established.
- Add compile-only/public-header validation to the relevant build configuration if adopted by maintainers.

## Implementation notes and test strategy

Record compile definitions and dependency requirements alongside any public declaration change. Validate signatures and include behavior using the same include paths as the supported target; no compile-only check was run for this update.

## Performance and security / reliability

No performance target is proposed. Keep adapter error semantics explicit, avoid exposing secrets through public diagnostics, and specify thread-safety/lifetime guarantees only when supported by implementations.

## References

`database_adapter.hpp`, `themisdb_adapter.hpp`, `retry_executor.hpp`, `batch_executor.hpp`, `transaction.hpp`, and [`../../src/chimera/FUTURE_ENHANCEMENTS.md`](../../src/chimera/FUTURE_ENHANCEMENTS.md).

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
