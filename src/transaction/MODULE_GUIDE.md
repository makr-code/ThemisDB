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
Canonical references: `include/transaction/`, `src/transaction/`, `tests/transaction/CMakeLists.txt`
Related issues: #7093, #7167

# Transaction module guide

## Audience and ownership

For developers changing transaction lifecycle, isolation/locking, saga compensation, or distributed transaction state. Use storage documentation for persistence guarantees and sharding documentation for cross-shard contract details.

## Start with the state owner

- Local lifecycle: `transaction_manager.h` / `transaction_manager.cpp`.
- Lock grants/waits: `lock_manager.h` / `lock_manager.cpp`.
- Compensation: `saga.h` / `saga.cpp`.
- Distributed callbacks/state: `distributed_transaction_manager.h` / `distributed_transaction_manager.cpp`.

## Maintainer workflow

1. Identify the transaction state machine and the component that owns it.
2. Record success, timeout/denial, callback failure, and cleanup paths before changing behavior.
3. Preserve explicit lock outcomes and use only states present in that public interface.
4. Add focused tests in `tests/transaction/`; run the registered test target from the module `CMakeLists.txt`.
5. Update `API.md` when callers observe a contract change and `IMPLEMENTATION.md` when internal transitions change.

Useful focused tests include `test_adaptive_deadlock_prevention.cpp`, `test_saga_orchestration_hardening.cpp`, and `test_transaction_distributed_2pc.cpp`.
