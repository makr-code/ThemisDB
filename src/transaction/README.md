---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1; SOT domain: module-behavior. Canonical references: `include/transaction/transaction_manager.h`, `include/transaction/isolation_level.h`, `include/transaction/lock_manager.h`, `include/transaction/distributed_transaction_manager.h`, `include/transaction/saga.h`, `include/transaction/snapshot_manager.h`, `include/transaction/branch_manager.h`, `include/transaction/merge_engine.h`, `src/transaction/transaction_manager.cpp`, `src/transaction/saga.cpp`, `tests/transaction/CMakeLists.txt`, `benchmarks/transaction/CMakeLists.txt`, `cmake/ModularBuild.cmake`, and `cmake/CMakeLists.txt`. Related issue: #7390.

# ThemisDB Transaction Module

## Module Purpose

The transaction module provides local transaction lifecycle APIs, RocksDB-backed transaction operations, lock/isolation helpers, distributed coordination components, saga compensation, and snapshot/branch/merge utilities. The current headers and implementation establish local transaction behavior; they do not by themselves establish atomicity for arbitrary external side effects or every index/service boundary.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `transaction_manager.cpp` | ACID transaction lifecycle: begin, commit, rollback |
| `saga.cpp` | SAGA pattern: step execution + compensating actions |
| `saga_orchestrator.cpp` | SAGA orchestration and multi-step coordination |
| `distributed_saga.cpp` | Distributed SAGA across multiple nodes |
| `distributed_transaction_manager.cpp` | Two-Phase Commit (2PC) protocol implementation |
| `global_transaction_manager.cpp` | Global transaction coordination; specific multi-region/TrueTime guarantees require verification against its current implementation |

## Module Documentation Links

- [Public Header Overview](../../include/transaction/README.md)
- [Roadmap](./ROADMAP.md)
- [Future Enhancements](./FUTURE_ENHANCEMENTS.md)
- [Architecture](./ARCHITECTURE.md)
- [German Module Overview](../../docs/de/transaction/README.md)

## Current Delivery Status

The header-generated maturity label is not a release verification result. Build, test, production-readiness, and release claims require evidence from the canonical release/CI sources.

## Scope

**In Scope:**
- Local transaction lifecycle and isolation mechanisms backed by the RocksDB transaction wrapper; do not extend these claims to arbitrary external side effects
- Session-based transaction lifecycle (begin → operations → commit/rollback)
- Isolation levels declared in `isolation_level.h`: `READ_UNCOMMITTED`, `READ_COMMITTED`, `REPEATABLE_READ` (legacy `Snapshot` alias), and `SERIALIZABLE`
- RocksDB-backed transaction operations
- Saga compensation callbacks; callback effects outside the transaction are not covered by local atomicity
- Deadlock detection with configurable timeout
- Lock tracking and waiting-for graph analysis
- Transaction statistics and monitoring
- Local transaction mutations across implemented entity/graph/vector paths; TODO: verify atomicity guarantees across each external/index boundary before describing them as cross-layer atomic
- Branch metadata/sequence tracking and merge utilities (BranchManager, MergeEngine); branch switching does not route transaction writes
- Named tags via `SnapshotManager`; `restoreToTag()` validates a tag and records the current sequence in an audit tag, but does not restore or rewind stored data
- TODO: verify direct Changefeed integration and its commit-order guarantees

**Out of Scope:**
- Data storage implementation (handled by storage module)
- Index structure management (handled by index module)
- Query parsing and execution (handled by query module)
- Network protocol handling (handled by server module)

## Key Components

### TransactionManager
**Location:** `transaction_manager.cpp`, `../../include/transaction/transaction_manager.h`

Local transaction manager backed by the RocksDB transaction wrapper.

