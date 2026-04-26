# GPU Vector Indexing Implementation

## ⚠️ IMPORTANT NOTICE - v1.5.0+ Status

**GPU vector indexing is NOT currently available in ThemisDB v1.5.0+**

This document describes the future GPU implementation planned for v2.x. The current version (v1.5.0+) uses a CPU-only implementation.

**For current capabilities and roadmap, see:**
- **Current Status**: [GPU_SUPPORT_ROADMAP.md](GPU_SUPPORT_ROADMAP.md)
- **Future Plans**: [FUTURE_GPU_SUPPORT.md](FUTURE_GPU_SUPPORT.md)

---

## Overview (Future v2.x)

ThemisDB's GPU Vector Indexing (planned for v2.x) will provide high-performance vector similarity search across multiple GPU backends:

- **CUDA** (v2.1): NVIDIA GPU acceleration with advanced optimizations
- **Vulkan** (v2.2): Cross-platform GPU acceleration (Windows, Linux, macOS, Android)
- **HIP** (v2.3): AMD ROCm acceleration for AMD GPUs

## Current Implementation (v1.5.0+)

The current `GPUVectorIndex` class provides a **CPU-only** implementation:

```cpp
#include "index/gpu_vector_index.h"

using namespace themis::index;

// Create index (uses CPU backend automatically)
GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::AUTO;  // Falls back to CPU
config.metric = GPUVectorIndex::DistanceMetric::COSINE;

GPUVectorIndex index(config);
index.initialize(128); // 128-dimensional vectors

// Add vectors
index.addVector("id1", vector1);
index.addVectorBatch(ids, vectors);

// Search (executes on CPU with SIMD optimizations)
auto results = index.search(query, 10); // Top-10 nearest neighbors
```

### Current Backend Selection

In v1.5.0+, the system always uses CPU:

- ✅ **CPU**: Always available (SIMD-optimized)
- ❌ **CUDA**: Not available (planned for v2.1)
- ❌ **Vulkan**: Not available (planned for v2.2)
- ❌ **HIP**: Not available (planned for v2.3)

## Performance (Current CPU Implementation)

| Metric | Performance |
|--------|------------|
| Single Query | 0.5 ms |
| Batch (64) | 20 ms |
| Batch (512) | 150 ms |
| Throughput | 30K QPS |
| Index Build | 60 sec (1M vectors) |

**Optimizations Enabled**:
- ✅ SIMD vectorization (AVX-512, AVX2, NEON)
- ✅ Multi-threaded batch processing
- ✅ Cache-friendly memory layout
- ✅ NUMA-aware allocation

## Future GPU Architecture (v2.x)

### Unified Interface (Planned)

The API is designed to be forward-compatible with GPU backends:

```cpp
// Future v2.x code (GPU-enabled)
GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::CUDA;  // Will use CUDA in v2.1+
config.deviceId = 0;                             // GPU device ID
config.useMixedPrecision = true;                 // FP16/TF32 acceleration
config.maxVRAM_MB = 8192;                        // 8 GB VRAM limit

GPUVectorIndex index(config);
index.initialize(128);
```

### Distance Metrics

Supported distance metrics (both CPU and future GPU):

```cpp
enum class DistanceMetric {
    L2,              // Euclidean: ||a - b||²
    COSINE,          // Cosine: 1 - (a·b)/(||a|| ||b||)
    INNER_PRODUCT    // Inner product: max(0, -a·b)
};
```

## Migration Notes

### From v1.4.x to v1.5.x

If your code referenced GPU backends, update as follows:

**Before (v1.4.x):**
```cpp
config.backend = GPUVectorIndex::Backend::VULKAN;  // ❌ Not available
config.backend = GPUVectorIndex::Backend::CUDA;    // ❌ Not available
```

**After (v1.5.x):**
```cpp
config.backend = GPUVectorIndex::Backend::CPU;   // ✅ Available
config.backend = GPUVectorIndex::Backend::AUTO;  // ✅ Falls back to CPU
```

### Future Migration to v2.x

When v2.x releases, your current CPU code will continue to work:

```cpp
// This code works in v1.5.x (CPU) and will work in v2.x (GPU)
GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::AUTO;  // CPU now, GPU in v2.x
GPUVectorIndex index(config);
index.initialize(128);
```

## Alternatives for GPU Acceleration

If you need GPU acceleration now:

### 1. FAISS GPU (Recommended)

ThemisDB integrates FAISS, which has GPU support:

```cpp
#include "acceleration/faiss_gpu_backend.h"
// See docs/FAISS_MIGRATION_COMPLETE.md
```

### 2. External Vector Databases

- **Milvus**: GPU support via CUDA
- **Weaviate**: GPU via external vectorizers
- **Qdrant**: CPU-optimized (GPU planned)

## Roadmap

| Version | Timeline | GPU Support |
|---------|----------|------------|
| v1.5.x | Current | CPU-only |
| v2.1 | Q3 2026 | + CUDA (NVIDIA) |
| v2.2 | Q4 2026 | + Vulkan (cross-platform) |
| v2.3 | Q1 2027 | + HIP (AMD) |
| v2.4 | Q2 2027 | + Multi-GPU |

## References

- **Current Status**: [GPU_SUPPORT_ROADMAP.md](GPU_SUPPORT_ROADMAP.md)
- **Future Plans**: [FUTURE_GPU_SUPPORT.md](FUTURE_GPU_SUPPORT.md)
- **FAISS Integration**: [FAISS_MIGRATION_COMPLETE.md](FAISS_MIGRATION_COMPLETE.md)

---

**Status**: CPU-only in v1.5.0+, GPU planned for v2.x  
**Last Updated**:  April 2026
