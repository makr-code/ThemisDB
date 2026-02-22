# ThemisDB - MVCC Concurrency Guide

## Overview

This guide covers Multi-Version Concurrency Control (MVCC) as implemented in
ThemisDB, including transaction semantics, lock strategies, thread-safety patterns,
and common pitfalls.

## MVCC Fundamentals

### Version Numbering

Each write creates a new version of a record rather than updating it in place.
Versions are identified by a monotonically increasing timestamp:

```cpp
using Timestamp = uint64_t;

struct RecordVersion {
    Timestamp begin_ts;   // Transaction that created this version
    Timestamp end_ts;     // Transaction that deleted/updated it (kInfinity if live)
    std::vector<uint8_t> data;
};

constexpr Timestamp kInfinity = std::numeric_limits<Timestamp>::max();

// A record's version chain (newest first)
struct VersionChain {
    std::vector<RecordVersion> versions;
};
```

### Read/Write Transaction Semantics

Every transaction obtains a **read timestamp** (snapshot) at `BEGIN` and a
**write timestamp** at `COMMIT`:

```
BEGIN   → assign read_ts  = current_global_ts
READ    → visible version: begin_ts <= read_ts < end_ts
WRITE   → create new version with begin_ts = PENDING (write_ts assigned at commit)
COMMIT  → assign write_ts = ++current_global_ts; set new version begin_ts = write_ts
ABORT   → discard all pending versions
```

```cpp
class Transaction {
public:
    explicit Transaction(Timestamp readTs)
        : read_ts_(readTs), write_ts_(kInfinity), state_(State::Active) {}

    Timestamp readTs()  const { return read_ts_; }
    Timestamp writeTs() const { return write_ts_; }

    void commit(Timestamp ts) {
        write_ts_ = ts;
        state_ = State::Committed;
    }

    void abort() { state_ = State::Aborted; }

    bool isActive()    const { return state_ == State::Active; }
    bool isCommitted() const { return state_ == State::Committed; }

private:
    Timestamp read_ts_;
    Timestamp write_ts_;
    enum class State { Active, Committed, Aborted } state_;
};
```

### Snapshot Isolation vs. Serializable Isolation

| Property              | Snapshot Isolation          | Serializable Isolation              |
|-----------------------|-----------------------------|-------------------------------------|
| Read consistency      | Consistent snapshot at BEGIN| Consistent snapshot at BEGIN        |
| Write-write conflicts | First-writer-wins           | Detected and aborted                |
| Phantom reads         | Possible in range queries   | Prevented                           |
| Performance           | Higher throughput            | Lower throughput (more aborts)      |
| Use in ThemisDB       | Default for OLAP reads       | Required for financial writes       |

ThemisDB uses Snapshot Isolation by default. Enable serializable mode per
transaction when required:

```cpp
auto tx = db.beginTransaction(IsolationLevel::Serializable);
```

## Visibility Rules

A version `v` is visible to transaction `tx` if and only if:

```
v.begin_ts <= tx.read_ts  AND  tx.read_ts < v.end_ts
```

```cpp
bool isVisible(const RecordVersion& v, const Transaction& tx) {
    return v.begin_ts <= tx.readTs() && tx.readTs() < v.end_ts;
}

// Walk the version chain to find the visible version
const RecordVersion* findVisible(const VersionChain& chain,
                                  const Transaction& tx) {
    for (const auto& v : chain.versions) {
        if (isVisible(v, tx)) return &v;
    }
    return nullptr;  // No visible version (record did not exist at read_ts)
}
```

## Lock Strategies in ThemisDB

### Optimistic vs. Pessimistic Locking

| Strategy    | How it works                                  | When to use                        |
|-------------|-----------------------------------------------|------------------------------------|
| Optimistic  | No locks during execution; validate at commit | Low-contention workloads, reads    |
| Pessimistic | Acquire lock before accessing record          | High-contention, critical sections |

ThemisDB defaults to **optimistic concurrency** for reads and uses pessimistic
write locks only when two transactions may modify the same record:

```cpp
// Optimistic: no lock during read, validate at commit
auto readOptimistic(Transaction& tx, const Key& key) -> std::optional<Value> {
    auto* chain = index_.find(key);
    if (!chain) return std::nullopt;
    auto* v = findVisible(*chain, tx);
    if (!v) return std::nullopt;
    tx.addReadSet(key, v->begin_ts);  // Track for validation
    return deserialize(v->data);
}

// Validate read set at commit time
bool validateReadSet(const Transaction& tx) {
    for (const auto& [key, observed_ts] : tx.readSet()) {
        auto* chain = index_.find(key);
        if (!chain) return false;
        // Check that the version we read is still the latest visible one
        auto* current = findVisible(*chain, tx);
        if (!current || current->begin_ts != observed_ts) return false;
    }
    return true;
}
```

