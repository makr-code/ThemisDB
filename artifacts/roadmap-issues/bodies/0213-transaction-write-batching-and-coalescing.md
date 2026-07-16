### Context

This issue implements the roadmap item 'Write Batching and Coalescing' for the transaction domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Write Batching and Coalescing

### Goal

Deliver the scoped changes for Write Batching and Coalescing in src/transaction/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Write Batching and Coalescing
**Priority:** Medium  
**Target Version:** v1.8.0

Automatic batching of concurrent small transactions for improved throughput.

**Features:**
- Automatic transaction grouping
- Configurable batch window (1-100ms)
- Fair scheduling (prevent starvation)
- Per-table/per-key batching policies
- Adaptive batch sizing

**Architecture:**
```cpp
class TransactionBatcher {
public:
    struct BatchConfig {
        std::chrono::microseconds window{5000};  // 5ms batch window
        size_t max_batch_size = 1000;
        size_t min_batch_size = 10;
        bool enable_adaptive = true;
    };
    
    void setBatchConfig(const BatchConfig& config);
    
    // Submit transaction for batching
    std::future<Status> submitAsync(Transaction&& txn);
    
    // Force flush current batch
    void flush();
};

// Example: High-throughput ingestion
TransactionBatcher batcher;
batcher.setBatchConfig({
    .window = std::chrono::milliseconds(10),
    .max_batch_size = 5000,
    .enable_adaptive = true
});

// Submit many small transactions
std::vector<std::future<Status>> futures;
for (const auto& user : users) {
    auto txn = txn_mgr.begin();
    txn.putEntity("users", user);
    futures.push_back(batcher.submitAsync(std::move(txn)));
}

// All transactions batched and committed together
// 10-100x throughput improvement
for (auto& future : futures) {
    auto status = future.get();
}
```

**Performance Gains:**
- Small transactions: 10-100x throughput improvement
- Reduced WAL sync overhead
- Better CPU/disk utilization
- Latency trade-off: +1-10ms per transaction

---

### Acceptance Criteria

- [ ] Automatic transaction grouping
- [ ] Configurable batch window (1-100ms)
- [ ] Fair scheduling (prevent starvation)
- [ ] Per-table/per-key batching policies
- [ ] Adaptive batch sizing
- [ ] Small transactions: 10-100x throughput improvement
- [ ] Reduced WAL sync overhead
- [ ] Better CPU/disk utilization
- [ ] Latency trade-off: +1-10ms per transaction

### Relationships

- Roadmap row: #213 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/transaction/FUTURE_ENHANCEMENTS.md#write-batching-and-coalescing
- Source key: roadmap:213:transaction:v1.8.0:write-batching-and-coalescing

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:213:transaction:v1.8.0:write-batching-and-coalescing -->
<!-- roadmap-ref: row=213;module=transaction;target=v1.8.0 -->
<!-- roadmap-detail: src/transaction/FUTURE_ENHANCEMENTS.md#write-batching-and-coalescing -->
