# GPU Vector Indexing Architecture

## ⚠️ IMPORTANT NOTICE - v1.5.0+ Status

**This document describes the future GPU architecture planned for v2.x.**

The current version (v1.5.0+) uses a CPU-only implementation. GPU support was removed in v1.5.0 and is being redesigned for v2.x.

**For current information, see:**
- [GPU_SUPPORT_ROADMAP.md](GPU_SUPPORT_ROADMAP.md) - Current status and migration guide
- [FUTURE_GPU_SUPPORT.md](FUTURE_GPU_SUPPORT.md) - Detailed future plans

---

## Executive Summary (Future v2.x)

This document describes the planned architecture of ThemisDB's GPU-accelerated vector indexing system. The implementation will provide high-performance vector similarity search across multiple GPU backends (CUDA, Vulkan, HIP) with automatic backend selection and graceful CPU fallback.

## Current Status (v1.5.0+)

- ❌ GPU backends (CUDA, Vulkan, HIP) removed in v1.5.0
- ✅ CPU-only implementation available
- ✅ SIMD-optimized (AVX-512, AVX2, NEON)
- ✅ Multi-threaded batch processing
- ⏳ GPU support planned for v2.x series

## Design Goals (v2.x)

1. **Unified API**: Single interface for all GPU backends
2. **Performance**: 250,000+ queries/second on modern GPUs
3. **Portability**: Support NVIDIA, AMD, Intel, and Apple GPUs
4. **Reliability**: Graceful degradation to CPU when GPU unavailable
5. **Efficiency**: Optimized memory usage and compute utilization

## Planned System Architecture (v2.x)

### Component Hierarchy

```
GPUVectorIndex (Public API)
    ├── Backend::AUTO (auto-detection)
    ├── Backend::CUDA (NVIDIA, v2.1)
    ├── Backend::VULKAN (cross-platform, v2.2)
    ├── Backend::HIP (AMD, v2.3)
    └── Backend::CPU (fallback, always available)
```

### Current Implementation (v1.5.x)

```
GPUVectorIndex (Public API)
    └── Backend::CPU (SIMD-optimized)
        ├── L2 Distance (AVX-512 vectorized)
        ├── Cosine Distance (AVX-512 vectorized)
        └── Inner Product (AVX-512 vectorized)
```

## Future Backend Designs

### CUDA Backend (v2.1)

**Target GPUs**: NVIDIA (Compute Capability 7.0+)

**Features**:
- Mixed precision (FP16, TF32, INT8)
- Tensor Core acceleration
- CUDA graphs for kernel fusion
- Unified memory support
- Multi-GPU via NCCL

**Performance Target**: 250K QPS

### Vulkan Backend (v2.2)

**Target GPUs**: Cross-platform (NVIDIA, AMD, Intel, Apple)

**Features**:
- Compute shaders
- Buffer management
- Pipeline optimization
- Cross-platform portability

**Performance Target**: 200K QPS

### HIP Backend (v2.3)

**Target GPUs**: AMD (RDNA2, RDNA3, CDNA)

**Features**:
- rocBLAS integration
- RCCL for multi-GPU
- AMD-specific optimizations
- Wave size tuning

**Performance Target**: 200K QPS

## Current CPU Architecture (v1.5.x)

### Distance Computation

```cpp
// CPU-optimized with SIMD
float computeL2Distance(const float* a, const float* b, int dim) {
    // AVX-512 vectorized implementation
    // Processes 16 floats per iteration
}
```

### Batch Processing

```cpp
// Multi-threaded parallel execution
std::vector<Results> searchBatch(const std::vector<Query>& queries, size_t k) {
    // OpenMP or std::thread parallelization
    // Scales to available CPU cores
}
```

### Performance Characteristics

| Operation | Latency | Throughput |
|-----------|---------|-----------|
| Single Query | 0.5 ms | - |
| Batch (64) | 20 ms | 3,200 QPS |
| Batch (512) | 150 ms | 3,400 QPS |
| Max Throughput | - | 30K QPS |