### Lock Modes

```cpp
enum class LockMode {
    Read,        // Shared — multiple readers allowed
    Write,       // Exclusive — blocks all other writers and readers
    IntentRead,  // Signals intent to acquire Read locks on children
    IntentWrite  // Signals intent to acquire Write locks on children
};
```

Lock compatibility matrix:

|              | Read | Write | IntentRead | IntentWrite |
|--------------|------|-------|------------|-------------|
| Read         | ✅   | ❌    | ✅         | ❌          |
| Write        | ❌   | ❌    | ❌         | ❌          |
| IntentRead   | ✅   | ❌    | ✅         | ✅          |
| IntentWrite  | ❌   | ❌    | ✅         | ✅          |

### Deadlock Detection and Prevention

**Prevention** (preferred): Use a strict lock ordering to avoid cycles:

```cpp
// Always acquire locks in key-sorted order
void acquireMultipleLocks(Transaction& tx, std::vector<Key>& keys) {
    std::sort(keys.begin(), keys.end());  // Canonical order
    for (const auto& key : keys) {
        lockManager_.acquire(tx.id(), key, LockMode::Write);
    }
}
```

**Detection** (fallback): Use a waits-for graph with periodic cycle detection:

```cpp
class DeadlockDetector {
public:
    // Called when tx_waiter is blocked on tx_holder
    void addWaitsFor(TxId waiter, TxId holder) {
        std::lock_guard lock(graph_mutex_);
        waits_for_[waiter].insert(holder);
    }

    // Returns a transaction to abort if a cycle is found, else nullopt
    std::optional<TxId> detectCycle() {
        std::lock_guard lock(graph_mutex_);
        // DFS-based cycle detection; abort youngest transaction in cycle
        return findCycleVictim(waits_for_);
    }

private:
    std::mutex graph_mutex_;
    std::unordered_map<TxId, std::unordered_set<TxId>> waits_for_;
};
```

## Transaction Lifecycle

```
BEGIN
  └─ assign read_ts = atomic_fetch_add(global_ts, 0)  // snapshot, no increment
       │
       ├─ READ(key) → findVisible(chain, read_ts) → add to read_set
       │
       └─ WRITE(key, value) → create pending version, add to write_set
              │
              ├─ COMMIT
              │    ├─ acquire write locks for all keys in write_set
              │    ├─ validateReadSet() → abort if stale reads detected
              │    ├─ write_ts = atomic_fetch_add(global_ts, 1)  // increment
              │    ├─ finalize all pending versions (begin_ts = write_ts)
              │    ├─ release locks
              │    └─ append to transaction log
              │
              └─ ABORT / ROLLBACK
                   ├─ discard all pending versions
                   └─ release locks
```

### Conflict Resolution

When a write-write conflict is detected at commit time, abort the later
transaction (first-writer-wins):

```cpp
CommitResult commitTransaction(Transaction& tx) {
    std::lock_guard txLock(tx_commit_mutex_);

    // 1. Acquire write locks in canonical order
    for (const auto& key : tx.writtenKeys()) {
        if (!lockManager_.tryAcquire(tx.id(), key, LockMode::Write)) {
            tx.abort();
            lockManager_.releaseAll(tx.id());
            return CommitResult::Conflict;
        }
    }

    // 2. Validate read set (snapshot isolation check)
    if (!validateReadSet(tx)) {
        tx.abort();
        lockManager_.releaseAll(tx.id());
        return CommitResult::StaleRead;
    }

    // 3. Assign write timestamp and finalize
    Timestamp wts = globalTs_.fetch_add(1, std::memory_order_acq_rel) + 1;
    tx.commit(wts);
    applyWriteSet(tx, wts);

    lockManager_.releaseAll(tx.id());
    return CommitResult::Success;
}
```

### Rollback Handling

On abort, no version chain modifications are visible because pending versions
have `begin_ts = kPending`. They are simply removed:

```cpp
void rollbackTransaction(Transaction& tx) {
    for (const auto& [key, pendingVersion] : tx.writeSet()) {
        auto* chain = index_.find(key);
        if (chain) {
            // Remove the pending version from the chain head
            chain->versions.erase(
                std::remove_if(chain->versions.begin(), chain->versions.end(),
                    [&] (const RecordVersion& v) {
                        return v.begin_ts == kPending &&
                               v.tx_id == tx.id();
                    }),
                chain->versions.end()
            );
        }
    }
    tx.abort();
    lockManager_.releaseAll(tx.id());
}
```

