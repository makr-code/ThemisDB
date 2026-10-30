---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: module-behavior
Canonical references: `include/sharding/sharding_api_contract.h`, `include/sharding/shard_router.h`, `src/sharding/shard_router.cpp`, `src/sharding/cross_shard_transaction.cpp`, `tests/sharding/test_sharding_contract_hardening_focused.cpp`
Related issues: #7093, #7167

# Sharding implementation

## Purpose and boundary

For developers changing shard routing, cross-shard transaction handling, WAL/recovery, or migration. The frozen v1.x contract in `include/sharding/sharding_api_contract.h` is normative for the behaviors it specifies.

## Implementation map

- `shard_router.cpp`: routes point operations and query work using the configured resolver/executor.
- `cross_shard_transaction.cpp`: cross-shard transaction state and decision handling.
- `wal_manager.cpp`, `wal_applier.cpp`, and migration sources implement separate durability and topology-change paths; follow the matching public contract before editing.

## Invariants and failure cases

- The contract defines stable key placement for an unchanged ring and bounded fallback when a primary shard is unavailable.
- A transaction left `IN_DOUBT` must block conflicting writes until resolved; preserve that state instead of guessing commit or abort.
- WAL replay corruption is a contract failure, not a successful partial recovery.
- Migration is stateful (planned, transferring, verifying, complete); failed work must follow the contract’s rollback/fault path.
- Routing, transaction, WAL, and migration errors must remain distinguishable through their declared error taxonomy.

## Change and validation steps

1. Locate the relevant section in `sharding_api_contract.h` and the corresponding implementation.
2. Add focused coverage in `tests/sharding/`; the contract regression suite is `test_sharding_contract_hardening_focused.cpp`.
3. Run the affected target registered by `tests/sharding/CMakeLists.txt`; for routing or end-to-end behavior, select the closest sharding core/integration test.

See `API.md`, `MODULE_GUIDE.md`, and `ROADMAP.md` for contracts and module context.
