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

Source level: 1; SOT domain: api-contract. Canonical references: `transaction_manager.h`, `isolation_level.h`, `lock_manager.h`, `distributed_transaction_manager.h`, `transaction_coordinator.h`, `saga.h`, `snapshot_manager.h`, `branch_manager.h`, `merge_engine.h`, `tests/transaction/CMakeLists.txt`, `benchmarks/transaction/CMakeLists.txt`, `cmake/ModularBuild.cmake`, and `cmake/CMakeLists.txt`. Related issue: #7390.

# ThemisDB Transaction Module - Header Files

This directory contains transaction-related headers. The local manager and `Saga` are in `themis`; `SnapshotManager`, `BranchManager`, and `MergeEngine` are in `themis::transaction`. The snippets below assume the required objects are initialized and the relevant namespaces are in scope. Header presence alone does not establish a stable/versioned API guarantee.

## Headers Overview

### transaction_manager.h
Local transaction lifecycle and RocksDB-backed operations. The header does not establish atomicity for arbitrary external side effects.

**Key Classes:**
- `TransactionManager` - Local transaction manager
- `Transaction` - Individual transaction handle
- `IsolationLevel` - Enum with `READ_UNCOMMITTED`, `READ_COMMITTED`, `REPEATABLE_READ`, and `SERIALIZABLE`; legacy aliases include `ReadCommitted`, `Snapshot`, and `SerializableSnapshot`.

**Usage:**
```cpp
#include "transaction/transaction_manager.h"

themis::TransactionManager txn_mgr(db, sec_idx, graph_idx, vec_idx);
auto txn_id = txn_mgr.beginTransaction();
auto txn = txn_mgr.getTransaction(txn_id);
txn->putEntity("users", user);
txn_mgr.commitTransaction(txn_id);
```

### saga.h
Local SAGA compensation callbacks; this type does not itself coordinate remote services.

**Key Classes:**
- `Saga` - SAGA coordinator
- `Step` - Single operation with compensation
- `CompensatingAction` - Undo function type

**Usage:**
```cpp
#include "transaction/saga.h"

themis::Saga saga;

saga.compensate();  // invokes registered compensation callbacks
```

### snapshot_manager.h
Named tags associated with changefeed sequence numbers.

**Key Classes:**
- `SnapshotManager` - Snapshot lifecycle management
- `Snapshot` - Snapshot metadata
- `SnapshotStats` - Statistics

**Usage:**
```cpp
#include "transaction/snapshot_manager.h"

themis::transaction::SnapshotManager snapshot_mgr(db, changefeed);
snapshot_mgr.createTag("v1.0.0", "Release tag", "admin");
auto tags = snapshot_mgr.listTags();
```

### branch_manager.h
Persistent branch metadata, sequence references, and active-branch tracking; switching does not route `TransactionManager` operations.

**Key Classes:**
- `BranchManager` - Branch lifecycle management
- `Branch` - Branch metadata
- `CreateBranchOptions` - Branch creation parameters
- `BranchStats` - Statistics

**Usage:**
```cpp
#include "transaction/branch_manager.h"

themis::transaction::BranchManager branch_mgr(db, changefeed, snapshot_mgr);
branch_mgr.createBranch("feature-x", "main", "New feature", "dev");
branch_mgr.switchBranch("feature-x");
// This updates branch metadata; transaction writes are not redirected by this call.
branch_mgr.switchBranch("main");
```

### merge_engine.h
Three-way merge with conflict detection.

**Key Classes:**
- `MergeEngine` - Merge coordinator
- `MergeEngine::Conflict` - Merge conflict details
- `MergeEngine::ConflictType` - Enum for conflict types
- `MergeEngine::MergeStrategy` - Resolution strategies
- `MergeEngine::MergeResult` - Merge outcome

**Usage:**
```cpp
#include "transaction/merge_engine.h"

themis::transaction::MergeEngine merge_engine(diff_engine, snapshot_mgr, changefeed);
themis::transaction::MergeEngine::MergeOptions options;
options.strategy = themis::transaction::MergeEngine::MergeStrategy::THEIRS;
auto result = merge_engine.mergeByTag("base-tag", "feature-tag", "main-tag", options);

if (!result.success) {
    // Handle conflicts
    for (const auto& conflict : result.conflicts) {
        // Resolve manually
    }
}
```

### Additional Headers

