### Context

This issue implements the roadmap item 'Temporal Indexes' for the temporal domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.2.0.

Primary detail section: Temporal Indexes

### Goal

Deliver the scoped changes for Temporal Indexes in src/temporal/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### Temporal Indexes
**Priority:** High  
**Target Version:** v1.2.0

Specialized indexes for efficient temporal queries.

**Features:**
- Time-range indexes for efficient period queries
- Bi-temporal indexes (transaction + valid time)
- Covering indexes for common temporal patterns
- Index-only scans for temporal queries
- Automatic index selection

**Implementation:**
```cpp
class TemporalIndexManager {
public:
    enum class TemporalIndexType {
        TIME_RANGE,        // Optimized for range queries
        BI_TEMPORAL,       // Transaction + Valid time
        SNAPSHOT,          // Point-in-time queries
        INTERVAL_TREE      // Overlapping period detection
    };
    
    struct TemporalIndexSpec {
        TemporalIndexType type;
        std::vector<std::string> columns;
        std::string time_column;
        bool include_current = true;
        bool include_history = true;
    };
    
    // Create temporal index
    Result<std::string> createTemporalIndex(
        const std::string& table_name,
        const std::string& index_name,
        const TemporalIndexSpec& spec
    );
    
    // Query using temporal index
    Result<std::vector<VersionedDocument>> queryWithTemporalIndex(
        const std::string& index_name,
        const TimeRange& range
    );
};
```

**Index Types:**
1. **Time-Range Index**: B-tree on (start_time, end_time) for range queries
2. **Bi-Temporal Index**: Composite index on transaction time + valid time
3. **Snapshot Index**: Optimized for AS OF queries
4. **Interval Tree**: Efficient overlap detection for period queries

**Performance Impact:**
- AS OF queries: 10-100x speedup with snapshot index
- Range queries: 5-50x speedup with time-range index
- Overlap detection: 50-1000x speedup with interval tree

---

### Acceptance Criteria

- [ ] Time-range indexes for efficient period queries
- [ ] Bi-temporal indexes (transaction + valid time)
- [ ] Covering indexes for common temporal patterns
- [ ] Index-only scans for temporal queries
- [ ] Automatic index selection
- [ ] **Time-Range Index**: B-tree on (start_time, end_time) for range queries
- [ ] **Bi-Temporal Index**: Composite index on transaction time + valid time
- [ ] **Snapshot Index**: Optimized for AS OF queries
- [ ] **Interval Tree**: Efficient overlap detection for period queries
- [ ] AS OF queries: 10-100x speedup with snapshot index
- [ ] Range queries: 5-50x speedup with time-range index
- [ ] Overlap detection: 50-1000x speedup with interval tree

### Relationships

- Roadmap row: #26 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/temporal/FUTURE_ENHANCEMENTS.md#temporal-indexes
- Source key: roadmap:26:temporal:v1.2.0:temporal-indexes

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:26:temporal:v1.2.0:temporal-indexes -->
<!-- roadmap-ref: row=26;module=temporal;target=v1.2.0 -->
<!-- roadmap-detail: src/temporal/FUTURE_ENHANCEMENTS.md#temporal-indexes -->