## Future Implementation Details (v2.x)

### Memory Management

**Host Memory**:
- Vector storage (CPU side)
- Query buffers
- Result buffers

**Device Memory** (GPU):
- Vector database (GPU side)
- Intermediate computation buffers
- Distance matrices

**Transfer Strategy**:
- Batch queries to amortize PCIe overhead
- Keep vectors on GPU when possible
- Async transfers with compute overlap

### Distance Kernels (CUDA Example)

```cuda
__global__ void computeL2DistanceKernel(
    const float* queries, const float* vectors,
    float* distances, int numQueries, int numVectors, int dim) {
    
    int qIdx = blockIdx.x * blockDim.x + threadIdx.x;
    int vIdx = blockIdx.y * blockDim.y + threadIdx.y;
    
    if (qIdx < numQueries && vIdx < numVectors) {
        float sum = 0.0f;
        for (int i = 0; i < dim; i++) {
            float diff = queries[qIdx * dim + i] - vectors[vIdx * dim + i];
            sum += diff * diff;
        }
        distances[qIdx * numVectors + vIdx] = sum;
    }
}
```

### Optimization Strategies

**Compute**:
- Warp-level primitives
- Shared memory tiling
- Memory coalescing
- Tensor Core acceleration (where applicable)

**Memory**:
- Minimize host-device transfers
- Use pinned memory for DMA
- Async transfers with compute overlap
- Unified memory for large datasets

**Scaling**:
- Multi-GPU data parallelism
- Load balancing across devices
- NCCL/RCCL for collective operations

## API Design (Forward-Compatible)

### Current Usage (v1.5.x)

```cpp
#include "index/gpu_vector_index.h"

GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::CPU;  // Only option
config.metric = GPUVectorIndex::DistanceMetric::COSINE;

GPUVectorIndex index(config);
index.initialize(128);
index.addVectorBatch(ids, vectors);
auto results = index.search(query, 10);
```

### Future Usage (v2.x)

```cpp
#include "index/gpu_vector_index.h"

GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::CUDA;  // GPU in v2.x
config.deviceId = 0;
config.useMixedPrecision = true;
config.maxVRAM_MB = 8192;

GPUVectorIndex index(config);
index.initialize(128);
index.addVectorBatch(ids, vectors);
auto results = index.search(query, 10);
```

**No breaking changes**: Existing CPU code continues to work.

## Performance Comparison

### Current (v1.5.x CPU)

| Workload | Performance |
|----------|------------|
| Small batch (<64) | Good |
| Large batch (>512) | Medium |
| High dimensionality | Slow |
| Real-time queries | Good |

### Future (v2.x GPU)

| Workload | Performance |
|----------|------------|
| Small batch (<64) | Medium (PCIe overhead) |
| Large batch (>512) | Excellent |
| High dimensionality | Good |
| Real-time queries | Good (with batching) |

## Migration Path

### Phase 1: v1.5.x (Current)
- Use CPU-optimized implementation
- Tune HNSW parameters for performance
- Use batch processing for throughput

### Phase 2: v2.1 (CUDA Support)
- Add `config.backend = Backend::CUDA`
- Enable mixed precision
- Test on NVIDIA hardware

### Phase 3: v2.2+ (Multi-Backend)
- Switch to Vulkan for portability
- Use HIP for AMD GPUs
- Multi-GPU for extreme scale

## References

- **Current Status**: [GPU_SUPPORT_ROADMAP.md](GPU_SUPPORT_ROADMAP.md)
- **Future Plans**: [FUTURE_GPU_SUPPORT.md](FUTURE_GPU_SUPPORT.md)
- **Implementation**: [GPU_VECTOR_INDEXING_IMPLEMENTATION.md](GPU_VECTOR_INDEXING_IMPLEMENTATION.md)

---

**Status**: CPU-only in v1.5.0+, GPU planned for v2.x  
**Last Updated**: February 2026
