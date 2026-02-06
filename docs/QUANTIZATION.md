# ThemisDB Quantization Architecture

**Version:** 1.5.0  
**Last Updated:** 2026-02-06  
**Status:** Complete FAISS Migration

---

## Overview

ThemisDB provides multiple vector quantization strategies for efficient storage and fast similarity search. As of v1.5.0, all quantizers support optional FAISS acceleration while maintaining standalone custom implementations for maximum flexibility.

### Quantization Methods

| Quantizer | Compression | Use Case | FAISS Support |
|-----------|-------------|----------|---------------|
| **ProductQuantizer** | 8-32x | General-purpose compression | ✅ Optional (K-means acceleration) |
| **BinaryQuantizer** | 32x | Maximum compression | ✅ Optional (binary operations) |
| **ResidualQuantizer** | 8-32x per stage | High-accuracy compression | ✅ Via ProductQuantizer |
| **AdvancedVectorIndex** | 10-100x | Production-scale search | ✅ Full FAISS integration |

---

## Architecture

### Conditional Compilation

All quantizers use conditional compilation to support both FAISS-accelerated and custom implementations:

```cpp
#ifdef THEMIS_HAS_FAISS
    // FAISS-accelerated path
    use_faiss_ = config.prefer_faiss;
#else
    // Custom implementation fallback
    use_faiss_ = false;
#endif
```

### Backend Selection

Users can control backend selection via configuration:

```cpp
// ProductQuantizer with FAISS acceleration (if available)
ProductQuantizer::Config config;
config.prefer_faiss = true;  // Default: true
ProductQuantizer pq(dimension, config);

// BinaryQuantizer with custom implementation
BinaryQuantizer::Config bq_config;
bq_config.prefer_faiss = false;  // Force custom backend
BinaryQuantizer bq(dimension, bq_config);

// Check which backend is being used
const char* backend = pq.getBackend();  // Returns "faiss" or "custom"
```

---

## Quantization Methods in Detail

### 1. ProductQuantizer

**Purpose:** General-purpose vector compression using product quantization

**Algorithm:**
- Divides vectors into M subvectors
- Runs K-means clustering on each subvector independently  
- Encodes vectors as M x 8-bit codes (256 centroids per subvector)

**FAISS Integration:**
- Uses FAISS K-means for faster training (20-30% speedup)
- SIMD-optimized distance computations
- Falls back to custom K-means if FAISS unavailable

**Performance:**
```
Training Time (100K vectors, 128D):
- FAISS:  5-8 seconds
- Custom: 8-12 seconds

Encoding Speed:
- Both:   ~1M vectors/second

Compression: 8-32x (configurable via num_subquantizers)
```

**Usage:**
```cpp
#include "index/product_quantizer.h"

ProductQuantizer::Config config;
config.num_subquantizers = 8;    // 128D / 8 = 16D subvectors
config.num_centroids = 256;       // 8-bit codes
config.prefer_faiss = true;       // Use FAISS if available

ProductQuantizer pq(128, config);

// Training
std::vector<std::vector<float>> training_data = /* ... */;
auto status = pq.train(training_data);

// Encoding
std::vector<float> vector = /* ... */;
std::vector<uint8_t> codes = pq.encode(vector);  // 8 bytes

// Decoding
std::vector<float> reconstructed = pq.decode(codes);

// Asymmetric distance (query vs codes)
float dist = pq.computeAsymmetricDistance(query, codes);
```

### 2. BinaryQuantizer

**Purpose:** Maximum compression using binary quantization

**Algorithm:**
- Centers each dimension by subtracting the mean
- Binarizes: bit = 1 if value ≥ 0, else 0
- Packs bits into uint8 bytes

**FAISS Integration:**
- Uses FAISS binary operations for Hamming distance
- Optimized bit manipulation with SIMD
- Falls back to custom bit counting if FAISS unavailable

**Performance:**
```
Training Time: < 1 second (simple statistics)
Encoding Speed: ~5M vectors/second
Compression: 32x (float32 → 1 bit per dimension)
```

**Usage:**
```cpp
#include "index/binary_quantizer.h"

BinaryQuantizer::Config config;
config.center_values = true;      // Subtract mean before binarization
config.normalize_input = false;
config.prefer_faiss = true;       // Use FAISS if available

BinaryQuantizer bq(128, config);

// Training
auto status = bq.train(training_data);

// Encoding
std::vector<uint8_t> codes = bq.encode(vector);  // 16 bytes (128 bits)

// Hamming distance
float hamming = bq.hammingDistance(codes_a, codes_b);

// Asymmetric distance
float dist = bq.asymmetricDistance(query, codes);
```

### 3. ResidualQuantizer

**Purpose:** High-accuracy compression using multi-stage refinement

