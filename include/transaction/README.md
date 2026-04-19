> **Build:** `cmake --preset release && cmake --build build/release`

# ThemisDB Transaction Module - Header Files

This directory contains the public header files for the ThemisDB Transaction module.

## Headers Overview

### transaction_manager.h
Core transaction management with ACID guarantees.

**Key Classes:**
- `TransactionManager` - Main transaction coordinator
- `Transaction` - Individual transaction handle
- `IsolationLevel` - Enum for ReadCommitted/Snapshot

**Usage:**
```cpp
#include "transaction/transaction_manager.h"

TransactionManager txn_mgr(db, sec_idx, graph_idx, vec_idx);
auto txn_id = txn_mgr.beginTransaction();
auto txn = txn_mgr.getTransaction(txn_id);
txn->putEntity("users", user);
txn_mgr.commitTransaction(txn_id);
```

### saga.h
SAGA pattern for distributed transactions with compensating actions.

**Key Classes:**
- `Saga` - SAGA coordinator
- `Step` - Single operation with compensation
- `CompensatingAction` - Undo function type

**Usage:**
```cpp
#include "transaction/saga.h"

auto txn = txn_mgr.begin();
auto& saga = txn.getSaga();

performOperation();
saga.addStep("operation", []() { compensate(); });

txn.commit();  // Auto-compensates on failure
```

### snapshot_manager.h
Git-like named snapshots for MVCC.

**Key Classes:**
- `SnapshotManager` - Snapshot lifecycle management
- `Snapshot` - Snapshot metadata
- `SnapshotStats` - Statistics

**Usage:**
```cpp
#include "transaction/snapshot_manager.h"

SnapshotManager snapshot_mgr(db, changefeed);
snapshot_mgr.createSnapshot("v1.0.0", "Release snapshot", "admin");
auto snapshots = snapshot_mgr.listSnapshots();
```

### branch_manager.h
Git-like persistent branches for parallel workflows.

**Key Classes:**
- `BranchManager` - Branch lifecycle management
- `Branch` - Branch metadata
- `CreateBranchOptions` - Branch creation parameters
- `BranchStats` - Statistics

**Usage:**
```cpp
#include "transaction/branch_manager.h"

BranchManager branch_mgr(db, changefeed, snapshot_mgr);
branch_mgr.createBranch("feature-x", "main", {}, "New feature", "dev");
branch_mgr.switchBranch("feature-x");
// ... work on feature branch ...
branch_mgr.switchBranch("main");
```

### merge_engine.h
Three-way merge with conflict detection.

**Key Classes:**
- `MergeEngine` - Merge coordinator
- `Conflict` - Merge conflict details
- `ConflictType` - Enum for conflict types
- `MergeStrategy` - Resolution strategies
- `MergeResult` - Merge outcome

**Usage:**
```cpp
#include "transaction/merge_engine.h"

MergeEngine merge_engine(db, snapshot_mgr, changefeed, diff_engine);
auto result = merge_engine.merge(
    "feature-branch",
    "main",
    MergeEngine::MergeStrategy::THEIRS,
    false  // not dry-run
);

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
| `distributed_transaction_manager.h` | `DistributedTransactionManager` | 2PC/3PC distributed transaction coordinator |
| `global_transaction_manager.h` | `GlobalTransactionManager` | Cross-shard global transaction ID management |
| `isolation_level.h` | `IsolationLevel` | Isolation level enum definitions |
| `lock_manager.h` | `LockManager`, `LockMode` | Row/range lock acquisition and release |
| `saga_orchestrator.h` | `SagaOrchestrator` | Choreography-based SAGA orchestration |
| `transaction_auditor.h` | `TransactionAuditor` | Audit trail recording for compliance |
| `transaction_batcher.h` | `TransactionBatcher` | Micro-batching for throughput optimisation |
| `transaction_semantic_advisor.h` | `TransactionSemanticAdvisor` | <!-- TODO: verify --> Semantic hints for query planning |

---

## Thread Safety

| Header | Classes | Thread Safety |
|--------|---------|---------------|
| transaction_manager.h | TransactionManager | ✅ Thread-safe |
| transaction_manager.h | Transaction | ❌ Single-threaded |
| saga.h | Saga | ❌ Single-threaded |
| snapshot_manager.h | SnapshotManager | ✅ Thread-safe |
| branch_manager.h | BranchManager | ✅ Thread-safe |
| merge_engine.h | MergeEngine | ✅ Thread-safe |

---

## API Stability

All headers in this directory are part of the **stable public API** and follow semantic versioning:

- **Major version changes**: Breaking API changes
- **Minor version changes**: Backward-compatible additions
- **Patch version changes**: Bug fixes only

Current version: **1.7.0**

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

## Usage Patterns

### Basic Transaction
```cpp
#include "transaction/transaction_manager.h"

