---
name: 🗜️ Columnar Storage: LZ4/Snappy Compression Integration
about: Integrate LZ4 and Snappy compression codecs for general-purpose columnar compression
title: "[COLUMNAR] LZ4/Snappy Compression Integration"
labels: priority:P2, type:enhancement, area:storage, effort:small, component:columnar-format
assignees: ''
---

## 🗜️ Columnar Storage Enhancement - Phase 2

**Current Status:** RLE, Dictionary, Bit-Packing, and Frame-of-Reference codecs implemented  
**Priority:** P2 (Medium)  
**Effort:** 1-2 weeks  
**Target Version:** v1.4.2  
**Parent PR:** #XXX (Columnar Storage Format Optimization)  
**Related Files:**
- `include/storage/columnar_format.h`
- `src/storage/columnar_format.cpp`
- `tests/test_columnar_format.cpp`

---

## 📋 Problem Description

The columnar storage format currently supports 4 specialized compression codecs:
- **RLE**: Run-Length Encoding (2-10x compression)
- **Dictionary**: Categorical data encoding (5-20x compression)
- **Bit-Packing**: Variable-width numeric encoding (2-4x compression)
- **Frame-of-Reference**: Delta encoding (2-5x compression)

However, general-purpose compression codecs **LZ4 and Snappy** are currently stubbed out with TODO comments. These codecs are important for:
- **Float/Double columns** where specialized codecs don't work well
- **String columns** with high cardinality (>30% unique values)
- **Mixed data patterns** that don't fit specialized codec profiles
- **Fallback compression** when other codecs underperform

**Performance Impact:** Missing **10-30% additional compression** on mixed workloads and float-heavy data.

---

## 🎯 Requirements

### Must Have (P2)

- [ ] **LZ4 Integration**
  - Add vcpkg dependency: `vcpkg install lz4`
  - Implement `GenericCompressionCodec::compressLZ4()`
  - Implement `GenericCompressionCodec::decompressLZ4()`
  - Support compression levels (fast, default, high)
  - Error handling for allocation failures

- [ ] **Snappy Integration**
  - Add vcpkg dependency: `vcpkg install snappy`
  - Implement `GenericCompressionCodec::compressSnappy()`
  - Implement `GenericCompressionCodec::decompressSnappy()`
  - Handle Snappy framing format
  - Error handling for invalid data

- [ ] **Codec Selection Logic**
  - Update `ColumnSegment::selectOptimalCodec()` to include LZ4/Snappy
  - Add heuristics for when to use general-purpose compression:
    - Float/Double columns → LZ4 (fast)
    - High-cardinality strings → Snappy (better ratio)
    - Fallback for underperforming specialized codecs
  - Benchmark-based codec selection

- [ ] **Build System Updates**
  - Add LZ4 and Snappy to CMakeLists.txt dependencies
  - Add feature flags: `THEMIS_ENABLE_LZ4`, `THEMIS_ENABLE_SNAPPY`
  - Conditional compilation support
  - vcpkg.json manifest updates

### Should Have (P2)

- [ ] **Compression Level Configuration**
  - Per-column codec configuration API
  - Runtime codec selection based on data sampling
  - Compression ratio monitoring and auto-adjustment

- [ ] **Performance Optimization**
  - Zero-copy decompression where possible
  - Memory pooling for compression buffers
  - Parallel compression for large segments

- [ ] **Testing**
  - Unit tests for LZ4 encode/decode roundtrips
  - Unit tests for Snappy encode/decode roundtrips
  - Compression ratio benchmarks vs specialized codecs
  - Error handling tests (truncated data, invalid headers)
  - Large dataset scalability tests (>1GB columns)

### Could Have (P3)

- [ ] **Advanced Features**
  - Streaming compression for very large columns
  - Dictionary pre-compression before LZ4/Snappy
  - Hybrid codec chains (e.g., RLE + LZ4)
  - Compression statistics and telemetry

---

## 📐 Technical Design

### Implementation Approach

