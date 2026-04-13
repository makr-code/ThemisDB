<!-- Status: current | validated: 2026-04-10 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Transaction Module - Future Enhancements

The transaction module provides ACID transaction semantics for all ThemisDB data operations. It covers: multi-version concurrency control (MVCC) via RocksDB transactions, three isolation levels (`ReadCommitted`, `Snapshot`, `SerializableSnapshot`/SSI with predicate locking), optimistic concurrency control (OCC) with per-entity version numbers, two-phase commit (2PC) coordinator for distributed multi-shard transactions, SAGA orchestration (sequential and parallel DAG), distributed SAGA with cross-service compensation, named savepoints with partial rollback, bulk write batching and coalescing, read-only transaction fast path, and a transaction audit trail. Affected source files: `transaction_manager.cpp`, `transaction_manager.h`, `saga_manager.cpp`, and associated headers under `include/transaction/`.

---

## Design Constraints

- [ ] All transaction types (regular, OCC, distributed) must be created through the same `TransactionManager` public API; callers must not need to know the underlying implementation details.
- [ ] MVCC snapshot isolation must be implemented using RocksDB `OptimisticTransactionDB` or `TransactionDB` APIs; no custom MVCC layer is permitted.
- [ ] The 2PC coordinator log must be persisted to RocksDB before sending `PREPARE` votes; coordinator crashes must be recoverable without data loss or duplicate commits.
- [ ] SAGA compensation functions must be idempotent; the orchestrator may invoke them more than once during failure recovery without producing inconsistent state.
- [ ] OCC version keys (`occ:ver:{table}:{pk}`) must never collide with entity data keys; the `occ:ver:` prefix is reserved and must be rejected in user-facing table and key names.
- [ ] Transaction timeouts and deadlock detection must not rely on busy-waiting; all timeouts use OS-level condition variables or RocksDB deadline mechanisms.

---

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `TransactionManager::begin(isolation)` | All data mutation paths (AQL, REST, graph, vector) | Returns a `Transaction` handle; isolation level determines lock and snapshot behavior |
| `Transaction::optimisticPut(table, entity, expected_version)` | OCC retry loops in query engine | Fails with version-mismatch error when stored version ≠ `expected_version` |
| `Transaction::createSavepoint(name)` / `rollbackToSavepoint(name)` | Multi-step write operations, SAGA steps | Backed by RocksDB `SetSavePoint` / `RollbackToSavePoint` |
| `DistributedTransactionManager::beginDistributed(participants)` | Distributed query coordinator, shard manager | Returns a `TransactionId`; drives 2PC prepare/commit/abort lifecycle |
| `SAGAOrchestrator::execute(saga_definition)` | Business logic layer, REST saga endpoint | Executes steps in dependency order; invokes compensation in reverse order on failure |
| `TransactionAuditor::queryAuditLog(user_id, start, end, limit)` | Admin API, compliance export | Returns `AuditRecord` list; backed by a dedicated RocksDB column family |

---

## Planned Features

### Serializable Snapshot Isolation (SSI)
**Priority:** High  
**Target Version:** v1.8.0

Add full serializability with snapshot isolation using predicate locking and conflict detection.

**Features:**
- Predicate lock tracking for range queries
- Read-write conflict detection
- Write-write conflict detection  
- Automatic serialization failure detection
- Transaction retry with exponential backoff

**Architecture:**
```cpp
enum class IsolationLevel {
    ReadCommitted,
    Snapshot,
    SerializableSnapshot  // New level
};

class TransactionManager {
public:
    struct SSIConfig {
        bool enable_predicate_locking = true;
        size_t max_predicate_locks = 10000;
        std::chrono::milliseconds conflict_detection_interval{100};
    };
    
    void setSSIConfig(const SSIConfig& config);
    
    // Predicate lock management
    void trackPredicateLock(TransactionId txn_id, 
                           const PredicateLock& predicate);
    
    // Conflict detection
    std::vector<SerializationConflict> detectConflicts(TransactionId txn_id);
};

// Example usage
auto txn = txn_mgr.begin(IsolationLevel::SerializableSnapshot);
txn.putEntity("accounts", account);  // Tracked for conflicts
auto result = txn.commit();  // May fail with serialization error
if (!result.ok && result.message.find("Serialization") != std::string::npos) {
    // Retry transaction
}
```

**Implementation Details:**
- SIREAD locks for reads that may cause conflicts
- Commit-time validation of read/write sets
- False positive rate: <5% (tunable with granularity)
- Performance overhead: 10-15% vs Snapshot isolation

