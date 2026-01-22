---
name: 📊 Columnar Storage: Bitmap Indices for Categorical Data
about: Implement bitmap indices to accelerate queries on low-cardinality categorical columns
title: "[COLUMNAR] Bitmap Indices for Categorical Data"
labels: priority:P2, type:enhancement, area:storage, effort:medium, component:columnar-format
assignees: ''
---

## 📊 Columnar Storage Enhancement - Phase 3

**Current Status:** Zone maps implemented for min/max filtering  
**Priority:** P2 (Medium)  
**Effort:** 2-3 weeks  
**Target Version:** v1.5.0  
**Parent PR:** #XXX (Columnar Storage Format Optimization)  
**Related Files:**
- `include/storage/columnar_format.h`
- `src/storage/columnar_format.cpp`
- `include/index/bitmap_index.h` (new)
- `src/index/bitmap_index.cpp` (new)

---

## 📋 Problem Description

The columnar storage format currently uses **zone maps** for segment-level filtering. While effective for range queries, zone maps are limited for **categorical/discrete value queries**:

**Current Behavior:**
```sql
SELECT * FROM orders WHERE status = 'SHIPPED';
-- Zone map checks: min_str="CANCELLED", max_str="SHIPPED"
-- Result: Cannot skip segment, must scan all rows
```

**Desired Behavior with Bitmap Index:**
```sql
SELECT * FROM orders WHERE status = 'SHIPPED';
-- Bitmap index lookup: O(1) to find matching segments
-- Result: Skip segments with zero matches, scan only relevant rows
```

**Performance Impact:** Missing **10-100x speedup** on categorical column queries in OLAP workloads.

### Use Cases

1. **E-commerce:** Order status, product category, shipping method
2. **IoT:** Device type, sensor status, alert level  
3. **Logs:** Log level, service name, environment
4. **Analytics:** User segment, campaign type, A/B test variant

---

## 🎯 Requirements

### Must Have (P2)

- [ ] **Bitmap Index Structure**
  - Sparse bitmap representation (Roaring Bitmaps recommended)
  - One bitmap per unique value in the column
  - Efficient storage for low-cardinality columns (<1000 unique values)
  - Support for equality queries: `column = value`
  - Support for IN queries: `column IN (value1, value2, ...)`

- [ ] **Index Building**
  - Automatic bitmap index creation during segment encoding
  - Incremental update support for new segments
  - Memory-efficient construction (streaming build)
  - Parallel index building for large datasets

- [ ] **Query Integration**
  - Extend `ColumnarFormatManager::filterSegments()` to use bitmap indices
  - Fast value lookup: O(1) to find matching bitmaps
  - Bitmap operations: AND, OR, NOT for complex predicates
  - Result set materialization from bitmap positions

- [ ] **Storage Format**
  - Serialize bitmaps with column segments
  - Compressed bitmap storage (run-length encoding)
  - Header with index metadata (cardinality, size)
  - Deserialization with lazy loading

### Should Have (P2)

- [ ] **Advanced Queries**
  - NOT EQUAL queries: `column != value`
  - NULL value handling
  - Multi-column bitmap operations (e.g., `status = 'ACTIVE' AND region = 'US'`)
  - Bitmap caching for frequently accessed values

- [ ] **Optimization**
  - Cardinality threshold: Only build bitmaps for columns with <1000 unique values
  - Automatic decision: Bitmap vs full scan based on selectivity
  - Memory budget limits for bitmap cache
  - Compression of sparse bitmaps

- [ ] **Monitoring**
  - Bitmap index hit rate metrics
  - Memory usage tracking
  - Query speedup measurements
  - Index effectiveness scoring

### Could Have (P3)

- [ ] **Advanced Features**
  - Range queries with bitmap acceleration (BETWEEN)
  - Bitmap join optimization
  - Dynamic bitmap updates (without full rebuild)
  - Distributed bitmap indices across shards

---

## 📐 Technical Design

### Bitmap Index Structure

