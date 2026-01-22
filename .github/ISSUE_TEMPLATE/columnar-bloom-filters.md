---
name: 🔍 Columnar Storage: Bloom Filters for Sparse Columns
about: Implement Bloom filters to accelerate membership tests on high-cardinality sparse columns
title: "[COLUMNAR] Bloom Filters for Sparse Columns"
labels: priority:P2, type:enhancement, area:storage, effort:medium, component:columnar-format
assignees: ''
---

## 🔍 Columnar Storage Enhancement - Phase 3

**Current Status:** Zone maps and bitmap indices for filtering  
**Priority:** P2 (Medium)  
**Effort:** 2-3 weeks  
**Target Version:** v1.5.0  
**Parent PR:** #XXX (Columnar Storage Format Optimization)  
**Related Files:**
- `include/storage/columnar_format.h`
- `src/storage/columnar_format.cpp`
- `include/index/bloom_filter.h` (new)
- `src/index/bloom_filter.cpp` (new)

---

## 📋 Problem Description

Current columnar filtering strategies have limitations for **high-cardinality sparse columns**:

1. **Zone Maps:** Only work for range queries (min/max), not membership tests
2. **Bitmap Indices:** Memory-inefficient for high cardinality (>1000 unique values)

**Example Problem:**
```sql
-- High-cardinality column (millions of unique user IDs)
SELECT * FROM events WHERE user_id = '550e8400-e29b-41d4-a716-446655440000';

-- Zone map: Cannot help (user IDs don't have meaningful min/max)
-- Bitmap index: Too memory-intensive (1M+ unique IDs)
-- Result: Must scan entire segment
```

**Bloom Filter Solution:**
- Probabilistic membership test: "Is user_id in this segment?"
- False positive rate: <1% (configurable)
- Memory: ~10 bits per element (much smaller than bitmap)
- Query optimization: Skip segments with 99%+ confidence

**Performance Impact:** Missing **10-50x speedup** on high-cardinality point queries.

---

## 🎯 Requirements

### Must Have (P2)

- [ ] **Bloom Filter Implementation**
  - Configurable false positive rate (default: 1%)
  - Multiple hash functions (MurmurHash3, xxHash)
  - Bit array storage with efficient bit operations
  - Support for string, INT64, UUID data types

- [ ] **Segment-Level Bloom Filters**
  - One Bloom filter per column segment
  - Automatic creation during segment encoding
  - Fast membership test: O(k) where k = hash functions
  - Serialization with segment metadata

- [ ] **Query Integration**
  - Extend `ColumnarFormatManager::filterSegments()` to use Bloom filters
  - Segment skip decisions based on Bloom filter negatives
  - Fallback to zone maps for non-Bloom-filter columns
  - Integration with query optimizer

- [ ] **Memory Management**
  - Bloom filter size estimation based on cardinality
  - Configurable memory budget per segment
  - Lazy loading of Bloom filters (load on demand)
  - LRU cache for frequently accessed filters

### Should Have (P2)

- [ ] **Adaptive Bloom Filters**
  - Dynamic sizing based on actual cardinality
  - False positive rate monitoring
  - Automatic rebuild if FP rate exceeds threshold
  - Multi-level Bloom filters (segment + block)

- [ ] **Advanced Features**
  - Counting Bloom filters (support for deletions)
  - Scalable Bloom filters (dynamic growth)
  - Compressed Bloom filters (RLE on bit arrays)
  - Bloom filter statistics and telemetry

- [ ] **Optimization**
  - SIMD-accelerated hash functions
  - Cache-friendly bit array layout
  - Parallel Bloom filter construction
  - Bloom filter merging for query planning

### Could Have (P3)

- [ ] **Extended Data Types**
  - Bloom filters for binary/blob columns
  - Geographic coordinates (lat/lon pairs)
  - JSON path expressions
  - Complex types (arrays, structs)

---

## 📐 Technical Design

### Bloom Filter Structure

```cpp
// New file: include/index/bloom_filter.h
namespace themis {
namespace index {

class BloomFilter {
public:
    // Constructor with target false positive rate
    BloomFilter(size_t expected_elements, double false_positive_rate = 0.01);
    
    // Add element to filter
    void add(const std::string& element);
    void add(int64_t element);
    void add(const void* data, size_t size);
    
    // Membership test
    bool mightContain(const std::string& element) const;
    bool mightContain(int64_t element) const;
    bool mightContain(const void* data, size_t size) const;
    
    // Metadata
    size_t capacity() const { return num_elements_; }
    size_t bitArraySize() const { return bit_array_.size(); }
    double estimatedFPRate() const;
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    static Result<BloomFilter> deserialize(const std::vector<uint8_t>& data);
    
private:
    std::vector<uint64_t> bit_array_;  // Bit array (packed in uint64_t)
    size_t num_elements_;              // Number of elements added
    size_t num_bits_;                  // Total bits in filter
    uint8_t num_hash_functions_;       // Number of hash functions
    
    // Hash functions
    uint64_t hash1(const void* data, size_t size) const;
    uint64_t hash2(const void* data, size_t size) const;
    
    // Bit operations
    void setBit(size_t bit_index);
    bool getBit(size_t bit_index) const;
};

} // namespace index
} // namespace themis
```

