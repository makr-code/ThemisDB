### Context

This issue implements the roadmap item 'Parallel Query Execution (Intra-Query)' for the query domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Parallel Query Execution (Intra-Query)

### Goal

Deliver the scoped changes for Parallel Query Execution (Intra-Query) in src/query/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Parallel Query Execution (Intra-Query)
**Priority:** High  
**Target Version:** v1.7.0

Parallelize single query execution across multiple CPU cores.

**Parallelization Strategies:**
```cpp
class ParallelExecutor {
public:
    struct ParallelConfig {
        size_t max_threads = std::thread::hardware_concurrency();
        size_t morsel_size = 1024;  // Rows per task
        bool enable_parallel_scan = true;
        bool enable_parallel_join = true;
        bool enable_parallel_aggregate = true;
    };
    
    // Parallel table scan
    Result<std::vector<BaseEntity>> parallelScan(
        const std::string& table,
        const Expression& filter,
        size_t num_threads);
    
    // Parallel hash join
    Result<std::vector<JoinTuple>> parallelHashJoin(
        const Table& left,
        const Table& right,
        const JoinSpec& spec,
        size_t num_threads);
    
    // Parallel aggregation
    Result<AggregateResult> parallelAggregate(
        const Table& input,
        const AggregateSpec& spec,
        size_t num_threads);
};

// Example: 4-way parallel scan
auto results = parallel_exec.parallelScan(
    "large_table",
    parse("field > 100"),
    4  // 4 threads
);
// Each thread processes table_size/4 rows

// Parallel hash join (partitioned)
// 1. Partition both sides by join key hash
// 2. Each thread builds hash table for its partition
// 3. Each thread probes its hash table
// 4. Merge results
auto join_result = parallel_exec.parallelHashJoin(
    left_table, right_table, join_spec, 8
);
```

**Morsel-Driven Parallelism:**
```
Table (100M rows)
    ↓
Split into morsels (1024 rows each)
    ↓
┌────────┬────────┬────────┬────────┐
│Thread 1│Thread 2│Thread 3│Thread 4│
└────────┴────────┴────────┴────────┘
    ↓        ↓        ↓        ↓
Process morsels (scan, filter, project)
    ↓        ↓        ↓        ↓
Merge results → Next operator
```

**Performance Targets:**
- Linear scaling up to 8 cores
- 70-80% efficiency at 16 cores
- Scans: 4x speedup on 4 cores
- Joins: 3x speedup on 4 cores

---

### Acceptance Criteria

- [ ] Linear scaling up to 8 cores
- [ ] 70-80% efficiency at 16 cores
- [ ] Scans: 4x speedup on 4 cores
- [ ] Joins: 3x speedup on 4 cores

### Relationships

- Roadmap row: #92 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/query/FUTURE_ENHANCEMENTS.md#parallel-query-execution-intra-query
- Source key: roadmap:92:query:v1.7.0:parallel-query-execution-intra-query

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:92:query:v1.7.0:parallel-query-execution-intra-query -->
<!-- roadmap-ref: row=92;module=query;target=v1.7.0 -->
<!-- roadmap-detail: src/query/FUTURE_ENHANCEMENTS.md#parallel-query-execution-intra-query -->
