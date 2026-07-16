# ITransactionCoordinator — Design & Architecture

<!-- Status: current | Issue: #5374 | Target: Q3 2026 -->
<!-- SOT: docs/ITRANSACTION_COORDINATOR.md -->

## 1. Overview

`ITransactionCoordinator` is the unified Strategy-pattern interface for all
commit-protocol coordinators in ThemisDB.  It replaces the fragmented set of
per-protocol APIs and lets callers interact with any coordinator
(2PC, 3PC, SAGA, Percolator, Calvin) through a single, stable contract.

**File:** `include/transaction/transaction_coordinator.h`  
**Namespace:** `themis::transaction`

---

## 2. Interface Hierarchy (ASCII-UML)

```
themis::transaction::ITransactionCoordinator
│
│  (common lifecycle + WAL/recovery contract)
│
├─ [2PC / 3PC coordinators]
│    Implements:  ITransactionCoordinator
│                 IRecoverableTwoPhaseCoordinator  ← IInDoubtRecoveryCoordinator
│    Classes:
│      TwoPhaseCommitCoordinator        (sharding/two_phase_commit_coordinator.h)
│      GlobalTransactionManager         (transaction/global_transaction_manager.h)
│      DistributedTransactionManager    (transaction/distributed_transaction_manager.h)
│      CrossShardTransactionCoordinator (sharding/cross_shard_transaction.h)
│
├─ [SAGA coordinators]
│    Implements:  ITransactionCoordinator
│    Classes:
│      SagaOrchestrator                 (transaction/saga_orchestrator.h)
│      DistributedSaga                  (transaction/distributed_saga.h)
│    Notes:
│      prepare() returns OK immediately (no voting round).
│      commit() executes forward steps.
│      abort()  executes compensating actions in reverse order.
│
├─ [Percolator coordinator]
│    Implements:  ITransactionCoordinator
│    Classes:
│      PercolatorCoordinator            (sharding/cross_shard_transaction.h)
│    Notes:
│      prepare() is a no-op (returns OK).
│      commit() performs prewrite + commit-timestamp write.
│      abort()  releases primary and secondary lock cells.
│
└─ [Calvin coordinator]
     Implements:  ITransactionCoordinator
     Classes:
       (via CrossShardTransactionCoordinator with CALVIN protocol flag)
     Notes:
       prepare() is a no-op (returns OK).
       commit() applies deterministically ordered write batch.
       abort()  discards the queued batch.
```

---

## 3. Key Types

### 3.1 `CommitProtocol` (enum class)

| Value              | Description                                              |
|--------------------|----------------------------------------------------------|
| `TWO_PHASE_COMMIT` | Classical blocking 2PC (Gray 1978)                       |
| `THREE_PHASE_COMMIT` | Non-blocking 3PC: CanCommit / PreCommit / DoCommit     |
| `SAGA`             | SAGA with per-step compensating actions                  |
| `PERCOLATOR`       | Google Percolator optimistic MVCC protocol               |
| `CALVIN`           | Calvin deterministic pre-ordered execution               |
| `CUSTOM`           | User-defined extension protocol                          |

### 3.2 `CoordinatorCapabilities` (struct)

| Field                      | Protocols         | Meaning                              |
|----------------------------|-------------------|--------------------------------------|
| `supports_prepare_phase`   | 2PC, 3PC          | prepare() drives a voting round      |
| `supports_pre_commit`      | 3PC               | Intermediate PreCommit phase exists  |
| `supports_compensation`    | SAGA              | abort() executes compensating steps  |
| `supports_optimistic_mvcc` | Percolator        | Optimistic read-then-CAS writes      |
| `supports_deterministic`   | Calvin            | Write set is deterministically ordered|
| `supports_wal_recovery`    | All with WAL      | recoverInDoubt() uses durable WAL    |
| `supports_snapshot_read`   | Percolator, MVCC  | MVCC snapshot reads exposed          |

### 3.3 `TxnCoordinatorResult` (struct)

```cpp
TxnCoordinatorResult r = coord->begin("txn-1");
if (!r) {
    switch (r.code) {
        case ErrorCode::INVALID_STATE:   /* duplicate ID */  break;
        case ErrorCode::INTERNAL_ERROR:  /* WAL failure */   break;
        // …
    }
}
```

