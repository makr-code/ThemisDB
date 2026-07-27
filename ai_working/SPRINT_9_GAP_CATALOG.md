# Sprint 9: Concurrency Gap Catalog

**Date:** 2026-07-27  
**Total Gaps:** 20 concurrency-related violations  
**CWE Categories:** CWE-366 (Data Race), CWE-760 (Predictable Salt)  
**Status:** Ready for implementation starting Week 33 (2026-08-11)

---

## Gap Classification by Module

### Sharding Module (6 gaps) — HIGH PRIORITY

| Gap ID | File | Function | Issue | CWE | Complexity | FP Risk | Pattern |
|--------|------|----------|-------|-----|------------|---------|---------|
| S9-001 | src/sharding/partition_manager.cpp | updatePartitionState() | Unsynchronized partition_state vector access | 366 | Medium | Low | Type A (Data Race) |
| S9-002 | src/sharding/shard_coordinator.cpp | getShard() | Double-checked locking on shard_cache (missing memory barrier) | 366 | Medium | Medium | Type C (DCL) |
| S9-003 | src/sharding/partition_replicas.cpp | notifyReplicaReady() | Lost wakeup on replica_ready condition variable | 366 | Medium | Low | Type B (Lost Wakeup) |
| S9-004 | src/sharding/transaction_coordinator.cpp | acquireLocks() | Deadlock risk: lock ordering not enforced (participant → lock) | 366 | Complex | High | Type A (Deadlock) |
| S9-005 | src/sharding/rebalance_manager.cpp | moveShardData() | Concurrent modification during rebalance (unsync map access) | 366 | High | Low | Type D (Container) |
| S9-006 | src/sharding/metadata_cache.cpp | cacheMetadata() | Race condition on metadata_version atomic (relaxed ordering insufficient) | 366 | Medium | Medium | Type A (Atomic Ordering) |

### Replication Module (5 gaps) — HIGH PRIORITY

| Gap ID | File | Function | Issue | CWE | Complexity | FP Risk | Pattern |
|--------|------|----------|-------|-----|------------|---------|---------|
| S9-007 | src/replication/wal_manager.cpp | syncWALEntry() | Lost wakeup on wal_flush condition variable | 366 | Medium | Low | Type B (Lost Wakeup) |
| S9-008 | src/replication/replica_sync.cpp | startSync() | Data race on sync_state (atomic without acquire-release) | 366 | Medium | Medium | Type A (Data Race) |
| S9-009 | src/replication/log_applier.cpp | applyEntry() | Unsynchronized pending_entries vector during concurrent apply | 366 | Medium | Low | Type D (Container) |
| S9-010 | src/replication/snapshot_manager.cpp | createSnapshot() | Race between snapshot creation and WAL trim | 366 | Complex | Medium | Type A (Complex Race) |
| S9-011 | src/replication/follower_tracker.cpp | trackFollowerProgress() | Lock ordering: global_mutex then per_follower_mutex (not consistently enforced) | 366 | High | Low | Type A (Deadlock) |

### Transaction Module (4 gaps) — MEDIUM PRIORITY

| Gap ID | File | Function | Issue | CWE | Complexity | FP Risk | Pattern |
|--------|------|----------|-------|-----|------------|---------|---------|
| S9-012 | src/transaction/transaction_coordinator.cpp | initParticipant() | Double-checked locking on participants array (missing barriers) | 366 | Medium | Medium | Type C (DCL) |
| S9-013 | src/transaction/lock_manager.cpp | acquireLock() | Potential lost wakeup on lock_available condition variable | 366 | Medium | Low | Type B (Lost Wakeup) |
| S9-014 | src/transaction/2pc_protocol.cpp | prepare() | Race on phase flags during concurrent prepare/abort | 366 | Complex | Medium | Type A (Complex Race) |
| S9-015 | src/transaction/participant.cpp | setState() | Unsynchronized state transitions (state machine race) | 366 | Medium | Low | Type A (Data Race) |

### Network Module (3 gaps) — MEDIUM PRIORITY

| Gap ID | File | Function | Issue | CWE | Complexity | FP Risk | Pattern |
|--------|------|----------|-------|-----|------------|---------|---------|
| S9-016 | src/network/connection_pool.cpp | addConnection() | Unsynchronized vector push_back during resize | 366 | Simple | Low | Type D (Container) |
| S9-017 | src/network/message_queue.cpp | enqueue() | Lost wakeup on queue_not_empty condition variable | 366 | Simple | Low | Type B (Lost Wakeup) |
| S9-018 | src/network/protocol_handler.cpp | processMessage() | Race on connection_state (atomic without ordering) | 366 | Medium | Medium | Type A (Atomic Ordering) |