**Algorithm:**
- Stage 1: Quantize vector with ProductQuantizer
- Stage 2: Quantize residual (error from stage 1)
- Stage 3: Quantize residual of residual, etc.
- Final encoding: concatenated codes from all stages

**FAISS Integration:**
- Each stage uses ProductQuantizer with optional FAISS acceleration
- Composition benefits from FAISS K-means speedup per stage
- 30% faster training with FAISS enabled

**Performance:**
```
Training Time (2 stages, 100K vectors):
- FAISS:  10-15 seconds
- Custom: 15-20 seconds

Accuracy: 97-99% recall@10 (vs 95-98% for single-stage PQ)
Compression: Configurable per stage (default 8x per stage)
```

**Usage:**
```cpp
#include "index/residual_quantizer.h"

ResidualQuantizer::Config config;
config.num_stages = 2;            // 2-stage refinement
config.num_subquantizers = 8;     // Per stage
config.num_centroids = 256;       // Per stage

ResidualQuantizer rq(128, config);

// Training
auto status = rq.train(training_data);

// Encoding
std::vector<uint8_t> codes = rq.encode(vector);  // 16 bytes (2 stages × 8 bytes)

// Decoding (sum of all stage reconstructions)
std::vector<float> reconstructed = rq.decode(codes);

// Asymmetric distance
float dist = rq.asymmetricDistance(query, codes);

// Inspect individual stages
const ProductQuantizer* stage0 = rq.getStageQuantizer(0);
const char* backend = stage0->getBackend();  // "faiss" or "custom"
```

### 4. AdvancedVectorIndex (Production-Scale)

**Purpose:** Full FAISS integration for production vector search

**Algorithm:**
- Uses FAISS IVF+PQ, IVF+Flat, or HNSW+Flat
- Integrated training, indexing, and search
- GPU acceleration support

**Performance:**
```
Index Size: 10-100x compression with IVF+PQ
Search Speed: 1-10ms for 1M vectors, k=10
GPU Acceleration: Yes (NVIDIA, AMD)
```

**Usage:**
```cpp
#include "index/advanced_vector_index.h"

AdvancedVectorIndex::Config config;
config.index_type = AdvancedVectorIndex::Config::Type::IVF_PQ;
config.nlist = 1024;              // Number of IVF clusters
config.nprobe = 64;               // Clusters to search
config.use_pq = true;             // Enable PQ compression
config.pq_m = 8;                  // PQ subquantizers
config.use_gpu = true;            // GPU acceleration

AdvancedVectorIndex index(128, config);

// Training
index.train(training_vectors, num_vectors);

// Adding vectors
index.add(vectors, num_vectors);

// Searching
auto results = index.search(query, k=10);
```

---

## Build Configuration

### Enabling FAISS Support

FAISS is **optional** and auto-detected by CMake:

```cmake
# cmake/Dependencies.cmake
if(THEMIS_ENABLE_CUDA)
    find_package(faiss QUIET)
    if(faiss_FOUND)
        message(STATUS "FAISS found - enabling GPU vector search")
        add_compile_definitions(THEMIS_HAS_FAISS=1)
    else()
        message(STATUS "FAISS not found - using custom implementations")
    endif()
endif()
```

### Build Options

**With FAISS:**
```bash
# Install FAISS
sudo apt-get install libfaiss-dev  # Ubuntu/Debian
# OR
vcpkg install faiss  # Windows/Cross-platform

# Build ThemisDB
cmake -B build -DTHEMIS_ENABLE_CUDA=ON
cmake --build build
```

**Without FAISS (Fallback):**
```bash
# Build without CUDA/FAISS
cmake -B build -DTHEMIS_ENABLE_CUDA=OFF
cmake --build build
```

### Runtime Behavior

| Scenario | THEMIS_HAS_FAISS | prefer_faiss | Backend Used |
|----------|------------------|--------------|--------------|
| FAISS installed, prefer enabled | ✅ Defined | true | FAISS |
| FAISS installed, prefer disabled | ✅ Defined | false | Custom |
| FAISS not installed | ❌ Undefined | true/false | Custom |

---

## Performance Comparison

### Training Speed (100K vectors, 128D)

| Quantizer | FAISS Backend | Custom Backend | Speedup |
|-----------|---------------|----------------|---------|
| ProductQuantizer | 5-8s | 8-12s | **25-30%** |
| BinaryQuantizer | <1s | <1s | ~0% (simple stats) |
| ResidualQuantizer (2-stage) | 10-15s | 15-20s | **30%** |

### Encoding Speed

| Quantizer | Speed | Notes |
|-----------|-------|-------|
| ProductQuantizer | ~1M vectors/s | Similar for both backends |
| BinaryQuantizer | ~5M vectors/s | Bit packing dominates |
| ResidualQuantizer | ~500K vectors/s | Multi-stage overhead |

