### Context

This issue implements the roadmap item '`IndexRecommender`: Access-Pattern Persistence and ML Model' for the metadata domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `IndexRecommender`: Access-Pattern Persistence and ML Model

### Goal

Deliver the scoped changes for `IndexRecommender`: Access-Pattern Persistence and ML Model in src/metadata/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `IndexRecommender`: Access-Pattern Persistence and ML Model
**Priority:** Medium
**Target Version:** v1.8.0

`index_recommender.cpp` maintains access statistics only in memory (`stats_` map). On restart, all access history is lost and recommendations revert to the "no data" state. The recommendation algorithm uses simple heuristics (sort usage ratio vs. equality ratio) rather than a query-workload-aware model.

**Implementation Notes:**
- `[ ]` Persist `stats_` snapshots to RocksDB under key prefix `meta_idx_stats::` on a configurable interval (default 5 min) or on graceful shutdown.
- `[ ]` On `IndexRecommender` construction, load the persisted stats snapshot; merge with any post-restart activity.
- `[ ]` Replace the threshold-based heuristic in `recommend()` (line 122) with a cost-model estimate: use `StatisticsCollector` cardinality/selectivity to estimate index benefit vs. write amplification.
- `[ ]` Emit `metadata.index_recommendation.generated_total` metric per recommendation cycle.

---


**Priority:** High  
**Target Version:** v1.6.0

Comprehensive table and index statistics for query optimization.

**Features:**
- Cardinality estimation
- Data distribution histograms
- Index selectivity
- NULL ratio tracking
- Update frequency tracking

**Implementation:**
```cpp
class StatisticsCollector {
public:
    struct TableStats {
        size_t row_count;
        uint64_t total_size_bytes;
        std::map<std::string, ColumnStats> column_stats;
        std::chrono::system_clock::time_point last_updated;
    };
    
    struct ColumnStats {
        size_t distinct_count;
        size_t null_count;
        double selectivity;
        std::optional<Histogram> distribution;
    };
    
    Result<TableStats> collectStats(const std::string& table_name);
    Result<bool> updateStats(const std::string& table_name);
    Result<TableStats> getStats(const std::string& table_name);
};
```

**Expected Performance:**
- Statistics collection: 1-10 seconds per table
- Update overhead: <5% during collection
- Storage overhead: 1-5% of table size

---

### Acceptance Criteria

- [ ] Persist `stats_` snapshots to RocksDB under key prefix `meta_idx_stats::` on a configurable interval (default 5 min) or on graceful shutdown.
- [ ] On `IndexRecommender` construction, load the persisted stats snapshot; merge with any post-restart activity.
- [ ] Replace the threshold-based heuristic in `recommend()` (line 122) with a cost-model estimate: use `StatisticsCollector` cardinality/selectivity to estimate index benefit vs. write amplification.
- [ ] Emit `metadata.index_recommendation.generated_total` metric per recommendation cycle.
- [ ] Cardinality estimation
- [ ] Data distribution histograms
- [ ] Index selectivity
- [ ] NULL ratio tracking
- [ ] Update frequency tracking
- [ ] Statistics collection: 1-10 seconds per table
- [ ] Update overhead: <5% during collection
- [ ] Storage overhead: 1-5% of table size

### Relationships

- Roadmap row: #186 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/metadata/FUTURE_ENHANCEMENTS.md#indexrecommender-access-pattern-persistence-and-ml-model
- Source key: roadmap:186:metadata:v1.8.0:indexrecommender-access-pattern-persistence-and-ml-model

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:186:metadata:v1.8.0:indexrecommender-access-pattern-persistence-and-ml-model -->
<!-- roadmap-ref: row=186;module=metadata;target=v1.8.0 -->
<!-- roadmap-detail: src/metadata/FUTURE_ENHANCEMENTS.md#indexrecommender-access-pattern-persistence-and-ml-model -->