### Cache Module (2 gaps) — LOW PRIORITY

| Gap ID | File | Function | Issue | CWE | Complexity | FP Risk | Pattern |
|--------|------|----------|-------|-----|------------|---------|---------|
| S9-019 | src/cache/eviction_policy.cpp | evict() | Race condition during concurrent eviction (unsync candidates map) | 366 | Medium | Low | Type D (Container) |
| S9-020 | src/cache/cache_stats.cpp | updateStats() | Data race on stat counters (atomic without proper ordering) | 366 | Simple | Low | Type A (Atomic Ordering) |

---

## Gap Details by Complexity

### Simple Gaps (2 gaps, 0.5h each)

#### S9-016: Network Connection Pool Resize Race

**Module:** network  
**File:** `src/network/connection_pool.cpp`  
**Function:** `addConnection()`

**Problem:**
```cpp
class ConnectionPool {
  std::vector<Connection> connections;  // NOT protected
  
  void addConnection(const Connection& conn) {
    connections.push_back(conn);  // DATA RACE: vector reallocation not atomic
  }
};
```

**Impact:** During vector resize, memory reallocation can cause concurrent iterator/pointer invalidation.

**Fix Pattern:** Add mutex protection around entire vector operation.

**False Positive Risk:** Very Low - straightforward container synchronization.

---

#### S9-017: Message Queue Lost Wakeup

**Module:** network  
**File:** `src/network/message_queue.cpp`  
**Function:** `enqueue()`

**Problem:**
```cpp
class MessageQueue {
  std::condition_variable queue_not_empty;
  std::vector<Message> messages;
  
  Message dequeue() {
    std::unique_lock<std::mutex> lock(mutex);
    queue_not_empty.wait(lock);  // LOST WAKEUP: no predicate!
    if (messages.empty()) return {};  // Consumer woken but queue empty
    return messages.pop_back();
  }
};
```

**Impact:** Woken consumer may race with another consumer draining queue.

**Fix Pattern:** Add predicate to wait() call; check !empty() condition.

**False Positive Risk:** Very Low - clear lost wakeup pattern.

---

### Medium Gaps (14 gaps, 1.5h each average)

#### S9-001: Partition State Race

**Module:** sharding  
**File:** `src/sharding/partition_manager.cpp`  
**Function:** `updatePartitionState()`

**Problem:**
```cpp
class PartitionManager {
  std::vector<PartitionState> partition_state;  // NOT protected
  
  void updatePartitionState(int partition_id, State new_state) {
    partition_state[partition_id].state = new_state;  // DATA RACE
  }
};
```

**Impact:** Multiple threads updating partition state concurrently causes torn writes and stale reads.

**Fix Pattern:** Add mutex, use atomic<State>, or RCU pattern.

**Recommended:** Mutex (simplest, given existing patterns in module).

---

#### S9-002: Shard Cache Double-Checked Locking

**Module:** sharding  
**File:** `src/sharding/shard_coordinator.cpp`  
**Function:** `getShard()`

**Problem:**
```cpp
Shard* getShard(int shard_id) {
  if (shard_cache[shard_id] == nullptr) {  // First check WITHOUT lock
    std::lock_guard<std::mutex> lock(cache_mutex);
    if (shard_cache[shard_id] == nullptr) {  // Second check WITH lock
      shard_cache[shard_id] = createShard(shard_id);  // RACE: write not visible
    }
  }
  return shard_cache[shard_id];  // May see stale nullptr
}
```

**Impact:** Missing memory barriers allow write to shard_cache not visible to first-check reader.

**Fix Pattern:** Use `std::atomic<Shard*>` with acquire-release semantics.

**False Positive Risk:** Medium - DCL patterns often have false positives due to C++ memory model.

---

#### S9-003: Replica Ready Lost Wakeup

**Module:** sharding  
**File:** `src/sharding/partition_replicas.cpp`  
**Function:** `notifyReplicaReady()`

**Problem:**
```cpp
void waitForReplica(int replica_id) {
  std::unique_lock<std::mutex> lock(mutex);
  replica_ready.wait(lock);  // LOST WAKEUP
}

void notifyReplicaReady(int replica_id) {
  std::lock_guard<std::mutex> lock(mutex);
  replicas[replica_id].ready = true;
  replica_ready.notify_one();
}
```

**Impact:** Multiple waiters may miss notification if queue is drained between check and wait.

**Fix Pattern:** Add atomic flag; predicate-based wait.

---

#### S9-006: Metadata Version Atomic Ordering

**Module:** sharding  
**File:** `src/sharding/metadata_cache.cpp`  
**Function:** `cacheMetadata()`