**Features:**
- **Isolation Levels**: `ReadCommitted` (declared default), `Snapshot` (legacy alias for `REPEATABLE_READ`), and `SERIALIZABLE` (SSI conflict checking)
- **MVCC Support**: Multi-version concurrency control via RocksDB transactions
- **Transaction operations**: entity, graph, vector, savepoint, and lifecycle APIs; do not infer atomicity for independent external effects
- **Deadlock Detection**: Optional manager detection path; source-backed default interval is not asserted here
- **Transaction Statistics**: Total begun/committed/aborted, active count, duration metrics
- **Lock-Free Stats**: Sequence lock pattern for monitoring without contention
- **Session API**: `beginTransaction()` returns an ID; `getTransaction()` returns a shared transaction handle; `begin()` is also declared
- **SAGA Integration**: Each transaction has associated SAGA for compensating actions

**Thread Safety:**
- Manager shared state uses internal synchronization; do not infer that every operation is safe for concurrent calls
- Transaction IDs generated atomically
- Transaction map protected by internal mutex
- No blanket `Transaction` handle thread-safety guarantee is asserted here; use one owner unless the API documents otherwise

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
txn_mgr.setDeadlockTimeout(std::chrono::seconds(1)); // header default is 1000 ms
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

// Commit the local transaction
status = txn_mgr.commitTransaction(txn_id);
```

**Usage Example - Snapshot Isolation:**
```cpp
// Use snapshot isolation for consistent reads
auto txn_id = txn_mgr.beginTransaction(IsolationLevel::Snapshot);
auto txn = txn_mgr.getTransaction(txn_id);

// Read through the transaction handle so the lookup uses its transaction path.
auto user_json = txn->readEntityJson("users", "user-123");
if (!user_json) {
    txn_mgr.rollbackTransaction(txn_id);
    return;
}

txn_mgr.commitTransaction(txn_id);
```

**Performance:** No verified module-wide throughput, latency, lock-cost, or detection-interval measurements are established by this README. See `PERFORMANCE_EXPECTATIONS.md` for mapped targets and treat them as targets, not measured results.

---

### Transaction Class
**Location:** `../include/transaction/transaction_manager.h` (nested class)

Represents a local transaction handle backed by the RocksDB transaction wrapper; this description does not extend to arbitrary external side effects.

**Lifecycle:**
```
BEGIN → READ/WRITE OPERATIONS → COMMIT/ROLLBACK
  ↓           ↓                       ↓
Created   RocksDB transaction  Committed or rolled back
          wrapper              (local storage boundary)
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
// Add an edge and update the graph index through the transaction wrapper
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
// Commit through the underlying RocksDB transaction
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

**Commit boundary:**
`Transaction::commit()` commits the underlying RocksDB transaction. This does not establish atomicity for arbitrary external side effects or every separately managed index/service.

```cpp
// Example: Multi-layer atomic update
auto txn = txn_mgr.begin(IsolationLevel::ReadCommitted);

txn.putEntity("users", user);              // Entity operation
txn.addEdge(friend_edge);                  // Graph operation
txn.addVector(user, "embedding");          // Vector operation

auto status = txn.commit();  // Commits through the underlying RocksDB transaction
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
- Do **not** mix the anonymous stack API (`setSavePoint` / `rollbackToSavePoint` / `popSavePoint`) with the named savepoint API on the same transaction.  Both APIs manipulate the same underlying RocksDB savepoint stack. Interleaving them can produce mismatched name-to-stack-index bookkeeping, incorrect rollback targets, or failed savepoint operations. TODO: verify any persisted-data impact before describing this as data corruption.

---

### SAGA Pattern
**Location:** `saga.cpp`, `../../include/transaction/saga.h`

Implements compensation callback storage and execution. Distributed orchestration is provided by separate components; a local `Saga` does not itself coordinate remote services.

**Features:**
- **Compensating Actions**: Each operation records an undo function
- **Automatic Rollback**: Executes compensations in reverse order
- **Long-Running Transactions**: TODO: verify supported duration/timeout contract
- **Eventual Consistency**: No general consistency guarantee for user-provided compensations
- **Distributed Support**: Remote coordination is not provided by this class alone
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

// If the enclosing transaction times out or its commit fails, compensation
// callbacks registered with its Saga are invoked. External effects need
// application-defined compensation and are not atomically committed.
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

// Commit covers the local RocksDB transaction; registered callbacks are not a distributed commit protocol.
auto status = txn.commit();
if (!status.ok) {
    // Registered compensation callbacks are invoked; external effects are not atomic.
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
**Location:** `snapshot_manager.cpp`, `../../include/transaction/snapshot_manager.h`

Associates named tags with changefeed sequence numbers. `restoreToTag()` validates a tag and writes an audit restore-point tag at the current sequence; it does not restore or rewind stored data.

**Features:**
- **Named Tags**: Semantic labels for database states
- **Changefeed Integration**: Maps tags to sequence numbers
- **`restoreToTag()`**: validates the referenced tag and writes a current-sequence audit tag; it is not data restoration or point-in-time recovery
- **Audit Checkpoints**: Compliance and governance snapshots
- **Tag Metadata**: Description, creator, timestamp
- **REST API**: TODO: verify current endpoint registration before claiming it

**Usage Example:**
```cpp
#include "transaction/snapshot_manager.h"