### Integration with ColumnSegment

```cpp
// Update ColumnSegment to include Bloom filter
class ColumnSegment {
public:
    // ... existing methods ...
    
    // Bloom filter accessors
    bool hasBloomFilter() const { return bloom_filter_.has_value(); }
    const BloomFilter& bloomFilter() const { return *bloom_filter_; }
    
    // Build Bloom filter for high-cardinality column
    Result<void> buildBloomFilter(
        double false_positive_rate = 0.01,
        size_t min_cardinality = 1000
    );
    
private:
    // ... existing members ...
    std::optional<BloomFilter> bloom_filter_;
};

// Bloom filter-aware segment filtering
Result<std::vector<size_t>> ColumnarFormatManager::filterSegments(
    const std::vector<ColumnSegment>& segments,
    size_t column_index,
    const void* filter_value
) {
    const auto& segment = segments[column_index];
    
    // Try Bloom filter first (for high-cardinality columns)
    if (segment.hasBloomFilter()) {
        std::string value = *static_cast<const std::string*>(filter_value);
        
        if (!segment.bloomFilter().mightContain(value)) {
            // Definite negative: Skip this segment
            spdlog::debug("Bloom filter: Segment {} skipped (value not present)", 
                         column_index);
            return std::vector<size_t>();
        }
        
        // Possible positive: Must scan segment (might be false positive)
        spdlog::debug("Bloom filter: Segment {} may contain value (FP possible)", 
                     column_index);
    }
    
    // Fallback to zone map or full scan
    // ...
}
```

### Optimal Parameter Calculation

```cpp
// Calculate optimal Bloom filter parameters
struct BloomFilterParams {
    size_t num_bits;
    uint8_t num_hash_functions;
};

BloomFilterParams calculateOptimalParams(
    size_t expected_elements,
    double target_fp_rate
) {
    BloomFilterParams params;
    
    // Optimal number of bits: m = -n * ln(p) / (ln(2)^2)
    // where n = elements, p = false positive rate
    params.num_bits = static_cast<size_t>(
        -expected_elements * std::log(target_fp_rate) / 
        (std::log(2) * std::log(2))
    );
    
    // Optimal number of hash functions: k = (m/n) * ln(2)
    params.num_hash_functions = static_cast<uint8_t>(
        (params.num_bits / expected_elements) * std::log(2)
    );
    
    // Clamp to reasonable range
    params.num_hash_functions = std::max(uint8_t(1), 
                                        std::min(uint8_t(10), 
                                                params.num_hash_functions));
    
    return params;
}
```

---

## ✅ Acceptance Criteria

- [ ] Bloom filters correctly identify segment membership with <1% FP rate
- [ ] 10-50x speedup on high-cardinality point queries
- [ ] Memory overhead: <10 bits per element
- [ ] Automatic Bloom filter creation for columns with >1000 unique values
- [ ] Serialization/deserialization preserves filter integrity
- [ ] All existing columnar format tests still pass
- [ ] 10+ new tests for Bloom filter operations
- [ ] Documentation includes usage guidelines and tuning recommendations

---

## 📊 Performance Targets

| Metric | Target | Measurement |
|--------|--------|-------------|
| Membership test time | <1 μs | Single lookup |
| False positive rate | <1% | Configurable |
| Memory per element | <10 bits | Optimal hash functions |
| Build time | <500 ms | Per 1M elements |
| Segment skip rate | >95% | On negative queries |
| Query speedup | 10-50x | vs full segment scan |

---

## 🔗 Dependencies

- **Hash Functions:**
  - MurmurHash3 (fast, good distribution)
  - xxHash (ultra-fast, cryptographically weak)
  - Or: Use existing hash utilities in ThemisDB

- **ThemisDB Components:**
  - ColumnSegment (extended with Bloom filter)
  - ColumnarFormatManager (filtering logic)
  - Serialization framework

---

## 📚 References

- **Bloom Filter Paper:** "Space/Time Trade-offs in Hash Coding with Allowable Errors" (Bloom, 1970)
- **Bloom Filter Calculator:** https://hur.st/bloomfilter/
- **Implementation Guide:** "Bloom Filters by Example" (Tarkoma, 2012)
- **Columnar Storage Implementation:** `COLUMNAR_STORAGE_IMPLEMENTATION.md`
- **Parent PR:** #XXX (Columnar Storage Format Optimization)

---

## 🎯 Success Metrics

- [ ] 10-50x speedup demonstrated on real-world high-cardinality queries
- [ ] <1% false positive rate maintained across all workloads
- [ ] Zero performance regression on low-cardinality columns
- [ ] Production deployment with >1B row datasets
- [ ] Integration with ThemisDB query optimizer and execution engine