**Problem:**
```cpp
std::atomic<uint64_t> metadata_version;

void cacheMetadata() {
  // Update metadata contents
  metadata = newMetadata;
  
  // Increment version (default memory_order_seq_cst)
  metadata_version++;  // Should use acquire-release semantics
}
```

**Impact:** Relaxed ordering may allow reader to see old metadata despite new version number.

**Fix Pattern:** Use `memory_order_release` for write; `memory_order_acquire` for read.

**False Positive Risk:** Medium - atomic ordering subtlety often misunderstood.

---

#### S9-007: WAL Flush Lost Wakeup

**Module:** replication  
**File:** `src/replication/wal_manager.cpp`  
**Function:** `syncWALEntry()`

**Problem:**
```cpp
void waitForWALFlush() {
  std::unique_lock<std::mutex> lock(wal_mutex);
  wal_flush.wait(lock);  // LOST WAKEUP if multiple flushed between check and wait
}
```

**Impact:** Writer may flush WAL but waiter misses notification.

**Fix Pattern:** Atomic flush counter; predicate-based condition variable.

---

#### S9-008: Replica Sync State Race

**Module:** replication  
**File:** `src/replication/replica_sync.cpp`  
**Function:** `startSync()`

**Problem:**
```cpp
std::atomic<SyncState> sync_state;  // Default: memory_order_seq_cst

void startSync() {
  sync_state = SyncState::IN_PROGRESS;  // May not use acquire-release
}
```

**Impact:** Reader in different thread may see stale state value.

**Fix Pattern:** Explicit memory_order_release for writes; memory_order_acquire for reads.

---

#### S9-009: Pending Entries Vector Race

**Module:** replication  
**File:** `src/replication/log_applier.cpp`  
**Function:** `applyEntry()`

**Problem:**
```cpp
class LogApplier {
  std::vector<LogEntry> pending_entries;  // NOT protected
  
  void applyEntry(const LogEntry& entry) {
    pending_entries.push_back(entry);  // DATA RACE: multiple appliers
  }
};
```

**Impact:** Concurrent push_back on shared vector causes memory corruption during reallocation.

**Fix Pattern:** Add mutex protection.

---

#### S9-010: Snapshot Creation Race

**Module:** replication  
**File:** `src/replication/snapshot_manager.cpp`  
**Function:** `createSnapshot()`

**Problem:** Race between snapshot creation and concurrent WAL trim; trimmed entries may be needed by snapshot.

**Impact:** Snapshot may be incomplete or corrupted.

**Fix Pattern:** Atomic snapshot_in_progress flag; WAL trim checks flag before proceeding.

**False Positive Risk:** Medium - complex distributed semantics.

---

#### S9-012: Participant Array DCL

**Module:** transaction  
**File:** `src/transaction/transaction_coordinator.cpp`  
**Function:** `initParticipant()`

**Problem:** Similar to S9-002; double-checked locking on participants array without memory barriers.

**Impact:** Writer's participant initialization not visible to concurrent reader.

**Fix Pattern:** Use atomic wrapper or explicit memory barriers.

---

#### S9-013: Lock Manager Lost Wakeup

**Module:** transaction  
**File:** `src/transaction/lock_manager.cpp`  
**Function:** `acquireLock()`

**Problem:** Condition variable on lock_available without predicate checking.

**Impact:** Multiple lock requestors may miss wakeup notification.

**Fix Pattern:** Add predicate-based wait with explicit lock availability check.

---

#### S9-014: 2PC Prepare Phase Race

**Module:** transaction  
**File:** `src/transaction/2pc_protocol.cpp`  
**Function:** `prepare()`

**Problem:** Race between prepare, abort, and commit phases on phase flags.

**Impact:** Participant may abort while coordinator thinks prepare succeeded.

**Fix Pattern:** Atomic phase flags; explicit state machine transitions with acquire-release.

---

#### S9-018: Protocol Handler Connection State

**Module:** network  
**File:** `src/network/protocol_handler.cpp`  
**Function:** `processMessage()`

**Problem:** Atomic connection_state without proper memory ordering.

**Impact:** Reader may see inconsistent connection state + message sequence.

**Fix Pattern:** Acquire-release ordering on connection_state updates.

---

#### S9-019: Cache Eviction Candidate Race

**Module:** cache  
**File:** `src/cache/eviction_policy.cpp`  
**Function:** `evict()`

**Problem:** Concurrent eviction threads accessing candidates map without synchronization.

**Impact:** Duplicate evictions; memory corruption.

**Fix Pattern:** Add mutex protection to candidates map.

---

#### S9-020: Cache Statistics Counter Race

**Module:** cache  
**File:** `src/cache/cache_stats.cpp`  
**Function:** `updateStats()`

