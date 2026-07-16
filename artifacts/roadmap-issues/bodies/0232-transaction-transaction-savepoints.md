### Context

This issue implements the roadmap item 'Transaction Savepoints' for the transaction domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0) and targets milestone v1.8.0.

Primary detail section: Transaction Savepoints

### Goal

Deliver the scoped changes for Transaction Savepoints in src/transaction/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

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

### Acceptance Criteria

- [ ] Named savepoints within transaction (`createSavepoint`, `rollbackToSavepoint`, `releaseSavepoint`)
- [ ] Rollback to savepoint — undoes writes without aborting the full transaction
- [ ] Automatic savepoint cleanup on rollback/release
- [ ] Savepoint stacking with correct LIFO ordering
- [ ] SAGA step trimming — compensating actions added after a savepoint are discarded on rollback
- [ ] Complex multi-step operations
- [ ] Error recovery within transaction
- [ ] Conditional processing
- [ ] Nested logic with partial rollback
- [ ] Backed by RocksDB `SetSavePoint` / `RollbackToSavePoint` / `PopSavePoint`
- [ ] `savepoints_` vector in `Transaction` tracks named entries in creation order
- [ ] SAGA `trimToSize` removes compensating actions added after the rollback point

### Relationships

- Roadmap row: #232 (🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/transaction/FUTURE_ENHANCEMENTS.md#transaction-savepoints
- Source key: roadmap:232:transaction:v1.8.0:transaction-savepoints

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:232:transaction:v1.8.0:transaction-savepoints -->
<!-- roadmap-ref: row=232;module=transaction;target=v1.8.0 -->
<!-- roadmap-detail: src/transaction/FUTURE_ENHANCEMENTS.md#transaction-savepoints -->
