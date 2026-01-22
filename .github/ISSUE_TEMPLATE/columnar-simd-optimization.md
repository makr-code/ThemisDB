---
name: ⚡ Columnar Storage: SIMD-Optimized Codec Implementation
about: Implement SIMD vectorization for encode/decode operations to improve compression throughput
title: "[COLUMNAR] SIMD-Optimized Codec Implementation"
labels: priority:P3, type:performance, area:storage, effort:large, component:columnar-format
assignees: ''
---

## ⚡ Columnar Storage Enhancement - Phase 4

**Current Status:** Scalar codec implementations with good compression ratios  
**Priority:** P3 (Nice to Have)  
**Effort:** 4-5 weeks  
**Target Version:** v1.6.0  
**Parent PR:** #XXX (Columnar Storage Format Optimization)  
**Related Files:**
- `src/storage/columnar_format.cpp`
- `include/storage/columnar_format.h`
- `src/storage/columnar_format_simd.cpp` (new)

---

## 📋 Problem Description

Current columnar compression codecs use **scalar operations** that process one value at a time:

```cpp
// Current scalar RLE encoding
for (size_t i = 0; i < data.size(); ++i) {
    int32_t value = data[i];
    size_t run_length = 1;
    while (i + run_length < data.size() && 
           data[i + run_length] == value) {
        run_length++;
    }
    // Write run...
}
```

**Performance Limitations:**
- Single-threaded processing
- No CPU vector instruction utilization
- Limited by memory bandwidth, not compute
- Encode/decode throughput: ~500 MB/s

**SIMD Potential:**
- Process 4-16 values per instruction (SSE/AVX2/AVX-512)
- Parallel compression/decompression
- Throughput improvement: **4-10x faster**
- Better cache utilization

---

## 🎯 Requirements

### Must Have (P3)

- [ ] **SIMD RLE Encoding**
  - AVX2 implementation for INT32/INT64
  - Vectorized equality comparison (16 values at once)
  - Parallel run-length counting
  - Fallback to scalar for remainder elements

- [ ] **SIMD Bit-Packing**
  - AVX2 byte shuffle for variable-width packing
  - Parallel normalization (subtract min value)
  - Vectorized bit shifting and masking
  - Efficient handling of 8/16/32/64-bit widths

- [ ] **SIMD Dictionary Encoding**
  - Hash table lookups with SIMD
  - Parallel string comparison (for small strings)
  - Vectorized index generation
  - SIMD bitmap for seen values

- [ ] **Runtime CPU Detection**
  - Auto-detect AVX2/SSE4.2 support at runtime
  - Fallback to scalar codecs on older CPUs
  - Compile-time feature flags for portability
  - Performance logging for SIMD utilization

### Should Have (P3)

- [ ] **AVX-512 Support**
  - 512-bit vector operations (32 INT16 at once)
  - Mask registers for conditional operations
  - Scatter/gather for non-contiguous data

- [ ] **ARM NEON Support**
  - SIMD for ARM64 platforms (mobile, Apple Silicon)
  - 128-bit vector operations
  - Fallback to scalar on older ARM

- [ ] **Benchmarking**
  - Encode/decode throughput benchmarks
  - CPU instruction profiling (perf, VTune)
  - Comparison: Scalar vs SSE vs AVX2 vs AVX-512
  - Real-world workload validation

### Could Have (P3)

- [ ] **Advanced Optimizations**
  - Cache-line alignment for vector loads
  - Prefetching for streaming data
  - Multi-threaded batch encoding
  - GPU offload for very large columns (CUDA/OpenCL)

---

## 📐 Technical Design

### SIMD RLE Encoding (AVX2)

```cpp
#ifdef __AVX2__
#include <immintrin.h>

Result<std::vector<uint8_t>> RLECodec::encodeInt32SIMD(
    const std::vector<int32_t>& data) {
    
    if (data.empty()) {
        return std::vector<uint8_t>();
    }
    
    std::vector<uint8_t> encoded;
    encoded.reserve(data.size() * sizeof(int32_t) / 2);
    
    size_t i = 0;
    
    // Process 8 INT32 at a time with AVX2
    while (i + 8 <= data.size()) {
        // Load 8 INT32 values into AVX2 register
        __m256i values = _mm256_loadu_si256(
            reinterpret_cast<const __m256i*>(&data[i])
        );
        
        // Broadcast first value for comparison
        __m256i first = _mm256_set1_epi32(data[i]);
        
        // Compare all 8 values with first
        __m256i cmp = _mm256_cmpeq_epi32(values, first);
        
        // Count consecutive equal values
        int mask = _mm256_movemask_epi8(cmp);
        int run_length = __builtin_ctz(~mask) / 4; // Count trailing set bits
        
        if (run_length == 0) run_length = 1;
        
        // Continue counting with scalar for runs > 8
        while (i + run_length < data.size() && 
               data[i + run_length] == data[i] && 
               run_length < 255) {
            run_length++;
        }
        
        // Write run
        encoded.push_back(static_cast<uint8_t>(run_length));
        const uint8_t* value_bytes = reinterpret_cast<const uint8_t*>(&data[i]);
        encoded.insert(encoded.end(), value_bytes, value_bytes + sizeof(int32_t));
        
        i += run_length;
    }
    
    // Process remaining elements with scalar
    while (i < data.size()) {
        // Scalar RLE for remainder...
    }
    
    return encoded;
}
#endif // __AVX2__
```