| Header | Key Types | Description |
|---|---|---|
| `crash_recovery_manager.h` | `CrashRecoveryManager` | WAL-based crash recovery and redo/undo |
| `deadlock_predictor.h` | `DeadlockPredictor` | Wait-for graph analysis and deadlock detection |
| `distributed_saga.h` | `DistributedSaga`, `DistributedStep` | Multi-node SAGA coordination |
| `distributed_transaction_manager.h` | `DistributedTransactionManager` | Participant-based distributed 2PC coordinator |
| `global_transaction_manager.h` | `GlobalTransactionManager` | Cross-shard global transaction ID management |
| `isolation_level.h` | `IsolationLevel` | Isolation level enum definitions |
| `lock_manager.h` | `LockManager`, `LockType`, `LockStatus` | Lock acquisition, release, waiting, and 2PL status |
| `saga_orchestrator.h` | `SagaOrchestrator` | Choreography-based SAGA orchestration |
| `transaction_auditor.h` | `TransactionAuditor` | Audit trail recording for compliance |
| `transaction_batcher.h` | `TransactionBatcher` | Micro-batching for throughput optimisation |
| `transaction_semantic_advisor.h` | `TransactionSemanticAdvisor` | Semantic batch-affinity (shared-entity grouping) and conflict-probability hints for pending transactions |
| `in_doubt_recovery_coordinator.h` | `IInDoubtRecoveryCoordinator` | Recovery interface for in-doubt distributed transactions |
| `recoverable_two_phase_coordinator.h` | `IRecoverableTwoPhaseCoordinator`, `GlobalTwoPhaseCommitRecoveryManager` | Shared 2PC recovery contract and global recovery reports |

---

## Coordinator and recovery interfaces

`transaction_coordinator.h` declares the protocol-agnostic `ITransactionCoordinator` contract, and `recoverable_two_phase_coordinator.h` declares a recoverable 2PC contract. These declarations do not establish concrete implementations for every listed protocol. `DistributedTransactionManager` declares `DistributedTxnState`, which has no `IN_DOUBT` value.

---

## Thread Safety

| Header | Classes | Thread Safety |
|--------|---------|---------------|
| transaction_manager.h | `TransactionManager` | Manager state is synchronized; consult API comments for call-specific guarantees |
| transaction_manager.h | `Transaction` | No blanket thread-safety guarantee asserted here |
| saga.h | `Saga` | No concurrent-use guarantee documented here |
| other headers | Other types | Consult the individual header; this file does not assert blanket thread safety |

---

## API Stability

This directory does not declare a semver policy or current API version. **TODO:** document stability/version guarantees from the canonical release policy if a transaction-specific promise is intended.

---

## Dependencies

### Internal
- `storage/rocksdb_wrapper.h` - Storage layer
- `storage/base_entity.h` - Entity types
- `index/secondary_index.h` - Secondary indexes
- `index/graph_index.h` - Graph indexes
- `index/vector_index.h` - Vector indexes
- `cdc/changefeed.h` - Change data capture
- `analytics/diff_engine.h` - Diff operations

### External
- `<memory>` - Smart pointers
- `<string>` - String types
- `<string_view>` - String views
- `<chrono>` - Time types
- `<functional>` - Callbacks
- `<optional>` - Optional values
- `<vector>` - Collections
- `<unordered_map>` - Hash maps
- `<nlohmann/json.hpp>` - JSON serialization

---

## Configuration Options

The transaction API exposes runtime tuning via `TransactionManager`:

| API | Default | Purpose |
|-----|---------|---------|
| `setDeadlockDetection(bool)` | `false` | Enable/disable deadlock detection |
| `setDeadlockTimeout(std::chrono::milliseconds)` | `1000ms` | Deadlock timeout setting |
| `setDefaultTransactionTimeout(std::chrono::milliseconds)` | `0ms` | Apply default per-transaction timeout (`0` disables) |
| `setTransactionTimeout(std::chrono::milliseconds)` | `0ms` | Global timeout sweep for active transactions (`0` disables) |
| `setSSIConfig(const SSIConfig&)` | Default-initialized `SSIConfig`: `enable_predicate_locking=true`, `max_predicate_locks=10000`, `conflict_detection_interval=100ms` | Configure SSI behavior |

Per transaction, `Transaction::setTimeout(...)` and `Transaction::setReadOnly(...)` override lifecycle behavior for that transaction handle.

---

## Usage Patterns

