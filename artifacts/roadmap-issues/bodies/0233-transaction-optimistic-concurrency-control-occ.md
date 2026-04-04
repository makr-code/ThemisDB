### Context

This issue implements the roadmap item 'Optimistic Concurrency Control (OCC)' for the transaction domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0) and targets milestone v1.8.0.

Primary detail section: Optimistic Concurrency Control (OCC)

### Goal

Deliver the scoped changes for Optimistic Concurrency Control (OCC) in src/transaction/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

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
- Tests: `tests/test_transaction_occ.cpp` (11 unit tests covering creation, update, erase, version-conflict detection, retry pattern, and isolation-level compatibility)
- Benchmarks: `benchmarks/bench_transaction_throughput.cpp` — `OccOptimisticPut`, `OccReadVersionAndUpdate`, `OccOptimisticErase`

---

### Acceptance Criteria

- [ ] Per-entity version numbers stored under `occ:ver:{table}:{pk}` as a little-endian uint64_t
- [ ] `getEntityVersion` — read current OCC version without acquiring a lock (returns 0 for non-existent entities, `std::nullopt` when transaction is inactive)
- [ ] `optimisticPut` — write entity only when stored version matches `expected_version`; atomically increments version to `expected_version + 1` on success; pass `expected_version = 0` to create a new entity
- [ ] `optimisticErase` — delete entity only when stored version matches `expected_version`; resets version to 0 (entity gone) on success
- [ ] Version conflict detection with descriptive error messages (includes `expected=N actual=M`)
- [ ] Secondary-index updates (`SecondaryIndexManager`) applied atomically within the OCC write
- [ ] SSI / predicate-lock integration: `checkSerializableWriteConflict` is called by both `optimisticPut` and `optimisticErase` so SERIALIZABLE transactions remain correct
- [ ] Works with all isolation levels (`READ_COMMITTED`, `REPEATABLE_READ`, `SERIALIZABLE`)
- [ ] Transaction-timeout guard: returns an error instead of proceeding when `isTimedOut()` is true
- [ ] Low-contention workloads (>90% success rate)
- [ ] Short-lived transactions
- [ ] Read-mostly workloads
- [ ] Mobile/offline sync scenarios
- [ ] 2-3x faster than pessimistic locking (no contention)
- [ ] Graceful degradation under contention
- [ ] Retry cost: ~1ms per attempt
- [ ] Version keys are stored in RocksDB alongside entity data under `occ:ver:{table}:{pk}`
- [ ] Version encoding: 8-byte little-endian uint64_t (`encodeVersion` / `decodeVersion` helpers)
- [ ] `optimisticPut` / `optimisticErase` use `mvcc_txn_->put` / `mvcc_txn_->del` so all writes participate in the MVCC snapshot; MVCC conflict errors are surfaced as `Status::Error`
- [ ] Tests: `tests/test_transaction_occ.cpp` (11 unit tests covering creation, update, erase, version-conflict detection, retry pattern, and isolation-level compatibility)
- [ ] Benchmarks: `benchmarks/bench_transaction_throughput.cpp` — `OccOptimisticPut`, `OccReadVersionAndUpdate`, `OccOptimisticErase`

### Relationships

- Roadmap row: #233 (🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/transaction/FUTURE_ENHANCEMENTS.md#optimistic-concurrency-control-occ
- Source key: roadmap:233:transaction:v1.8.0:optimistic-concurrency-control-occ

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:233:transaction:v1.8.0:optimistic-concurrency-control-occ -->
<!-- roadmap-ref: row=233;module=transaction;target=v1.8.0 -->
<!-- roadmap-detail: src/transaction/FUTURE_ENHANCEMENTS.md#optimistic-concurrency-control-occ -->