## Thread Safety Patterns

### RWLocks for Hot Paths

Use `std::shared_mutex` for data structures with many readers and few writers:

```cpp
class VersionIndex {
public:
    // ✅ Multiple threads can read concurrently
    std::optional<Value> read(const Key& key, Timestamp ts) const {
        std::shared_lock lock(mutex_);  // Shared (read) lock
        auto it = chains_.find(key);
        if (it == chains_.end()) return std::nullopt;
        auto* v = findVisible(it->second, ts);
        return v ? deserialize(v->data) : std::optional<Value>{};
    }

    // ✅ Only one thread writes at a time
    void insertVersion(const Key& key, RecordVersion version) {
        std::unique_lock lock(mutex_);  // Exclusive (write) lock
        chains_[key].versions.insert(chains_[key].versions.begin(),
                                     std::move(version));
    }

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<Key, VersionChain> chains_;
};
```

### Atomic Operations vs. Mutex

Use atomics for simple counters and flags; use a mutex for compound operations:

```cpp
class TransactionManager {
public:
    // ✅ Atomic increment — no lock needed
    Timestamp allocateReadTimestamp() {
        return global_ts_.load(std::memory_order_acquire);
    }

    Timestamp allocateWriteTimestamp() {
        return global_ts_.fetch_add(1, std::memory_order_acq_rel) + 1;
    }

    // ✅ Compound operation — needs mutex
    void registerTransaction(TxId id, Timestamp readTs) {
        std::lock_guard lock(active_tx_mutex_);
        active_transactions_[id] = readTs;
        min_active_ts_ = computeMinTs();  // Depends on full map state
    }

private:
    std::atomic<Timestamp> global_ts_{0};
    std::mutex active_tx_mutex_;
    std::unordered_map<TxId, Timestamp> active_transactions_;
    Timestamp min_active_ts_{0};
};
```

### Memory Ordering (Acquire/Release Semantics)

Use the minimum ordering required to avoid unnecessary barriers:

```cpp
// ✅ Correct: release on write, acquire on read
// Writer thread:
committed_ts_.store(new_ts, std::memory_order_release);
// → All writes above this store are visible to threads that acquire

// Reader thread:
Timestamp ts = committed_ts_.load(std::memory_order_acquire);
// → Sees all writes that happened before the corresponding release

// ✅ For counters with no ordering dependency: relaxed
stats_counter_.fetch_add(1, std::memory_order_relaxed);

// ❌ Avoid: seq_cst unless genuinely needed — adds full fence overhead
committed_ts_.store(new_ts, std::memory_order_seq_cst);
```

## Version Garbage Collection

Old versions that are no longer visible to any active transaction can be
removed. Compute the **minimum read timestamp** across all active transactions:

```cpp
void garbageCollect() {
    Timestamp minTs = transactionManager_.minActiveReadTs();

    for (auto& [key, chain] : index_) {
        // Remove versions whose end_ts <= minTs (no active tx can see them)
        auto& versions = chain.versions;
        versions.erase(
            std::remove_if(versions.begin(), versions.end(),
                [minTs] (const RecordVersion& v) {
                    return v.end_ts != kInfinity && v.end_ts <= minTs;
                }),
            versions.end()
        );
    }
}

// Run GC periodically in a background thread
std::thread gc_thread_([this] {
    while (!shutdown_) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        garbageCollect();
    }
});
```

## Common Pitfalls

### Race Conditions in Concurrent Updates

```cpp
// ❌ Bad: Non-atomic check-then-act (TOCTOU)
if (!index_.contains(key)) {
    index_.insert(key, value);  // Another thread may insert between check and insert
}

// ✅ Good: Atomic insert-or-update with lock
{
    std::unique_lock lock(mutex_);
    index_.emplace(key, value);  // emplace is a no-op if key exists
}
```

### Lost Updates Detection

