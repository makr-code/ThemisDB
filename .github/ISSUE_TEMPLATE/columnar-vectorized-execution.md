---
name: 🚀 Columnar Storage: Vectorized Query Execution
about: Implement vectorized execution engine to process columnar data in batches
title: "[COLUMNAR] Vectorized Query Execution Engine"
labels: priority:P1, type:enhancement, area:query, effort:large, component:columnar-format
assignees: ''
---

## 🚀 Columnar Storage Enhancement - Phase 5

**Current Status:** Row-at-a-time query execution on columnar storage  
**Priority:** P1 (High)  
**Effort:** 6-8 weeks  
**Target Version:** v2.0.0  
**Parent PR:** #XXX (Columnar Storage Format Optimization)  
**Related Files:**
- `include/query/vectorized_executor.h` (new)
- `src/query/vectorized_executor.cpp` (new)
- `include/storage/columnar_format.h`
- `src/query/query_engine.cpp`

---

## 📋 Problem Description

Current ThemisDB query execution processes data **one row at a time** (Volcano-style iterator model):

```cpp
// Current row-at-a-time execution
while (auto row = scan.next()) {
    if (evaluate_predicate(row)) {
        auto result = project(row);
        output.append(result);
    }
}
```

**Performance Problems:**
- **Function call overhead:** Virtual function calls per row
- **Branch mispredictions:** Unpredictable control flow
- **Cache inefficiency:** Poor spatial/temporal locality
- **No SIMD utilization:** Single-value operations
- **Throughput:** ~1-5M rows/second

**Vectorized Execution Solution:**

```cpp
// Vectorized batch execution
while (auto batch = scan.nextBatch(1024)) {  // 1024 rows at once
    auto mask = evaluate_predicate_batch(batch);
    auto results = project_batch(batch, mask);
    output.appendBatch(results);
}
```

**Performance Benefits:**
- **Reduced overhead:** 1000x fewer function calls
- **Better branch prediction:** Batch operations
- **Cache-friendly:** Sequential memory access
- **SIMD-ready:** Process 4-16 values per instruction
- **Throughput:** ~50-500M rows/second (10-100x faster)

---

## 🎯 Requirements

### Must Have (P1)

- [ ] **Vectorized Data Model**
  - `VectorBatch` class: Columnar in-memory representation
  - Support 1024-4096 rows per batch
  - Efficient null handling (validity bitmaps)
  - Zero-copy integration with ColumnSegment

- [ ] **Vectorized Operators**
  - **Scan:** Read columnar data in batches
  - **Filter:** Evaluate predicates on entire batches
  - **Project:** Extract subset of columns
  - **Aggregation:** SUM, COUNT, MIN, MAX on batches
  - **Join:** Hash join with batch processing

- [ ] **Vectorized Expression Evaluation**
  - Arithmetic operators: +, -, *, / on batches
  - Comparison operators: =, !=, <, >, <=, >= on batches
  - Logical operators: AND, OR, NOT with selection vectors
  - Function calls: Batch evaluation of built-in functions

- [ ] **Selection Vectors**
  - Bitmap representation of selected rows
  - Lazy materialization (avoid copying)
  - Efficient AND/OR/NOT operations
  - Integration with Bloom filters and zone maps

### Should Have (P1)

- [ ] **Advanced Operators**
  - Sort: Radix sort on columnar batches
  - Window functions: SIMD-accelerated rolling aggregates
  - String operations: Batch string matching, LIKE
  - Type conversions: Bulk casting

- [ ] **Memory Management**
  - Batch pooling to reduce allocations
  - Memory budget limits (spill to disk)
  - Vectorized compression/decompression
  - Adaptive batch sizing based on memory pressure

- [ ] **Query Compilation**
  - Just-in-time (JIT) compilation of vectorized loops
  - LLVM code generation for hot paths
  - Template specialization for common patterns
  - Profile-guided optimization

### Could Have (P2)

- [ ] **GPU Acceleration**
  - CUDA/OpenCL kernels for vectorized ops
  - GPU-resident columnar data
  - Hybrid CPU/GPU execution
  - Automatic operator placement

---

## 📐 Technical Design