themis::transaction::SnapshotManager snapshot_mgr(rocksdb, changefeed);

// Create a named tag
auto result = snapshot_mgr.createTag(
    "pre-deployment-v2.1",
    "Snapshot before deploying schema changes",
    "admin@company.com"
);

if (result) {
    std::cout << "Created tag at sequence: "
              << result->sequence_number << std::endl;
}

// List tags
auto tags = snapshot_mgr.listTags();
for (const auto& tag : tags) {
    std::cout << tag.tag_name << " @ seq "
              << tag.sequence_number << std::endl;
}

// Get a specific tag
auto snap = snapshot_mgr.getTag("pre-deployment-v2.1");
if (snap) {
    std::cout << "Created at: " << snap->timestamp_ms
              << " by " << snap->created_by << std::endl;
}

// Delete an old tag
snapshot_mgr.deleteTag("old-tag");

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
- Named changefeed sequence references for operational tracking; `restoreToTag()` does not rewind or restore stored data.

---

### BranchManager
**Location:** `branch_manager.cpp`, `../../include/transaction/branch_manager.h`

Persists branch metadata and sequence references and tracks an active-branch label. Branch switching does not route `TransactionManager` operations or create an isolated data namespace.

**Features:**
- **Named Branches**: Persisted branch metadata and sequence references
- **Parent Tracking**: Branch hierarchy and lineage
- **Branch Switching**: Update the stored active-branch label and append history; transaction writes are not redirected by this call
- **Branch Creation**: From tags, sequences, or timestamps
- **Merge Support**: Integration with MergeEngine
- **Tenant isolation**: No branch-specific tenant isolation is established by this API.
- **REST API**: TODO: verify current endpoint registration before claiming it

**Branch Lifecycle:**
```
CREATE → DEVELOP → MERGE → DELETE (optional)
  ↓         ↓         ↓         ↓
Tag/Seq  External  Resolve   Cleanup
         workflow  Conflicts
```

