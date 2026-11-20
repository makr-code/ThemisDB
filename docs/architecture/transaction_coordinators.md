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
`TwoPhaseCommitCoordinator` is canonical for direct 2PC and owns CrossShard 2PC decisions and
replay. Focused validation and remaining recovery-edge tests remain open under
[DIS-05](../../audit/ROADMAP.md#ws2--distributed-consistency--ha).

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
> most WAL formats remain coordinator-specific. The DIS-05 CrossShard adapter is an explicit
> exception: its 2PC transactions use the canonical coordinator WAL. This recovery-interface sharing is distinct from
> DIS-05's execution contract and does not imply repo-wide 2PC unification.
> Outside that named-pair adapter, a transaction started with coordinator A can still only be fully recovered by coordinator A;
> cross-coordinator recovery tooling is planned for v3.0.0 (see §7 below).

> **DIS-05 status (2026-11-19):** The maintainer selected `TwoPhaseCommitCoordinator` as the
> canonical direct 2PC coordinator. CrossShard's public lifecycle delegates participant dispatch,
> decisions, and replay to that coordinator; CrossShard retains its multi-protocol facade and
> non-2PC WAL/snapshot path. Canonical BEGIN records persist participant endpoints for recovery.
> Unresolved legacy CrossShard-only 2PC records fail startup rather than using a second recovery
> path. Focused validation, decision-write failure coverage, and remaining recovery edges are
> still open. See the accepted
> [`DIS-05 ADR`](../../audit/ADR_DIS-05_CANONICAL_2PC_COORDINATOR.md).
>
> **Prior execution-contract work (2026-11-01):** The two named classes first shared phase
> ordering through `include/sharding/detail/two_phase_commit_contract.h`. The subsequent DIS-05
> adapter moved 2PC participant dispatch and recovery ownership to `TwoPhaseCommitCoordinator`;
> non-2PC CrossShard transports and recovery remain separate. The separate
> `DistributedTransactionCoordinator` is not covered.

---

## 2. Coordinator Comparison

### 2.1 Feature Matrix

| Feature | `TwoPhaseCommitCoordinator` | `CrossShardTransactionCoordinator` | `DistributedTransactionCoordinator` |
|---|---|---|---|
| **Protocols supported** | 2PC | 2PC · 3PC · SAGA · Percolator · Calvin | 2PC (+ optional Percolator fast-path) |
| **2PC execution contract** | Canonical coordinator for direct and CrossShard adapter paths | Public lifecycle/SSI facade over canonical 2PC adapter | Outside DIS-05 |
| **WAL backend** | Canonical `WALManager` / `WALLoggingHelper` WAL for both named entry points | `TransactionWAL` remains for non-2PC protocols | `WALManager` via `WALLoggingHelper` |
| **Snapshot/state restore** | Canonical WAL recovery including endpoint restoration | CrossShard WAL + `TransactionSnapshotManager` restore for non-2PC | Own recovery entry point |
| **Phase-two recovery behavior** | Re-drives canonical WAL decision; resolves only after all acks and completion record | Delegates 2PC replay to canonical coordinator; rejects unresolved legacy CrossShard-only 2PC records | Separate coordinator-specific behavior |
| **Deadlock detection** | ✗ (timeout only) | ✅ (distributed wait-for graph) | ✅ (timeout + cycle detection) |
| **Distributed FK validation** | ✗ | ✅ (injected `CrossShardForeignKeyValidator`) | ✗ |
| **Participant model** | `ShardRpcClientAdapter` | Canonical `ShardRpcClientAdapter` for 2PC; CrossShard participant state for other protocols | `TransactionParticipant` (`shard_id` + `endpoint`) |
| **noexcept callbacks** | N/A | Required (see §4) | N/A |
| **Recovery entry point** | `recoverInDoubtTransactions()` | `recoverFromWAL()` | `recoverInDoubtTransactions()` |
| **Thread safety** | ✅ (internal mutex) | ✅ (timed mutex) | ✅ (internal mutex) |

### 2.2 Use-Case Guide — When to Use Which Coordinator

`TwoPhaseCommitCoordinator` is the selected canonical direct 2PC entry point. Use
`CrossShardTransactionCoordinator` when its broader protocol/lifecycle features are required;
its 2PC lifecycle delegates to the canonical coordinator while its non-2PC persistence remains
separate. `DistributedTransactionCoordinator` is outside this decision.

| Scenario | Recommended Coordinator |
|---|---|
| Standalone 2PC using the `RequestHandler` participant API / `ShardRPCClientAdapter` | `TwoPhaseCommitCoordinator` (canonical direct 2PC) |
| Multi-protocol orchestration (3PC non-blocking, SAGA long-running, Calvin deterministic), CrossShard lifecycle, SSI, or its endpoint transport | `CrossShardTransactionCoordinator`; 2PC uses the canonical adapter |
| Transaction-manager layer with `IDistributedParticipantCallback` injection | `DistributedTransactionManager` (`transaction/`) |
| New direct 2PC code / greenfield | `TwoPhaseCommitCoordinator`; do not assume its WAL/recovery is interchangeable with another coordinator |

### 2.3 Known Limitations

- **No general cross-coordinator recovery (CC-5):** The DIS-05 CrossShard 2PC adapter uses
  canonical 2PC WAL recovery, but other coordinator/WAL pairs remain separate. Recovery tooling
  that reads all coordinator formats is planned for v3.0.0 (see §7).
- **DIS-05 validation is incomplete:** The adapter and canonical WAL ownership are implemented,
  but focused build/test evidence, decision-write failure coverage, and remaining recovery edges
  must pass before release readiness is claimed. Legacy CrossShard-only unresolved 2PC records
  intentionally fail startup.
- **Incompatible WAL formats (CC-5):** The three coordinators write different JSON schemas
  to their respective WAL backends (see §5 for full specification).  A single unified WAL
  format is a v3.0.0 goal.
- **Non-2PC CrossShard persistence:** `persistTransactionState()` writes JSONL state transitions
  alongside its `TransactionWAL`/snapshot backend. 2PC bypasses these writes and uses the
  canonical coordinator WAL instead; recovery tooling must not infer that either legacy log is
  authoritative for an unresolved 2PC transaction.

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

**Canonical execution and recovery (DIS-05):** Both public entry points dispatch 2PC through
`TwoPhaseCommitCoordinator`. Failed PREPARE selects ABORT; a durable decision precedes phase two;
and a phase-two failure does not reverse the selected decision or report successful completion.
CrossShard exposes unresolved outcomes as `UNKNOWN`. With canonical WAL enabled,
`recoverInDoubtTransactions()` restores endpoint-backed participants and re-sends the selected
phase-two decision, counting resolution only after participant acknowledgements and a completion
record. CrossShard `TransactionWAL` and snapshots are still used by its non-2PC protocols.
Unresolved 2PC records present only in legacy CrossShard WAL/snapshots fail startup; no implicit
fallback or conversion is performed.

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
`WALManager`) for its non-2PC protocols. Its log entries are written to a segmented binary log
under the configured WAL directory. CrossShard 2PC no longer writes to this WAL: the canonical
`TwoPhaseCommitCoordinator` WAL owns its BEGIN, decision, completion, and phase-two recovery.
Canonical BEGIN entries retain participant endpoint mappings for restart.

The following 2PC records are legacy CrossShard WAL entries. If they describe unresolved
transactions without a corresponding canonical 2PC record, startup fails closed; the coordinator
does not convert or replay them through a second decision path.

```jsonc
// Legacy 2PC entry that may remain in an existing CrossShard TransactionWAL
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

> ⚠️ **There is no general cross-format WAL replay.** The DIS-05 named pair shares canonical
> 2PC WAL only through the explicit CrossShard adapter; existing CrossShard-only 2PC WAL entries
> are not silently migrated. Other coordinator formats remain incompatible. Broader
> cross-format recovery tooling is planned for v3.0.0 (see §7).

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
└─► Use TwoPhaseCommitCoordinator for direct 2PC.
      Choose CrossShardTransactionCoordinator only when its broader lifecycle,
      SSI, or non-2PC protocol features are also required; its 2PC path delegates
      to the canonical coordinator.
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
