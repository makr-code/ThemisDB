# Vector Encryption - Performance Optimization Notes

**Date:** December 15, 2025  
**Status:** Future Enhancements  
**Priority:** P2 (Optional)  
**Version:** v1.3.0  
**Kategorie:** 🔒 Security

---

## 📑 Table of Contents

- [Overview](#overview)
- [Phase 2 (HNSW Index Encryption) Optimizations](#phase-2-hnsw-index-encryption-optimizations)

---

## Overview

This document tracks potential performance optimizations for the vector encryption implementation. These are not critical for Phase 1-2 functionality but could improve performance for large-scale deployments.

---

## Phase 2 (HNSW Index Encryption) Optimizations

### 1. Memory Copies in Binary Serialization

**Issue:** Converting between `std::vector<uint8_t>` and `std::string` creates unnecessary copies for large HNSW indexes (multi-GB).

**Current Implementation:**
```cpp
// serialize: vector<uint8_t> → string (copy)
std::string serialize(const std::vector<uint8_t>& value) {
    return std::string(value.begin(), value.end());
}

// deserialize: string → vector<uint8_t> (copy)
std::vector<uint8_t> deserialize(const std::string& str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}
```

**Memory Impact:**
- 3 GB HNSW index
- 2 copies: 6 GB peak memory usage
- Additional overhead during encryption/decryption

**Optimization Options:**

#### Option A: Move Semantics
```cpp
template<>
std::string EncryptedField<std::vector<uint8_t>>::serialize(std::vector<uint8_t>&& value) {
    // Move data instead of copy
    return std::string(
        std::make_move_iterator(value.begin()), 
        std::make_move_iterator(value.end())
    );
}
```

**Pros:** Reduces copies  
**Cons:** Requires API changes (rvalue references)

#### Option B: String Views
```cpp
// Use string_view to avoid intermediate copies
std::string_view serializeView(const std::vector<uint8_t>& value) {
    return std::string_view(
        reinterpret_cast<const char*>(value.data()), 
        value.size()
    );
}
```

**Pros:** Zero-copy  
**Cons:** Lifetime management complexity

#### Option C: Memory-Mapped Files
```cpp
// Encrypt directly from/to memory-mapped files
class MemoryMappedEncryption {
    void encryptFile(const std::string& input, const std::string& output);
    void decryptFile(const std::string& input, const std::string& output);
};
```

**Pros:** No in-memory copies, handles huge files  
**Cons:** Platform-specific, more complex

**Recommendation:** Option C for large indexes (>1 GB)

**Estimated Impact:**
- Memory reduction: 50% (6 GB → 3 GB peak)
- Performance: 10-20% faster for large indexes

---

### 2. File Reading Efficiency

**Issue:** Using `istreambuf_iterator` for multi-GB files is inefficient.

**Current Implementation:**
```cpp
std::ifstream file(path, std::ios::binary);
std::vector<uint8_t> data(
    (std::istreambuf_iterator<char>(file)),
    std::istreambuf_iterator<char>()
);
```

**Problems:**
- Character-by-character iteration
- No buffering optimization
- Slow for large files

**Optimization:**

#### Option A: Chunked Reading
```cpp
std::vector<uint8_t> readFileChunked(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Pre-allocate
    std::vector<uint8_t> data(size);
    
    // Read in chunks
    const size_t chunk_size = 64 * 1024 * 1024; // 64 MB chunks
    for (size_t i = 0; i < size; i += chunk_size) {
        size_t read_size = std::min(chunk_size, size - i);
        file.read(reinterpret_cast<char*>(data.data() + i), read_size);
    }
    
    return data;
}
```

**Pros:** Much faster for large files  
**Cons:** Slightly more complex

**Estimated Impact:**
- 3 GB file: 20 seconds → 5 seconds (4x faster)

#### Option B: Memory-Mapped I/O
```cpp
#include <sys/mman.h>

class MMapFile {
    void* map(const std::string& path, size_t& size);
    void unmap(void* addr, size_t size);
};
```

**Pros:** Fastest possible, OS-optimized  
**Cons:** Platform-specific (Linux/Windows differ)

**Recommendation:** Option A for cross-platform, Option B for maximum performance

---

### 3. Encryption/Decryption Parallelization

**Issue:** Large HNSW indexes encrypted/decrypted sequentially.

**Current Implementation:**
```cpp
// Single-threaded encryption
EncryptedField<std::vector<uint8_t>> encField;
encField.encrypt(indexData, "hnsw_index");  // 3 GB takes 3 seconds
```

**Optimization: Chunk-Based Parallel Encryption**

```cpp
class ParallelEncryption {
    std::vector<EncryptedChunk> encryptParallel(
        const std::vector<uint8_t>& data,
        const std::string& key_id,
        size_t num_threads = 8
    ) {
        const size_t chunk_size = 256 * 1024 * 1024; // 256 MB chunks
        std::vector<EncryptedChunk> chunks;
        
        #pragma omp parallel for
        for (size_t i = 0; i < data.size(); i += chunk_size) {
            size_t len = std::min(chunk_size, data.size() - i);
            
            std::vector<uint8_t> chunk(data.begin() + i, data.begin() + i + len);
            
            EncryptedField<std::vector<uint8_t>> encField;
            encField.encrypt(chunk, key_id);
            
            #pragma omp critical
            chunks.push_back({i, encField});
        }
        
        return chunks;
    }
};
```

**Estimated Impact:**
- 3 GB encryption: 3 seconds → 0.5 seconds (8 cores)
- Near-linear scaling with core count

**Challenges:**
- Need to store chunk metadata
- Reconstruction on decryption
- Slightly more complex format

---

### 4. Compression Before Encryption

**Issue:** HNSW indexes have some redundancy that could be compressed.

**Optimization:**
```cpp
class CompressedEncryption {
    Status saveCompressedEncrypted(const std::string& directory) {
        // 1. Save HNSW index
        appr->saveIndex(tempPath);
        
        // 2. Load into memory
        auto data = readFile(tempPath);
        
        // 3. Compress (e.g., zstd, lz4)
        auto compressed = zstd::compress(data, level=3);
        
        // 4. Encrypt compressed data
        EncryptedField<std::vector<uint8_t>> encField;
        encField.encrypt(compressed, "hnsw_index");
        
        // 5. Save
        writeFile(encryptedPath, encField.toBase64());
    }
};
```

**Estimated Impact:**
- Compression ratio: 0.7-0.9 (depends on HNSW structure)
- Storage savings: 10-30%
- CPU overhead: +10-20% (compression time)

**Trade-off:** CPU time for storage space

---

## Phase 1 (Vector Encryption) Optimizations

### 1. Batch Decryption During Index Rebuild

**Issue:** Vectors decrypted one-by-one during `rebuildFromStorage()`.

**Current Implementation:**
```cpp
for (auto& entity : entities) {
    auto enc = EncryptedField<std::vector<float>>::fromBase64(entity.encryptedField);
    auto vector = enc.decrypt();  // Sequential
    cache[pk] = vector;
}
```

**Optimization: Parallel Batch Decryption**

```cpp
// Collect all encrypted fields
std::vector<EncryptedField<std::vector<float>>> encrypted_batch;
for (auto& entity : entities) {
    encrypted_batch.push_back(
        EncryptedField<std::vector<float>>::fromBase64(entity.encryptedField)
    );
}

// Parallel decryption
std::vector<std::vector<float>> decrypted_batch(encrypted_batch.size());

#pragma omp parallel for
for (size_t i = 0; i < encrypted_batch.size(); ++i) {
    decrypted_batch[i] = encrypted_batch[i].decrypt();
}

// Cache results
for (size_t i = 0; i < entities.size(); ++i) {
    cache[entities[i].pk] = decrypted_batch[i];
}
```

**Estimated Impact:**
- 1M vectors: 16 minutes → 2 minutes (8 cores, 8x speedup)

---

## Implementation Priority

| Optimization | Phase | Priority | Effort | Impact |
|--------------|-------|----------|--------|--------|
| **Chunked file reading** | 2 | P1 | Low | High |
| **Memory-mapped files** | 2 | P2 | Medium | High |
| **Parallel batch decrypt** | 1 | P2 | Medium | High |
| **Move semantics** | 2 | P2 | Low | Medium |
| **Compression** | 2 | P3 | Medium | Medium |
| **Parallel encryption** | 2 | P3 | High | Medium |

---

## Recommendations

### Short-term (Next Sprint)
1. Implement chunked file reading (easy win)
2. Add batch decryption for index rebuild

### Medium-term (1-2 months)
3. Memory-mapped I/O for HNSW indexes
4. Compression before encryption (optional)

### Long-term (3+ months)
5. Parallel chunk-based encryption
6. Hardware acceleration (AES-NI already used by OpenSSL)

---

## Benchmarking Plan

Before optimizations:
1. Measure baseline: 1M vectors, 3 GB HNSW index
2. Profile bottlenecks: CPU, memory, I/O

After each optimization:
1. Re-measure performance
2. Verify correctness
3. Document improvement

---

## References

- EncryptedField implementation: `src/security/encrypted_field.cpp`
- HNSW save/load: `src/index/vector_index.cpp`
- Performance analysis: `PHASE2_IMPLEMENTATION_REPORT.md`

---

**Status:** Documentation complete  
**Implementation:** Future work  
**Priority:** P2-P3 (optional performance enhancements)