// Initialize
TransactionManager txn_mgr(db, sec_idx, graph_idx, vec_idx);

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

SnapshotManager snapshot_mgr(db, changefeed);

// Create snapshot
auto result = snapshot_mgr.createSnapshot(
    "backup-2024-01-01",
    "Daily backup",
    "backup-service"
);

// List snapshots
auto snapshots = snapshot_mgr.listSnapshots();

// Delete old snapshot
snapshot_mgr.deleteSnapshot("backup-2023-12-01");
```

### Branch Workflow
```cpp
#include "transaction/branch_manager.h"

BranchManager branch_mgr(db, changefeed, snapshot_mgr);

// Create feature branch
branch_mgr.createBranch("feature-123", "main", {}, "Feature", "dev@co.com");

// Work on feature
branch_mgr.switchBranch("feature-123");
// ... make changes ...

// Merge back to main
branch_mgr.switchBranch("main");
merge_engine.merge("feature-123", "main", MergeStrategy::MANUAL);

// Cleanup
branch_mgr.deleteBranch("feature-123");
```

### Merge with Conflict Resolution
```cpp
#include "transaction/merge_engine.h"

MergeEngine merge_engine(db, snapshot_mgr, changefeed, diff_engine);

// Try automatic merge
auto result = merge_engine.merge(
    "source-branch",
    "target-branch",
    MergeEngine::MergeStrategy::FAST_FORWARD,
    false
);

if (!result.success && !result.conflicts.empty()) {
    // Manual resolution required
    std::vector<MergeEngine::ConflictResolution> resolutions;

    for (const auto& conflict : result.conflicts) {
        // Decide resolution strategy per conflict
        resolutions.push_back({
            conflict.key,
            preferred_value,
            "Resolution reason"
        });
    }

    // Apply with resolutions
    auto resolved = merge_engine.mergeWithResolutions(
        "source-branch",
        "target-branch",
        resolutions
    );
}
```

---

## Error Handling

All operations return `Status` or `std::optional`:

```cpp
// Check Status
auto status = txn->putEntity("users", user);
if (!status.ok) {
    std::cerr << "Error: " << status.message << std::endl;
    return;
}

// Check optional
auto snapshot = snapshot_mgr.getSnapshot("tag-name");
if (!snapshot) {
    std::cerr << "Snapshot not found" << std::endl;
    return;
}
```

---

## Performance Considerations

### Transaction Size
Keep transactions small (< 1000 operations) to minimize lock hold time:

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
- Use `ReadCommitted` for OLTP (default, fastest)
- Use `Snapshot` for analytics (consistent reads, slower)

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
txn.putEntity("users", user);  // Indexes updated automatically
txn.commit();
```

---

## Build Integration

### CMake
```cmake
target_link_libraries(your_target
    PRIVATE
        themis::transaction
        themis::storage
        themis::index
)
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

Headers are tested via:
- Unit tests: `tests/transaction_manager_test.cpp`
- Integration tests: `tests/transaction_integration_test.cpp`
- Benchmarks: `benchmarks/transaction_benchmark.cpp`

---

## Documentation

For detailed implementation documentation, see:
- [Source Implementation README](../../src/transaction/README.md)
- [Future Enhancements](FUTURE_ENHANCEMENTS.md)
- [API Reference](../../docs/api/transaction.md)

---

## Support

- Report issues: [GitHub Issues](https://github.com/themisdb/themisdb/issues)
- Discussions: [GitHub Discussions](https://github.com/themisdb/themisdb/discussions)
- Email: support@themisdb.io

---

## License

Copyright © 2024 ThemisDB Contributors. Licensed under Apache 2.0.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