### Basic Transaction
```cpp
#include "transaction/transaction_manager.h"

// Initialize
themis::TransactionManager txn_mgr(db, sec_idx, graph_idx, vec_idx);

// Execute transaction
auto txn_id = txn_mgr.beginTransaction(IsolationLevel::ReadCommitted);
auto txn = txn_mgr.getTransaction(txn_id);

auto status = txn->putEntity("users", user);
if (!status.ok) {
    txn_mgr.rollbackTransaction(txn_id);
    return status;
}

status = txn_mgr.commitTransaction(txn_id);
```

### SAGA Pattern
```cpp
#include "transaction/transaction_manager.h"
#include "transaction/saga.h"

auto txn = txn_mgr.begin();
auto& saga = txn.getSaga();

// Forward operations with compensations
reserveInventory();
saga.addStep("inventory", []() { releaseInventory(); });

chargePayment();
saga.addStep("payment", []() { refundPayment(); });

// Auto-compensate on failure
txn.commit();
```

### Snapshot Management
```cpp
#include "transaction/snapshot_manager.h"

themis::transaction::SnapshotManager snapshot_mgr(db, changefeed);

// Create a named tag
auto result = snapshot_mgr.createTag(
    "backup-2024-01-01",
    "Daily backup",
    "backup-service"
);

// List tags
auto tags = snapshot_mgr.listTags();

// Delete an old tag
snapshot_mgr.deleteTag("backup-2023-12-01");
```

### Branch Workflow
```cpp
#include "transaction/branch_manager.h"

themis::transaction::BranchManager branch_mgr(db, changefeed, snapshot_mgr);

// Create feature branch
branch_mgr.createBranch("feature-123", "main", "Feature", "dev@co.com");

// Work on feature
branch_mgr.switchBranch("feature-123");
// ... make changes ...

// The header also exposes mergeBranches(source, target); merge requires
// a MergeEngine supplied to BranchManager when it is constructed.
branch_mgr.switchBranch("main");
branch_mgr.deleteBranch("feature-123");
```

### Merge with Conflict Resolution
```cpp
#include "transaction/merge_engine.h"

// diff_engine, snapshot_mgr, and changefeed are initialized dependencies.
themis::transaction::MergeEngine merge_engine(diff_engine, snapshot_mgr, changefeed);

themis::transaction::MergeEngine::MergeOptions options;
options.strategy = themis::transaction::MergeEngine::MergeStrategy::THEIRS;
auto result = merge_engine.mergeByTag("base-tag", "source-tag", "target-tag", options);

if (!result.success && !result.conflicts.empty()) {
    // Provide key/value choices and retry with manual resolution.
    options.strategy = themis::transaction::MergeEngine::MergeStrategy::MANUAL;
    for (const auto& conflict : result.conflicts) {
        options.manual_resolutions.push_back({conflict.key, conflict.source_value});
    }
    auto resolved = merge_engine.mergeByTag("base-tag", "source-tag", "target-tag", options);
}
```

---

## Error Handling

Return types vary by operation (`Status`, booleans, optionals, result structs, and `void`); check each declaration's signature:

```cpp
// Check Status
auto status = txn->putEntity("users", user);
if (!status.ok) {
    std::cerr << "Error: " << status.message << std::endl;
    return;
}

// Check optional
auto snapshot = snapshot_mgr.getTag("tag-name");
if (!snapshot) {
    std::cerr << "Snapshot not found" << std::endl;
    return;
}
```

---

## Performance Considerations

### Transaction Size
Bound transaction size according to workload and measured behavior; no source-backed numeric maximum is established here.

```cpp
// ❌ Bad: Large transaction
auto txn = txn_mgr.begin();
for (int i = 0; i < 1000000; i++) {
    txn.putEntity("users", users[i]);
}
txn.commit();  // Holds locks too long

// ✅ Good: Batched transactions
for (int batch = 0; batch < 1000; batch++) {
    auto txn = txn_mgr.begin();
    for (int i = 0; i < 1000; i++) {
        txn.putEntity("users", users[batch * 1000 + i]);
    }
    txn.commit();
}
```

### Isolation Level Selection
- `ReadCommitted` is the declared default.
- `Snapshot` is a legacy alias for `REPEATABLE_READ`; it provides snapshot isolation.
- `SERIALIZABLE` adds SSI conflict checks. Choose an isolation level based on the consistency requirements and verify workload costs with measurements.

```cpp
// OLTP workload
auto txn = txn_mgr.begin(IsolationLevel::ReadCommitted);

// Analytics workload
auto txn = txn_mgr.begin(IsolationLevel::Snapshot);
```

### Deadlock Prevention
Acquire locks in consistent order:

