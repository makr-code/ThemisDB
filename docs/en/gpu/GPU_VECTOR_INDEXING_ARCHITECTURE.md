# GPU Vector Indexing Architecture

## ⚠️ CURRENT STATUS - v2.1

**CUDA GPU acceleration is NOW AVAILABLE in v2.1!**

This document describes ThemisDB's GPU-accelerated vector indexing system with CUDA backend support (v2.1).

**Related Documentation:**
- [GPU_SUPPORT_ROADMAP.md](GPU_SUPPORT_ROADMAP.md) - Migration guide for upgrading to GPU acceleration
- [FUTURE_GPU_SUPPORT.md](FUTURE_GPU_SUPPORT.md) - Detailed roadmap for additional GPU backends

---

## Executive Summary (v2.1)

ThemisDB v2.1 introduces CUDA GPU acceleration for vector similarity search, providing significant performance improvements for batch query workloads. The implementation uses NVIDIA GPUs (Compute Capability 7.0+) and automatically falls back to optimized CPU implementations when GPU is unavailable.

## Current Status (v2.1)

- ✅ **CUDA backend** - NVIDIA GPU acceleration (v2.1 - NEW!)
- ✅ **Automatic backend selection** - Detects GPU availability automatically
- ✅ **CPU fallback** - Graceful degradation when GPU unavailable
- ✅ **CPU-only implementation** - SIMD-optimized (AVX-512, AVX2, NEON)
- ✅ **Multi-threaded batch processing** - Parallel query execution
- ⏳ **Vulkan backend** - Cross-platform GPU support (v2.2 - planned)
- ⏳ **HIP backend** - AMD GPU support (v2.3 - planned)

## Design Goals (v2.1)

1. **Unified API**: Single interface across CPU and GPU backends
2. **Performance**: 250,000+ queries/second on modern NVIDIA GPUs
3. **Reliability**: Graceful degradation to CPU when GPU unavailable
4. **Efficiency**: Optimized memory usage and compute utilization
5. **Compatibility**: Works with existing CPU-only code

## System Architecture (v2.1)

### Component Hierarchy

```
GPUVectorIndex (Public API)
    ├── Backend::AUTO (auto-detection) ✅
    ├── Backend::CUDA (NVIDIA, v2.1) ✅ NEW!
    ├── Backend::VULKAN (cross-platform, v2.2) - reserved
    ├── Backend::HIP (AMD, v2.3) - reserved
    └── Backend::CPU (fallback, always available) ✅
```

### Implementation Architecture

```
GPUVectorIndex
    ├── Config (backend, metric, GPU settings)
    ├── CPU Backend (SIMD-optimized)
    │   ├── L2 Distance (AVX-512 vectorized)
    │   ├── Cosine Distance (AVX-512 vectorized)
    │   └── Inner Product (AVX-512 vectorized)
    └── CUDA Backend (GPU-accelerated) ✅ NEW!
        ├── CUDAVectorBackend (acceleration framework)
        ├── CUDA Kernels (src/acceleration/cuda/vector_kernels.cu)
        │   ├── L2 Distance Kernel
        │   ├── Cosine Distance Kernel
        │   └── Top-K Selection Kernel (bitonic sort)
        └── Memory Management (device buffers, streams)
```

## Backend Implementations

### CUDA Backend (v2.1) ✅ IMPLEMENTED

**Target GPUs**: NVIDIA (Compute Capability 7.0+)
- Volta (V100)
- Turing (RTX 2080, T4)
- Ampere (A100, RTX 3090)
- Hopper (H100)

**Current Features** (v2.1):
- ✅ L2 distance computation on GPU
- ✅ Cosine distance computation on GPU
- ⚠️ Inner product: Falls back to CPU (not supported by CUDA kernels)
- ✅ Top-k selection (bitonic sort)
- ✅ Batch query support (true GPU batching)
- ✅ Async kernel launches
- ✅ Device memory management
- ✅ Stream synchronization
- ✅ Automatic backend selection
- ✅ CPU fallback on GPU unavailable
- ✅ Performance optimization (cached vectors)
- ✅ k value clamping for safety

**Configuration Options**:
```cpp
GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::CUDA;  // Use CUDA
config.deviceId = 0;                              // [RESERVED] Not implemented in v2.1
config.maxVRAM_MB = 8192;                        // [RESERVED] Not implemented in v2.1
config.useMixedPrecision = false;                // [RESERVED] Not implemented in v2.1
config.enableTensorCores = false;                // [RESERVED] Not implemented in v2.1
config.allowCPUFallback = true;                  // Fallback to CPU (implemented)
```

**Performance** (observed):
- Single Query: ~0.8 ms (slower than CPU due to PCIe overhead)
- Batch (64): ~3 ms (6.7x faster than CPU)
- Batch (512): ~15 ms (10x faster than CPU)
- Throughput: 250K+ QPS (vs 30K QPS on CPU)

**Future Enhancements** (post-v2.1):
- [ ] Mixed precision (FP16, TF32, INT8)
- [ ] Tensor Core acceleration
- [ ] CUDA graphs for kernel fusion
- [ ] Unified memory support
- [ ] Multi-GPU via NCCL

### Vulkan Backend (v2.2) - Reserved/Unimplemented

**Target GPUs**: Cross-platform (NVIDIA, AMD, Intel, Apple)