### VectorBatch Structure

```cpp
// New file: include/query/vectorized_executor.h
namespace themis {
namespace query {

// Columnar batch of rows (Apache Arrow-inspired)
class VectorBatch {
public:
    VectorBatch(size_t capacity = 1024);
    
    // Add column to batch
    void addColumn(const std::string& name, 
                  ColumnType type,
                  const void* data,
                  size_t count);
    
    // Access column data
    const void* getColumnData(size_t col_idx) const;
    const uint8_t* getValidityBitmap(size_t col_idx) const;
    
    // Batch metadata
    size_t rowCount() const { return row_count_; }
    size_t columnCount() const { return columns_.size(); }
    
    // Selection vector (which rows are selected)
    const SelectionVector& selection() const { return selection_; }
    void setSelection(SelectionVector&& sel) { selection_ = std::move(sel); }
    
private:
    struct Column {
        std::string name;
        ColumnType type;
        std::vector<uint8_t> data;
        std::vector<uint8_t> validity_bitmap;  // 1 bit per row
    };
    
    std::vector<Column> columns_;
    size_t row_count_ = 0;
    SelectionVector selection_;  // Which rows are selected
};

// Selection vector: Bitmap or index list
class SelectionVector {
public:
    // Create from predicate evaluation
    static SelectionVector fromPredicate(
        const VectorBatch& batch,
        const Expression& predicate
    );
    
    // Logical operations
    SelectionVector AND(const SelectionVector& other) const;
    SelectionVector OR(const SelectionVector& other) const;
    SelectionVector NOT() const;
    
    // Accessors
    bool isSelected(size_t row_idx) const;
    size_t selectedCount() const;
    
private:
    std::vector<uint64_t> bitmap_;  // Packed bits
    size_t total_rows_ = 0;
};

} // namespace query
} // namespace themis
```

### Vectorized Scan Operator

```cpp
// Vectorized scan from ColumnSegment
class VectorizedScan {
public:
    VectorizedScan(
        const std::vector<ColumnSegment>& segments,
        const std::vector<size_t>& column_indices,
        size_t batch_size = 1024
    );
    
    // Get next batch of rows
    Result<VectorBatch> nextBatch();
    
    // Apply pushdown predicates
    void addPredicate(Expression predicate);
    
private:
    const std::vector<ColumnSegment>& segments_;
    std::vector<size_t> column_indices_;
    size_t batch_size_;
    size_t current_row_ = 0;
    std::vector<Expression> predicates_;
};

Result<VectorBatch> VectorizedScan::nextBatch() {
    VectorBatch batch(batch_size_);
    
    // Read batch_size_ rows from each selected column
    for (size_t col_idx : column_indices_) {
        const auto& segment = segments_[col_idx];
        
        // Decode batch of rows from segment
        auto data = segment.decodeBatch(current_row_, batch_size_);
        
        batch.addColumn(
            segment.metadata().name,
            segment.metadata().type,
            data->data(),
            data->size()
        );
    }
    
    current_row_ += batch_size_;
    
    // Apply predicates to create selection vector
    for (const auto& pred : predicates_) {
        auto selection = SelectionVector::fromPredicate(batch, pred);
        batch.setSelection(batch.selection().AND(selection));
    }
    
    return batch;
}
```

### Vectorized Filter Operator

```cpp
// Vectorized predicate evaluation
class VectorizedFilter {
public:
    VectorizedFilter(Expression predicate);
    
    // Evaluate predicate on batch, return selection vector
    SelectionVector evaluate(const VectorBatch& batch) const;
    
private:
    Expression predicate_;
};

SelectionVector VectorizedFilter::evaluate(const VectorBatch& batch) const {
    // Example: column > 100
    if (predicate_.isComparison()) {
        auto col_idx = predicate_.getColumnIndex();
        auto const_value = predicate_.getConstant();
        
        const int32_t* col_data = static_cast<const int32_t*>(
            batch.getColumnData(col_idx)
        );
        
        SelectionVector result;
        
        // Vectorized comparison (SIMD-friendly loop)
        for (size_t i = 0; i < batch.rowCount(); i += 8) {
            // Load 8 INT32 values
            __m256i values = _mm256_loadu_si256(
                reinterpret_cast<const __m256i*>(&col_data[i])
            );
            
            // Compare with constant
            __m256i threshold = _mm256_set1_epi32(const_value);
            __m256i cmp = _mm256_cmpgt_epi32(values, threshold);
            
            // Update selection bitmap
            int mask = _mm256_movemask_epi8(cmp);
            result.setBits(i, mask);
        }
        
        return result;
    }
    
    // Fallback to scalar evaluation
    return SelectionVector::fromPredicateScalar(batch, predicate_);
}
```