**Usage Example:**
```cpp
#include "transaction/branch_manager.h"

// diff_engine is initialized by the application.
themis::transaction::MergeEngine merge_engine(diff_engine, snapshot_mgr, changefeed);
themis::transaction::BranchManager branch_mgr(
    rocksdb, changefeed, snapshot_mgr, &merge_engine);

// Create branch from current state
auto result = branch_mgr.createBranch(
    "feature-new-schema",
    "main",  // parent branch
    "Experimental schema changes",
    "developer@company.com"
);

// Create branch from specific tag
themis::transaction::BranchManager::CreateBranchOptions opts;
opts.from_tag = "pre-deployment-v2.1";
branch_mgr.createBranch(
    "test-deployment",
    "main",
    "Testing v2.1 deployment",
    "qa@company.com",
    opts
);

// Update the active-branch metadata; this does not redirect transaction writes.
branch_mgr.switchBranch("feature-new-schema");

// List branches
auto branches = branch_mgr.listBranches();
for (const auto& branch : branches) {
    std::cout << branch.branch_name;
    if (branch.is_active) std::cout << " (active)";
    std::cout << std::endl;
}

// MergeEngine is configured on the manager above.
auto merge_result = branch_mgr.mergeBranches("feature-new-schema", "main");

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
- Branch metadata and lineage tracking for changefeed sequence points
- Merge preview and resolution using the configured `MergeEngine`

---

### MergeEngine
**Location:** `merge_engine.cpp`, `../../include/transaction/merge_engine.h`

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
// Nested in themis::transaction::MergeEngine
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

// These dependencies are initialized by the application.
themis::transaction::MergeEngine merge_engine(diff_engine, snapshot_mgr, changefeed);

// Merge tags using the source-side value when a conflict requires resolution.
themis::transaction::MergeEngine::MergeOptions options;
options.strategy = themis::transaction::MergeEngine::MergeStrategy::THEIRS;
auto result = merge_engine.mergeByTag("base-tag", "feature-tag", "main-tag", options);

if (result.success) {
    std::cout << "Merge succeeded" << std::endl;
    std::cout << "Changes applied: " << result.changes_applied.size() << std::endl;
} else {
    std::cout << "Merge failed: " << result.message << std::endl;
    std::cout << "Conflicts detected: " << result.conflicts.size() << std::endl;

    // Examine conflicts
    for (const auto& conflict : result.conflicts) {
        std::cout << "Conflict on key: " << conflict.key << std::endl;
        std::cout << "Type: ";
        using ConflictType = themis::transaction::MergeEngine::ConflictType;
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
// Supply a key/value choice for each conflict requiring manual resolution.
themis::transaction::MergeEngine::MergeOptions options;
options.strategy = themis::transaction::MergeEngine::MergeStrategy::MANUAL;
options.manual_resolutions.push_back({"conflicting-key", std::string("chosen value")});
auto result = merge_engine.mergeByTag("base-tag", "feature-tag", "main-tag", options);
```

**Merge Strategies:**
- **OURS**: Prefer target branch changes (keep existing)
- **THEIRS**: Prefer source branch changes (accept incoming)
- **MANUAL**: Require explicit resolution for conflicts
- **FAST_FORWARD**: Only succeed if no conflicts exist

**Performance:** No source-backed complexity or latency bound is asserted here. TODO: add benchmark evidence before publishing numerical or algorithmic performance guarantees.

---

## ACID Properties

### Atomicity
The transaction wrapper commits through its underlying RocksDB transaction. This does not imply atomicity for arbitrary external side effects or every separately managed index/service.

**Implementation:**
```cpp
// Transaction operations are applied through the underlying RocksDB transaction
txn.putEntity("users", user);
txn.addEdge(edge);
txn.addVector(embedding);

// Commit the underlying transaction
auto status = txn.commit();
// Handle each operation status; no broader external-side-effect guarantee is implied.
```

### Consistency
Application/data-model consistency is not established by the transaction API alone.

**Validation:**
- TODO: identify current schema, reference, uniqueness, and type-validation call sites before asserting these constraints.

### Isolation
Isolation level determines documented visibility and conflict behavior; the API does not promise that concurrent transactions never interfere.

**Isolation Levels:**

**ReadCommitted (Default):**
- Reads see only committed data
- No dirty reads
- Non-repeatable reads possible
- Phantom reads possible

**Snapshot:**
- Consistent point-in-time view
- Repeatable reads guaranteed
- Phantom behavior depends on the read/predicate path; do not claim blanket phantom prevention for `REPEATABLE_READ`/legacy `Snapshot`.

`SERIALIZABLE` is documented in `isolation_level.h` as SSI with write-conflict detection. Do not infer implementation-wide performance comparisons from these isolation descriptions.

