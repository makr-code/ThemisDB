### Context

This issue implements the roadmap item 'Adaptive Join Strategies' for the query domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Adaptive Join Strategies

### Goal

Deliver the scoped changes for Adaptive Join Strategies in src/query/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Adaptive Join Strategies
**Priority:** High  
**Target Version:** v1.7.0

Intelligent join algorithm selection based on data characteristics and runtime statistics.

**Join Algorithms:**
```cpp
enum JoinAlgorithm {
    HASH_JOIN,           // Build hash table on smaller side
    MERGE_JOIN,          // Sorted inputs, O(n+m) merge
    NESTED_LOOP_JOIN,    // Small left side (<1000 rows)
    INDEX_NESTED_LOOP,   // Right side has index
    BROADCAST_JOIN,      // Distributed: broadcast small table
    SHUFFLE_JOIN,        // Distributed: repartition both sides
    GRACE_HASH_JOIN      // Partitioned hash join (out-of-core)
};

class AdaptiveJoinExecutor {
public:
    // Choose best join algorithm at runtime
    JoinResult executeJoin(
        const JoinSpec& spec,
        const Table& left,
        const Table& right,
        const RuntimeStats& stats);
    
private:
    JoinAlgorithm selectAlgorithm(
        size_t left_rows, size_t right_rows,
        bool left_sorted, bool right_sorted,
        bool has_index, size_t memory_budget);
};

// Cost model
double estimateJoinCost(JoinAlgorithm algo, 
                       size_t left_rows, 
                       size_t right_rows) {
    switch (algo) {
        case HASH_JOIN:
            return left_rows + right_rows;  // Build + probe
        case MERGE_JOIN:
            return left_rows + right_rows +  // Scan both
                   (left_sorted ? 0 : left_rows * log(left_rows)) +
                   (right_sorted ? 0 : right_rows * log(right_rows));
        case NESTED_LOOP_JOIN:
            return left_rows * right_rows;  // Quadratic
        default:
            return std::numeric_limits<double>::max();
    }
}
```

**Adaptive Selection Criteria:**
- **Hash Join**: Default for large equi-joins
- **Merge Join**: Both inputs sorted on join key
- **Nested Loop**: Left side <1000 rows
- **Index Nested Loop**: Right has index, left <10K rows
- **Grace Hash**: Memory budget exceeded

---

### Acceptance Criteria

- [ ] **Hash Join**: Default for large equi-joins
- [ ] **Merge Join**: Both inputs sorted on join key
- [ ] **Nested Loop**: Left side <1000 rows
- [ ] **Index Nested Loop**: Right has index, left <10K rows
- [ ] **Grace Hash**: Memory budget exceeded

### Relationships

- Roadmap row: #91 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/query/FUTURE_ENHANCEMENTS.md#adaptive-join-strategies
- Source key: roadmap:91:query:v1.7.0:adaptive-join-strategies

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:91:query:v1.7.0:adaptive-join-strategies -->
<!-- roadmap-ref: row=91;module=query;target=v1.7.0 -->
<!-- roadmap-detail: src/query/FUTURE_ENHANCEMENTS.md#adaptive-join-strategies -->
