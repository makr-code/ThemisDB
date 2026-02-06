# GPU Support Roadmap - User Migration Guide

## Overview

This document provides guidance for users who need GPU acceleration for vector indexing in ThemisDB.

## TL;DR - Quick Facts

- ❌ **GPU vector indexing is NOT available in v1.5.x**
- ✅ **CPU-optimized implementation is available and fast**
- 🚀 **GPU support planned for v2.x (Q3 2026+)**
- 🔧 **Current performance: 30K+ QPS on CPU**
- 📈 **Target GPU performance: 250K+ QPS (v2.1)**

## Current State (v1.5.0+)

### What Changed

In v1.5.0, we removed incomplete GPU backend stubs:
- Removed: `gpu_vector_index_cuda.cpp` (384 lines, 3 TODOs)
- Removed: `gpu_vector_index_vulkan.cpp` (385 lines, 6 TODOs)
- Removed: `gpu_vector_index_hip.cpp` (419 lines, 4 TODOs)
- Removed: GPU-specific CMake configuration
- Removed: GPU backend classes from public API

**Total cleanup**: 1500+ LOC of non-functional code

### Why This Happened

The GPU backends were exploration/research code that never reached production quality. Rather than mislead users about capabilities, we removed the stubs and are planning proper GPU support for v2.x.

## Current CPU Performance

Don't underestimate the CPU implementation! It's optimized and fast:

### Performance Characteristics

| Operation | Performance | Notes |
|-----------|------------|-------|
| Single Query | 0.5 ms | Good for real-time |
| Batch (64) | 20 ms | Parallel execution |
| Batch (512) | 150 ms | High throughput |
| Throughput | 30K QPS | Multi-threaded |
| Index Build | 60 sec (1M vectors) | One-time cost |

### CPU Optimizations Enabled

- ✅ **SIMD Vectorization**: AVX-512, AVX2, NEON
- ✅ **Multi-threading**: Parallel batch processing
- ✅ **Cache-Friendly Layout**: Optimized memory access
- ✅ **NUMA-Aware Allocation**: For multi-socket systems
- ✅ **Branch Prediction Hints**: Optimized hot paths

## Migration Guide

### If Your Code References GPU Backends

