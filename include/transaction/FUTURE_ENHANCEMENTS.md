# Transaction Module Headers - Future Enhancements

## Scope

- Public API enhancements for `include/transaction/` headers
- SSI transaction interface (`IsolationLevel::SerializableSnapshot`, `SSIConfig`)
- OCC mode API (`Transaction::optimisticPut`, `getVersion`)
- Savepoint API (`createSavepoint`, `rollbackToSavepoint`, `releaseSavepoint`) — already implemented
- SAGA orchestration interface (`SAGAOrchestrator`, `SAGADefinition` with DAG execution)
- Distributed 2PC coordinator API (`DistributedTransactionManager`)

## Design Constraints

- [ ] SSI and OCC modes are mutually exclusive per session; setting both returns an error
- [ ] Savepoints are hierarchical; releasing a parent savepoint implicitly releases all children
- [ ] SAGA steps MUST declare a `compensate` function; steps without compensation are rejected at registration
- [ ] SAGA step functions are idempotent by contract; the orchestrator MAY retry on transient failure
- [ ] `DistributedTransactionManager` requires all participants to be reachable before `beginDistributed`

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `TransactionManager::setSSIConfig(SSIConfig)` | Query engine | Must be called before `beginTransaction` |
| `Transaction::optimisticPut(table, entity, expected_version)` | OCC clients | Returns `Status::VERSION_CONFLICT` on mismatch |
| `SAGAOrchestrator::execute(SAGADefinition)` | Business logic layer | Async-safe; supports parallel steps |
| `DistributedTransactionManager::beginDistributed(participants)` | Cluster coordinator | Requires mTLS per participant |
| `TransactionAuditor::queryAuditLog(user, start, end, limit)` | Compliance API | Append-only read interface |

## Planned API Additions

### Serializable Snapshot Isolation (SSI)
**Target Version:** v1.8.0

Add new isolation level to enum:

```cpp
// transaction_manager.h
enum class IsolationLevel {
    ReadCommitted,
    Snapshot,
    SerializableSnapshot  // NEW
};

class TransactionManager {
public:
    struct SSIConfig {
        bool enable_predicate_locking = true;
        size_t max_predicate_locks = 10000;
    };

    void setSSIConfig(const SSIConfig& config);  // NEW
};
```

---

### Optimistic Concurrency Control
**Target Version:** v1.8.0

Add version-based operations to Transaction:

```cpp
// transaction_manager.h
class Transaction {
public:
    // NEW: Optimistic operations
    Status optimisticPut(std::string_view table,
                        const BaseEntity& entity,
                        uint64_t expected_version);

    std::optional<uint64_t> getVersion(std::string_view table,
                                      std::string_view pk);
};
```

---

### Transaction Savepoints
**Status: ✅ Implemented** (v1.x)
**Target Version:** v1.8.0

Named savepoint support is fully implemented in `TransactionManager::Transaction`.  See `createSavepoint`, `rollbackToSavepoint`, `releaseSavepoint`, `getSavepoints`, and `hasSavepoint` in `transaction_manager.h`.

```cpp
// transaction_manager.h — already implemented
class Transaction {
public:
    Status createSavepoint(std::string_view name);
    Status rollbackToSavepoint(std::string_view name);
    Status releaseSavepoint(std::string_view name);

    std::vector<std::string> getSavepoints() const;
    bool hasSavepoint(std::string_view name) const;
};
```

---

### Distributed Transaction Coordinator
**Target Version:** v1.9.0

New header: `distributed_transaction_manager.h`

```cpp
// distributed_transaction_manager.h (NEW FILE)
#pragma once

#include "transaction/transaction_manager.h"
#include <vector>
#include <string>

namespace themis {

class DistributedTransactionManager {
public:
    struct Participant {
        std::string node_id;
        std::string endpoint;
        std::set<std::string> affected_keys;
    };

    struct DistributedTransaction {
        TransactionManager::TransactionId txn_id;
        std::vector<Participant> participants;
        enum State { INIT, PREPARED, COMMITTED, ABORTED };
        State state;
        std::chrono::system_clock::time_point timeout;
    };

    // Coordinator API
    TransactionManager::TransactionId beginDistributed(
        const std::vector<Participant>& participants
    );

    Status prepareDistributed(TransactionManager::TransactionId txn_id);
    Status commitDistributed(TransactionManager::TransactionId txn_id);
    void abortDistributed(TransactionManager::TransactionId txn_id);

    // Participant API
    Status voteOnPrepare(TransactionManager::TransactionId txn_id,
                        bool can_commit);
    Status applyCommit(TransactionManager::TransactionId txn_id);
    Status applyAbort(TransactionManager::TransactionId txn_id);
};

}  // namespace themis
```