### Runtime CPU Detection

```cpp
// New file: src/storage/columnar_format_simd.cpp
namespace themis {
namespace storage {

enum class SIMDLevel {
    NONE,
    SSE42,
    AVX2,
    AVX512
};

class SIMDDetector {
public:
    static SIMDLevel detectBestSIMD() {
        // Use CPUID to detect CPU features
        #ifdef __AVX512F__
        if (hasAVX512()) return SIMDLevel::AVX512;
        #endif
        
        #ifdef __AVX2__
        if (hasAVX2()) return SIMDLevel::AVX2;
        #endif
        
        #ifdef __SSE4_2__
        if (hasSSE42()) return SIMDLevel::SSE42;
        #endif
        
        return SIMDLevel::NONE;
    }
    
private:
    static bool hasAVX2();
    static bool hasAVX512();
    static bool hasSSE42();
};

// Dispatch to best available codec
Result<std::vector<uint8_t>> RLECodec::encodeInt32(
    const std::vector<int32_t>& data) {
    
    static SIMDLevel level = SIMDDetector::detectBestSIMD();
    
    switch (level) {
        #ifdef __AVX2__
        case SIMDLevel::AVX2:
            return encodeInt32SIMD_AVX2(data);
        #endif
        
        #ifdef __SSE4_2__
        case SIMDLevel::SSE42:
            return encodeInt32SIMD_SSE42(data);
        #endif
        
        default:
            return encodeInt32Scalar(data); // Fallback
    }
}

} // namespace storage
} // namespace themis
```

---

## ✅ Acceptance Criteria

- [ ] SIMD codecs achieve 4-10x throughput improvement over scalar
- [ ] Automatic CPU feature detection works on all platforms
- [ ] Fallback to scalar codecs on CPUs without SIMD support
- [ ] Bit-identical output between SIMD and scalar implementations
- [ ] All existing columnar format tests pass with SIMD enabled
- [ ] Benchmarks demonstrate throughput improvements
- [ ] No performance regression on non-SIMD CPUs
- [ ] Cross-platform builds succeed (Linux, Windows, macOS, ARM)

---

## 📊 Performance Targets

| Operation | Scalar | AVX2 | Speedup |
|-----------|--------|------|---------|
| RLE encode INT32 | 500 MB/s | 3 GB/s | 6x |
| RLE decode INT32 | 800 MB/s | 4 GB/s | 5x |
| Bit-pack encode | 300 MB/s | 2 GB/s | 6.6x |
| Bit-pack decode | 600 MB/s | 3.5 GB/s | 5.8x |
| Dictionary encode | 200 MB/s | 1.2 GB/s | 6x |

---

## 🔗 Dependencies

- **Compiler Support:**
  - GCC 7+ with `-mavx2` flag
  - Clang 5+ with `-mavx2` flag
  - MSVC 2019+ with `/arch:AVX2`

- **ThemisDB Components:**
  - Existing columnar codec infrastructure
  - CMake build system for SIMD flags
  - Benchmark framework for performance validation

---

## 📚 References

- **Intel Intrinsics Guide:** https://www.intel.com/content/www/us/en/docs/intrinsics-guide/
- **AVX2 Tutorial:** https://www.officedaytime.com/simd512e/
- **SIMD Compression Paper:** "SIMD Compression and the Intersection of Sorted Integers" (Lemire et al.)
- **Columnar Storage Implementation:** `COLUMNAR_STORAGE_IMPLEMENTATION.md`

---

## 🎯 Success Metrics

- [ ] 4-10x encode/decode throughput improvement demonstrated
- [ ] Production deployment with SIMD-accelerated codecs
- [ ] Zero correctness issues (bit-identical output)
- [ ] Positive impact on OLAP query latency (lower decompression time)
- [ ] Documentation includes SIMD usage guidelines
