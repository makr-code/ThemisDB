### Context

This issue implements the roadmap item 'Adaptive Deadlock Prevention' for the transaction domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.9.0.

Primary detail section: Adaptive Deadlock Prevention

### Goal

Deliver the scoped changes for Adaptive Deadlock Prevention in src/transaction/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

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

### Acceptance Criteria

- [ ] Historical deadlock pattern analysis
- [ ] Lock acquire order recommendation
- [ ] Proactive transaction reordering
- [ ] Dynamic timeout adjustment
- [ ] Deadlock probability scoring
- [ ] Features: Lock patterns, transaction duration, active count
- [ ] Algorithm: Gradient boosting classifier
- [ ] Training: Online learning from deadlock events
- [ ] Accuracy target: >85% precision, >90% recall

### Relationships

- Roadmap row: #261 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/transaction/FUTURE_ENHANCEMENTS.md#adaptive-deadlock-prevention
- Source key: roadmap:261:transaction:v1.9.0:adaptive-deadlock-prevention

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:261:transaction:v1.9.0:adaptive-deadlock-prevention -->
<!-- roadmap-ref: row=261;module=transaction;target=v1.9.0 -->
<!-- roadmap-detail: src/transaction/FUTURE_ENHANCEMENTS.md#adaptive-deadlock-prevention -->