### Memory Overhead

| Quantizer | FAISS Backend | Custom Backend |
|-----------|---------------|----------------|
| ProductQuantizer | +10% | Baseline |
| BinaryQuantizer | +5% | Baseline |

---

## Migration Guide

### From Custom to FAISS-Accelerated

**No code changes required!** Simply rebuild with FAISS available:

```cpp
// Same code works with both backends
ProductQuantizer::Config config;
config.prefer_faiss = true;  // Auto-detects FAISS
ProductQuantizer pq(dimension, config);

// Check which backend is active
if (strcmp(pq.getBackend(), "faiss") == 0) {
    THEMIS_INFO("Using FAISS-accelerated backend");
} else {
    THEMIS_INFO("Using custom fallback backend");
}
```

### From AdvancedVectorIndex to Standalone Quantizers

If you need standalone encode/decode (not integrated search):

```cpp
// Before: AdvancedVectorIndex (integrated search)
AdvancedVectorIndex index(dimension, config);
index.train(vectors, count);
index.add(vectors, count);
auto results = index.search(query, k);

// After: ProductQuantizer (standalone encode/decode)
ProductQuantizer pq(dimension, config);
pq.train(training_vectors);

// Manual encoding/storage
for (const auto& vec : vectors) {
    auto codes = pq.encode(vec);
    storage.insert(id, codes);  // Your custom storage
}

// Manual search
for (const auto& [id, codes] : storage) {
    float dist = pq.computeAsymmetricDistance(query, codes);
    candidates.push_back({id, dist});
}
std::sort(candidates.begin(), candidates.end(), 
         [](auto& a, auto& b) { return a.second < b.second; });
```

---

## Testing

### Unit Tests

All quantizers have comprehensive tests for both backends:

```bash
# Run quantization tests
./build/tests/test_product_quantizer
./build/tests/test_binary_quantizer
./build/tests/test_residual_quantizer

# Tests automatically run with both backends if FAISS is available
```

### Benchmarks

```bash
# Compare FAISS vs custom performance
./build/benchmarks/bench_product_quantization
./build/benchmarks/bench_binary_quantization
./build/benchmarks/bench_residual_quantization
```

---

## Future Work

### Planned Enhancements

1. **FAISS ProductQuantizer Direct Integration**
   - Expose standalone encode/decode from FAISS PQ
   - Full SIMD optimization for encoding (not just training)

2. **GPU Quantizers**
   - GPU-accelerated encoding/decoding
   - Batch processing for large-scale ingestion

3. **Advanced Quantization Methods**
   - Scalar Quantization (SQ4, SQ6, SQ8)
   - Learned Quantization (neural network-based)
   - Additive Quantization (AQ)

---

## References

### Papers

1. **Product Quantization**  
   Jégou, H., Douze, M., & Schmid, C. (2011). "Product Quantization for Nearest Neighbor Search." IEEE TPAMI.  
   DOI: 10.1109/TPAMI.2010.57

2. **Residual Quantization**  
   Chen, Y., Guan, T., & Wang, C. (2010). "Approximate Nearest Neighbor Search by Residual Vector Quantization." Sensors 10(12).  
   DOI: 10.3390/s101211259

3. **DiskANN**  
   Subramanya, S. J., et al. (2019). "DiskANN: Fast Accurate Billion-point Nearest Neighbor Search on a Single Node." NeurIPS.

### Libraries

- **FAISS**: https://github.com/facebookresearch/faiss
- **License**: MIT
- **Documentation**: https://faiss.ai/

---

## Changelog

### v1.5.0 (2026-02-06) - Complete FAISS Migration

- ✅ Added conditional FAISS support to ProductQuantizer
- ✅ Added conditional FAISS support to BinaryQuantizer  
- ✅ Updated ResidualQuantizer to use FAISS-accelerated ProductQuantizer
- ✅ Added `prefer_faiss` configuration option
- ✅ Added `getBackend()` method to report active backend
- ✅ Documented architecture and migration paths
- ✅ Maintained backward compatibility

### v1.4.1 (2024-12-10)

- Simplified BinaryQuantizer implementation
- Marked BinaryQuantizer as deprecated for production use
- Recommended FAISS IndexBinaryFlat for production

### v1.3.0 (2024-06-15)

- Initial custom quantization implementations
- ProductQuantizer, BinaryQuantizer, ResidualQuantizer

---

**For more information:**
- See [PERFORMANCE.md](PERFORMANCE.md) for benchmarks
- See [INSTALL.md](INSTALL.md) for build instructions
- See [CHANGELOG.md](../CHANGELOG.md) for release notes
