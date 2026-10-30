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
Canonical references: `include/transaction/transaction_manager.h`, `include/transaction/lock_manager.h`, `include/transaction/saga.h`, `include/transaction/distributed_transaction_manager.h`, `src/transaction/transaction_manager.cpp`, `src/transaction/lock_manager.cpp`, `tests/transaction/test_transaction_distributed_2pc.cpp`
Related issues: #7093, #7167

# Transaction implementation

## Purpose and boundary

For developers changing local transaction lifecycle, lock coordination, saga compensation, or the distributed transaction manager. Storage durability and sharding coordination are related but separate contracts.

## Implementation map

- `transaction_manager.cpp`: local transaction lifecycle and manager operations.
- `lock_manager.cpp`: lock compatibility, waiting, timeout, and lock result behavior.
- `saga.cpp`: compensating-step handling and retry entry point.
- `distributed_transaction_manager.cpp`: participant voting and explicit distributed transaction states.

## Invariants and failure cases

- Lock results distinguish `GRANTED`, `TIMEOUT`, and `DENIED`; preserve the distinction and 2PL phase behavior.
- Transaction timeouts and deadlock outcomes must remain visible to the caller; do not reinterpret a timeout as a successful commit.
- Saga compensation is a separate action from transaction rollback. Its callbacks can fail; retain the outcome and retry limits.
- `DistributedTxnState` declares `INIT`, `PREPARING`, `PREPARED`, `COMMITTING`, `COMMITTED`, `ABORTING`, and `ABORTED`. Do not document an `IN_DOUBT` enum value for this interface.

## Change and validation steps

1. Identify the local lock, saga, or distributed-state contract being changed.
2. Trace state transitions and cleanup on timeout, rejection, callback failure, and coordinator error.
3. Use `tests/transaction/test_adaptive_deadlock_prevention.cpp`, `test_saga_orchestration_hardening.cpp`, or `test_transaction_distributed_2pc.cpp` as appropriate.
4. Run the affected target registered in `tests/transaction/CMakeLists.txt`.

See `API.md`, `MODULE_GUIDE.md`, `ARCHITECTURE.md`, and `RUNBOOK.md`.
