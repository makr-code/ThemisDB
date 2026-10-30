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
SOT domain: api-contract
Canonical references: `include/transaction/transaction_manager.h`, `include/transaction/lock_manager.h`, `include/transaction/saga.h`, `include/transaction/distributed_transaction_manager.h`
Related issues: #7093, #7167

# Transaction API contract

## Public surfaces

- `TransactionManager` coordinates local transaction lifecycle and exposes transaction state/timeout behavior.
- `LockManager::acquireLock()` and `upgradeLock()` return `LockResult` with `GRANTED`, `TIMEOUT`, or `DENIED`; the default lock wait is five seconds. The API also exposes release and two-phase-locking phase operations.
- `Saga` registers compensating callbacks and exposes `compensate()` / `compensateWithRetry()`; retry count defaults to three.
- `DistributedTransactionManager` uses participant callbacks (`onPrepare`, `onCommit`, `onAbort`) and the declared distributed-state enum. A prepare vote can fail; commit/abort operations are not interchangeable.

## Caller obligations and failure cases

- Check lock status before proceeding with protected work; handle timeout and denial separately.
- Treat negative/non-positive transaction timeouts according to `Transaction::setTimeout()` (normalized to no timeout in the header).
- Compensation callbacks may fail; do not claim that a saga is fully compensated without checking its state/metrics.
- Use only the states declared in the specific distributed interface. This enum does not declare `IN_DOUBT`; recovery APIs have their own contract.

## Validation references

Use the closest focused suite: `tests/transaction/test_adaptive_deadlock_prevention.cpp`, `test_saga_orchestration_hardening.cpp`, and `test_transaction_distributed_2pc.cpp`. Targets are registered by `tests/transaction/CMakeLists.txt`.

See `IMPLEMENTATION.md` and `MODULE_GUIDE.md` for source and workflow guidance.