---

### SAGA Orchestration
**Status: ✅ Implemented** (v1.8.0)
**Target Version:** v1.8.0

Implemented in `include/transaction/saga_orchestrator.h` and
`src/transaction/saga_orchestrator.cpp`.  Tests in
`tests/test_saga_orchestrator.cpp`.

Extend saga.h with orchestration:

```cpp
// saga_orchestrator.h (NEW FILE)
#pragma once

#include "transaction/saga.h"
#include <functional>
#include <set>
#include <map>

namespace themis {

class SAGAOrchestrator {
public:
    struct Step {
        std::string name;
        std::function<void()> forward;
        std::function<void()> compensate;
        std::set<std::string> depends_on;
        std::chrono::milliseconds timeout{5000};
        size_t max_retries = 3;
    };

    struct SAGADefinition {
        std::string name;
        std::vector<Step> steps;
        bool enable_parallel = true;
    };

    Status execute(const SAGADefinition& saga);

    struct ExecutionStatus {
        std::string saga_name;
        std::map<std::string, StepState> step_states;
        size_t completed_steps;
        size_t failed_steps;
        size_t pending_steps;
    };

    ExecutionStatus getStatus(const std::string& saga_id);
};

}  // namespace themis
```

---

### Transaction Batching
**Target Version:** v1.8.0

New header: `transaction_batcher.h`

```cpp
// transaction_batcher.h (NEW FILE)
#pragma once

#include "transaction/transaction_manager.h"
#include <future>

namespace themis {

class TransactionBatcher {
public:
    struct BatchConfig {
        std::chrono::microseconds window{5000};
        size_t max_batch_size = 1000;
        size_t min_batch_size = 10;
        bool enable_adaptive = true;
    };

    explicit TransactionBatcher(TransactionManager& txn_mgr);

    void setBatchConfig(const BatchConfig& config);

    std::future<TransactionManager::Status> submitAsync(
        TransactionManager::Transaction&& txn
    );

    void flush();
};

}  // namespace themis
```

---

### Transaction Auditing
**Status: ✅ Implemented** (v1.8.0)
**Target Version:** v1.8.0

New header: `transaction_auditor.h`

```cpp
// transaction_auditor.h (NEW FILE)
#pragma once

#include "transaction/transaction_manager.h"
#include <vector>
#include <optional>

namespace themis {

class TransactionAuditor {
public:
    struct AuditRecord {
        TransactionManager::TransactionId txn_id;
        std::string user_id;
        std::string session_id;
        std::chrono::system_clock::time_point timestamp;
        IsolationLevel isolation;

        struct Operation {
            enum Type { PUT, DELETE, ADD_EDGE, DELETE_EDGE, ADD_VECTOR };
            Type type;
            std::string table;
            std::string key;
            std::optional<std::string> old_value;
            std::optional<std::string> new_value;
        };

        std::vector<Operation> operations;

        enum Result { COMMITTED, ABORTED, DEADLOCK };
        Result result;
        uint64_t duration_us;
    };

    void enableAuditing(bool enabled);

    std::vector<AuditRecord> queryAuditLog(
        std::optional<std::string> user_id,
        std::optional<std::chrono::system_clock::time_point> start_time,
        std::optional<std::chrono::system_clock::time_point> end_time,
        size_t limit = 1000
    );

    Status exportToKafka(const std::string& topic);
    Status exportToS3(const std::string& bucket, const std::string& prefix);
};

}  // namespace themis
```

---

### Deadlock Predictor
**Target Version:** v1.9.0

New header: `deadlock_predictor.h`

```cpp
// deadlock_predictor.h (NEW FILE)
#pragma once

#include "transaction/transaction_manager.h"
#include <vector>
#include <set>

namespace themis {

class DeadlockPredictor {
public:
    struct LockPattern {
        std::vector<std::string> keys;
        std::chrono::microseconds hold_time;
        size_t frequency;
    };

    void recordTransaction(
        TransactionManager::TransactionId txn_id,
        const std::vector<std::string>& locks_acquired,
        std::chrono::microseconds duration
    );

    double predictDeadlockProbability(
        const std::vector<std::string>& proposed_locks,
        const std::set<TransactionManager::TransactionId>& active_transactions
    );

    std::vector<std::string> recommendLockOrder(
        const std::vector<std::string>& keys
    );

    std::chrono::milliseconds recommendTimeout(
        const std::vector<std::string>& keys
    );
};

}  // namespace themis
```

---

### Read-Only Optimization
**Target Version:** v1.8.0

Add to Transaction class:

```cpp
// transaction_manager.h
class Transaction {
public:
    // NEW: Read-only optimization
    void setReadOnly(bool read_only = true);
    bool isReadOnly() const;
    bool hasWrites() const;
};
```