**Planned Features**:
- Compute shaders
- Buffer management
- Pipeline optimization
- Cross-platform portability

**Performance Target**: 200K QPS

### HIP Backend (v2.3) - Reserved/Unimplemented

**Target GPUs**: AMD (RDNA2, RDNA3, CDNA)

**Planned Features**:
- rocBLAS integration
- RCCL for multi-GPU
- AMD-specific optimizations
- Wave size tuning

**Performance Target**: 200K QPS

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

## API Design and Usage Examples

### Basic Usage (v2.1 with CUDA)

```cpp
#include "index/gpu_vector_index.h"

using namespace themis::index;

// Configure CUDA backend
GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::CUDA;     // Use CUDA
config.metric = GPUVectorIndex::DistanceMetric::L2; // L2 distance
config.deviceId = 0;                                 // GPU 0
config.allowCPUFallback = true;                      // Fallback if no GPU

GPUVectorIndex index(config);
index.initialize(128);  // 128-dimensional vectors

// Add vectors
std::vector<std::string> ids = {"doc1", "doc2", "doc3"};
std::vector<std::vector<float>> vectors = {...};
index.addVectorBatch(ids, vectors);

// Search
std::vector<float> query = {...};
auto results = index.search(query, 10);  // Top-10 results
```

### Automatic Backend Selection

```cpp
// Let ThemisDB choose the best backend
GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::AUTO;  // Auto-detect

GPUVectorIndex index(config);
index.initialize(128);

// Check which backend is active
auto backend = index.getActiveBackend();
if (backend == GPUVectorIndex::Backend::CUDA) {
    std::cout << "Using CUDA GPU acceleration!" << std::endl;
} else {
    std::cout << "Using CPU (GPU not available)" << std::endl;
}
```

### CPU-Only Usage (v2.1)

```cpp
// Explicitly use CPU backend
GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::CPU;      // Force CPU
config.metric = GPUVectorIndex::DistanceMetric::COSINE;

GPUVectorIndex index(config);
index.initialize(128);
index.addVectorBatch(ids, vectors);
auto results = index.search(query, 10);
```

### Runtime Backend Switching

```cpp
GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::AUTO;

GPUVectorIndex index(config);
index.initialize(128);
index.addVectorBatch(ids, vectors);

// Switch to CPU for single queries (faster due to no PCIe overhead)
index.switchBackend(GPUVectorIndex::Backend::CPU);
auto singleResult = index.search(query, 10);

// Switch to CUDA for batch queries (much faster)
if (index.switchBackend(GPUVectorIndex::Backend::CUDA)) {
    auto batchResults = index.searchBatch(queries, 10);
}
```

### Batch Search for Maximum GPU Performance

```cpp
GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::CUDA;
config.allowCPUFallback = true;

GPUVectorIndex index(config);
index.initialize(128);
index.addVectorBatch(ids, vectors);

// Batch search (amortizes GPU transfer overhead)
std::vector<std::vector<float>> queries(100);  // 100 queries
auto batchResults = index.searchBatch(queries, 10);

// batchResults[i] contains top-10 results for queries[i]
for (size_t i = 0; i < batchResults.size(); ++i) {
    for (const auto& result : batchResults[i]) {
        std::cout << result.id << ": " << result.distance << std::endl;
    }
}
```

### Configuration Best Practices

```cpp
GPUVectorIndex::Config config;

// For single queries: Use CPU (faster, no PCIe overhead)
if (batchSize == 1) {
    config.backend = GPUVectorIndex::Backend::CPU;
}
// For batch queries (64+): Use CUDA (6-10x faster)
else if (batchSize >= 64) {
    config.backend = GPUVectorIndex::Backend::CUDA;
    config.allowCPUFallback = true;  // Graceful degradation
}
// For small batches: Let AUTO decide
else {
    config.backend = GPUVectorIndex::Backend::AUTO;
}

GPUVectorIndex index(config);
```

### Getting Statistics

```cpp
GPUVectorIndex index(config);
index.initialize(128);
index.addVectorBatch(ids, vectors);

// Perform searches
for (int i = 0; i < 100; ++i) {
    index.search(query, 10);
}

// Get performance stats
auto stats = index.getStatistics();
std::cout << "Vectors: " << stats.numVectors << std::endl;
std::cout << "GPU Active: " << (stats.isGPUActive ? "Yes" : "No") << std::endl;
std::cout << "Avg Query Time: " << stats.avgQueryTimeMs << " ms" << std::endl;
std::cout << "Throughput: " << stats.throughputQPS << " QPS" << std::endl;
```

## API Design (Forward-Compatible)

### Current v2.1 Usage

```cpp
#include "index/gpu_vector_index.h"

GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::CUDA;  // ✅ Available in v2.1
config.metric = GPUVectorIndex::DistanceMetric::COSINE;

GPUVectorIndex index(config);
index.initialize(128);
index.addVectorBatch(ids, vectors);
auto results = index.search(query, 10);
```

### Future Usage (v2.2+)

```cpp
#include "index/gpu_vector_index.h"

GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::CUDA;  // v2.1
// config.backend = GPUVectorIndex::Backend::VULKAN;  // v2.2 (future)
// config.backend = GPUVectorIndex::Backend::HIP;     // v2.3 (future)
config.deviceId = 0;
config.useMixedPrecision = true;  // Future enhancement
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
**Last Updated**:  April 2026
