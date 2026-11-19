---
Author: "ThemisDB Contributors"
Created: "2026-07-17"
Last Updated: "2026-11-19"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: level4
SOT_Domain: architecture-governance
Canonical_References:
  - ../../ROADMAP.md
  - ../../include/sharding/detail/two_phase_commit_contract.h
  - ../../include/sharding/cross_shard_transaction.h
  - ../../include/sharding/two_phase_commit_coordinator.h
  - ../../include/sharding/distributed_transaction.h
  - ../../src/sharding/cross_shard_transaction.cpp
  - ../../src/sharding/two_phase_commit_coordinator.cpp
  - ../../tests/test_two_phase_commit.cpp
Related_Issue: "#5372 (unification planning), #5373 (DIS-05), #7552"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Transaction Coordinators — Architecture Reference

<!-- Status: review | Issues: #5372 (unification planning), #5373 (DIS-05), #7552 | Decision accepted: 2026-11-19 -->
<!-- SOT: coordinator implementation and contracts; this document is a level-4 explanatory derivative -->

## 1. Overview

ThemisDB has multiple concrete transaction coordinators with independent execution APIs and
WAL/recovery implementations. `IRecoverableTwoPhaseCoordinator` normalizes coordinator-local
recovery snapshots; it does not select a canonical runtime coordinator, unify WAL formats, or
provide a system readiness gate. `ITransactionCoordinator` remains a proposed unification target
for migration planning
([`docs/ITRANSACTION_COORDINATOR.md`](../ITRANSACTION_COORDINATOR.md)).