---

### Cross-Branch Operations
**Target Version:** v1.9.0

New header: `cross_branch_transaction.h`

```cpp
// cross_branch_transaction.h (NEW FILE)
#pragma once

#include "transaction/transaction_manager.h"
#include "transaction/branch_manager.h"

namespace themis {

class CrossBranchTransaction {
public:
    explicit CrossBranchTransaction(TransactionManager& txn_mgr,
                                   BranchManager& branch_mgr);

    std::optional<BaseEntity> getFromBranch(
        const std::string& branch_name,
        std::string_view table,
        std::string_view pk
    );

    TransactionManager::Status putToBranch(
        const std::string& branch_name,
        std::string_view table,
        const BaseEntity& entity
    );

    TransactionManager::Status atomicMerge(
        const std::string& source_branch,
        const std::string& target_branch,
        const std::vector<std::string>& keys
    );

    TransactionManager::Status commit();
    void rollback();
};

}  // namespace themis
```

---

## Breaking Changes (v2.0.0)

### Transaction Builder Pattern

Replace current API with builder:

```cpp
// transaction_manager.h (v2.0)
class TransactionManager {
public:
    // NEW builder-style API
    class TransactionBuilder {
    public:
        TransactionBuilder& isolation(IsolationLevel level);
        TransactionBuilder& readOnly(bool is_read_only = true);
        TransactionBuilder& timeout(std::chrono::milliseconds timeout);
        TransactionBuilder& withSavepoints(bool enable = true);

        // Execute with automatic commit/rollback
        template<typename Fn>
        Status execute(Fn&& fn);

        // Traditional begin-commit style
        Transaction begin();
    };

    TransactionBuilder transaction();

    // Deprecated in v2.0, removed in v3.0
    [[deprecated("Use transaction().begin() instead")]]
    Transaction begin(IsolationLevel isolation = IsolationLevel::ReadCommitted);
};

// Example usage
auto result = txn_mgr.transaction()
    .isolation(IsolationLevel::Snapshot)
    .timeout(std::chrono::seconds(30))
    .execute([&](auto& txn) {
        txn.putEntity("users", user);
    });
```

---

### Separate Read/Write Transaction Types

```cpp
// transaction_manager.h (v2.0)
class ReadTransaction {
public:
    // Only read operations
    std::optional<BaseEntity> getEntity(std::string_view table,
                                       std::string_view pk);
    // No commit needed - RAII cleanup
};

class WriteTransaction {
public:
    // Both read and write operations
    std::optional<BaseEntity> getEntity(std::string_view table,
                                       std::string_view pk);
    Status putEntity(std::string_view table, const BaseEntity& entity);
    Status commit();
    void rollback();
};

class TransactionManager {
public:
    ReadTransaction beginRead();
    WriteTransaction beginWrite();
};
```

---

## API Stability Guarantees

### Stable (No Breaking Changes)
- `TransactionManager::beginTransaction()`
- `TransactionManager::commitTransaction()`
- `TransactionManager::rollbackTransaction()`
- `Transaction::putEntity()`
- `Transaction::eraseEntity()`
- `Transaction::addEdge()`
- `Transaction::deleteEdge()`
- `Transaction::commit()`
- `Transaction::rollback()`
- `Saga::addStep()`
- `Saga::compensate()`
- `SnapshotManager::createSnapshot()`
- `SnapshotManager::deleteSnapshot()`
- `BranchManager::createBranch()`
- `BranchManager::switchBranch()`
- `MergeEngine::merge()`

### Experimental (May Change)
- `TransactionManager::getStatsLockFree()` - Implementation may optimize
- `Transaction::addVector()` - May add more parameters
- `MergeEngine::mergeWithResolutions()` - Resolution format may evolve

### Deprecated (Will Be Removed)
- `TransactionManager::begin()` - Use `beginTransaction()` instead (v2.0)

---

## Header Dependencies (Future)

New dependency graph:

```
transaction_manager.h (core)
    ↓
    ├─→ saga.h (SAGA pattern)
    ├─→ transaction_batcher.h (batching)
    ├─→ transaction_auditor.h (auditing)
    └─→ distributed_transaction_manager.h (2PC)

snapshot_manager.h (versioning)
    ↓
    ├─→ branch_manager.h (branches)
    └─→ merge_engine.h (merging)

saga.h (compensation)
    ↓
    └─→ saga_orchestrator.h (DAG execution)

(NEW) deadlock_predictor.h
(NEW) cross_branch_transaction.h
```

---

## Forward Compatibility

To prepare for v2.0 changes:

```cpp
// Use long-form API (forward compatible)
auto txn_id = txn_mgr.beginTransaction();
auto txn = txn_mgr.getTransaction(txn_id);
// ... operations ...
txn_mgr.commitTransaction(txn_id);

// Avoid short-form API (will be deprecated)
// auto txn = txn_mgr.begin();  // Avoid this
```

---

## Testing Infrastructure

New test utilities:

```cpp
// transaction_test_utils.h (NEW FILE)
#pragma once

#include "transaction/transaction_manager.h"
#include <gtest/gtest.h>

namespace themis::test {

class TransactionTestFixture : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<TransactionManager> txn_mgr;
    std::unique_ptr<RocksDBWrapper> db;
};

// Helpers
void assertTransactionCommitted(TransactionManager::TransactionId txn_id);
void assertTransactionAborted(TransactionManager::TransactionId txn_id);
void simulateDeadlock(/* ... */);
void injectFailure(/* ... */);

}  // namespace themis::test
```

---

## Documentation Requirements

All new headers must include:

1. **File-level documentation**
   - Purpose and use cases
   - Thread safety guarantees
   - Performance characteristics

2. **Class documentation**
   - Brief description
   - Usage example
   - Related classes

3. **Method documentation**
   - Parameters with constraints
   - Return value meaning
   - Exception behavior
   - Thread safety

4. **Example code**
   - Basic usage
   - Error handling
   - Integration with other components

---

## Test Strategy

- Unit tests: SSI conflict detection with concurrent read-write overlaps
- Unit tests: OCC `optimisticPut` returns `VERSION_CONFLICT` when version mismatches
- Unit tests: savepoint hierarchy — rollback to grandparent releases intermediate savepoints
- Integration tests: SAGA with compensation — inject failure mid-saga and verify all completed steps are compensated
- Integration tests: distributed 2PC with one participant voting NO — verify global abort propagates
- Stress tests: concurrent SSI transactions to detect false-positive serialization failures

## Performance Targets

- `Transaction::createSavepoint` call latency: ≤ 1 ms
- `SAGAOrchestrator` step registration (`SAGADefinition` construction): ≤ 500 µs per step
- OCC `Transaction::getVersion` call latency: ≤ 100 µs
- `TransactionBatcher` flush latency (batch of 100 transactions): ≤ 5 ms
- `DistributedTransactionManager::prepareDistributed` (LAN, 3 nodes): ≤ 50 ms

## Security / Reliability

- `TransactionAuditor` log is append-only; no delete or update API is exposed
- `DistributedTransactionManager` requires mTLS for all participant connections; plaintext is rejected
- No cross-tenant transaction visibility: `beginDistributed` validates all participant `node_id`s are within the same tenant scope
- `SAGAOrchestrator` compensation is always attempted in reverse order, even if the forward step timed out
- `Transaction::optimisticPut` does not expose old value on conflict to prevent information leakage

## See Also

- [Implementation README](../../src/transaction/README.md) - Implementation details
- [Implementation Future Enhancements](../../src/transaction/FUTURE_ENHANCEMENTS.md) - Implementation plans
- [API Versioning Policy](../../docs/api_versioning.md)
- [Migration Guide](../../docs/migration/v1_to_v2.md)

---

## License

Copyright © 2024 ThemisDB Contributors. Licensed under Apache 2.0.

---

## Paper 2 — Layer 5: TransactionSemanticAdvisor (IMPL-B5)

> Research paper: `docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md` §Layer 5
> Issue: `docs/issues/optimization_layers/IMPL-B5-transaction-semantics.md`

### Scope
- `TransactionSemanticAdvisor` complements the existing `DeadlockPredictor` with semantic-level batch ordering hints
- Inputs: `TransactionBatch` (write-set key ranges + operation types), `DeadlockPredictor::predict()` score
- Outputs: `BatchAffinityHint { hint_type, affected_keys, confidence, retry_reduction_estimate }`

### Design Constraints
- Hint computation must complete ≤ 2 ms p99 at 10 k transactions/s
- Must not access GDPR-tagged field values — key-range analysis only
- Non-blocking: advisor runs on a separate thread pool; results are advisory, never blocking

### Integration Notes
- `TransactionBatcher::setBatchConfig()` consumers may optionally query the advisor before submitting a batch
- Advisor writes `DecisionRecord` to `AIDecisionAuditor` for every non-trivial hint (confidence ≥ 0.75)
- Future: cross-shard hint sharing via `distributed_knowledge` Layer 11 workload fingerprint

### Performance Targets
- Hint computation: ≤ 2 ms p99 for 1 000-transaction batch
- Retry reduction: ≥ 15 % fewer retries in simulation scenarios with high write-set overlap