### Durability
Durability settings depend on the configured RocksDB wrapper and transaction options. **TODO:** document the active write/sync options and recovery guarantee from their canonical configuration; this README does not claim a particular WAL sync mode.

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
1. Background detector wait interval: TODO: verify current interval/default from implementation before documenting a numeric value
2. Builds wait-for graph from held_locks_ and waiting_for_ maps
3. Performs depth-first search to find cycles
4. If cycle detected:
   - Select a victim according to the configured `DeadlockVictimPolicy` (default: `YOUNGEST`)
   - Abort victim transaction
   - Log deadlock information
   - Wake up waiters

**Configuration:**
```cpp
// Enable/disable detection
txn_mgr.setDeadlockDetection(true);

// Set deadlock timeout (header default: 1000 ms)
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

**Performance overhead:** No verified numeric cost is established here; TODO: add benchmark evidence before documenting timing or per-transaction memory estimates.

---

## Integration with Other Modules

### Storage Module
```cpp
// TransactionManager wraps RocksDB operations
auto txn = txn_mgr.begin();
txn.putEntity("users", entity);  // → underlying RocksDB transaction
txn.commit();                     // → underlying transaction commit
```

### Index Module
```cpp
// Entity write; any secondary-index behavior is determined by the transaction implementation
auto txn = txn_mgr.begin();
txn.putEntity("users", user);
// TODO: verify which index mutations are enlisted by this API before asserting automatic updates
txn.commit();
```

### Graph Module
```cpp
// Add an edge through the transaction and graph-index APIs
auto txn = txn_mgr.begin();
txn.addEdge(edge);
txn.commit();
```

### Vector Module
```cpp
// Vector mutation and index update use the transaction wrapper
auto txn = txn_mgr.begin();
txn.putEntity("documents", doc);
txn.addVector(doc, "embedding");
txn.commit();
```

### CDC Module
```cpp
// TODO: verify direct Changefeed integration and commit ordering before asserting capture
auto txn = txn_mgr.begin();
txn.putEntity("users", user);
txn.commit();
```

---

## Performance Tuning

### Transaction Size
**Guideline:** Keep transactions bounded for workload-specific lock/latency needs. No source-backed numeric maximum is established in this guide.

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
**ReadCommitted:** This is the declared default; workload suitability and relative speed require measurements.
**Snapshot:** Legacy alias for `REPEATABLE_READ`, described by the header as snapshot isolation.

> TODO: verify the exact write-skew and predicate-read behavior of the current transaction paths before stating workload-specific guarantees for snapshot isolation.

**SerializableSnapshot:** Legacy alias for `SERIALIZABLE`; the header describes it as SSI with write-conflict detection. Do not infer measured latency or abort costs here.

```cpp
// OLTP: frequent, small transactions (fast, but write-skew possible)
auto txn = txn_mgr.begin(IsolationLevel::ReadCommitted);

// Analytics: long-running, consistent reads
// TODO: verify write-skew and predicate-read semantics for the specific read path.
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
themis::transaction::MergeEngine::MergeOptions options;
options.strategy = themis::transaction::MergeEngine::MergeStrategy::FAST_FORWARD;
auto result = merge_engine.mergeByTag("base-tag", "feature-tag", "main-tag", options);