**Benefits:**
- True serializability without holding locks
- Eliminates write skew anomalies
- Prevents lost updates in complex scenarios
- Better than traditional 2PL for read-heavy workloads

---

### Optimistic Concurrency Control (OCC)
**Status: ✅ Implemented** (v1.x)  
**Priority:** Medium  
**Target Version:** v1.8.0

Optimistic locking with per-entity version numbers is fully implemented in `TransactionManager::Transaction`.

**Implemented Features:**
- Per-entity version numbers stored under `occ:ver:{table}:{pk}` as a little-endian uint64_t
- `getEntityVersion` — read current OCC version without acquiring a lock (returns 0 for non-existent entities, `std::nullopt` when transaction is inactive)
- `optimisticPut` — write entity only when stored version matches `expected_version`; atomically increments version to `expected_version + 1` on success; pass `expected_version = 0` to create a new entity
- `optimisticErase` — delete entity only when stored version matches `expected_version`; resets version to 0 (entity gone) on success
- Version conflict detection with descriptive error messages (includes `expected=N actual=M`)
- Secondary-index updates (`SecondaryIndexManager`) applied atomically within the OCC write
- SSI / predicate-lock integration: `checkSerializableWriteConflict` is called by both `optimisticPut` and `optimisticErase` so SERIALIZABLE transactions remain correct
- Works with all isolation levels (`READ_COMMITTED`, `REPEATABLE_READ`, `SERIALIZABLE`)
- Transaction-timeout guard: returns an error instead of proceeding when `isTimedOut()` is true

**Architecture:**
```cpp
class Transaction {
public:
    // Read current version (no lock acquired)
    std::optional<uint64_t> getEntityVersion(std::string_view table,
                                             std::string_view pk);

    // Write entity only if version matches expected_version
    Status optimisticPut(std::string_view table,
                         const BaseEntity& entity,
                         uint64_t expected_version);

    // Delete entity only if version matches expected_version
    Status optimisticErase(std::string_view table,
                            std::string_view pk,
                            uint64_t expected_version);
};

// Example: Read-modify-write (OCC retry pattern)
bool committed = false;
while (!committed) {
    auto id  = mgr.beginTransaction();
    auto txn = mgr.getTransaction(id);

    auto ver = txn->getEntityVersion("users", user_id);
    if (!ver) { mgr.rollbackTransaction(id); break; } // txn inactive

    user.age += 1;
    auto st = txn->optimisticPut("users", user, *ver);
    if (!st.ok) {
        mgr.rollbackTransaction(id);
        continue; // version conflict – retry
    }
    committed = mgr.commitTransaction(id).ok;
}
```

**Use Cases:**
- Low-contention workloads (>90% success rate)
- Short-lived transactions
- Read-mostly workloads
- Mobile/offline sync scenarios

**Performance:**
- 2-3x faster than pessimistic locking (no contention)
- Graceful degradation under contention
- Retry cost: ~1ms per attempt

**Implementation:**
- Version keys are stored in RocksDB alongside entity data under `occ:ver:{table}:{pk}`
- Version encoding: 8-byte little-endian uint64_t (`encodeVersion` / `decodeVersion` helpers)
- `optimisticPut` / `optimisticErase` use `mvcc_txn_->put` / `mvcc_txn_->del` so all writes participate in the MVCC snapshot; MVCC conflict errors are surfaced as `Status::Error`
- Tests: `tests/test_transaction_occ.cpp` (13 unit tests covering creation, update, erase, version-conflict detection, retry pattern, isolation-level compatibility, and null-opt guard)
- Benchmarks: `benchmarks/bench_transaction_throughput.cpp` — `OccOptimisticPut`, `OccReadVersionAndUpdate`, `OccOptimisticErase`

---