Error codes: `NONE`, `UNKNOWN_TRANSACTION`, `INVALID_STATE`, `PARTICIPANT_ABORT`,
`TIMEOUT`, `RECOVERY_NEEDED`, `INTERNAL_ERROR`.

### 3.4 `TxnCoordinatorOptions` (struct)

```cpp
TxnCoordinatorOptions opts;
opts.isolation = themis::IsolationLevel::SERIALIZABLE;
opts.deadline  = std::chrono::system_clock::now() + 5s;
opts.metadata  = "session=abc123";
coord->begin("txn-42", opts);
```

### 3.5 `InDoubtTxnDescriptor` (struct)

Returned by `getInDoubtTransactions()`:
```cpp
for (const auto& d : coord->getInDoubtTransactions()) {
    spdlog::warn("in-doubt: {} prepare_logged={} commit_decided={}",
                 d.txn_id, d.prepare_logged, d.commit_decided);
}
```

---

## 4. Lifecycle State Machine

States use the `TxnLifecycleState` enum defined in `transaction_coordinator.h`.
Single-round protocols (SAGA, Percolator, Calvin) use `ACTIVE` for the in-flight
state and `COMPLETED` for the terminal state; the intermediate states
(`PREPARING`, `PREPARED`, `COMMITTING`, `ABORTING`) are only used by voting
protocols (2PC, 3PC).

```
               begin()
  [NONE] ──────────────────► [ACTIVE]
                                  │
                  prepare()        │   (voting protocols)
              ┌───────────────┐   │
              ▼               │   │
         [PREPARING] ─────────┘   │   (no-op for SAGA/Percolator/Calvin)
              │                   │
              │ all voted COMMIT   │
              ▼                   │
          [PREPARED] ◄────────────┘   (or ACTIVE for single-round)
              │
    commit()  │  abort()
     ┌────────┼────────────┐
     ▼                     ▼
 [COMMITTING]          [ABORTING]
     │                     │
     ▼                     ▼
 [COMPLETED]          [COMPLETED]
     │                     │
     └─────────────────────┘
         (terminal states)

  [FAILED] ← recoverInDoubt() if WAL replay cannot make progress
```

---

## 5. WAL/Recovery Contracts (Per Method)

| Method              | WAL record written     | Recovery guarantee                            |
|---------------------|------------------------|-----------------------------------------------|
| `begin()`           | `BEGIN`                | ID is durable; restart skips re-begin         |
| `prepare()`         | `PREPARE_OK/ABORT`     | Restart re-drives to terminal state           |
| `commit()`          | `COMMIT_DECISION`      | Written before contacting any participant     |
| `abort()`           | `ABORT_DECISION`       | Written before returning to caller            |
| `recoverInDoubt()`  | *(re-drives existing)* | Every prepared txn reaches terminal state     |

For SAGA: `commit()` writes each step's forward result; `abort()` writes each
compensating action's result.  On restart, only the unexecuted actions are
re-driven.

---

## 6. Protocol-Specific Behaviour of `prepare()`

| Protocol   | prepare() behaviour                                          |
|------------|--------------------------------------------------------------|
| 2PC        | Sends PREPARE to all participants, collects YES/NO votes     |
| 3PC        | Sends CanCommit; non-blocking PreCommit follows internally   |
| SAGA       | Returns `OK` immediately; no distributed vote                |
| Percolator | Returns `OK` immediately; locking is done inside `commit()`  |
| Calvin     | Returns `OK` immediately; ordering is done before `begin()`  |

---

## 7. Migration Plan for Existing Coordinators

### Phase 1 — Adapter Wrappers (non-breaking, Q3 2026)

Add `ITransactionCoordinator` to the `public` inheritance list of each
existing coordinator without changing any existing method signatures:

```
TwoPhaseCommitCoordinator
  : public IRecoverableTwoPhaseCoordinator   // existing
  , public ITransactionCoordinator           // new
```

Implement the new interface methods by delegating to the existing private
implementation.  Existing callers are unaffected.

**Affected files (Phase 1):**

| Class                          | Header                                          |
|--------------------------------|-------------------------------------------------|
| `TwoPhaseCommitCoordinator`    | `include/sharding/two_phase_commit_coordinator.h` |
| `GlobalTransactionManager`     | `include/transaction/global_transaction_manager.h` |
| `DistributedTransactionManager`| `include/transaction/distributed_transaction_manager.h` |
| `CrossShardTransactionCoordinator` | `include/sharding/cross_shard_transaction.h` |
| `PercolatorCoordinator`        | `include/sharding/cross_shard_transaction.h`    |
| `SagaOrchestrator`             | `include/transaction/saga_orchestrator.h`       |
| `DistributedSaga`              | `include/transaction/distributed_saga.h`        |