Source inspection currently finds five classes implementing this recovery interface:
`TwoPhaseCommitCoordinator`, `CrossShardTransactionCoordinator`,
`DistributedTransactionCoordinator`, `GlobalTransactionManager`, and
`DistributedTransactionManager`. The table below compares only three of them and is not an
exhaustive inventory. The canonical-coordinator selection is recorded in
[`ADR_DIS-05_CANONICAL_2PC_COORDINATOR.md`](../../audit/ADR_DIS-05_CANONICAL_2PC_COORDINATOR.md):
`TwoPhaseCommitCoordinator` is canonical for direct 2PC. The adapter migration remains open
under [DIS-05](../../audit/ROADMAP.md#ws2--distributed-consistency--ha).

| Class | Header | Namespace | Primary Protocol |
|---|---|---|---|
| `TwoPhaseCommitCoordinator` | `include/sharding/two_phase_commit_coordinator.h` | `themis::sharding` | Standalone 2PC |
| `CrossShardTransactionCoordinator` | `include/sharding/cross_shard_transaction.h` | `themisdb::sharding` | Multi-protocol (2PC / 3PC / SAGA / Percolator / Calvin) |
| `DistributedTransactionCoordinator` | `include/sharding/distributed_transaction.h` | `themis::sharding` | 2PC (+ optional Percolator fast-path) |

The three classes in this comparison inherit from `themis::transaction::IRecoverableTwoPhaseCoordinator`, which
in turn inherits from `themis::transaction::IInDoubtRecoveryCoordinator`. The proposed
`ITransactionCoordinator` unification contract is documented at
[`docs/ITRANSACTION_COORDINATOR.md`](../ITRANSACTION_COORDINATOR.md).

> **Audit Note (CC-5):** Historically these coordinators evolved independently.  As of
> issue #5372 (v2.0.0), they share the `IRecoverableTwoPhaseCoordinator` recovery contract.
> `WALLoggingHelper` centralizes WAL write logic only for `WALManager`-backed coordinators;
> WAL formats remain coordinator-specific. This recovery-interface sharing is distinct from
> DIS-05's execution contract and does not imply repo-wide 2PC unification.
> A transaction started with coordinator A can still only be fully recovered by coordinator A;
> cross-coordinator recovery tooling is planned for v3.0.0 (see §7 below).

> **DIS-05 status (2026-11-19):** The maintainer selected `TwoPhaseCommitCoordinator` as the
> canonical direct 2PC coordinator. `CrossShardTransactionCoordinator` remains the multi-protocol
> facade, but still has its own 2PC participant and persistence callbacks: the shared helper is
> not a completed adapter. The decision does not unify the two WAL formats or their recovery.
> The adapter must define an authoritative durable decision/recovery path before implementation
> can be counted complete. See the accepted
> [`DIS-05 ADR`](../../audit/ADR_DIS-05_CANONICAL_2PC_COORDINATOR.md).
>
> **Prior execution-contract work (2026-11-01):** `CrossShardTransactionCoordinator` and
> `TwoPhaseCommitCoordinator` now share coordinator-side 2PC phase ordering and failure
> semantics through `include/sharding/detail/two_phase_commit_contract.h`. This does not
> unify their transports, transaction lifecycles, WAL formats, or recovery. `CrossShardTransactionCoordinator`'s
> `recoverFromWAL()` restores state and reports resume candidates; it does not automatically
> re-drive phase-two RPCs. The separate `DistributedTransactionCoordinator` is not covered.

---

## 2. Coordinator Comparison

### 2.1 Feature Matrix

| Feature | `TwoPhaseCommitCoordinator` | `CrossShardTransactionCoordinator` | `DistributedTransactionCoordinator` |
|---|---|---|---|
| **Protocols supported** | 2PC | 2PC · 3PC · SAGA · Percolator · Calvin | 2PC (+ optional Percolator fast-path) |
| **2PC execution contract** | Shared with CrossShard under DIS-05 | Shared with standalone 2PC under DIS-05 | Outside DIS-05 |
| **WAL backend** | `WALManager` via `WALLoggingHelper` | `TransactionWAL` (dedicated per-txn WAL) | `WALManager` via `WALLoggingHelper` |
| **Snapshot/state restore** | WAL recovery | WAL + `TransactionSnapshotManager` restore | Own recovery entry point |
| **Phase-two recovery behavior** | Re-drives WAL decision when WAL recovery is configured; resolves only after all acks and completion record | Restores state/reports resume candidates; does not automatically re-drive phase two | Separate coordinator-specific behavior |
| **Deadlock detection** | ✗ (timeout only) | ✅ (distributed wait-for graph) | ✅ (timeout + cycle detection) |
| **Distributed FK validation** | ✗ | ✅ (injected `CrossShardForeignKeyValidator`) | ✗ |
| **Participant model** | `ShardRpcClientAdapter` | `CrossShardParticipant` (state machine per shard) | `TransactionParticipant` (`shard_id` + `endpoint`) |
| **noexcept callbacks** | N/A | Required (see §4) | N/A |
| **Recovery entry point** | `recoverInDoubtTransactions()` | `recoverFromWAL()` | `recoverInDoubtTransactions()` |
| **Thread safety** | ✅ (internal mutex) | ✅ (timed mutex) | ✅ (internal mutex) |

### 2.2 Use-Case Guide — When to Use Which Coordinator

`TwoPhaseCommitCoordinator` is the selected canonical direct 2PC entry point. The CrossShard
2PC implementation currently shares the phase contract but is not yet an adapter. Keep using
`CrossShardTransactionCoordinator` when its broader protocol/lifecycle features are required;
do not infer that its WAL/recovery behavior is interchangeable with the canonical coordinator.
`DistributedTransactionCoordinator` is outside this decision.

| Scenario | Recommended Coordinator |
|---|---|
| Standalone 2PC using the `RequestHandler` participant API / `ShardRPCClientAdapter` | `TwoPhaseCommitCoordinator` (canonical direct 2PC) |
| Multi-protocol orchestration (3PC non-blocking, SAGA long-running, Calvin deterministic), CrossShard lifecycle, SSI, or its endpoint transport | `CrossShardTransactionCoordinator`; its 2PC adapter migration remains open |
| Transaction-manager layer with `IDistributedParticipantCallback` injection | `DistributedTransactionManager` (`transaction/`) |
| New direct 2PC code / greenfield | `TwoPhaseCommitCoordinator`; do not assume its WAL/recovery is interchangeable with another coordinator |

### 2.3 Known Limitations

- **No cross-coordinator recovery (CC-5):** A transaction begun with coordinator A cannot
  be recovered by coordinator B.  Recovery tooling that reads all three WAL formats is
  planned for v3.0.0 (see §7).
- **DIS-05 adapter is not implemented:** The canonical choice is accepted, but
  `CrossShardTransactionCoordinator` still executes its own 2PC callback path. Adapter
  implementation must settle durable-decision ownership and replay mapping before rollout.
- **Incompatible WAL formats (CC-5):** The three coordinators write different JSON schemas
  to their respective WAL backends (see §5 for full specification).  A single unified WAL
  format is a v3.0.0 goal.
- **`CrossShardTransactionCoordinator::persistTransactionState()`** uses a JSONL file log
  for per-state transitions in addition to the `TransactionWAL` backend — the two logs must
  be kept consistent during manual recovery.

---

## 3. State Machine Diagrams

### 3.1 Two-Phase Commit (2PC)

```mermaid
stateDiagram-v2
    [*] --> ACTIVE : begin()
    ACTIVE --> PREPARING : prepare()
    PREPARING --> PREPARED : all votes YES
    PREPARING --> ABORTING : any vote NO / timeout
    PREPARED --> COMMITTING : commit()
    PREPARED --> ABORTING : abort()
    COMMITTING --> COMPLETED : all participants ACK
    ABORTING --> COMPLETED : all participants ACK
    COMPLETED --> [*]
    PREPARING --> FAILED : coordinator crash (WAL recovery required)
    COMMITTING --> FAILED : coordinator crash (WAL recovery required)
```

**Shared execution contract (DIS-05):** For both named coordinators, failed PREPARE selects ABORT;
the configured decision-record callback must succeed before phase two; and a phase-two failure
does not reverse the selected decision or report successful completion. A failed participant
acknowledgement WAL write in CrossShard stops subsequent phase-two fan-out and leaves the decision
unresolved. CrossShard exposes that outcome as `UNKNOWN`; the standalone coordinator returns
`ERROR` and retains its decision in the recoverable transaction view.

**Coordinator-specific recovery:** `TwoPhaseCommitCoordinator::recoverInDoubtTransactions()`
replays its WAL and re-sends a recorded phase-two decision; if no decision exists it conservatively
selects ABORT. It counts a transaction resolved only after participant acknowledgements and a
completion record. `CrossShardTransactionCoordinator::recoverFromWAL()` restores state and reports
resume candidates, but does not automatically re-issue PREPARE/COMMIT/ABORT RPCs. Its
`TransactionWAL`/snapshot format and transport remain separate.

### 3.2 Three-Phase Commit (3PC)

```mermaid
stateDiagram-v2
    [*] --> ACTIVE : begin()
    ACTIVE --> PREPARING : prepare() / CanCommit
    PREPARING --> PREPARED : all CanCommit YES
    PREPARING --> ABORTING : any CanCommit NO / timeout
    PREPARED --> COMMITTING : PreCommit phase (Phase 2a)
    COMMITTING --> COMPLETING : DoCommit phase (Phase 3)
    COMPLETING --> COMPLETED : all participants ACK
    PREPARED --> ABORTING : PreCommit timeout / NACK
    ABORTING --> COMPLETED : abort propagated
    COMPLETED --> [*]
```

**Non-blocking guarantee:** If the coordinator crashes after PreCommit is durable at all
participants but before DoCommit, any participant can safely commit autonomously (no in-doubt
window after PreCommit phase).

> **CST-6:** The `execute3PC()` implementation in `CrossShardTransactionCoordinator` requires
> a PreCommit RPC callback to be registered via `setPreCommitCallback()`.  If no callback is
> installed, `execute3PC()` **fails closed** (abort all participants) rather than silently
> downgrading to 2PC.  See §4.2 for the complete callback contract.

### 3.3 SAGA (Compensating Transaction)

```mermaid
stateDiagram-v2
    [*] --> ACTIVE : begin()
    ACTIVE --> COMMITTING : commit() — execute forward steps
    COMMITTING --> COMPLETED : all steps succeed
    COMMITTING --> ABORTING : step failure — compensate in reverse
    ABORTING --> COMPLETED : all compensations applied
    COMPLETED --> [*]
```

`prepare()` is a no-op for SAGA (returns `OK` immediately).  The coordinator drives
forward execution and backwards compensation internally.

---

## 4. Callback Contracts and noexcept Requirements

### 4.1 State-Change Callback (`CrossShardTransactionCoordinator`)

```cpp
// Signature (CrossShardTransactionCoordinator)
using OnStateChangeFn = std::function<void(
    const std::string& txn_id,
    TransactionState   from,
    TransactionState   to)>;
```

**Contract:**
- The callback **must not throw**.  Any exception propagates to the coordinator's internal
  thread and terminates the transaction with `FAILED` state.
- Must complete within the coordinator's deadlock-detection timeout; avoid blocking I/O.

### 4.2 3PC PreCommit RPC Callback (CST-6)

```cpp
// Signature (CrossShardTransactionCoordinator)
using PreCommitRpcFn = std::function<bool(
    const std::string& shard_id,
    const std::string& txn_id)>;
```

**Contract:**
- **Must not throw.**  If it throws, `execute3PC()` treats the result as a NACK and aborts
  the transaction.
- Returns `true` on participant acknowledgement, `false` on NACK.
- If no callback is registered when `execute3PC()` runs, the transaction **fails closed**
  (abort all participants) — no silent downgrade to 2PC.

### 4.3 Deferred PreCommit Retry Callback (3PC non-blocking mode)

```cpp
// Signature (CrossShardTransactionCoordinator)
using DeferredPreCommitFn = std::function<void(
    const std::string&              txn_id,
    const std::vector<std::string>& failed_shards)>;
```

**Contract:**
- **Must not throw.**  A throw is treated as a fatal orchestration error and causes
  `execute3PC()` to fail closed.
- Callback receives the list of shards that did not acknowledge PreCommit; it is
  responsible for scheduling a retry or escalating.

### 4.4 Participant Callbacks (`IDistributedParticipantCallback`)

Used by `DistributedTransactionManager` (transaction module), not by
`DistributedTransactionCoordinator`.

```cpp
// Interface (include/transaction/distributed_transaction_manager.h)
struct IDistributedParticipantCallback {
    virtual bool onPrepare(const std::string& txn_id,
                           const std::set<std::string>& keys) = 0;
    virtual void onCommit(const std::string& txn_id) = 0;
    virtual void onAbort(const std::string& txn_id) = 0;
};
```

**Contract:**
- `onPrepare()` — may return `false` to cast an ABORT vote; **must not throw** (a throw is
  counted as an ABORT vote).
- `onCommit()` / `onAbort()` — **must not throw**; exceptions are caught and logged.
- All three methods are called from the coordinator's internal thread; avoid coordinator
  re-entrancy (do not call `commit()`/`abort()` from within the callback).

---

## 5. WAL Format Specification

### 5.1 Shared WALEntry JSON Schema (`WALLoggingHelper`)

`TwoPhaseCommitCoordinator` and `DistributedTransactionCoordinator` write entries through
`themis::sharding::WALLoggingHelper` into a `WALManager`-backed binary WAL.  The JSON
payload embedded in `WALEntry::data` follows this schema:

```jsonc
// BEGIN_TX entry (WALEntryType::BEGIN_TX)
{
    "transaction_id":  "string",      // UUID or caller-assigned opaque string
    "coordinator_id":  "string",      // Coordinator instance identifier
    "participants":    [              // Array; may be empty at BEGIN time
        {
            "shard_id": "string",
            "endpoint": "string"
        }
    ],
    "isolation_level": "integer"      // Optional — IsolationLevel cast to int
}

// PREPARE_TX entry (WALEntryType::PREPARE_TX)
{
    "transaction_id":  "string",
    "phase":           "prepare",
    "participants":    [{ "shard_id": "string", "endpoint": "string" }]
}

// COMMIT_TX entry (WALEntryType::COMMIT_TX)
{
    "transaction_id":      "string",
    "phase":               "string",   // e.g. "commit" | "complete"
    "decision":            "commit",
    "commit_timestamp_ns": "integer",  // nanoseconds since epoch (0 if unused)
    "participants":        [
        {
            "shard_id":  "string",
            "endpoint":  "string",
            "prepared":  "bool",
            "committed": "bool"
        }
    ]
}

// ABORT_TX entry (WALEntryType::ABORT_TX)
{
    "transaction_id": "string",
    "phase":          "string",
    "decision":       "abort",
    "reason":         "string",       // Optional human-readable abort reason
    "participants":   [ /* same as COMMIT_TX */ ]
}
```

### 5.2 CrossShardTransactionCoordinator — Dedicated TransactionWAL

`CrossShardTransactionCoordinator` uses the `sharding::TransactionWAL` class (separate from
`WALManager`).  Its log entries are written to a segmented binary log under the configured
WAL directory.  The protocol-specific fields extend the base schema:

For 2PC, this distinct WAL format does not imply a distinct execution contract: CrossShard uses
the same decision ordering and phase-two outcome rules as `TwoPhaseCommitCoordinator`. A failed
participant acknowledgement record stops subsequent phase-two RPCs; the selected decision remains
in-doubt. CrossShard WAL/snapshot recovery currently restores state and reports resume candidates
only—it does not automatically replay that decision to participants.

```jsonc
// 2PC commit entry written by CrossShardTransactionCoordinator
{
    "protocol":     "2PC",
    "participants": [ "shard_id_1", "shard_id_2" ]    // string array of shard IDs
}

// 3PC PreCommit entry
{
    "protocol":     "3PC",
    "participants": [ /* shard IDs */ ]
}
```

Additionally, `persistTransactionState()` writes **JSONL** lines to a separate
`transaction_log_path_` file with the following schema:

```jsonc
{
    "timestamp":          "integer",   // ms since epoch
    "transaction_id":     "string",
    "state":              "integer",   // TransactionState cast to int
    "protocol":           "integer",   // TransactionProtocol cast to int
    "isolation_level":    "integer",
    "snapshot_timestamp": "integer",
    "commit_timestamp":   "integer",
    "participants": [
        {
            "shard_id":  "string",
            "endpoint":  "string",
            "prepared":  "bool",
            "committed": "bool",
            "aborted":   "bool"
        }
    ]
}
```

### 5.3 Format Compatibility Warning

> ⚠️ **The three coordinators write to incompatible WAL formats.**  Do not attempt to replay
> a WAL written by coordinator A through coordinator B.  Cross-format recovery tooling is
> planned for v3.0.0 (see §7).

---

## 6. Decision Tree — Selecting a Coordinator

```
Start: I need distributed transaction support
│
├─► Do I need a protocol other than 2PC, or CrossShard lifecycle/SSI features?
│     YES → CrossShardTransactionCoordinator
│     NO  ↓
│
├─► Am I integrating at the transaction-manager layer using IDistributedParticipantCallback?
│     YES → DistributedTransactionManager
│     NO  ↓
│
└─► Choose the 2PC API that matches participant transport and lifecycle:
      TwoPhaseCommitCoordinator (RequestHandler/adapter, WALManager recovery), or
      CrossShardTransactionCoordinator (ShardRPCClient, TransactionWAL/snapshot restore)
      Both named paths use the same 2PC execution contract; CrossShard does not auto-redrive.
```

**Protocol selection within `CrossShardTransactionCoordinator`:**

```
Start: Which commit protocol for my use case?
│
├─► Need non-blocking coordinator-crash recovery? → 3PC
│     ⚠ Requires PreCommitRpcFn callback (CST-6)
│
├─► Long-running workflow with compensation logic? → SAGA
│
├─► High-contention reads with optimistic locking? → Percolator
│
├─► Deterministic replay / auditing? → Calvin
│
└─► Default: 2PC (simplest, widely supported)
```

---

## 7. Migration Guide

### 7.1 v1.x → v2.0 (Current API Surface)

The current v2.0 coordinators still expose concrete coordinator APIs for execution.
Use the concrete coordinator API for protocol operations, and the shared recovery
contract (`IRecoverableTwoPhaseCoordinator`) for crash-recovery orchestration.

```cpp
// Current v2.0 execution API (TwoPhaseCommitCoordinator)
TwoPhaseCommitCoordinator coord("my-coord");
auto outcome = coord.commit(
    "txn-1",
    {{"shard-a", nlohmann::json::array()}}
);
if (outcome.committed()) {
    // committed
}
```

Recovery contract alignment:
- `recoverInDoubtTransactions()` is part of `IRecoverableTwoPhaseCoordinator`.
- The `ITransactionCoordinator` API in `docs/ITRANSACTION_COORDINATOR.md` is the
  documented migration target for a future unification phase.

### 7.2 v2.0 → v3.0 (Unified WAL Format — Planned)

The v3.0.0 goal is a single `UnifiedTransactionWAL` that all three coordinators write to,
enabling cross-coordinator recovery.  The planned migration steps are:

1. Add `UnifiedTransactionWAL::readEntries(CoordinatorType)` that can parse all three
   current formats.
2. Add a one-time migration tool `themis-wal-migrate` that rewrites v1/v2 WAL segments to
   the unified schema.
3. Swap each coordinator's WAL backend to `UnifiedTransactionWAL` in a single release.

See [ROADMAP.md](../../ROADMAP.md) (section "Distributed Transactions — v3.0.0") for the
current target date and acceptance criteria.

---

## 8. Related Documents

| Document | Content |
|---|---|
| [`docs/ITRANSACTION_COORDINATOR.md`](../ITRANSACTION_COORDINATOR.md) | Full `ITransactionCoordinator` interface reference, type definitions, and migration phases |
| [`include/transaction/transaction_coordinator.h`](../../include/transaction/transaction_coordinator.h) | Canonical interface header |
| [`include/sharding/wal_logging_helper.h`](../../include/sharding/wal_logging_helper.h) | Shared WAL-logging utility (`WALLoggingHelper` namespace) |
| [`include/sharding/two_phase_commit_coordinator.h`](../../include/sharding/two_phase_commit_coordinator.h) | Standalone 2PC coordinator API |
| [`include/sharding/cross_shard_transaction.h`](../../include/sharding/cross_shard_transaction.h) | Multi-protocol coordinator API |
| [`include/sharding/distributed_transaction.h`](../../include/sharding/distributed_transaction.h) | Distributed coordinator API |
| [`tests/transaction/test_itransaction_coordinator.cpp`](../../tests/transaction/test_itransaction_coordinator.cpp) | ITC-1..ITC-41: interface contract tests |
| [`tests/transaction/test_transaction_distributed_2pc.cpp`](../../tests/transaction/test_transaction_distributed_2pc.cpp) | AC-1..AC-20: DistributedTransactionManager 2PC tests |
| [`tests/transaction/test_cross_coordinator_wal_recovery.cpp`](../../tests/transaction/test_cross_coordinator_wal_recovery.cpp) | CWR-1..CWR-8: cross-coordinator WAL recovery contract tests |