### Distributed Transaction Coordinator (2PC)
**Status: ✅ Implemented** (v1.9.0, Issue: #123)  
**Priority:** High  
**Target Version:** v1.9.0

Implement two-phase commit for multi-shard distributed transactions.

**Implemented in:**
- `include/transaction/distributed_transaction_manager.h`
- `src/transaction/distributed_transaction_manager.cpp`
- Tests: `tests/test_transaction_distributed_2pc.cpp` (32 tests, `TransactionDistributed2PCFocusedTests`)
- CI: `.github/workflows/transaction-distributed-2pc-ci.yml`

**Features:**
- Coordinator role for distributed transactions
- Prepare phase with voting
- Commit/abort phase coordination
- Participant recovery
- Timeout handling
- Failure detection

**Architecture:**
```cpp
class DistributedTransactionManager {
public:
    struct Participant {
        std::string node_id;
        std::string endpoint;
        std::set<std::string> affected_keys;
    };
    
    struct DistributedTransaction {
        TransactionId txn_id;
        std::vector<Participant> participants;
        enum State { INIT, PREPARED, COMMITTED, ABORTED };
        State state;
        std::chrono::system_clock::time_point timeout;
    };
    
    // Coordinator API
    TransactionId beginDistributed(const std::vector<Participant>& participants);
    Status prepareDistributed(TransactionId txn_id);
    Status commitDistributed(TransactionId txn_id);
    void abortDistributed(TransactionId txn_id);
    
    // Participant API
    Status voteOnPrepare(TransactionId txn_id, bool can_commit);
    Status applyCommit(TransactionId txn_id);
    Status applyAbort(TransactionId txn_id);
};

// Example: Distributed transaction
std::vector<Participant> participants = {
    {"shard1", "10.0.0.1:8080", {"users:123"}},
    {"shard2", "10.0.0.2:8080", {"accounts:456"}}
};

auto dtxn_id = dist_txn_mgr.beginDistributed(participants);

// Phase 1: Prepare
auto prepare_status = dist_txn_mgr.prepareDistributed(dtxn_id);
if (!prepare_status.ok) {
    dist_txn_mgr.abortDistributed(dtxn_id);
    return;
}

// Phase 2: Commit
auto commit_status = dist_txn_mgr.commitDistributed(dtxn_id);
```

**Failure Handling:**
- Coordinator crash: Recovery from persistent log
- Participant crash: Replay from WAL
- Network partition: Timeout-based abort
- Partial commit: Automatic rollback

**Performance:**
- Latency: 2-5ms per phase (local network)
- Throughput: Limited by coordinator bottleneck
- Optimization: Batched prepare/commit messages

---

### SAGA Orchestration Engine
**Status: ✅ Implemented** (v1.8.0)  
**Priority:** Medium  
**Target Version:** v1.8.0

Advanced SAGA coordination with parallel execution and conditional logic.

**Features:**
- Parallel step execution (DAG-based)
- Conditional branching
- Retry policies per step
- Timeout management
- SAGA templates
- Visual workflow designer

**Architecture:**
```cpp
class SAGAOrchestrator {
public:
    struct Step {
        std::string name;
        std::function<void()> forward;
        std::function<void()> compensate;
        std::set<std::string> depends_on;  // Dependencies
        std::chrono::milliseconds timeout{5000};
        size_t max_retries = 3;
        std::chrono::milliseconds retry_delay{1000};
    };
    
    struct SAGADefinition {
        std::string name;
        std::vector<Step> steps;
        bool enable_parallel = true;
    };
    
    // Execute SAGA with orchestration
    Status execute(const SAGADefinition& saga);
    
    // Get execution status
    struct ExecutionStatus {
        std::string saga_name;
        std::map<std::string, StepState> step_states;
        size_t completed_steps;
        size_t failed_steps;
        size_t pending_steps;
    };
    
    ExecutionStatus getStatus(const std::string& saga_id);
};

// Example: Parallel SAGA
SAGAOrchestrator::SAGADefinition order_saga;
order_saga.name = "process_order";
order_saga.enable_parallel = true;

// These can run in parallel (no dependencies)
order_saga.steps.push_back({
    "reserve_inventory",
    []() { inventory_service.reserve(); },
    []() { inventory_service.release(); },
    {}  // No dependencies
});

order_saga.steps.push_back({
    "validate_customer",
    []() { customer_service.validate(); },
    []() { /* no compensation */ },
    {}  // No dependencies
});

// This waits for both above steps
order_saga.steps.push_back({
    "charge_payment",
    []() { payment_service.charge(); },
    []() { payment_service.refund(); },
    {"reserve_inventory", "validate_customer"}  // Dependencies
});

saga_orchestrator.execute(order_saga);
```

**Visualization:**
```
reserve_inventory ──┐
                    ├──> charge_payment ──> ship_order
validate_customer ──┘
```

**Benefits:**
- 2-3x faster than sequential SAGA
- Better resource utilization
- Complex workflow support
- Automatic dependency resolution

---

### Transaction Savepoints
**Status: ✅ Implemented** (v1.x)  
**Priority:** Medium  
**Target Version:** v1.8.0

Named savepoint support with partial rollback is fully implemented in `TransactionManager::Transaction`.

**Implemented Features:**
- Named savepoints within transaction (`createSavepoint`, `rollbackToSavepoint`, `releaseSavepoint`)
- Rollback to savepoint — undoes writes without aborting the full transaction
- Automatic savepoint cleanup on rollback/release
- Savepoint stacking with correct LIFO ordering
- SAGA step trimming — compensating actions added after a savepoint are discarded on rollback

**Architecture:**
```cpp
class Transaction {
public:
    // Savepoint API — implemented in transaction_manager.h
    Status createSavepoint(std::string_view name);
    Status rollbackToSavepoint(std::string_view name);
    Status releaseSavepoint(std::string_view name);
    
    // Query savepoints
    std::vector<std::string> getSavepoints() const;
    bool hasSavepoint(std::string_view name) const;
};

// Example usage
auto txn = txn_mgr.begin();

txn.putEntity("users", user1);
txn.createSavepoint("after_user1");

txn.putEntity("users", user2);
txn.createSavepoint("after_user2");

txn.putEntity("users", user3);

// Oops, user3 was invalid - rollback to after_user2
txn.rollbackToSavepoint("after_user2");

// user1 and user2 still in transaction, user3 discarded
txn.commit();
```

**Use Cases:**
- Complex multi-step operations
- Error recovery within transaction
- Conditional processing
- Nested logic with partial rollback

**Implementation:**
- Backed by RocksDB `SetSavePoint` / `RollbackToSavePoint` / `PopSavePoint`
- `savepoints_` vector in `Transaction` tracks named entries in creation order
- SAGA `trimToSize` removes compensating actions added after the rollback point

---

### Adaptive Deadlock Prevention
**Priority:** Low  
**Target Version:** v1.9.0

Machine learning-based deadlock prediction and prevention.

**Features:**
- Historical deadlock pattern analysis
- Lock acquire order recommendation
- Proactive transaction reordering
- Dynamic timeout adjustment
- Deadlock probability scoring

**Architecture:**
```cpp
class DeadlockPredictor {
public:
    struct LockPattern {
        std::vector<std::string> keys;
        std::chrono::microseconds hold_time;
        size_t frequency;
    };
    
    // Learn from transaction history
    void recordTransaction(TransactionId txn_id,
                          const std::vector<std::string>& locks_acquired,
                          std::chrono::microseconds duration);
    
    // Predict deadlock probability
    double predictDeadlockProbability(
        const std::vector<std::string>& proposed_locks,
        const std::set<TransactionId>& active_transactions
    );
    
    // Recommend lock order
    std::vector<std::string> recommendLockOrder(
        const std::vector<std::string>& keys
    );
    
    // Suggest timeout
    std::chrono::milliseconds recommendTimeout(
        const std::vector<std::string>& keys
    );
};

// Example integration
auto predictor = deadlock_predictor.predictDeadlockProbability(
    {"users:123", "accounts:456"},
    active_transactions
);

if (predictor > 0.8) {
    // High deadlock risk - reorder or delay
    auto recommended_order = deadlock_predictor.recommendLockOrder(
        {"users:123", "accounts:456"}
    );
    // Acquire in recommended order
}
```

**ML Model:**
- Features: Lock patterns, transaction duration, active count
- Algorithm: Gradient boosting classifier
- Training: Online learning from deadlock events
- Accuracy target: >85% precision, >90% recall

---

### Write Batching and Coalescing
**Priority:** Medium  
**Target Version:** v1.8.0  
**Status:** ✅ Implemented

Automatic batching of concurrent small transactions for improved throughput.

**Implementation:**
- `include/transaction/transaction_batcher.h` — `TransactionBatcher` class
- `src/transaction/transaction_batcher.cpp` — implementation
- `tests/test_transaction_batcher.cpp` — 26 focused tests (`TransactionBatcherFocusedTests`)
- `.github/workflows/transaction-write-batching-ci.yml` — CI workflow

**Features (all implemented):**
- [x] Automatic transaction grouping via background flush thread + `std::deque` queue
- [x] Configurable batch window (1–100 ms) with clamping in `setBatchConfig()`
- [x] Fair FIFO scheduling — items processed in submission order, no starvation
- [x] Per-table/per-key batching policies via `setTablePolicy(table, BatchPolicy)`
- [x] Adaptive batch sizing — window widened (+10%) under low load, narrowed (-10%) near overflow

**Architecture (as delivered):**
```cpp
class TransactionBatcher {
public:
    struct BatchConfig {
        std::chrono::microseconds window{5000};  // 5ms batch window (1–100ms)
        size_t max_batch_size = 1000;
        size_t min_batch_size = 10;
        bool enable_adaptive = true;
    };

    struct BatchPolicy {
        std::chrono::microseconds window{0};   // 0 = use global
        size_t max_batch_size{0};              // 0 = use global
        size_t min_batch_size{0};              // 0 = use global
    };

    void setBatchConfig(const BatchConfig& config);

    // Submit a commit callable for batched asynchronous execution
    std::future<Status> submitAsync(std::function<Status()> commit_fn,
                                    const std::string& table_hint = "");

    // Force flush current batch
    void flush();

    void setTablePolicy(const std::string& table, const BatchPolicy& policy);
};

// Example: High-throughput ingestion
TransactionBatcher batcher;
batcher.setBatchConfig({
    .window = std::chrono::milliseconds(10),
    .max_batch_size = 5000,
    .enable_adaptive = true
});

// Submit many small transactions
std::vector<std::future<TransactionBatcher::Status>> futures;
for (const auto& user : users) {
    futures.push_back(batcher.submitAsync(
        [&mgr, user]() -> TransactionBatcher::Status {
            auto id  = mgr.beginTransaction();
            auto* tx = mgr.getTransaction(id);
            tx->putEntity("users", user);
            auto st  = mgr.commitTransaction(id);
            return st.ok ? TransactionBatcher::Status::OK()
                         : TransactionBatcher::Status::Error(st.message);
        },
        "users"
    ));
}

// All transactions batched and committed together
for (auto& f : futures) {
    auto status = f.get();
}
```

**Performance Gains:**
- Small transactions: 10-100x throughput improvement (amortised WAL sync)
- Reduced WAL sync overhead via batched commits
- Better CPU/disk utilization
- Latency trade-off: +1-10ms per transaction (configurable via `window`)

---

### Read-Only Transaction Optimization
**Status: ✅ Implemented** (v1.8.0)  
**Priority:** Low  
**Target Version:** v1.8.0

Lightweight read-only transactions with no locking overhead.

**Features:**
- Automatic read-only detection
- No lock acquisition
- No WAL logging
- Snapshot reuse across transactions
- Query result caching

**Architecture:**
```cpp
class Transaction {
public:
    // Mark transaction as read-only
    void setReadOnly(bool read_only = true);
    bool isReadOnly() const;
    
    // Automatic detection
    bool hasWrites() const;
};

// Explicit read-only
auto txn = txn_mgr.begin(IsolationLevel::Snapshot);
txn.setReadOnly(true);
auto users = executeQuery("FOR u IN users RETURN u", txn);
txn.commit();  // No-op, just releases snapshot

// Automatic detection
auto txn = txn_mgr.begin();
auto users = executeQuery("FOR u IN users RETURN u", txn);
// No writes detected - optimized as read-only
txn.commit();  // Fast path
```

**Optimizations:**
- No WriteBatch allocation
- No lock tracking
- Reuse RocksDB snapshot across multiple transactions
- 5-10x faster commit for read-only workloads

---

### Distributed SAGA Coordinator
**Status: ✅ Implemented** (v1.9.0)  
**Priority:** High  
**Target Version:** v1.9.0

Cross-cluster SAGA coordination with failure recovery.

**Features:**
- ✅ Multi-cluster orchestration — `RemoteStep` + `executeDistributed()` with pluggable `RemoteStepExecutor` transport
- ✅ Persistent SAGA state — append-only JSON-lines journal (`journal_path`)
- ✅ Automatic recovery after coordinator crash — `recoverInProgressSAGAs()` reads journal, identifies orphaned SAGAs
- ✅ Compensation retry policies — per-step `max_retries` + exponential backoff on both forward and compensating actions
- ✅ SAGA visualization and debugging — `visualize()` emits Graphviz DOT + plain-text summary
- ✅ Manual intervention API — `forceCompensate()` / `forceComplete()` for stuck SAGAs

**Architecture:**
```cpp
// Remote step (cross-cluster)
struct RemoteStep {
    std::string service_endpoint;   // e.g. "http://inventory:8080"
    std::string operation;          // e.g. "/reserve"
    nlohmann::json params;
    std::string compensate_operation; // e.g. "/release"
    nlohmann::json compensate_params;
    std::string name;
    std::set<std::string> depends_on;
    std::chrono::milliseconds forward_timeout{5000ms};
    std::chrono::milliseconds compensate_timeout{10000ms};
    size_t max_retries{3};
    std::chrono::milliseconds retry_backoff{100ms};
};

struct DistributedSAGADefinition {
    std::string saga_id;
    std::vector<RemoteStep> steps;
    std::map<std::string, std::string> context;
};

// Pluggable transport
using RemoteStepExecutor =
    std::function<DistributedSagaStatus(endpoint, operation, params)>;

class DistributedSagaCoordinator {
public:
    // Multi-cluster execution
    DistributedSagaReport executeDistributed(const DistributedSAGADefinition& saga);

    // Status query
    std::optional<DistributedSagaReport> getDistributedStatus(const std::string& saga_id) const;

    // Crash recovery
    std::vector<std::string> recoverInProgressSAGAs();

    // Visualization
    SagaVisualization visualize(const DistributedSagaDefinition& saga) const;

    // Manual intervention
    bool forceCompensate(const std::string& saga_id);
    bool forceComplete(const std::string& saga_id);
};
```

**Implementation:**
- `include/transaction/distributed_saga.h` — header with all types and methods
- `src/transaction/distributed_saga.cpp` — full implementation
- `tests/test_distributed_saga.cpp` — comprehensive unit tests (700+ lines)

**Failure Recovery:**
- ✅ Persistent SAGA log (append-only JSON-lines journal file, `journal_path` config)
- ✅ Automatic step retry with exponential backoff (capped at 30 s, per-step configurable)
- ✅ `recoverInProgressSAGAs()` detects orphaned SAGAs from journal on coordinator restart
- ✅ Manual intervention API (`forceCompensate`, `forceComplete`) for stuck SAGAs
- Note: Full RocksDB persistence and coordinator election are post-v1.9.0 enhancements;
  journal-based persistence + manual recovery covers the v1.9.0 acceptance criteria.

---

### Transaction Audit Trail
**Status: ✅ Implemented** (v1.8.0)  
**Priority:** Medium  
**Target Version:** v1.8.0

Comprehensive transaction logging for compliance and debugging.

**Features:**
- Full transaction lifecycle logging
- Operation-level audit records
- User/session attribution
- Retention policies
- Query interface for audit logs
- Export to external systems (Kafka, S3)

**Architecture:**
```cpp
class TransactionAuditor {
public:
    struct AuditRecord {
        TransactionId txn_id;
        std::string user_id;
        std::string session_id;
        std::chrono::system_clock::time_point timestamp;
        IsolationLevel isolation;
        std::vector<Operation> operations;
        enum Result { COMMITTED, ABORTED, DEADLOCK };
        Result result;
        uint64_t duration_us;
    };
    
    struct Operation {
        enum Type { PUT, DELETE, ADD_EDGE, DELETE_EDGE, ADD_VECTOR };
        Type type;
        std::string table;
        std::string key;
        std::optional<std::string> old_value;
        std::optional<std::string> new_value;
    };
    
    // Enable auditing
    void enableAuditing(bool enabled);
    
    // Query audit log
    std::vector<AuditRecord> queryAuditLog(
        std::optional<std::string> user_id,
        std::optional<std::chrono::system_clock::time_point> start_time,
        std::optional<std::chrono::system_clock::time_point> end_time,
        size_t limit = 1000
    );
    
    // Export audit log
    Status exportToKafka(const std::string& topic);
    Status exportToS3(const std::string& bucket, const std::string& prefix);
};

// Example: Enable auditing and query
txn_auditor.enableAuditing(true);

// ... transactions execute ...

// Query recent failed transactions
auto failed_txns = txn_auditor.queryAuditLog(
    std::nullopt,  // all users
    std::chrono::system_clock::now() - std::chrono::hours(1),
    std::chrono::system_clock::now()
);

for (const auto& record : failed_txns) {
    if (record.result == TransactionAuditor::AuditRecord::ABORTED) {
        std::cout << "Transaction " << record.txn_id 
                  << " aborted by " << record.user_id << std::endl;
    }
}
```

**Storage:**
- Separate column family in RocksDB
- Automatic archival to cold storage
- Configurable retention (7-365 days)
- Index on user_id, timestamp, transaction_id

---

### Cross-Branch Transactions
**Priority:** Low  
**Target Version:** v1.9.0

Atomic operations spanning multiple branches for advanced workflows.

**Features:**
- Read from multiple branches in one transaction
- Atomic merge operations
- Cross-branch constraints
- Branch synchronization primitives

**Architecture:**
```cpp
class CrossBranchTransaction {
public:
    // Read from specific branch
    std::optional<BaseEntity> getFromBranch(
        const std::string& branch_name,
        std::string_view table,
        std::string_view pk
    );
    
    // Write to specific branch
    Status putToBranch(
        const std::string& branch_name,
        std::string_view table,
        const BaseEntity& entity
    );
    
    // Atomic cross-branch operation
    Status atomicMerge(
        const std::string& source_branch,
        const std::string& target_branch,
        const std::vector<std::string>& keys
    );
};

// Example: Sync branches
auto cross_txn = txn_mgr.beginCrossBranch();

// Read from feature branch
auto feature_user = cross_txn.getFromBranch(
    "feature-branch", "users", "user123"
);

// Write to main branch
if (feature_user) {
    cross_txn.putToBranch("main", "users", *feature_user);
}

cross_txn.commit();  // Atomic across branches
```

---

## Research & Experimental

### Hardware Transactional Memory (HTM)
**Priority:** Low  
**Target Version:** Research

Explore Intel TSX / ARM TME for lock-free transactions.

**Potential Benefits:**
- 10-100x faster than software locks
- Zero-contention for non-conflicting transactions
- Automatic rollback on conflicts

**Challenges:**
- Hardware support limited
- Capacity constraints (L1 cache size)
- Fallback to software locks required
- Non-deterministic behavior

---

### Blockchain-Inspired Immutable Transaction Log
**Priority:** Low  
**Target Version:** Research

Cryptographically verifiable transaction history.

**Features:**
- Merkle tree of transaction batches
- Tamper-evident audit trail
- Zero-knowledge proofs for privacy
- Smart contract integration

**Use Cases:**
- Regulatory compliance (financial services)
- Supply chain traceability
- Multi-party computation
- Data provenance

---

## Performance Targets (v1.8.0)

| Metric | Current | Target |
|--------|---------|--------|
| Throughput (simple txns) | 50K/sec | 200K/sec |
| Latency p50 (commit) | 1ms | 100μs |
| Latency p99 (commit) | 10ms | 5ms |
| Deadlock detection overhead | 5% | 1% |
| SAGA compensation time | 100ms | 20ms |
| Distributed 2PC latency | 10ms | 5ms |
| Max concurrent transactions | 10K | 100K |

---

## Breaking Changes (v2.0.0 Consideration)

### Transaction API Redesign
- Move to builder pattern for transaction configuration
- Separate read and write transaction types
- Fluent API for operations
- RAII-style automatic rollback

```cpp
// Current API
auto txn = txn_mgr.begin(IsolationLevel::Snapshot);
txn.putEntity("users", user);
txn.commit();

// Proposed v2.0 API
auto result = txn_mgr.transaction()
    .isolation(IsolationLevel::Snapshot)
    .execute([&](auto& txn) {
        txn.put("users", user);
    });  // Automatic commit/rollback
```

---

## Community Requests

Track requests from GitHub issues:

- **#123**: Add transaction size limits and warnings
- **#456**: Support for nested transactions beyond savepoints
- **#789**: Transaction replay for debugging
- **#234**: Prometheus metrics for transaction stats
- **#567**: Grafana dashboard for deadlock visualization

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | ≥ 90% line coverage in `transaction_manager.cpp` | Cover all isolation levels, OCC version conflict, savepoint LIFO ordering, and transaction timeout detection |
| OCC | 13 existing tests in `tests/test_transaction_occ.cpp` must pass; add concurrent-contention tests | Verify ≥ 90% commit success rate at 10% contention ratio using a 10-thread stress driver |
| 2PC | Simulate coordinator crash after PREPARE; verify recovery commits or aborts correctly without duplicates | Use in-memory RocksDB for coordinator log in unit tests; inject crash via a test-hook API |
| SAGA | Sequential and parallel SAGA with injected step failures; verify compensation executes in reverse dependency order | At least one parallel SAGA test with 4+ steps and 2 injected failures at non-leaf nodes |
| SSI | Write-skew anomaly must be prevented under `SerializableSnapshot`; confirm retry loop resolves | Phantom-read test using concurrent range-scan + insert pattern; assert zero anomalies across 1000 iterations |
| Performance | Commit throughput ≥ 200K simple txns/sec on an 8-core host (from `## Performance Targets (v1.8.0)`) | `benchmarks/bench_transaction_throughput.cpp`; runs in CI as a non-blocking advisory check |

---

## Security / Reliability

- `SerializableSnapshot` isolation is the only mode that prevents write-skew; documentation and default admin configurations must steer security-sensitive workloads toward `IsolationLevel::SerializableSnapshot`.
- Transaction audit records must be written to a dedicated RocksDB column family with restricted write access; only the internal `TransactionAuditor` code path may insert or delete audit rows.
- Audit log entries must never contain decrypted field values; column-level encryption is applied before the operation is recorded.
- The 2PC coordinator log is append-only; replaying the log after a coordinator crash must be idempotent and must not duplicate already-committed writes.
- SAGA compensation endpoints exposed over HTTP must be protected by the same authentication middleware as the forward operations; unauthenticated compensation calls are rejected with `401 Unauthorized`.
- `TransactionId` values are generated as random 128-bit UUIDs; sequential or guessable IDs that could enable transaction-injection or replay attacks are prohibited.
- The deadlock-detection watchdog runs in a dedicated thread; if the watchdog fails to respond within 500 ms, a fallback timer-based abort activates to prevent transactions from holding locks indefinitely.

---

## See Also

- [Main README](README.md) - Current transaction features
- [RocksDB Optimistic Transactions](https://github.com/facebook/rocksdb/wiki/OptimisticTransaction)
- [PostgreSQL Serializable Snapshot Isolation](https://drkp.net/papers/ssi-vldb12.pdf)
- [SAGA Pattern](https://microservices.io/patterns/data/saga.html)
- [Two-Phase Commit](https://en.wikipedia.org/wiki/Two-phase_commit_protocol)

---

## Scientific References (IEEE)

The following references support the research basis for planned and implemented features in this module.

**Serializable Snapshot Isolation (SSI)**

[1] M. J. Cahill, U. Röhm, and A. D. Fekete, "Serializable isolation for snapshot databases," *ACM Trans. Database Syst.*, vol. 34, no. 4, pp. 1–42, Dec. 2009, doi: 10.1145/1620585.1620587.

[2] D. R. K. Ports and K. Grittner, "Serializable snapshot isolation in PostgreSQL," *Proc. VLDB Endow.*, vol. 5, no. 12, pp. 1850–1861, Aug. 2012, doi: 10.14778/2367502.2367523.

**Two-Phase Commit and Distributed Transactions**

[3] J. Gray, "Notes on data base operating systems," in *Operating Systems: An Advanced Course*, R. Bayer, R. M. Graham, and G. Seegmüller, Eds. Berlin, Germany: Springer, 1978, pp. 393–481.

[4] P. A. Bernstein, V. Hadzilacos, and N. Goodman, *Concurrency Control and Recovery in Database Systems*. Reading, MA, USA: Addison-Wesley, 1987. Available: https://www.microsoft.com/en-us/research/people/philbe/book/

[5] D. Spanner: Google's Globally-Distributed Database, J. C. Corbett *et al.*, "Spanner: Google's globally distributed database," *ACM Trans. Comput. Syst.*, vol. 31, no. 3, pp. 1–22, Aug. 2013, doi: 10.1145/2491245.

**SAGA Pattern and Compensation**

[6] H. Garcia-Molina and K. Salem, "Sagas," *ACM SIGMOD Rec.*, vol. 16, no. 3, pp. 249–259, Dec. 1987, doi: 10.1145/38714.38742.

[7] C. Richardson, *Microservices Patterns: With Examples in Java*. Shelter Island, NY, USA: Manning Publications, 2018, ch. 4 (Saga Pattern). ISBN: 978-1617294549.

**Optimistic Concurrency Control**

[8] H. T. Kung and J. T. Robinson, "On optimistic methods for concurrency control," *ACM Trans. Database Syst.*, vol. 6, no. 2, pp. 213–226, Jun. 1981, doi: 10.1145/319566.319567.

**Deadlock Detection and Prevention**

[9] A. Silberschatz, P. B. Galvin, and G. Gagne, *Operating System Concepts*, 10th ed. Hoboken, NJ, USA: Wiley, 2018, ch. 8 (Deadlocks). ISBN: 978-1119320913.

[10] E. Knapp, "Deadlock detection in distributed databases," *ACM Comput. Surv.*, vol. 19, no. 4, pp. 303–328, Dec. 1987, doi: 10.1145/46157.46158.

**MVCC and Snapshot Isolation**

[11] A. Fekete, D. Liarokapis, E. O'Neil, P. O'Neil, and D. Shasha, "Making snapshot isolation serializable," *ACM Trans. Database Syst.*, vol. 30, no. 2, pp. 492–528, Jun. 2005, doi: 10.1145/1071610.1071615.

[12] T. Neumann, T. Mühlbauer, and A. Kemper, "Fast serializable multi-version concurrency control for main-memory database systems," in *Proc. ACM SIGMOD Int. Conf. Manag. Data*, Melbourne, VIC, Australia, 2015, pp. 743–755, doi: 10.1145/2723372.2749436.

**Write Batching and Group Commit**

[13] T. Helland, "Life beyond distributed transactions: An apostate's opinion," in *Proc. 3rd Biennial Conf. Innovative Data Syst. Res. (CIDR)*, Asilomar, CA, USA, Jan. 2007.

[14] B. Lampson and H. Sturgis, "Crash recovery in a distributed data storage system," unpublished manuscript, Xerox Palo Alto Research Center, 1979. (Foundational group-commit reference.)