Lost update: T1 reads X, T2 reads X, T1 writes X, T2 writes X (T1's write lost).

ThemisDB detects this via read-set validation at commit:

```cpp
// If T2's read_set contains (key, T1.readTs) but the current version
// has been updated by T1 (begin_ts = T1.write_ts > T2.read_ts),
// validateReadSet() returns false and T2 is aborted.
```

### Phantom Reads in Range Queries

A phantom read occurs when a range query returns different rows in two reads
within the same transaction because another transaction inserted a row in that
range.

Prevent with predicate locks or range locks:

```cpp
// Register a range predicate in the read set
auto rangeQuery(Transaction& tx, const Key& lo, const Key& hi) {
    std::vector<Value> results;

    std::shared_lock lock(mutex_);
    for (auto it = index_.lower_bound(lo); it != index_.upper_bound(hi); ++it) {
        if (auto* v = findVisible(it->second, tx)) {
            results.push_back(deserialize(v->data));
        }
    }

    // Track the predicate so concurrent inserts into [lo, hi] trigger conflict
    tx.addRangePredicate(lo, hi);
    return results;
}
```

## Testing MVCC Correctness

```cpp
// Isolation: tx2 must not see tx1's uncommitted write
TEST(MVCCTest, SnapshotIsolation_UncommittedWriteNotVisible) {
    auto& db = TestDatabase::instance();
    const Key key = "test_key";
    db.put(key, "original");

    auto tx1 = db.beginTransaction();
    auto tx2 = db.beginTransaction();

    tx1.write(key, "tx1_value");
    // tx1 not committed yet
    auto val = tx2.read(key);
    EXPECT_EQ(val, "original");  // Must see original, not tx1_value

    tx1.commit();
    tx2.commit();
}

// Serializability: concurrent writes must not both succeed
TEST(MVCCTest, WriteWriteConflict_OneTransactionAborts) {
    auto& db = TestDatabase::instance();
    const Key key = "counter";
    db.put(key, "0");

    auto tx1 = db.beginTransaction();
    auto tx2 = db.beginTransaction();

    tx1.write(key, "1");
    tx2.write(key, "2");

    CommitResult r1 = tx1.commit();
    CommitResult r2 = tx2.commit();

    // Exactly one must succeed
    EXPECT_NE(r1 == CommitResult::Success, r2 == CommitResult::Success);
}

// Lost update prevention
TEST(MVCCTest, LostUpdateDetected) {
    auto& db = TestDatabase::instance();
    const Key key = "balance";
    db.put(key, "100");

    auto tx1 = db.beginTransaction();
    auto tx2 = db.beginTransaction();

    int v1 = std::stoi(tx1.read(key).value());  // Both read 100
    int v2 = std::stoi(tx2.read(key).value());

    tx1.write(key, std::to_string(v1 + 50));  // 150
    tx2.write(key, std::to_string(v2 - 30));  // 70

    EXPECT_EQ(tx1.commit(), CommitResult::Success);
    EXPECT_EQ(tx2.commit(), CommitResult::Conflict);  // Lost update prevented

    auto final_val = db.readLatest(key);
    EXPECT_EQ(final_val, "150");  // Only tx1's update applied
}
```

## Performance Considerations

### Transaction Log Size Management

Limit transaction log growth by:

1. **Periodic checkpoints**: Flush dirty pages and truncate the log
2. **Log compaction**: Merge consecutive updates to the same key
3. **TTL on versions**: Configurable version retention window

```cpp
// Checkpoint every N committed transactions
if (committed_count_.fetch_add(1) % CHECKPOINT_INTERVAL == 0) {
    scheduleCheckpoint();
}
```

### Snapshot Consistency Trade-offs

| Isolation Level    | Throughput | Consistency | Recommended For          |
|--------------------|-----------|-------------|--------------------------|
| Read Uncommitted   | Highest   | Lowest      | Never (dirty reads)      |
| Read Committed     | High      | Moderate    | Reporting / analytics    |
| Snapshot Isolation | Moderate  | High        | Default OLTP workloads   |
| Serializable       | Lower     | Strongest   | Financial / audit writes |

### Avoiding Long-Running Transactions

Long-running transactions prevent GC and grow the version chain:

- Set a transaction timeout (default: 30 s) and abort timed-out transactions
- Use read-only transactions (no write set) for analytical queries
- Split large bulk writes into batches with periodic commits

```cpp
// Configure timeout in transaction options
auto tx = db.beginTransaction({
    .timeout_ms = 5000,
    .read_only   = false,
    .isolation   = IsolationLevel::Snapshot
});
```

## Additional Resources

- Code Standards (thread safety patterns): [CODE_STANDARDS.md](CODE_STANDARDS.md)
- Testing Guide (concurrency tests): [TESTING_GUIDE.md](TESTING_GUIDE.md)
- Performance Profiling (threading analysis): [PERFORMANCE_PROFILING.md](PERFORMANCE_PROFILING.md)
- Project Architecture: `../../ARCHITECTURE.md`