### Phase 2 — Caller Migration (Q4 2026)

Migrate call sites (connection handlers, planner, integration tests) to accept
`ITransactionCoordinator*` or `std::unique_ptr<ITransactionCoordinator>`
instead of concrete types.  Introduce `CoordinatorFactory` to centralise
construction.

### Phase 3 — Legacy API Deprecation (Q1 2027)

Annotate the per-class non-interface lifecycle methods (`beginTransaction`,
`prepareDistributed`, `commitDistributed`, …) with `[[deprecated]]`.  They
remain functional through the v2.x line.

### Phase 4 — Removal (v3.0.0)

Remove deprecated per-class methods.  Coordinators expose only the unified
interface.  This is the only breaking change in the migration path.

---

## 8. Mock Pattern for Tests

```cpp
#include "transaction/transaction_coordinator.h"
#include <gtest/gtest.h>

class MockCoordinator : public themis::transaction::ITransactionCoordinator {
public:
    themis::transaction::CommitProtocol protocolType() const noexcept override {
        return themis::transaction::CommitProtocol::TWO_PHASE_COMMIT;
    }
    std::string_view protocolName() const noexcept override { return "2PC-mock"; }

    themis::transaction::CoordinatorCapabilities capabilities() const noexcept override {
        themis::transaction::CoordinatorCapabilities caps;
        caps.supports_prepare_phase = true;
        caps.supports_wal_recovery  = true;
        return caps;
    }

    themis::transaction::TxnCoordinatorResult begin(
        std::string_view, const themis::transaction::TxnCoordinatorOptions&) override {
        return themis::transaction::TxnCoordinatorResult::OK();
    }
    themis::transaction::TxnCoordinatorResult prepare(std::string_view) override {
        return themis::transaction::TxnCoordinatorResult::OK();
    }
    themis::transaction::TxnCoordinatorResult commit(std::string_view) override {
        return themis::transaction::TxnCoordinatorResult::OK();
    }
    themis::transaction::TxnCoordinatorResult abort(std::string_view) override {
        return themis::transaction::TxnCoordinatorResult::OK();
    }
    themis::transaction::TxnLifecycleState getState(std::string_view) const override {
        return themis::transaction::TxnLifecycleState::UNKNOWN;
    }
    std::size_t recoverInDoubt() override { return 0; }
    std::vector<themis::transaction::InDoubtTxnDescriptor> getInDoubtTransactions() const override {
        return {};
    }
};
```

---

## 9. Acceptance Criteria (Issue #5374)

| Criterion | Status | Evidence |
|-----------|--------|----------|
| Specification (Header, methods, UML/comment diagram) | ✅ Done | `include/transaction/transaction_coordinator.h` + this document |
| Interface is C++ OOP/SOLID-compliant | ✅ Done | Doxygen SOLID analysis in header; ISP via capabilities(), DIP via interface injection |
| Migration plan for existing coordinators | ✅ Done | Section 7 above (4 phases, Q3 2026 → v3.0.0) |
| Design described in architecture doc/README | ✅ Done | This document + `include/transaction/ARCHITECTURE.md` updated |

---

## 10. References

- `include/transaction/transaction_coordinator.h` — authoritative interface header
- `include/transaction/recoverable_two_phase_coordinator.h` — 2PC recovery base
- `include/transaction/in_doubt_recovery_coordinator.h` — in-doubt recovery base
- `include/transaction/isolation_level.h` — canonical isolation level enum
- `include/sharding/cross_shard_transaction.h` — CrossShardTransactionCoordinator
- `include/sharding/two_phase_commit_coordinator.h` — TwoPhaseCommitCoordinator
- `include/transaction/distributed_transaction_manager.h` — DistributedTransactionManager
- `include/transaction/global_transaction_manager.h` — GlobalTransactionManager
- `include/transaction/saga_orchestrator.h` — SagaOrchestrator
- `include/transaction/distributed_saga.h` — DistributedSaga
- `tests/transaction/test_itransaction_coordinator.cpp` — contract tests