```cpp
// Current stub (to be replaced)
Result<std::vector<uint8_t>> GenericCompressionCodec::compressLZ4(
    const std::vector<uint8_t>& data) {
    return tl::unexpected(Error(
        errors::ErrorCode::ERR_COMPRESSION_FAILED,
        "LZ4 compression not yet implemented - requires lz4 library"
    ));
}

// New implementation
Result<std::vector<uint8_t>> GenericCompressionCodec::compressLZ4(
    const std::vector<uint8_t>& data, int level = 0) {
    if (data.empty()) {
        return std::vector<uint8_t>();
    }
    
    // Calculate max compressed size
    int max_size = LZ4_compressBound(data.size());
    std::vector<uint8_t> compressed(max_size + 8); // +8 for header
    
    // Write header: uncompressed size
    uint64_t orig_size = data.size();
    std::memcpy(compressed.data(), &orig_size, sizeof(uint64_t));
    
    // Compress
    int compressed_size = LZ4_compress_default(
        reinterpret_cast<const char*>(data.data()),
        reinterpret_cast<char*>(compressed.data() + 8),
        data.size(),
        max_size
    );
    
    if (compressed_size <= 0) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_FAILED,
            "LZ4 compression failed"
        ));
    }
    
    compressed.resize(compressed_size + 8);
    return compressed;
}
```

### Codec Selection Logic

```cpp
CompressionCodec ColumnSegment::selectOptimalCodec(
    ColumnType type, const void* data, size_t row_count) {
    
    switch (type) {
        case ColumnType::FLOAT32:
        case ColumnType::FLOAT64:
            // Floats don't compress well with specialized codecs
            return CompressionCodec::LZ4;
            
        case ColumnType::STRING: {
            // Sample strings for cardinality analysis
            auto sample = sampleStrings(data, row_count, 1000);
            if (DictionaryCodec::shouldUseDictionary(sample)) {
                return CompressionCodec::DICTIONARY;
            }
            // High cardinality → Snappy
            return CompressionCodec::SNAPPY;
        }
            
        case ColumnType::INT32:
        case ColumnType::INT64:
            // Try RLE first, fallback to LZ4
            return CompressionCodec::RLE;
            
        default:
            return CompressionCodec::NONE;
    }
}
```

---

## ✅ Acceptance Criteria

- [ ] LZ4 compression/decompression working for all column types
- [ ] Snappy compression/decompression working for all column types
- [ ] Automatic codec selection includes LZ4/Snappy in decision tree
- [ ] Unit tests achieve 100% coverage for new codec paths
- [ ] Compression ratios meet or exceed baseline:
  - Float columns: ≥2x compression with LZ4
  - High-cardinality strings: ≥3x compression with Snappy
- [ ] No memory leaks detected in compression/decompression paths
- [ ] Build succeeds on all platforms (Linux, Windows, macOS)
- [ ] Documentation updated with codec selection guidelines

---

## 📊 Performance Targets

| Metric | Target | Measurement |
|--------|--------|-------------|
| LZ4 compression speed | >500 MB/s | Benchmark on 1GB float column |
| Snappy compression speed | >300 MB/s | Benchmark on 1GB string column |
| LZ4 decompression speed | >2 GB/s | Benchmark on compressed data |
| Snappy decompression speed | >1 GB/s | Benchmark on compressed data |
| Float column compression | ≥2x | Ratio on real-world analytics data |
| String column compression | ≥3x | Ratio on UUID/categorical strings |

---

## 🔗 Dependencies

- **External Libraries:**
  - LZ4 library (via vcpkg)
  - Snappy library (via vcpkg)

- **ThemisDB Components:**
  - ErrorRegistry (for compression error codes)
  - ColumnSegment (codec selection)
  - Test infrastructure (GTest)

---

## 📚 References

- **LZ4 Documentation:** https://github.com/lz4/lz4
- **Snappy Documentation:** https://github.com/google/snappy
- **Columnar Storage Implementation:** `COLUMNAR_STORAGE_IMPLEMENTATION.md`
- **Parent PR:** #XXX (Columnar Storage Format Optimization)

---

## 🎯 Success Metrics

- [ ] All 40+ existing columnar format tests still pass
- [ ] 20+ new tests added for LZ4/Snappy codecs
- [ ] Zero performance regression on existing RLE/Dictionary/Bit-Packing
- [ ] 10-30% additional compression on mixed workloads
- [ ] Production-ready error handling and validation