**Problem:** Data race on stat counters without atomic protection or memory ordering.

**Impact:** Lost updates; incorrect statistics.

**Fix Pattern:** Use std::atomic with memory_order_relaxed (sufficient for stats).

---

### Complex Gaps (4 gaps, 2.5h each)

#### S9-004: Transaction Coordinator Lock Ordering

**Module:** sharding  
**File:** `src/sharding/transaction_coordinator.cpp`  
**Function:** `acquireLocks()`

**Problem:** Potential deadlock due to inconsistent lock ordering across participant acquisition.

**Scenario:**
```
Thread A: Coord Lock → Participant 1 Lock → Participant 2 Lock
Thread B: Coord Lock → Participant 2 Lock → Participant 1 Lock
           → DEADLOCK (circular wait)
```

**Impact:** Critical - can deadlock entire transaction system.

**Fix Pattern:** 
1. Define canonical lock ordering (e.g., by participant ID)
2. Acquire locks in order: Coordinator → Participants (by increasing ID)
3. Document lock hierarchy with code comments
4. Add ThreadSanitizer verification of lock ordering

**False Positive Risk:** High - lock ordering analysis requires deep static analysis.

---

#### S9-005: Rebalance Shard Data Move

**Module:** sharding  
**File:** `src/sharding/rebalance_manager.cpp`  
**Function:** `moveShardData()`

**Problem:** Concurrent shard modifications during rebalance; unsynchronized map access.

**Impact:** Shard state corruption; data loss during move.

**Fix Pattern:** Freeze shard state during move; use version counters to detect concurrent modifications; RCU pattern for readers.

---

#### S9-011: Follower Progress Lock Ordering

**Module:** replication  
**File:** `src/replication/follower_tracker.cpp`  
**Function:** `trackFollowerProgress()`

**Problem:** Lock ordering: global_mutex must precede per_follower_mutex everywhere, but not consistently enforced.

**Scenario:**
```
Thread A: global_mutex → follower_A_mutex
Thread B: follower_A_mutex → global_mutex
         → DEADLOCK
```

**Impact:** Replication system may deadlock under load.

**Fix Pattern:** 
1. Enforce global_mutex always acquired first
2. Add comments explaining hierarchy
3. ThreadSanitizer verification

---

#### S9-015: Participant State Machine Race

**Module:** transaction  
**File:** `src/transaction/participant.cpp`  
**Function:** `setState()`

**Problem:** State transitions not atomic; race between state checks and modifications.

**Scenario:**
```
Thread A: Check state == INIT, then set to PREPARED
Thread B: Simultaneously check state == INIT, then set to PREPARED
         → Both think they performed the transition
```

**Impact:** Incorrect transaction state; protocol violations.

**Fix Pattern:** Atomic state with explicit compare-and-swap; validate transitions are legal.

---

## FP Analysis Reference

From Sprint 8 lessons learned:

- **Wave 1 (simple patterns):** 0% false positive rate
  - Example: straightforward container synchronization
  - Confidence: Very High

- **Wave 2 (medium patterns):** 80% false positive rate
  - Example: complex control flow with safe-by-design patterns
  - Confidence: Medium

- **Wave 3 (complex patterns):** 100% false positive rate
  - Example: distributed system semantics with implicit ordering
  - Confidence: Low

**Sprint 9 Application:**
- Gaps S9-001 to S9-018: Expected to be mostly true positives (Waves 1-2 complexity)
- Gaps S9-004, S9-005, S9-010, S9-011, S9-014, S9-015: High false positive risk (Wave 3 complexity)
- **Conservative Approach:** Require manual code review for all; document safe-by-design patterns thoroughly

---

## Success Criteria per Gap Type

### Type A (Data Race): Fix complete when
- [ ] Mutex or atomic protects all concurrent accesses
- [ ] ThreadSanitizer reports no data race for this location
- [ ] New test case stresses race condition (passes without fix, fails with race)

### Type B (Lost Wakeup): Fix complete when
- [ ] Condition variable uses predicate-based wait()
- [ ] Predicate checked both inside and outside wait()
- [ ] New test case: multiple waiters; verify all woken

### Type C (DCL): Fix complete when
- [ ] Uses atomic<> or explicit memory barriers (acquire-release)
- [ ] ThreadSanitizer reports no data race
- [ ] New test case: multiple threads racing on initialization

### Type D (Container): Fix complete when
- [ ] Mutex protects all push_back/erase operations
- [ ] Vector reallocation cannot race with access
- [ ] New test case: high concurrency stress test

---

**Created:** 2026-07-27  
**Review Date:** 2026-08-01 (before implementation kickoff)  
**Implementation Start:** 2026-08-11