if (!result.success) {
    if (!result.conflicts.empty()) {
        // Handle conflicts
        THEMIS_WARN("Merge conflicts detected: {}", result.conflicts.size());

        for (const auto& conflict : result.conflicts) {
            THEMIS_INFO("Conflict on key: {}", conflict.key);
            // Resolve manually or use different strategy
        }

        // Supply explicit key/value resolutions before retrying.
        options.strategy = themis::transaction::MergeEngine::MergeStrategy::MANUAL;
        options.manual_resolutions.push_back(
            {"conflicting-key", std::string("chosen value")});
        auto manual_result =
            merge_engine.mergeByTag("base-tag", "feature-tag", "main-tag", options);
    } else {
        THEMIS_ERROR("Merge failed: {}", result.message);
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

## Tests and benchmarks

`tests/transaction/CMakeLists.txt` conditionally declares executable targets when their source files exist. Examples of actual target/source pairs include:

| Executable target | Source |
|---|---|
| `test_transaction_manager_focused` | `test_transaction_manager.cpp` |
| `test_transaction_distributed_2pc_focused` | `test_transaction_distributed_2pc.cpp` |
| `test_adaptive_deadlock_prevention_focused` | `test_adaptive_deadlock_prevention.cpp` |
| `test_transaction_lifecycle_phase1_focused` | `test_transaction_lifecycle_phase1.cpp` |
| `test_transaction_distributed_phase2_focused` | `test_transaction_distributed_phase2.cpp` |
| `test_transaction_saga_compensation_phase2_focused` | `test_transaction_saga_compensation_phase2.cpp` |
| `test_transaction_fault_injection_phase3_focused` | `test_transaction_fault_injection_phase3.cpp` |

Eligible remaining `test_transaction_*.cpp` files are registered using the generated target pattern `module_transaction_<stem>_autofocused`; explicit targets and excluded sources are handled separately in the CMake file. The registered CTest names are distinct from executable target names. Benchmarks are registered in `benchmarks/transaction/CMakeLists.txt`, including `bench_transaction_phase4`, `bench_transaction_throughput`, and `bench_transaction_dedicated_gates`. Registration is not evidence that a target was built or executed.

---

## Thread Safety Summary

| Component | Thread Safety | Notes |
|-----------|---------------|-------|
| TransactionManager | Internal synchronization is present | Do not infer that every method/caller interaction is thread-safe |
| Transaction | No blanket guarantee asserted here | Consult current API comments |
| Saga | No concurrent-use guarantee asserted here | Consult current API comments |
| SnapshotManager, BranchManager, MergeEngine | TODO: verify each header's current guarantee | No blanket claim |

---

## Dependencies

**Internal:**
- `themis::RocksDBWrapper` - Underlying storage engine
- `themis::SecondaryIndexManager` - Secondary index operations
- `themis::GraphIndexManager` - Graph edge operations
- `themis::VectorIndexManager` - Vector index operations
- `themis::Changefeed` - Change data capture
- `themis::analytics::DiffEngine` - Diff operations used by merge utilities

**External:** Dependencies are determined by the current target configuration and headers; TODO: cite the actual dependency/version declarations before publishing a version-specific list.

---

## Configuration

The headers expose runtime setters, including transaction/deadlock timeouts, deadlock detection, SSI configuration, and per-transaction timeout/read-only options. **TODO:** document deployment environment variables or YAML keys only after confirming them in current configuration parsing and build/runtime wiring; no such mapping is established by the transaction headers cited here.

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
txn.putEntity("users", user);  // Does not assert automatic updates to independent indexes
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

## Sourcecode Verification (Module: transaction/readme)

- Verified core surfaces:
    - `src/transaction/transaction_manager.cpp`
    - `include/transaction/transaction_manager.h`
    - `src/transaction/distributed_transaction_manager.cpp`
    - `src/transaction/saga_orchestrator.cpp`
    - `src/transaction/distributed_saga.cpp`
    - `src/transaction/transaction_batcher.cpp`
    - `src/transaction/transaction_auditor.cpp`
- Verified behavior classes:
    - Transaction lifecycle and isolation entry points
    - Distributed prepare/commit/abort orchestration and recovery hooks
    - SAGA compensation/orchestration flows
    - Batching and audit utility paths
- Note:
    - Forward planning is tracked in `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md`.
    - Historical implementation record remains in `CHANGELOG.md`.

## Installation

Transaction sources are compiled through repository CMake configuration: monolithic source integration is in `cmake/CMakeLists.txt`, while modular builds declare the `themis_transaction` target in `cmake/ModularBuild.cmake`. No `themis::transaction` alias was found.
