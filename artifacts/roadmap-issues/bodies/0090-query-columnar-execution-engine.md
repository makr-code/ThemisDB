### Context

This issue implements the roadmap item 'Columnar Execution Engine' for the query domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Columnar Execution Engine

### Goal

Deliver the scoped changes for Columnar Execution Engine in src/query/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Columnar Execution Engine
**Priority:** High  
**Target Version:** v1.7.0

Vectorized columnar execution for analytical queries, inspired by DuckDB and ClickHouse. Delivered via `query::VectorizedExecutionEngine` (JSON facade) delegating to `analytics::ColumnarExecutionEngine` for batch execution and late materialization.

**Features:**
- Columnar data layout in memory
- Vectorized operators (1024 tuples/batch)
- Late materialization
- Columnar compression (dictionary, RLE, bit-packing)
- Adaptive row/columnar switching

**Architecture:**
```cpp
class ColumnarExecutionEngine {
public:
    struct ColumnBatch {
        std::vector<std::shared_ptr<Column>> columns;
        size_t row_count;
        SelectionVector selection;  // Filtered rows
    };
    
    // Execute query in columnar mode
    Result<std::vector<ColumnBatch>> executeColumnar(
        const QueryPlan& plan,
        const ExecutionContext& ctx);
    
    // Operators work on batches
    ColumnBatch filterBatch(const ColumnBatch& input, 
                           const Expression& predicate);
    ColumnBatch projectBatch(const ColumnBatch& input,
                            const std::vector<Expression>& projections);
    ColumnBatch aggregateBatch(const ColumnBatch& input,
                              const AggregateSpec& spec);
};

// Example: Vectorized filter
void filterColumn(const int64_t* input, int64_t threshold,
                 SelectionVector& output, size_t count) {
    // SIMD comparison (8 values at once)
    __m256i thresh_vec = _mm256_set1_epi64x(threshold);
    for (size_t i = 0; i < count; i += 4) {
        __m256i vals = _mm256_loadu_si256((__m256i*)(input + i));
        __m256i cmp = _mm256_cmpgt_epi64(vals, thresh_vec);
        int mask = _mm256_movemask_pd(_mm256_castsi256_pd(cmp));
        // Write selected indices
        if (mask & 1) output.push_back(i);
        if (mask & 2) output.push_back(i + 1);
        if (mask & 4) output.push_back(i + 2);
        if (mask & 8) output.push_back(i + 3);
    }
}
```

**Use Cases:**
- OLAP queries (aggregations, scans)
- Large table joins
- GROUP BY with high cardinality
- Window functions

**Performance Targets (Delivered):**
- Baseline: row-wise JSON execution without columnar batching or SIMD acceleration
- Scans: 3–5× faster than row-wise baseline on 1M-row synthetic dataset (batch=1,024, CPU-only with SIMD enabled)
- Aggregations: 5–10× faster on the same dataset (SUM/AVG/MIN/MAX/COUNT_DISTINCT)
- Joins: 3–6× faster for hash/merge joins on key-distributed 1M-row synthetic tables

---

### Acceptance Criteria

- [x] Columnar data layout in memory
- [x] Vectorized operators (1024 tuples/batch)
- [x] Late materialization
- [x] Columnar compression (dictionary, RLE, bit-packing)
- [x] Adaptive row/columnar switching
- [x] OLAP queries (aggregations, scans)
- [x] Large table joins
- [x] GROUP BY with high cardinality
- [x] Window functions
- [x] Scan performance target achieved (3–5× over row-wise baseline; see Performance Targets)
- [x] Aggregation performance target achieved (5–10× over row-wise baseline; see Performance Targets)
- [x] Join performance target achieved (3–6× over row-wise baseline; see Performance Targets)

### Relationships

- Roadmap row: #90 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/query/FUTURE_ENHANCEMENTS.md#columnar-execution-engine-delivered-v170
- Source key: roadmap:90:query:v1.7.0:columnar-execution-engine

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:90:query:v1.7.0:columnar-execution-engine -->
<!-- roadmap-ref: row=90;module=query;target=v1.7.0 -->
<!-- roadmap-detail: src/query/FUTURE_ENHANCEMENTS.md#columnar-execution-engine-delivered-v170 -->