```cpp
// New header file: include/index/bitmap_index.h
namespace themis {
namespace index {

// Roaring Bitmap wrapper for efficient sparse bitmap storage
class RoaringBitmapWrapper {
public:
    void add(uint32_t row_id);
    bool contains(uint32_t row_id) const;
    uint32_t cardinality() const;
    
    // Bitmap operations
    static RoaringBitmapWrapper AND(const RoaringBitmapWrapper& a, 
                                    const RoaringBitmapWrapper& b);
    static RoaringBitmapWrapper OR(const RoaringBitmapWrapper& a, 
                                   const RoaringBitmapWrapper& b);
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    static RoaringBitmapWrapper deserialize(const std::vector<uint8_t>& data);
    
private:
    roaring::Roaring bitmap_;
};

// Bitmap index for categorical column
class BitmapIndex {
public:
    // Build index from column data
    static Result<BitmapIndex> build(
        ColumnType type,
        const void* data,
        size_t row_count,
        size_t max_cardinality = 1000
    );
    
    // Query operations
    Result<RoaringBitmapWrapper> lookup(const std::string& value) const;
    Result<RoaringBitmapWrapper> lookupIN(const std::vector<std::string>& values) const;
    
    // Index metadata
    size_t cardinality() const { return value_to_bitmap_.size(); }
    size_t memoryUsage() const;
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    static Result<BitmapIndex> deserialize(const std::vector<uint8_t>& data);
    
private:
    std::unordered_map<std::string, RoaringBitmapWrapper> value_to_bitmap_;
    size_t total_rows_ = 0;
};

} // namespace index
} // namespace themis
```

### Integration with ColumnSegment

```cpp
// Update ColumnSegment to include bitmap index
class ColumnSegment {
public:
    // ... existing methods ...
    
    // Bitmap index accessors
    bool hasBitmapIndex() const { return bitmap_index_.has_value(); }
    const BitmapIndex& bitmapIndex() const { return *bitmap_index_; }
    
    // Build bitmap index if column is categorical
    Result<void> buildBitmapIndex(size_t max_cardinality = 1000);
    
private:
    // ... existing members ...
    std::optional<BitmapIndex> bitmap_index_;
};

// Update ColumnarFormatManager filtering
Result<std::vector<size_t>> ColumnarFormatManager::filterSegments(
    const std::vector<ColumnSegment>& segments,
    size_t column_index,
    const void* filter_value
) {
    if (column_index >= segments.size()) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "Column index out of range"
        ));
    }
    
    const auto& segment = segments[column_index];
    
    // Try bitmap index first (for categorical columns)
    if (segment.hasBitmapIndex()) {
        std::string value = *static_cast<const std::string*>(filter_value);
        auto bitmap_result = segment.bitmapIndex().lookup(value);
        
        if (bitmap_result) {
            // Fast path: Use bitmap to get matching row IDs
            std::vector<size_t> matching_indices;
            auto& bitmap = *bitmap_result;
            // Convert bitmap to segment indices...
            return matching_indices;
        }
    }
    
    // Fallback to zone map filtering
    std::vector<size_t> matching_indices;
    for (size_t i = 0; i < segments.size(); ++i) {
        if (!segments[i].canSkipSegment(filter_value)) {
            matching_indices.push_back(i);
        }
    }
    
    return matching_indices;
}
```

---

## ✅ Acceptance Criteria

- [ ] Bitmap indices built successfully for categorical columns
- [ ] Equality queries achieve 10-100x speedup on low-cardinality columns
- [ ] IN queries with multiple values execute efficiently using bitmap OR
- [ ] Memory usage per bitmap index: <10% of column size
- [ ] Automatic cardinality threshold: Only build for <1000 unique values
- [ ] Serialization/deserialization preserves index integrity
- [ ] All existing columnar format tests still pass
- [ ] 15+ new tests for bitmap index operations
- [ ] Documentation includes usage guidelines and examples

---

## 📊 Performance Targets

| Metric | Target | Measurement |
|--------|--------|-------------|
| Equality query speedup | 10-100x | vs full column scan |
| IN query speedup | 20-200x | vs multiple scans |
| Index build time | <1s per 1M rows | Parallel construction |
| Bitmap lookup time | <1ms | Single value lookup |
| Memory overhead | <10% | vs compressed column size |
| Compressed bitmap size | <5% | vs uncompressed column |

---

## 🔗 Dependencies

- **External Libraries:**
  - CRoaring library for Roaring Bitmaps (via vcpkg)
  - Or: Custom bitmap implementation

- **ThemisDB Components:**
  - ColumnSegment (extended with bitmap index)
  - ColumnarFormatManager (filtering logic)
  - Serialization framework

---

## 📚 References

- **Roaring Bitmaps:** https://roaringbitmap.org/
- **CRoaring Library:** https://github.com/RoaringBitmap/CRoaring
- **Bitmap Index Paper:** "Bitmap Index Design and Evaluation" (O'Neil et al.)
- **Columnar Storage Implementation:** `COLUMNAR_STORAGE_IMPLEMENTATION.md`
- **Parent PR:** #XXX (Columnar Storage Format Optimization)

---

## 🎯 Success Metrics

- [ ] 10-100x speedup demonstrated on real-world categorical queries
- [ ] Zero performance regression on non-categorical columns
- [ ] Production deployment with 1B+ row datasets
- [ ] Positive feedback from OLAP workload users
- [ ] Integration with ThemisDB query optimizer