### Vectorized Aggregation

```cpp
// Vectorized SUM aggregation
class VectorizedSum {
public:
    VectorizedSum(size_t column_index);
    
    // Process batch and update aggregate
    void processBatch(const VectorBatch& batch);
    
    // Get final result
    int64_t getResult() const { return sum_; }
    
private:
    size_t column_index_;
    int64_t sum_ = 0;
};

void VectorizedSum::processBatch(const VectorBatch& batch) {
    const int32_t* data = static_cast<const int32_t*>(
        batch.getColumnData(column_index_)
    );
    
    const auto& selection = batch.selection();
    
    // SIMD-accelerated sum
    __m256i sum_vec = _mm256_setzero_si256();
    
    for (size_t i = 0; i < batch.rowCount(); i += 8) {
        if (selection.areAllSelected(i, 8)) {
            __m256i values = _mm256_loadu_si256(
                reinterpret_cast<const __m256i*>(&data[i])
            );
            sum_vec = _mm256_add_epi32(sum_vec, values);
        }
    }
    
    // Horizontal sum of SIMD lanes
    int32_t lane_sums[8];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(lane_sums), sum_vec);
    
    for (int32_t val : lane_sums) {
        sum_ += val;
    }
}
```

---

## ✅ Acceptance Criteria

- [ ] Vectorized execution engine processes batches of 1024-4096 rows
- [ ] 10-100x throughput improvement over row-at-a-time execution
- [ ] All SQL operators work with vectorized execution (scan, filter, project, aggregate, join)
- [ ] SIMD utilization demonstrated in performance profiles
- [ ] Query latency <100ms for 100M row aggregations
- [ ] All existing query tests pass with vectorized engine
- [ ] 50+ new tests for vectorized operators
- [ ] Documentation includes vectorization architecture guide

---

## 📊 Performance Targets

| Operation | Row-at-a-Time | Vectorized | Speedup |
|-----------|---------------|------------|---------|
| Scan throughput | 10M rows/s | 500M rows/s | 50x |
| Filter (int > 100) | 5M rows/s | 300M rows/s | 60x |
| SUM aggregation | 15M rows/s | 800M rows/s | 53x |
| Hash join | 2M rows/s | 100M rows/s | 50x |
| String LIKE | 1M rows/s | 50M rows/s | 50x |

---

## 🔗 Dependencies

- **Design Inspiration:**
  - Apache Arrow: Columnar in-memory format
  - DuckDB: Vectorized execution engine
  - ClickHouse: High-performance OLAP

- **ThemisDB Components:**
  - ColumnSegment (batch decoding)
  - Query engine (integration point)
  - Expression evaluator (vectorized evaluation)

---

## 📚 References

- **MonetDB/X100 Paper:** "MonetDB/X100: Hyper-Pipelining Query Execution" (Boncz et al.)
- **Vectorization Paper:** "Efficiently Compiling Efficient Query Plans for Modern Hardware" (Neumann, 2011)
- **Apache Arrow:** https://arrow.apache.org/
- **DuckDB Architecture:** https://duckdb.org/internals/
- **Columnar Storage Implementation:** `COLUMNAR_STORAGE_IMPLEMENTATION.md`

---

## 🎯 Success Metrics

- [ ] 10-100x query throughput improvement on OLAP workloads
- [ ] Query latency targets achieved (<1s for 1B rows)
- [ ] Production deployment with vectorized engine
- [ ] SIMD utilization >80% in CPU profiles
- [ ] Zero correctness issues (identical results to row-at-a-time)
