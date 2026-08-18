# Architecture - Replication Module

<!-- Status: current | validated: 2026-08-18 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->
<!-- Wave A Block 2: Lock Ordering & Timeout Hardening -->

## Overview

The replication module composes replication orchestration, consensus/failover behavior, conflict resolution, logical replication/CDC streaming, and replication observability into a bounded high-availability subsystem.

## Main Execution Planes

1. Core orchestration plane
- replication manager lifecycle and mode control
- leader promotion/failover and topology management

2. Data propagation and conflict plane
- WAL/logical propagation and slot/event stream behavior
- HLC/LWW/CRDT conflict detection and merge behavior

3. Observability and policy plane
- lag/health/topology diagnostics and export behavior
- replication policy validation and assignment behavior

## Core Contracts

| Contract | Behavior |
|---|---|
| replication contract | deterministic init/replicate/promote semantics |
| consensus contract | explicit election/promotion transitions |
| conflict contract | deterministic conflict resolver outcomes per strategy |
| observability contract | explicit lag/health/topology visibility |

## Failure Semantics

- initialization and promotion failures are explicit.
- slot/stream/CDC path faults surface deterministic outcomes.
- conflict-resolution edge cases remain explicit and non-silent.
- degraded replica lag/health is observable via module surfaces.

## Lock Hierarchy (Wave A Block 2 Hardening)

To prevent circular deadlocks and ensure bounded lock contention, the replication module implements a strict multi-level lock hierarchy across all components:

### Level 1: Manager Collection Locks (Outermost)
- **Files:** `replication_slot.cpp`, `multi_tier_replication.cpp`, `logical_replication.cpp`
- **Locks:** `slots_mutex_`, `collection_tiers_mutex_`, `slots_mutex_` (shared_mutex)
- **Scope:** Slot/tier collection access (create/lookup/list)
- **Hold Time:** MINIMAL (~microseconds for map operations only)
- **Pattern:** Acquire shared/unique → map access → release → I/O outside
- **Mutex Type:** `std::mutex` or `std::shared_mutex` depending on read/write ratio

### Level 2: Per-Resource State Locks
- **Files:** `replication_slot.cpp`, `raft_v2.cpp`, `event_stream.cpp`, `logical_replication.cpp`
- **Locks:** `state_mutex_`, `config_mutex_`, `subs_mutex_`, `SlotRuntime::mutex`
- **Scope:** Individual slot/config state (pause/resume/query)
- **Hold Time:** MINIMAL (~microseconds for state copy)
- **Pattern:** Copy state under lock → release → I/O on copy
- **Mutex Type:** `std::mutex` or `std::lock_guard`

### Level 3: Background Worker and I/O Locks (Variable Hold Time)
- **Files:** `async_wal_shipper.cpp`
- **Locks:** `queue_mutex_`, `callback_mutex_`, `stats_mutex_`
- **Scope:** Queues, callbacks, metrics (external I/O operations)
- **Hold Time:** VARIABLE (depends on network, 10ms-1000ms typical)
- **Pattern:** Acquire → quick operation → release → invoke handler outside
- **Mutex Type:** `std::unique_lock` with optional timeout support

### Lock Ordering Diagram (ASCII)

```
Level 1 (Manager Collection)
  ├─→ slots_mutex_ (ReplicationSlotManager)
  ├─→ collection_tiers_mutex_ (MultiTierReplicationManager)
  └─→ slots_mutex_ (LogicalReplicationManager, shared_mutex)
      ↓
Level 2 (Per-Resource State)
  ├─→ state_mutex_ (ReplicationSlot)
  ├─→ config_mutex_ (RaftV2ClusterConfig)
  ├─→ subs_mutex_ (ReplicationEventStream)
  └─→ SlotRuntime::mutex (LogicalReplicationManager)
      ↓
Level 3 (Background I/O)
  ├─→ queue_mutex_ (AsyncWalShipper)
  ├─→ callback_mutex_ (AsyncWalShipper)
  └─→ stats_mutex_ (AsyncWalShipper)
      ↓
[Blocking Operations - NO LOCKS HELD]
  ├─→ File I/O (persist slots, state)
  ├─→ WAL append (wal_->append())
  ├─→ Network I/O (ship handler)
  └─→ Callbacks (event listeners)
```

### Critical Invariants

1. **Acquire-Only Forward Pattern:** Always acquire locks in increasing level order (1→2→3→I/O)
2. **No Backward Locks:** Never acquire Level N lock while holding Level N-1 lock
3. **Lock-Free I/O:** All blocking operations execute OUTSIDE all acquired locks
4. **State Copy Pattern:** Copy mutable state while holding lock, release, then use copy for I/O
5. **Timeout Guards:** Long-running operations (condition variable waits) use timeouts

### Deadlock Prevention Examples

#### ✅ Safe Pattern (Lock-Free I/O)
```cpp
bool ReplicationSlot::pause() {
    SlotState state_copy;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);  // Level 2
        state_.status = SlotStatus::PAUSED;
        state_copy = state_;
    }  // ← Lock RELEASED
    persistStateImpl(state_copy);  // ← I/O happens OUTSIDE lock
    return true;
}
```

#### ❌ Unsafe Pattern (I/O Under Lock) - FIXED in Wave A Block 2
```cpp
// BEFORE (UNSAFE - Lock ordering violation):
MembershipChangeEntry MembershipChangeManager::writeEntry(...) {
    entry = ...;
    wal_->append(wal_entry);  // ← I/O UNDER lock_guard
    return entry;
}

// AFTER (SAFE - I/O outside lock):
auto entry = writeEntry(...);  // ← Create entry under lock
{
    std::lock_guard<std::mutex> lock(mutex_);  // ← Release before I/O
    // ...
}
wal_->append(wal_entry);  // ← I/O outside lock
```

### Timeout Support

All long-running blocking operations use timeouts to ensure bounded wait times:

- **Condition Variable Waits:** `cv.wait_for(lock, timeout, predicate)`
- **Default Timeout:** 1-5 seconds depending on operation
- **Configuration:** Via `replication.timeout_ms` and related config keys

## Sourcecode Verification (Module: replication/architecture)

- Verified files (with lock hierarchy annotations):
  - src/replication/replication_slot.cpp (Level 1→2, lock-free I/O)
  - src/replication/raft_v2.cpp (Level 1→2, fixed WAL lock violation)
  - src/replication/event_stream.cpp (Level 1→2, callbacks outside locks)
  - src/replication/async_wal_shipper.cpp (Level 3, timeout-guarded worker)
  - src/replication/logical_replication.cpp (Level 1→2, shared_mutex)
  - src/replication/multi_tier_replication.cpp (Level 1→2, scope-optimized)

- Verified architecture claims:
  - orchestration + propagation/conflict + observability/policy plane split
  - explicit failure boundaries for init/promotion/slot/conflict behaviors
  - module-local ownership of replication-domain behavior surfaces
  - **NEW (Wave A Block 2):** strict 3-level lock hierarchy enforced
  - **NEW (Wave A Block 2):** all blocking I/O executes lock-free
  - **NEW (Wave A Block 2):** timeout guards on all waits
  - **NEW (Wave A Block 2):** zero circular lock ordering scenarios