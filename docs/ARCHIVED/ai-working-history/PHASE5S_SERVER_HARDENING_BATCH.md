# Phase 5-S Server Hardening Batch

## Scope
- Target: `P5-S01` with supporting `P5-S02` evidence sync
- Repository branch target: `develop`
- This batch is limited to wire-protocol retry/idempotency API hardening and roadmap/changelog synchronization.

## Acceptance Checks
- `RetryPolicy` / `IdempotencyCache` contracts stay backward-compatible for callers in the active major line.
- Idempotency lookups are safe under concurrent mutation and no longer expose unlocked internal storage.
- Zero-capacity idempotency windows fail safe by disabling retention instead of growing unbounded.
- Focused retry tests cover snapshot semantics and zero-window behavior.

## Validation Plan
- Attempt `cmake --preset community-release` before edits to establish baseline.
- Re-run focused retry validation after edits; if full CMake configure is blocked, record the blocker and use focused compilation for the touched retry code path.