**Before (v1.4.x - Don't do this):**
```cpp
#include "index/gpu_vector_index.h"

GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::VULKAN;  // ❌ Not available anymore
config.backend = GPUVectorIndex::Backend::CUDA;    // ❌ Not available anymore
config.backend = GPUVectorIndex::Backend::HIP;     // ❌ Not available anymore
```

**After (v1.5.x - Do this):**
```cpp
#include "index/gpu_vector_index.h"

GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::CPU;   // ✅ Available
config.backend = GPUVectorIndex::Backend::AUTO;  // ✅ Falls back to CPU
```

### Build Configuration Changes

**Before (v1.4.x):**
```cmake
option(THEMIS_ENABLE_VULKAN "Enable Vulkan GPU backend" OFF)
option(THEMIS_ENABLE_CUDA "Enable CUDA GPU backend" OFF)
option(THEMIS_ENABLE_HIP "Enable HIP GPU backend" OFF)
```

**After (v1.5.x):**
These options still exist for other GPU features (LoRA training, RoPE embeddings), but they **do not** enable GPU vector indexing.

### Code That Still Works

The following code continues to work without changes:

```cpp
// Initialize CPU-based vector index
GPUVectorIndex index;
index.initialize(128);  // 128-dimensional vectors

// Add vectors
index.addVector("vec1", vector1);
index.addVectorBatch(ids, vectors);

// Search
auto results = index.search(query, 10);  // Top 10 results

// Statistics
auto stats = index.getStatistics();
assert(stats.activeBackend == GPUVectorIndex::Backend::CPU);
```

## Alternatives for GPU Acceleration

If you need GPU acceleration now, consider these alternatives:

### Option 1: FAISS GPU (Recommended)

ThemisDB already integrates FAISS, which has GPU support:

```cpp
// FAISS GPU is available in ThemisDB
#include "acceleration/faiss_gpu_backend.h"

// Use FAISS for GPU-accelerated vector search
// Documentation: docs/FAISS_MIGRATION_COMPLETE.md
```

**Pros**:
- ✅ Production-ready GPU implementation
- ✅ Already integrated in ThemisDB
- ✅ Supports CUDA (NVIDIA GPUs)
- ✅ Well-tested and maintained

**Cons**:
- ❌ NVIDIA-only (no AMD/Intel GPU support)
- ❌ Different API than GPUVectorIndex

### Option 2: External Vector Databases

Use a dedicated vector database with GPU support:

**Milvus**:
- GPU support: Yes (CUDA)
- Performance: Excellent
- Deployment: Docker, Kubernetes
- Integration: Client/server via SDK

**Weaviate**:
- GPU support: Via external vectorizers
- Performance: Good
- Deployment: Docker, Kubernetes
- Integration: REST API

**Qdrant**:
- GPU support: Planned
- Performance: Good (CPU-optimized)
- Deployment: Docker, binary
- Integration: gRPC, REST API

### Option 3: Wait for ThemisDB v2.x

If you can wait 6-12 months, v2.x will have proper GPU support.

**Benefits of waiting**:
- ✅ Native integration with ThemisDB
- ✅ Unified API (no separate tools)
- ✅ Multi-backend support (CUDA, Vulkan, HIP)
- ✅ Automatic CPU fallback
- ✅ Production-quality implementation

## Optimization Tips (CPU Version)

Maximize CPU performance while waiting for GPU support:

### 1. Batch Your Queries

**Don't do this:**
```cpp
for (const auto& query : queries) {
    auto results = index.search(query, k);  // Slow: serial execution
}
```

**Do this:**
```cpp
auto results = index.searchBatch(queries, k);  // Fast: parallel execution
```

**Impact**: 5-10x speedup for large batches

### 2. Use Appropriate Vector Dimensions

Lower dimensions = faster search:

| Dimension | Search Time | Notes |
|-----------|------------|-------|
| 64 | 0.2 ms | Very fast |
| 128 | 0.5 ms | Fast (recommended) |
| 256 | 1.2 ms | Medium |
| 512 | 2.8 ms | Slow |
| 1024 | 6.0 ms | Very slow |

**Tip**: Use dimensionality reduction (PCA, UMAP) if possible.

### 3. Tune HNSW Parameters

Adjust for your performance/accuracy tradeoff:

```cpp
GPUVectorIndex::Config config;
config.M = 16;              // Default: good balance
config.efConstruction = 200; // Higher = better accuracy, slower build
config.efSearch = 64;        // Higher = better accuracy, slower search
```

**Quick tuning guide**:
- **Low latency**: `M=8, efSearch=32`
- **Balanced**: `M=16, efSearch=64` (default)
- **High accuracy**: `M=32, efSearch=128`

### 4. Enable SIMD Optimizations

Ensure your build has SIMD enabled:

```bash
# Check if AVX-512 is enabled
lscpu | grep avx512

# Build with SIMD support
cmake -DCMAKE_CXX_FLAGS="-mavx2 -mfma" ..
```

### 5. Use Multi-threading Effectively

```cpp
// Set thread count for parallel operations
// Default: uses all available cores
index.setBatchSize(512);  // Larger batches = better parallelization
```

## Future API (v2.x Preview)

This is what the GPU API will look like in v2.x:

```cpp
#include "index/gpu_vector_index.h"

// Configure GPU backend
GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::CUDA;  // NVIDIA GPU
config.deviceId = 0;                             // First GPU
config.useMixedPrecision = true;                 // FP16/TF32 acceleration
config.maxVRAM_MB = 8192;                        // 8 GB limit
config.allowCPUFallback = true;                  // Fallback if GPU fails

// Initialize
GPUVectorIndex index(config);
if (!index.initialize(128)) {
    // GPU initialization failed, check getActiveBackend()
    auto backend = index.getActiveBackend();
    if (backend == GPUVectorIndex::Backend::CPU) {
        std::cout << "Fell back to CPU\n";
    }
}

// Use as normal (API unchanged)
index.addVector("vec1", vector1);
auto results = index.search(query, 10);

// Check GPU statistics
auto stats = index.getStatistics();
std::cout << "VRAM usage: " << (stats.vramUsageBytes / 1024 / 1024) << " MB\n";
std::cout << "GPU active: " << stats.isGPUActive << "\n";
```

**No breaking changes**: Your current CPU code will continue to work.

## Frequently Asked Questions

### Q: Will v1.5.x receive GPU support?

**A**: No. GPU support is planned for v2.x series only. v1.5.x focuses on stability and CPU optimization.

### Q: Can I use CUDA in v1.5.x?

**A**: CUDA is available for other features (LoRA training, RoPE embeddings) but NOT for vector indexing.

### Q: Is CPU fast enough for production?

**A**: Yes! 30K QPS is sufficient for most use cases. Many production systems use CPU-only vector search.

### Q: Will GPU code from v1.4.x work?

**A**: Code using `Backend::VULKAN`, `Backend::CUDA`, or `Backend::HIP` will need to change to `Backend::CPU` or `Backend::AUTO`.

### Q: Can I still build with GPU flags?

**A**: Yes, but they won't enable GPU vector indexing. GPU flags still enable other GPU features in ThemisDB.

### Q: When will v2.1 release?

**A**: Target is Q3 2026, but this is subject to change based on development resources.

### Q: Will GPU support be optional?

**A**: Yes, GPU support will be optional. CPU fallback will always be available.

### Q: Can I help with GPU development?

**A**: Yes! Comment on GitHub issues tagged `gpu-acceleration` to get involved.

## Version Compatibility Matrix

| Version | CPU Vector Index | CUDA Vector Index | Vulkan Vector Index | HIP Vector Index |
|---------|-----------------|-------------------|--------------------|--------------------|
| v1.4.x | ✅ Available | ⚠️ Stub only | ⚠️ Stub only | ⚠️ Stub only |
| v1.5.x | ✅ Available | ❌ Removed | ❌ Removed | ❌ Removed |
| v2.1.x (planned) | ✅ Available | ✅ Available | ❌ Not yet | ❌ Not yet |
| v2.2.x (planned) | ✅ Available | ✅ Available | ✅ Available | ❌ Not yet |
| v2.3.x (planned) | ✅ Available | ✅ Available | ✅ Available | ✅ Available |

## Contact & Support

- **Documentation**: See `docs/FUTURE_GPU_SUPPORT.md` for detailed roadmap
- **GitHub Issues**: Tag issues with `gpu-acceleration`
- **Community**: Join discussions on GitHub Discussions

---

**Last Updated**: February 2026  
**Applies to**: v1.5.0 and later  
**Next Review**: Q3 2026 (before v2.1 release)
