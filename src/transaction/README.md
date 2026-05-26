> **Build:** `cmake --preset <platform>-release && cmake --build --preset <platform>-release` (e.g. `linux-release`, `windows-release`)

# ThemisDB Transaction Module

## Module Purpose

The Transaction module provides ThemisDB's ACID-compliant transaction management system with support for distributed transactions, MVCC (Multi-Version Concurrency Control), deadlock detection, and Git-like versioning capabilities. Built on RocksDB's native transaction support, it delivers atomic multi-layer updates across relational tables, graph edges, secondary indexes, and vector embeddings with configurable isolation levels and the SAGA pattern for long-running distributed transactions.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `transaction_manager.cpp` | ACID transaction lifecycle: begin, commit, rollback |
| `saga.cpp` | SAGA pattern: step execution + compensating actions |
| `saga_orchestrator.cpp` | SAGA orchestration and multi-step coordination |
| `distributed_saga.cpp` | Distributed SAGA across multiple nodes |
| `distributed_transaction_manager.cpp` | Two-Phase Commit (2PC) protocol implementation |
| `global_transaction_manager.cpp` | Multi-region ACID guarantees with TrueTime 2PC |

## Module Documentation Links

- [Public Header Overview](../../include/transaction/README.md)
- [Roadmap](./ROADMAP.md)
- [Future Enhancements](./FUTURE_ENHANCEMENTS.md)
- [Architecture](./ARCHITECTURE.md)
- [German Module Overview](../../docs/de/transaction/README.md)

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — ACID/MVCC transaction engine with SAGA and distributed transaction capabilities is production-grade.

## Scope

**In Scope:**
- ACID transaction guarantees (Atomicity, Consistency, Isolation, Durability)
- Session-based transaction lifecycle (begin → operations → commit/rollback)
- Isolation levels: ReadCommitted (default), Snapshot (point-in-time consistency), Serializable (SSI – prevents write skew and phantom reads)
- MVCC via RocksDB native transactions
- SAGA pattern for distributed transactions with compensating actions
- Deadlock detection with configurable timeout
- Lock tracking and waiting-for graph analysis
- Transaction statistics and monitoring
- Atomic multi-layer updates (relational, graph, vector, secondary indexes)
- Git-like branching and merging (BranchManager, MergeEngine)
- Named snapshots/tags for point-in-time recovery (SnapshotManager)
- Integration with Changefeed for CDC

**Out of Scope:**
- Data storage implementation (handled by storage module)
- Index structure management (handled by index module)
- Query parsing and execution (handled by query module)
- Network protocol handling (handled by server module)

## Key Components

### TransactionManager
**Location:** `transaction_manager.cpp`, `../include/transaction/transaction_manager.h`

Core transaction coordinator providing ACID guarantees through RocksDB WriteBatch.

