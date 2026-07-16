### Context

This issue implements the roadmap item 'Serializable Snapshot Isolation (SSI)' for the transaction domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Serializable Snapshot Isolation (SSI)

### Goal

Deliver the scoped changes for Serializable Snapshot Isolation (SSI) in src/transaction/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

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

### Acceptance Criteria

- [ ] Predicate lock tracking for range queries
- [ ] Read-write conflict detection
- [ ] Write-write conflict detection
- [ ] Automatic serialization failure detection
- [ ] Transaction retry with exponential backoff
- [ ] SIREAD locks for reads that may cause conflicts
- [ ] Commit-time validation of read/write sets
- [ ] False positive rate: <5% (tunable with granularity)
- [ ] Performance overhead: 10-15% vs Snapshot isolation
- [ ] True serializability without holding locks
- [ ] Eliminates write skew anomalies
- [ ] Prevents lost updates in complex scenarios
- [ ] Better than traditional 2PL for read-heavy workloads

### Relationships

- Roadmap row: #122 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/transaction/FUTURE_ENHANCEMENTS.md#serializable-snapshot-isolation-ssi
- Source key: roadmap:122:transaction:v1.8.0:serializable-snapshot-isolation-ssi

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:122:transaction:v1.8.0:serializable-snapshot-isolation-ssi -->
<!-- roadmap-ref: row=122;module=transaction;target=v1.8.0 -->
<!-- roadmap-detail: src/transaction/FUTURE_ENHANCEMENTS.md#serializable-snapshot-isolation-ssi -->