```cpp
// ✅ Good: Always lock in ascending ID order
void transfer(int from, int to, int amount) {
    auto txn = txn_mgr.begin();
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

---

## Runtime Behavior, Errors, and Limits

- **Commit:** the transaction wrapper commits its underlying RocksDB transaction; it invokes saga compensation on timeout or commit failure. Do not generalize this to external side effects.
- **Isolation:** `READ_UNCOMMITTED`, `READ_COMMITTED`, `REPEATABLE_READ` (legacy `Snapshot` alias), and `SERIALIZABLE` are declared in `IsolationLevel`.
- **Error reporting:** write and commit APIs return `TransactionManager::Status` (`ok`, `message`, optional conflict metadata).
- **Recommended numeric limits:** no source-backed transaction-size limit is established here. TODO: link a measured benchmark/release target before stating one.
- **Constraint:** do not mix the anonymous savepoint API and the named savepoint API in one transaction.

---

## Migration from Legacy API

### Old Direct WriteBatch
```cpp
// Old
rocksdb::WriteBatch batch;
batch.Put("users:123", user_data);
batch.Put("index:email:user@example.com", "123");
db->Write(rocksdb::WriteOptions(), &batch);
```

### New TransactionManager
```cpp
// New
auto txn = txn_mgr.begin();
txn.putEntity("users", user);  // Does not assert automatic updates to independent indexes
txn.commit();
```

---

## Build Integration

### CMake
```cmake
# Modular builds declare `themis_transaction` in `cmake/ModularBuild.cmake`.
# No `themis::transaction` alias was found; monolithic source integration is
# defined in `cmake/CMakeLists.txt`.
```

### Include Paths
```cpp
#include "transaction/transaction_manager.h"
#include "transaction/saga.h"
#include "transaction/snapshot_manager.h"
#include "transaction/branch_manager.h"
#include "transaction/merge_engine.h"
```

---

## Testing

The current test executable targets are registered in `tests/transaction/CMakeLists.txt` (conditional on each source being present). Examples include `test_transaction_manager_focused` (`test_transaction_manager.cpp`), `test_transaction_distributed_2pc_focused` (`test_transaction_distributed_2pc.cpp`), `test_adaptive_deadlock_prevention_focused` (`test_adaptive_deadlock_prevention.cpp`), and the phase targets `test_transaction_lifecycle_phase1_focused`, `test_transaction_distributed_phase2_focused`, and `test_transaction_fault_injection_phase3_focused`. The same CMake file also generates `module_transaction_<stem>_autofocused` for eligible `test_transaction_*.cpp` sources. These executable target names are distinct from the CTest registration names. Benchmarks are registered in `benchmarks/transaction/CMakeLists.txt`. Registration does not establish that a target was built or executed.

---

## Troubleshooting

| Symptom | Likely Cause | Action |
|---------|--------------|--------|
| `Deadlock` in commit status | Cyclic lock dependency between active writers | Enable deadlock detection, reduce transaction scope, and acquire entity locks in deterministic order |
| `OCC version conflict` | Concurrent update changed entity version | Re-read current version with `getEntityVersion(...)` and retry transaction |
| `Serialization conflict` | Serializable predicate overlap with concurrent writer | Retry transaction with backoff or narrow predicate range |
| Savepoint rollback behaves unexpectedly | Named and anonymous savepoint APIs were mixed | Use only one savepoint API style per transaction |

---

## Documentation

For detailed implementation documentation, see:
- [Source Implementation README](../../src/transaction/README.md)
- [Transaction Roadmap](../../src/transaction/ROADMAP.md)
- [Future Enhancements](../../src/transaction/FUTURE_ENHANCEMENTS.md)
- [Architecture Details](../../src/transaction/ARCHITECTURE.md)
- [German Module Overview](../../docs/de/transaction/README.md)
- [Primary Sources Index](../../docs/en/transaction/PRIMARY_SOURCES.md)

---

## Support

- Report issues: [GitHub Issues](https://github.com/themisdb/themisdb/issues)
- Discussions: [GitHub Discussions](https://github.com/themisdb/themisdb/discussions)
- Email: support@themisdb.io

---

## License

Copyright © 2024 ThemisDB Contributors. Licensed under Apache 2.0.

## Installation

This module is included as part of ThemisDB. The inspected build definitions do not establish an exported `THEMISDB_INCLUDE_DIR` variable or installed transaction package target. **TODO:** document installed-consumer instructions from the canonical packaging/export configuration.