**Features:**
- **Isolation Levels**: ReadCommitted (default), Snapshot (⚠️ write skew possible — see [Isolation Level Selection](#isolation-level-selection)), Serializable (SSI)
- **MVCC Support**: Multi-version concurrency control via RocksDB transactions
- **Atomic Updates**: Single WriteBatch for all layers (relational, graph, vector, indexes)
- **Deadlock Detection**: Background thread monitors lock wait graph
- **Transaction Statistics**: Total begun/committed/aborted, active count, duration metrics
- **Lock-Free Stats**: Sequence lock pattern for monitoring without contention
- **Session Management**: Long-lived transaction sessions with unique IDs
- **SAGA Integration**: Each transaction has associated SAGA for compensating actions

**Thread Safety:**
- Thread-safe for all operations
- Transaction IDs generated atomically
- Transaction map protected by internal mutex
- Each Transaction object is NOT thread-safe (use from single thread)
- Safe to commit/rollback from different threads

**Configuration:**
```cpp
#include "transaction/transaction_manager.h"

// Initialize with storage and index managers
TransactionManager txn_mgr(
    rocksdb_wrapper,
    secondary_index_manager,
    graph_index_manager,
    vector_index_manager
);

// Configure deadlock detection
txn_mgr.setDeadlockDetection(true);
txn_mgr.setDeadlockTimeout(std::chrono::seconds(30));
```

**Usage Example - Basic Transaction:**
```cpp
// Begin a transaction
auto txn_id = txn_mgr.beginTransaction(IsolationLevel::ReadCommitted);
auto txn = txn_mgr.getTransaction(txn_id);

// Perform operations
auto status = txn->putEntity("users", user_entity);
if (!status.ok) {
    txn_mgr.rollbackTransaction(txn_id);
    return status;
}

status = txn->addEdge(follows_edge);
if (!status.ok) {
    txn_mgr.rollbackTransaction(txn_id);
    return status;
}

status = txn->addVector(embedding_entity, "embedding");
if (!status.ok) {
    txn_mgr.rollbackTransaction(txn_id);
    return status;
}

// Commit all changes atomically
status = txn_mgr.commitTransaction(txn_id);
```

**Usage Example - Snapshot Isolation:**
```cpp
// Use snapshot isolation for consistent reads
auto txn_id = txn_mgr.beginTransaction(IsolationLevel::Snapshot);
auto txn = txn_mgr.getTransaction(txn_id);

// All reads see database state at transaction start time
// Writes are buffered and applied atomically at commit
// Provides repeatable reads within the transaction

auto entity1 = storage.get("users", "user123"); // Snapshot view
auto entity2 = storage.get("users", "user456"); // Same snapshot

txn_mgr.commitTransaction(txn_id);
```

**Performance Characteristics:**
- **Throughput**: 10K-50K transactions/sec (simple operations)
- **Latency**: Begin <1μs, Commit 100μs-5ms (depends on batch size)
- **Lock Overhead**: ~5ns per lock acquire (atomic operations)
- **Deadlock Detection**: 100ms check interval (configurable)

---

### Transaction Class
**Location:** `../include/transaction/transaction_manager.h` (nested class)

Represents a single ACID transaction with buffered operations.

**Lifecycle:**
```
BEGIN → READ/WRITE OPERATIONS → COMMIT/ROLLBACK
  ↓           ↓                       ↓
Created   Buffered in          Applied atomically
          WriteBatch          (or discarded)
```

**Operations:**

**Relational Operations:**
```cpp
// Insert or update entity
Status putEntity(std::string_view table, const BaseEntity& entity);

// Delete entity by primary key
Status eraseEntity(std::string_view table, std::string_view pk);
```

**Graph Operations:**
```cpp
// Add edge with automatic from/to vertex updates
Status addEdge(const BaseEntity& edgeEntity);

// Delete edge by ID
Status deleteEdge(std::string_view edgeId);
```

**Vector Operations:**
```cpp
// Add vector with entity
Status addVector(const BaseEntity& entity,
                std::string_view vectorField = "embedding");

// Update existing vector
Status updateVector(const BaseEntity& entity,
                   std::string_view vectorField = "embedding");

// Remove vector by primary key
Status removeVector(std::string_view pk);
```

**Transaction Control:**
```cpp
// Commit all buffered operations atomically
Status commit();

// Discard all buffered operations and execute compensating actions
void rollback();
```

**Metadata:**
```cpp
TransactionId getId() const;
IsolationLevel getIsolationLevel() const;
std::chrono::system_clock::time_point getStartTime() const;
uint64_t getDurationMs() const;
bool isFinished() const;
```

**Atomicity Guarantee:**
All operations within a transaction are applied in a single RocksDB WriteBatch. Either all succeed or all fail - no partial updates.

```cpp
// Example: Multi-layer atomic update
auto txn = txn_mgr.begin(IsolationLevel::ReadCommitted);

txn.putEntity("users", user);              // Main table
txn.addEdge(friend_edge);                  // Graph index
txn.addVector(user, "embedding");          // Vector index
// Secondary indexes updated automatically

auto status = txn.commit();  // All or nothing
```

**Named Savepoints (Partial Rollback):**

Savepoints allow a transaction to be rolled back to an intermediate point without aborting the entire transaction.  This is equivalent to SQL `SAVEPOINT` / `ROLLBACK TO SAVEPOINT` / `RELEASE SAVEPOINT`.

```cpp
// Named savepoint API
Status createSavepoint(std::string_view name);      // set a savepoint
Status rollbackToSavepoint(std::string_view name);  // partial rollback
Status releaseSavepoint(std::string_view name);     // discard without rollback

// Query active savepoints
std::vector<std::string> getSavepoints() const;
bool hasSavepoint(std::string_view name) const;
```

```cpp
// Example: partial rollback on validation failure
auto txn = txn_mgr.begin();

txn.putEntity("users", user1);
txn.createSavepoint("after_user1");

txn.putEntity("users", user2);
txn.createSavepoint("after_user2");

txn.putEntity("users", user3);       // user3 turns out to be invalid

// Discard only user3 — user1 and user2 remain in the transaction
txn.rollbackToSavepoint("after_user2");

txn.commit();  // commits user1 and user2 only
```

**Savepoint rules:**
- Savepoint names must be unique within the transaction and non-empty.
- `rollbackToSavepoint(name)` removes the named savepoint **and** all savepoints created after it.
- `releaseSavepoint(name)` removes the named savepoint and all newer ones **without** undoing any writes.
- SAGA compensating actions registered after a savepoint are trimmed when `rollbackToSavepoint` is called.
- Do **not** mix the anonymous stack API (`setSavePoint` / `rollbackToSavePoint` / `popSavePoint`) with the named savepoint API on the same transaction.  Both APIs manipulate the same underlying RocksDB savepoint stack.  Interleaving them causes mismatched name-to-stack-index mappings in the `savepoints_` bookkeeping vector, which results in incorrect rollback targets, failed savepoint operations, or silent data corruption.

---

### SAGA Pattern
**Location:** `saga.cpp`, `../include/transaction/saga.h`

Implements the SAGA pattern for distributed transactions with compensating actions.

**Features:**
- **Compensating Actions**: Each operation records an undo function
- **Automatic Rollback**: Executes compensations in reverse order
- **Long-Running Transactions**: Supports operations spanning minutes/hours
- **Eventual Consistency**: Guarantees consistency through compensation
- **Distributed Support**: Coordinates operations across multiple services
- **Step History**: Tracks all executed and compensated steps

**Pattern:**
```
Forward Phase:  Op1 → Op2 → Op3 → Op4 (failure!)
                 ↓     ↓     ↓
Compensating:  C1 ← C2 ← C3 ← (execute in reverse)
```

**Usage Example:**
```cpp
#include "transaction/saga.h"

auto txn = txn_mgr.begin();
auto& saga = txn.getSaga();

// Step 1: Reserve inventory
auto reserved_items = reserveInventory(order_id, items);
saga.addStep("reserve_inventory", [=]() {
    releaseInventory(order_id, reserved_items);
});

// Step 2: Charge payment
auto payment_id = chargePayment(customer_id, amount);
saga.addStep("charge_payment", [=]() {
    refundPayment(payment_id);
});

// Step 3: Send notification
auto notification_id = sendNotification(customer_id, "Order confirmed");
saga.addStep("send_notification", [=]() {
    sendNotification(customer_id, "Order cancelled");
});

// If any step fails, compensations execute automatically
if (!txn.commit().ok) {
    // SAGA automatically compensated in reverse order
    // notification → payment → inventory
}
```

**Advanced Features:**
```cpp
// Check SAGA status
size_t total_steps = saga.stepCount();
size_t compensated = saga.compensatedCount();
bool fully_compensated = saga.isFullyCompensated();

// Get execution history
std::vector<std::string> history = saga.getStepHistory();
for (const auto& step : history) {
    std::cout << "Executed: " << step << std::endl;
}

// Duration tracking
int64_t duration_ms = saga.getDurationMs();
```

**Distributed Transaction Example:**
```cpp
// Microservices orchestration with SAGA
auto txn = txn_mgr.begin();
auto& saga = txn.getSaga();

// Service 1: User service
auto user_updated = user_service.updateProfile(user_id, profile);
saga.addStep("update_user_profile", [=]() {
    user_service.revertProfile(user_id, old_profile);
});

// Service 2: Email service
auto email_sent = email_service.sendWelcome(user_id);
saga.addStep("send_welcome_email", [=]() {
    // Compensating action (may be no-op for emails)
    email_service.logCancellation(user_id);
});

// Service 3: Analytics service
auto event_logged = analytics_service.logEvent("user_updated", user_id);
saga.addStep("log_analytics_event", [=]() {
    analytics_service.deleteEvent(event_id);
});

// Commit coordinates all services
auto status = txn.commit();
if (!status.ok) {
    // All services compensated automatically
}
```

**Best Practices:**
- Keep compensating actions idempotent (can be called multiple times safely)
- Log compensation execution for audit trail
- Set reasonable timeouts for long-running steps
- Use SAGA for cross-service boundaries, not local operations
- Test compensation logic thoroughly

---

### SnapshotManager
**Location:** `snapshot_manager.cpp`, `../include/transaction/snapshot_manager.h`

Provides Git-like named snapshots for MVCC system.

**Features:**
- **Named Tags**: Semantic labels for database states
- **Changefeed Integration**: Maps tags to sequence numbers
- **Point-in-Time Recovery**: Restore to any tagged state
- **Audit Checkpoints**: Compliance and governance snapshots
- **Tag Metadata**: Description, creator, timestamp
- **REST API**: HTTP endpoints for tag management

**Usage Example:**
```cpp
#include "transaction/snapshot_manager.h"

SnapshotManager snapshot_mgr(rocksdb, changefeed);

// Create a named snapshot
auto result = snapshot_mgr.createSnapshot(
    "pre-deployment-v2.1",
    "Snapshot before deploying schema changes",
    "admin@company.com"
);

if (result) {
    std::cout << "Created snapshot at sequence: "
              << result->sequence_number << std::endl;
}

// List all snapshots
auto snapshots = snapshot_mgr.listSnapshots();
for (const auto& snap : snapshots) {
    std::cout << snap.tag_name << " @ seq "
              << snap.sequence_number << std::endl;
}

// Get specific snapshot
auto snap = snapshot_mgr.getSnapshot("pre-deployment-v2.1");
if (snap) {
    std::cout << "Created at: " << snap->timestamp_ms
              << " by " << snap->created_by << std::endl;
}

// Delete old snapshot
snapshot_mgr.deleteSnapshot("old-tag");

// Get statistics
auto stats = snapshot_mgr.getStats();
std::cout << "Total snapshots: " << stats.total_snapshots << std::endl;
std::cout << "Oldest sequence: " << stats.oldest_sequence << std::endl;
```

**Snapshot Metadata:**
```cpp
struct Snapshot {
    std::string tag_name;           // Unique identifier
    uint64_t sequence_number;       // Changefeed sequence
    int64_t timestamp_ms;           // Unix timestamp
    std::string description;        // Human-readable note
    std::string created_by;         // Creator identifier
};
```

**Use Cases:**
- Pre-deployment safe points
- Audit/compliance checkpoints
- Schema migration rollback points
- A/B testing baselines
- Time-travel queries

---

### BranchManager
**Location:** `branch_manager.cpp`, `../include/transaction/branch_manager.h`

Provides Git-like persistent branches for parallel development workflows.

**Features:**
- **Named Branches**: Persistent parallel universes of data
- **Parent Tracking**: Branch hierarchy and lineage
- **Branch Switching**: Change active branch like `git checkout`
- **Branch Creation**: From tags, sequences, or timestamps
- **Merge Support**: Integration with MergeEngine
- **Multi-Tenant**: Isolated branches per tenant/project
- **REST API**: HTTP endpoints for branch operations

**Branch Lifecycle:**
```
CREATE → DEVELOP → MERGE → DELETE (optional)
  ↓         ↓         ↓         ↓
Tag/Seq  Changes   Resolve   Cleanup
         Isolated  Conflicts
```

**Usage Example:**
```cpp
#include "transaction/branch_manager.h"

BranchManager branch_mgr(rocksdb, changefeed, snapshot_mgr);

// Create branch from current state
auto result = branch_mgr.createBranch(
    "feature-new-schema",
    "main",  // parent branch
    BranchManager::CreateBranchOptions{},
    "Experimental schema changes",
    "developer@company.com"
);

// Create branch from specific tag
BranchManager::CreateBranchOptions opts;
opts.from_tag = "pre-deployment-v2.1";
branch_mgr.createBranch(
    "test-deployment",
    "main",
    opts,
    "Testing v2.1 deployment",
    "qa@company.com"
);

// Switch to branch (changes current context)
branch_mgr.switchBranch("feature-new-schema");

// All operations now happen on feature-new-schema
// Original 'main' branch remains unchanged

// List branches
auto branches = branch_mgr.listBranches();
for (const auto& branch : branches) {
    std::cout << branch.branch_name;
    if (branch.is_active) std::cout << " (active)";
    std::cout << std::endl;
}

// Merge branch (using MergeEngine)
merge_engine.merge(
    "feature-new-schema",  // source
    "main",                 // target
    MergeEngine::MergeStrategy::MANUAL  // require conflict resolution
);

// Delete branch after merge
branch_mgr.deleteBranch("feature-new-schema");
```

**Branch Metadata:**
```cpp
struct Branch {
    std::string branch_name;        // Unique identifier
    std::string parent_branch;      // Parent (empty for root)
    uint64_t creation_sequence;     // Changefeed sequence at creation
    int64_t creation_timestamp_ms;  // Unix timestamp
    std::string description;        // Human-readable note
    std::string created_by;         // Creator identifier
    bool is_active;                 // Currently active branch
};
```

**Use Cases:**
- Schema migration testing
- A/B testing with isolated datasets
- What-if analysis scenarios
- Parallel development workflows
- Multi-tenant data isolation
- Feature flag implementations

---

### MergeEngine
**Location:** `merge_engine.cpp`, `../include/transaction/merge_engine.h`

Provides three-way merge functionality with conflict detection.

**Features:**
- **Three-Way Merge**: Analyzes changes from common ancestor
- **Conflict Detection**: Identifies overlapping modifications
- **Resolution Strategies**: OURS, THEIRS, MANUAL, FAST_FORWARD
- **Dry-Run Mode**: Preview merge without applying changes
- **Diff Integration**: Uses DiffEngine for change analysis
- **Conflict Reporting**: Detailed conflict metadata

**Merge Algorithm:**
```
         Base (Common Ancestor)
           /        \
          /          \
    Source Branch  Target Branch
       (ours)        (theirs)
          \          /
           \        /
         Merged Result
      (with conflicts resolved)
```

**Conflict Types:**
```cpp
enum class ConflictType {
    MODIFY_MODIFY,  // Both sides modified same key
    DELETE_MODIFY,  // One deleted, other modified
    MODIFY_DELETE,  // One modified, other deleted
    DELETE_DELETE   // Both deleted (auto-resolve)
};
```

**Usage Example:**
```cpp
#include "transaction/merge_engine.h"

MergeEngine merge_engine(rocksdb, snapshot_mgr, changefeed, diff_engine);

// Attempt automatic merge with THEIRS strategy
auto result = merge_engine.merge(
    "feature-branch",   // source
    "main",             // target
    MergeEngine::MergeStrategy::THEIRS,
    false  // dry_run
);

if (result.success) {
    std::cout << "Merge succeeded" << std::endl;
    std::cout << "Changes applied: " << result.changes_applied << std::endl;
} else {
    std::cout << "Merge failed: " << result.error_message << std::endl;
    std::cout << "Conflicts detected: " << result.conflicts.size() << std::endl;

    // Examine conflicts
    for (const auto& conflict : result.conflicts) {
        std::cout << "Conflict on key: " << conflict.key << std::endl;
        std::cout << "Type: ";
        switch (conflict.type) {
            case ConflictType::MODIFY_MODIFY:
                std::cout << "MODIFY_MODIFY" << std::endl;
                std::cout << "Base: " << conflict.base_value.value_or("(none)") << std::endl;
                std::cout << "Source: " << conflict.source_value.value_or("(none)") << std::endl;
                std::cout << "Target: " << conflict.target_value.value_or("(none)") << std::endl;
                break;
            // ... handle other types
        }
    }
}
```

**Manual Conflict Resolution:**
```cpp
// Get conflicts from dry-run
auto dry_run = merge_engine.merge(
    "feature-branch",
    "main",
    MergeEngine::MergeStrategy::MANUAL,
    true  // dry_run
);

// Prepare resolutions
std::vector<MergeEngine::ConflictResolution> resolutions;
for (const auto& conflict : dry_run.conflicts) {
    if (should_prefer_source(conflict)) {
        resolutions.push_back({
            conflict.key,
            conflict.source_value,
            "Prefer feature branch changes"
        });
    } else {
        resolutions.push_back({
            conflict.key,
            conflict.target_value,
            "Keep main branch version"
        });
    }
}

// Apply merge with resolutions
auto result = merge_engine.mergeWithResolutions(
    "feature-branch",
    "main",
    resolutions
);
```

**Merge Strategies:**
- **OURS**: Prefer target branch changes (keep existing)
- **THEIRS**: Prefer source branch changes (accept incoming)
- **MANUAL**: Require explicit resolution for conflicts
- **FAST_FORWARD**: Only succeed if no conflicts exist

**Performance:**
- Merge time: O(n) where n = number of changed keys
- Conflict detection: ~1ms per 1000 keys
- Memory usage: O(conflicts) - only stores conflict metadata

---

## ACID Properties

### Atomicity
All operations within a transaction succeed or fail together. Implemented via RocksDB WriteBatch.

**Implementation:**
```cpp
// All operations buffered in WriteBatch
txn.putEntity("users", user);
txn.addEdge(edge);
txn.addVector(embedding);

// Single atomic write
auto status = txn.commit();
// If any operation fails, entire batch is discarded
```

### Consistency
Database moves from one valid state to another. Constraints enforced before commit.

**Validation:**
- Schema validation on entity writes
- Foreign key checks on edge operations
- Unique constraint verification
- Type checking for all fields

### Isolation
Concurrent transactions don't interfere with each other.

**Isolation Levels:**

**ReadCommitted (Default):**
- Reads see only committed data
- No dirty reads
- Non-repeatable reads possible
- Phantom reads possible

**Snapshot:**
- Consistent point-in-time view
- Repeatable reads guaranteed
- No phantom reads
- Higher overhead (snapshot maintenance)

**Isolation Comparison:**
| Property | ReadCommitted | Snapshot |
|----------|---------------|----------|
| Dirty Reads | ❌ | ❌ |
| Non-Repeatable Reads | ✅ | ❌ |
| Phantom Reads | ✅ | ❌ |
| Performance | High | Medium |
| Use Case | General OLTP | Analytical, Reports |

### Durability
Committed transactions survive crashes. Guaranteed by RocksDB WAL.

**Guarantees:**
- Write-Ahead Log (WAL) persists before commit returns
- Crash recovery replays WAL
- Configurable sync modes (fsync, fdatasync, none)

---

## Deadlock Detection

**Algorithm:**
The TransactionManager runs a background thread that periodically checks for deadlocks using cycle detection in the wait-for graph.

**Wait-For Graph:**
```
Transaction A → waiting for lock held by → Transaction B
Transaction B → waiting for lock held by → Transaction C
Transaction C → waiting for lock held by → Transaction A
                                           ↑____________↓
                                           DEADLOCK CYCLE
```

**Detection Process:**
1. Background thread wakes every 100ms (configurable)
2. Builds wait-for graph from held_locks_ and waiting_for_ maps
3. Performs depth-first search to find cycles
4. If cycle detected:
   - Select victim (youngest transaction)
   - Abort victim transaction
   - Log deadlock information
   - Wake up waiters

**Configuration:**
```cpp
// Enable/disable detection
txn_mgr.setDeadlockDetection(true);

// Set timeout (default: 30 seconds)
txn_mgr.setDeadlockTimeout(std::chrono::seconds(30));

// Get recent deadlocks
auto deadlocks = txn_mgr.getDeadlocks(std::chrono::hours(24));
for (const auto& dl : deadlocks) {
    std::cout << "Deadlock detected at: "
              << dl.detected_at << std::endl;
    std::cout << "Cycle: ";
    for (auto txn_id : dl.cycle) {
        std::cout << txn_id << " → ";
    }
    std::cout << std::endl;
    std::cout << "Victim: " << dl.victim_id << std::endl;
}
```

**Prevention Best Practices:**
- Acquire locks in consistent order
- Use short-lived transactions
- Minimize lock hold time
- Use optimistic locking when possible
- Set reasonable timeouts

---

## Transaction Statistics

**Available Metrics:**
```cpp
struct Stats {
    uint64_t total_begun;       // Total transactions started
    uint64_t total_committed;   // Total successful commits
    uint64_t total_aborted;     // Total rollbacks
    uint64_t active_count;      // Currently active transactions
    uint64_t avg_duration_ms;   // Average transaction duration
    uint64_t max_duration_ms;   // Maximum transaction duration
};

// Standard stats (eventually consistent)
auto stats = txn_mgr.getStats();

// Lock-free consistent snapshot (preferred for monitoring)
auto stats = txn_mgr.getStatsLockFree();
```

**Monitoring Example:**
```cpp
// Periodic monitoring
while (true) {
    auto stats = txn_mgr.getStatsLockFree();

    std::cout << "Active: " << stats.active_count << std::endl;
    std::cout << "Committed: " << stats.total_committed << std::endl;
    std::cout << "Aborted: " << stats.total_aborted << std::endl;
    std::cout << "Avg Duration: " << stats.avg_duration_ms << "ms" << std::endl;

    // Calculate success rate
    double total = stats.total_committed + stats.total_aborted;
    double success_rate = (stats.total_committed / total) * 100.0;
    std::cout << "Success Rate: " << success_rate << "%" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(10));
}
```

**Performance Overhead:**
- Stats collection: <5ns per operation (atomic increment)
- Lock-free read: <10ns (fast path, no contention)
- Memory overhead: ~64 bytes per transaction (metadata)

---

## Integration with Other Modules

### Storage Module
```cpp
// TransactionManager wraps RocksDB operations
auto txn = txn_mgr.begin();
txn.putEntity("users", entity);  // → RocksDB WriteBatch
txn.commit();                     // → RocksDB Write()
```

### Index Module
```cpp
// Secondary indexes updated atomically with entity
auto txn = txn_mgr.begin();
txn.putEntity("users", user);
// Secondary indexes on email, age, etc. updated automatically
txn.commit();  // Atomic: entity + all indexes
```

### Graph Module
```cpp
// Graph edges and vertices updated atomically
auto txn = txn_mgr.begin();
txn.addEdge(edge);
// Updates: edge entity, from_vertex outEdges, to_vertex inEdges
txn.commit();
```

### Vector Module
```cpp
// Vector embeddings indexed atomically with entity
auto txn = txn_mgr.begin();
txn.putEntity("documents", doc);
txn.addVector(doc, "embedding");
// Updates: entity + HNSW vector index
txn.commit();
```

### CDC Module
```cpp
// Changefeed captures transaction boundaries
auto txn = txn_mgr.begin();
txn.putEntity("users", user);
txn.commit();
// Changefeed emits: {txn_id, sequence, changes}
```

---

## Performance Tuning

### Transaction Size
**Guideline:** Keep transactions small (< 1000 operations per transaction)

```cpp
// ❌ Bad: Large transaction holds locks too long
auto txn = txn_mgr.begin();
for (int i = 0; i < 1000000; i++) {
    txn.putEntity("users", users[i]);  // Locks accumulate
}
txn.commit();  // Long commit time

// ✅ Good: Batch in smaller transactions
for (int batch = 0; batch < 1000; batch++) {
    auto txn = txn_mgr.begin();
    for (int i = 0; i < 1000; i++) {
        txn.putEntity("users", users[batch * 1000 + i]);
    }
    txn.commit();  // Fast commits
}
```

### Isolation Level Selection
**ReadCommitted:** Use for general OLTP workloads (fastest)
**Snapshot:** Use for analytical queries and reports (consistent reads)

> ⚠️ **Write-Skew / Phantom-Read Warning (SNAPSHOT):**
> Two concurrent SNAPSHOT transactions can each read the same data, make disjoint writes, and both commit — even when their combined effect violates an application invariant. Classic examples: double-booking, over-withdrawal, on-call scheduling (doctors problem). Use `SERIALIZABLE` whenever your workload requires strict invariant enforcement.

**SerializableSnapshot (SSI):** Use for strict ACID correctness — prevents write skew and phantom reads at the cost of more aborts and slightly higher latency.

```cpp
// OLTP: frequent, small transactions (fast, but write-skew possible)
auto txn = txn_mgr.begin(IsolationLevel::ReadCommitted);

// Analytics: long-running, consistent reads
// WARNING: write skew and phantom reads are possible at this level
auto txn = txn_mgr.begin(IsolationLevel::Snapshot);

// Strict correctness: prevents write skew and phantom reads
auto txn = txn_mgr.begin(IsolationLevel::SERIALIZABLE);
// On commit, a serialization conflict aborts the transaction:
auto status = txn_mgr.commitTransaction(txn_id);
if (!status.ok && status.message.find("Serialization") != std::string::npos) {
    // Retry with exponential back-off
}
```

### Deadlock Prevention
**Pattern:** Acquire locks in consistent order

```cpp
// ❌ Bad: Inconsistent lock order
void transfer1(int from, int to, int amount) {
    auto txn = txn_mgr.begin();
    txn.putEntity("accounts", from_account);  // Lock A
    txn.putEntity("accounts", to_account);    // Lock B
    txn.commit();
}

void transfer2(int from, int to, int amount) {
    auto txn = txn_mgr.begin();
    txn.putEntity("accounts", to_account);    // Lock B first!
    txn.putEntity("accounts", from_account);  // Lock A second
    txn.commit();  // Deadlock possible
}

// ✅ Good: Consistent lock order
void transfer_safe(int from, int to, int amount) {
    auto txn = txn_mgr.begin();
    // Always acquire locks in ascending ID order
    if (from < to) {
        txn.putEntity("accounts", from_account);
        txn.putEntity("accounts", to_account);
    } else {
        txn.putEntity("accounts", to_account);
        txn.putEntity("accounts", from_account);
    }
    txn.commit();
}
```

### Lock-Free Statistics
**Use:** `getStatsLockFree()` for high-frequency monitoring

```cpp
// ❌ Bad: Contention on shared lock
while (monitoring) {
    auto stats = txn_mgr.getStats();  // May wait for lock
    update_dashboard(stats);
    sleep(100ms);  // Frequent polling
}

// ✅ Good: Lock-free read
while (monitoring) {
    auto stats = txn_mgr.getStatsLockFree();  // No waiting
    update_dashboard(stats);
    sleep(100ms);
}
```

### Transaction Cleanup
**Best Practice:** Periodic cleanup of old completed transactions

```cpp
// Background cleanup task
void cleanup_task() {
    while (running) {
        // Remove transactions completed > 1 hour ago
        txn_mgr.cleanupOldTransactions(std::chrono::hours(1));
        std::this_thread::sleep_for(std::chrono::minutes(10));
    }
}
```

---

## Error Handling

### Transaction Errors
```cpp
auto txn = txn_mgr.begin();

auto status = txn.putEntity("users", user);
if (!status.ok) {
    THEMIS_ERROR("Put failed: {}", status.message);
    txn.rollback();
    return status;
}

status = txn.commit();
if (!status.ok) {
    // Commit failure - already rolled back
    if (status.message.find("Deadlock") != std::string::npos) {
        // Retry with exponential backoff
        return retry_transaction();
    }
    return status;
}
```

### SAGA Compensation Errors
```cpp
auto txn = txn_mgr.begin();
auto& saga = txn.getSaga();

try {
    performOperation1();
    saga.addStep("op1", []() { compensateOp1(); });

    performOperation2();
    saga.addStep("op2", []() {
        try {
            compensateOp2();
        } catch (const std::exception& e) {
            THEMIS_ERROR("Compensation failed: {}", e.what());
            // Log for manual intervention
        }
    });

    txn.commit();
} catch (const std::exception& e) {
    // SAGA compensates automatically
    THEMIS_ERROR("Transaction failed: {}", e.what());
}
```

### Merge Conflicts
```cpp
auto result = merge_engine.merge(
    "feature-branch",
    "main",
    MergeEngine::MergeStrategy::FAST_FORWARD,
    false
);

if (!result.success) {
    if (!result.conflicts.empty()) {
        // Handle conflicts
        THEMIS_WARN("Merge conflicts detected: {}", result.conflicts.size());

        for (const auto& conflict : result.conflicts) {
            THEMIS_INFO("Conflict on key: {}", conflict.key);
            // Resolve manually or use different strategy
        }

        // Retry with MANUAL resolution
        auto manual_result = merge_engine.merge(
            "feature-branch",
            "main",
            MergeEngine::MergeStrategy::MANUAL,
            false
        );
    } else {
        THEMIS_ERROR("Merge failed: {}", result.error_message);
    }
}
```

---

## Troubleshooting

| Symptom | Likely Cause | Mitigation |
|---------|--------------|------------|
| Commit returns `Deadlock detected` | Writers acquired locks in conflicting order | Enforce deterministic lock ordering and enable deadlock detection |
| `OCC version conflict` on `optimisticPut`/`optimisticErase` | Concurrent update changed entity version | Re-read version with `getEntityVersion(...)` and retry |
| `Serialization conflict` with `IsolationLevel::Serializable` | Predicate-lock overlap with concurrent writers | Retry with backoff and reduce wide range predicates |
| Savepoint rollback behaves unexpectedly | Named and anonymous savepoint APIs were mixed | Use only one savepoint API style per transaction |

---

## Testing

### Unit Tests
```bash
# Run transaction module tests
cd build
ctest -R transaction_test -V

# Specific test suites
./tests/transaction_manager_test
./tests/saga_test
./tests/snapshot_manager_test
./tests/branch_manager_test
./tests/merge_engine_test
./tests/test_savepoints        # named and anonymous savepoint tests (20 cases)
```

## Integration Tests
```bash
# End-to-end transaction tests
./tests/transaction_integration_test

# Deadlock detection tests
./tests/deadlock_test

# SAGA pattern tests
./tests/saga_integration_test
```

## Performance Benchmarks
```bash
# Transaction throughput benchmark (includes savepoint benchmarks)
./benchmarks/bench_transaction_throughput

# Savepoint-specific benchmarks:
#   SavepointCreateAndRollback  – partial rollback overhead
#   SavepointNested             – nesting depth overhead
#   SavepointRelease            – release (no rollback) cost

# Deadlock detection overhead
./benchmarks/deadlock_overhead_benchmark

# SAGA compensation performance
./benchmarks/saga_benchmark
```

---

## Thread Safety Summary

| Component | Thread Safety | Notes |
|-----------|---------------|-------|
| TransactionManager | ✅ Full | All methods thread-safe |
| Transaction | ❌ No | Use from single thread |
| SAGA | ❌ No | Accessed via Transaction |
| SnapshotManager | ✅ Full | Internal mutex protection |
| BranchManager | ✅ Full | Internal mutex protection |
| MergeEngine | ✅ Full | Stateless operations |

---

## Dependencies

**Internal:**
- `storage::RocksDBWrapper` - Underlying storage engine
- `index::SecondaryIndexManager` - Secondary index updates
- `index::GraphIndexManager` - Graph edge tracking
- `index::VectorIndexManager` - Vector embedding indexing
- `cdc::Changefeed` - Change data capture
- `analytics::DiffEngine` - Snapshot diffing for merge

**External:**
- RocksDB 8.x - MVCC and transaction support
- Boost.Thread - Threading primitives
- nlohmann/json - Metadata serialization

---

## Configuration

### Public API Configuration Knobs

The primary runtime configuration surface is the C++ API:

| API | Default | Purpose |
|-----|---------|---------|
| `TransactionManager::setDeadlockDetection(bool)` | `false` | Enables/disables background deadlock detection |
| `TransactionManager::setDeadlockTimeout(std::chrono::milliseconds)` | `1000ms` | Deadlock timeout and victim handling threshold |
| `TransactionManager::setDefaultTransactionTimeout(std::chrono::milliseconds)` | `0ms` | Applies default timeout to newly started transactions |
| `TransactionManager::setTransactionTimeout(std::chrono::milliseconds)` | `0ms` | Timeout sweep over active transactions |
| `TransactionManager::setSSIConfig(const SSIConfig&)` | `predicate_locking=true`, `max_predicate_locks=10000` | Tunes SERIALIZABLE predicate-lock behavior |
| `Transaction::setTimeout(std::chrono::milliseconds)` | `0ms` | Per-transaction timeout override |
| `Transaction::setReadOnly(bool)` | `false` | Enables read-only fast path for a transaction |

### Environment Variables
```bash
# Deadlock detection
THEMIS_DEADLOCK_DETECTION=true
THEMIS_DEADLOCK_TIMEOUT_MS=30000

# Transaction limits
THEMIS_MAX_TRANSACTION_SIZE=1000
THEMIS_TRANSACTION_CLEANUP_INTERVAL_SECS=600

# SAGA configuration
THEMIS_SAGA_MAX_STEPS=100
THEMIS_SAGA_COMPENSATION_TIMEOUT_MS=5000

# Snapshot configuration
THEMIS_MAX_SNAPSHOTS=1000
THEMIS_SNAPSHOT_CLEANUP_DAYS=90
```

## Config File (YAML)
```yaml
transaction:
  isolation_level: ReadCommitted  # or Snapshot

  deadlock_detection:
    enabled: true
    timeout_ms: 30000
    check_interval_ms: 100

  limits:
    max_transaction_size: 1000
    max_active_transactions: 10000
    cleanup_interval_secs: 600
    cleanup_max_age_secs: 3600

  saga:
    max_steps: 100
    compensation_timeout_ms: 5000
    enable_logging: true

  snapshots:
    max_count: 1000
    cleanup_days: 90
    auto_cleanup: true

  branches:
    max_branches: 100
    default_branch: "main"
    allow_force_delete: false
```

---

## Migration Guide

### From Legacy API
```cpp
// Old: Direct RocksDB WriteBatch
rocksdb::WriteBatch batch;
batch.Put("users:123", user_data);
batch.Put("index:email:user@example.com", "123");
db->Write(rocksdb::WriteOptions(), &batch);

// New: TransactionManager
auto txn = txn_mgr.begin();
txn.putEntity("users", user);  // Indexes updated automatically
txn.commit();
```

### Adding SAGA Support
```cpp
// Before: No rollback capability
updateInventory();
chargePayment();
sendNotification();

// After: With SAGA
auto txn = txn_mgr.begin();
auto& saga = txn.getSaga();

updateInventory();
saga.addStep("inventory", []() { revertInventory(); });

chargePayment();
saga.addStep("payment", []() { refundPayment(); });

sendNotification();
saga.addStep("notification", []() { cancelNotification(); });

txn.commit();  // Auto-compensate on failure
```

---

## See Also

- [Storage Module](../storage/README.md) - Underlying persistence layer
- [Index Module](../index/README.md) - Secondary, graph, and vector indexes
- [CDC Module](../cdc/README.md) - Change data capture integration
- [Query Module](../query/README.md) - AQL query execution
- [Public Header Documentation](../../include/transaction/README.md) - Public entry points and integration notes
- [Transaction Roadmap](./ROADMAP.md) - Delivery phases and production-readiness checklist
- [Future Enhancements](./FUTURE_ENHANCEMENTS.md) - Planned interfaces and constraints
- [German Transaction Docs](../../docs/de/transaction/README.md) - Consolidated module status and inventory
- [SAGA Pattern](https://microservices.io/patterns/data/saga.html) - Distributed transaction pattern
- [RocksDB Transactions](https://github.com/facebook/rocksdb/wiki/Transactions) - Native MVCC support

---

## License

Copyright © 2024 ThemisDB Contributors. Licensed under Apache 2.0.

## Scientific References

1. Gray, J., & Reuter, A. (1992). **Transaction Processing: Concepts and Techniques**. Morgan Kaufmann. ISBN: 978-1-558-60190-4

2. Bernstein, P. A., Hadzilacos, V., & Goodman, N. (1987). **Concurrency Control and Recovery in Database Systems**. Addison-Wesley. https://www.microsoft.com/en-us/research/wp-content/uploads/2016/05/ccontrol.zip

3. Kung, H. T., & Robinson, J. T. (1981). **On Optimistic Methods for Concurrency Control**. *ACM Transactions on Database Systems*, 6(2), 213–226. https://doi.org/10.1145/319566.319567

4. Garcia-Molina, H., & Salem, K. (1987). **Sagas**. *Proceedings of the 1987 ACM SIGMOD International Conference on Management of Data*, 249–259. https://doi.org/10.1145/38713.38742

5. Herlihy, M., & Wing, J. M. (1990). **Linearizability: A Correctness Condition for Concurrent Objects**. *ACM Transactions on Programming Languages and Systems*, 12(3), 463–492. https://doi.org/10.1145/78969.78972

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
