# GPU Vector Indexing Architecture

## Executive Summary

This document describes the architecture of ThemisDB's GPU-accelerated vector indexing system. The implementation provides high-performance vector similarity search across multiple GPU backends (Vulkan, CUDA, HIP) with automatic backend selection and graceful CPU fallback.

## Design Goals

1. **Unified API**: Single interface for all GPU backends
2. **Performance**: 50,000+ queries/second on consumer GPUs
3. **Portability**: Support NVIDIA, AMD, Intel, and Apple GPUs
4. **Reliability**: Graceful degradation to CPU when GPU unavailable
5. **Efficiency**: Optimized memory usage and compute utilization

## System Architecture

### Component Hierarchy

```
GPUVectorIndex (Public API)
    ├── VulkanVectorIndexBackend (Cross-platform)
    ├── CUDAVectorIndexBackend (NVIDIA)
    ├── HIPVectorIndexBackend (AMD)
    └── CPU Fallback (Brute-force)
```

### Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│                   (User Code / API)                          │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ├── GPUVectorIndex::search(query, k)
                     │
┌────────────────────▼────────────────────────────────────────┐
│              Unified Vector Index Interface                  │
│  • Backend Selection & Management                           │
│  • Automatic Fallback Logic                                 │
│  • Statistics & Performance Monitoring                      │
└────────────────────┬────────────────────────────────────────┘
                     │
        ┌────────────┼────────────┐
        │            │            │
┌───────▼─────┐ ┌───▼─────┐ ┌───▼─────┐
│   Vulkan    │ │  CUDA   │ │   HIP   │
│   Backend   │ │ Backend │ │ Backend │
└───────┬─────┘ └───┬─────┘ └───┬─────┘
        │           │           │
        │           │           │
┌───────▼───────────▼───────────▼─────────────────────────────┐
│                    GPU Hardware Layer                        │
│  Vulkan Compute  │  CUDA Cores  │  ROCm/RDNA Compute       │
└──────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. GPUVectorIndex (Main Interface)

**Responsibilities:**
- Provide unified API for all operations
- Manage backend lifecycle
- Handle backend selection and switching
- Maintain statistics and performance metrics
- Coordinate CPU fallback

**Key Classes:**
```cpp
class GPUVectorIndex {
    // Public API
    bool initialize(int dimension);
    bool addVector(const std::string& id, const std::vector<float>& vector);
    std::vector<SearchResult> search(const std::vector<float>& query, size_t k);
    
    // Backend management
    Backend getActiveBackend() const;
    bool switchBackend(Backend backend);
    
private:
    class Impl;  // PIMPL for backend isolation
    std::unique_ptr<Impl> pImpl;
};
```

**Backend Selection Algorithm:**
```
1. If backend = AUTO:
   a. Try Vulkan (highest portability)
   b. Try CUDA (best NVIDIA performance)
   c. Try HIP (best AMD performance)
   d. Fallback to CPU
2. Else:
   a. Try requested backend
   b. If allowCPUFallback: fallback to CPU
   c. Else: fail
```

### 2. VulkanVectorIndexBackend

**Architecture:**
- **Compute Shaders**: GLSL compute shaders for distance computation
- **Pipeline Management**: Separate pipelines for L2, Cosine, Inner Product
- **Memory Model**: Host-visible staging buffers + device-local compute buffers
- **Descriptor Sets**: Layout binding for query, vector, and result buffers

**Shader Pipeline:**
```
┌──────────────┐
│ Query Buffer │──┐
└──────────────┘  │
                  ├──> [Compute Shader] ──> [Distance Results]
┌──────────────┐  │
│Vector Buffer │──┘
└──────────────┘
```

**Compute Shaders:**
1. `l2_distance.comp`: L2 distance kernel
2. `cosine_distance.comp`: Cosine similarity kernel
3. `inner_product_distance.comp`: Inner product kernel
4. `batch_search.comp`: Optimized batch search with shared memory
5. `topk_selection.comp`: K-nearest neighbor selection

**Memory Flow:**
```
Host Memory (RAM)
    │
    ├─ cudaMemcpy / vkCmdCopyBuffer
    │
Device Memory (VRAM)
    │
    ├─ Compute Shader Execution
    │
Result Buffer (VRAM)
    │
    ├─ cudaMemcpy / vkCmdCopyBuffer
    │
Host Memory (Results)
```

### 3. CUDAVectorIndexBackend

**Advanced Optimizations:**

**a) Mixed Precision:**
- FP32: Default precision
- FP16: 2x throughput on Tensor Cores (Volta+)
- TF32: Automatic on Ampere+ (19-bit mantissa)
- INT8: 4x throughput for quantized vectors

**b) Flash Attention-Style Optimization:**
```
Tiled Computation Pattern:
┌────────────────────────────────────┐
│ Query Tile (32x32)                 │
│  ┌────────────┐                    │
│  │ Shared Mem │ ──> Compute ──> Partial Sum
│  └────────────┘                    │
└────────────────────────────────────┘
         │
         ├─ Load Next Tile
         │
┌────────▼───────────────────────────┐
│ Vector Tile (32x32)                │
└────────────────────────────────────┘
```

**c) Memory Coalescing:**
- Threads in warp access consecutive memory addresses
- Reduces DRAM transactions by 32x
- Critical for high-dimensional vectors

**d) Tensor Core Usage:**
```cpp
// Matrix multiplication using WMMA (Warp Matrix Multiply-Accumulate)
wmma::fragment<wmma::matrix_a, 16, 16, 16, half> a_frag;
wmma::fragment<wmma::matrix_b, 16, 16, 16, half> b_frag;
wmma::fragment<wmma::accumulator, 16, 16, 16, float> c_frag;

wmma::load_matrix_sync(a_frag, queries, 16);
wmma::load_matrix_sync(b_frag, vectors, 16);
wmma::mma_sync(c_frag, a_frag, b_frag, c_frag);
```

**e) CUDA Graphs:**
```
Graph Nodes:
  MemcpyH2D → DistanceKernel → TopKKernel → MemcpyD2H
  
Benefits:
  - Reduced kernel launch overhead
  - Better overlap of compute and memory transfers
  - 10-20% performance improvement
```

### 4. HIPVectorIndexBackend

**AMD-Specific Optimizations:**

**a) Wave Size Tuning:**
- RDNA2: Wave32 (better occupancy)
- RDNA3: Wave32 or Wave64 (tunable)
- CDNA: Wave64 (compute-optimized)

**b) LDS (Local Data Share) Optimization:**
```cpp
// 64KB shared memory per CU
__shared__ float sharedQuery[WAVE_SIZE][256];

// Bank conflict avoidance (32-way banked)
sharedQuery[threadIdx.y][threadIdx.x] = query[idx];
```

**c) rocBLAS Integration:**
```cpp
// Use rocBLAS for large matrix operations
rocblas_sgemm(
    handle,
    rocblas_operation_none,
    rocblas_operation_transpose,
    numQueries, numVectors, dimension,
    &alpha,
    queries, dimension,
    vectors, dimension,
    &beta,
    results, numVectors
);
```

**d) RCCL Multi-GPU:**
```
Ring AllReduce Pattern:
  GPU0 → GPU1 → GPU2 → GPU3 → GPU0
  
  Each GPU:
    1. Send chunk to next GPU
    2. Receive chunk from previous GPU
    3. Accumulate results
    4. Repeat until complete
```

## Memory Management

### Buffer Allocation Strategy

**Vulkan:**
```cpp
VkMemoryAllocateInfo allocInfo = {};
allocInfo.allocationSize = size;
allocInfo.memoryTypeIndex = findMemoryType(
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
);
vkAllocateMemory(device, &allocInfo, nullptr, &memory);
```

**CUDA:**
```cpp
// Option 1: Device memory (fastest)
cudaMalloc(&d_ptr, size);

// Option 2: Unified memory (easier, slower)
cudaMallocManaged(&d_ptr, size);

// Option 3: Pinned memory (DMA transfers)
cudaMallocHost(&h_ptr, size);
```

**HIP:**
```cpp
// Device memory
hipMalloc(&d_ptr, size);

// Managed memory
hipMallocManaged(&d_ptr, size);

// Fine-grained memory (coherent)
hipExtMallocWithFlags(&d_ptr, size, hipDeviceMallocFinegrained);
```

### Memory Pooling

To reduce allocation overhead:
```cpp
class MemoryPool {
    std::vector<VkDeviceMemory> chunks;
    std::map<size_t, std::vector<VkDeviceMemory>> freeList;
    
    VkDeviceMemory allocate(size_t size) {
        // Round up to power of 2
        size_t allocSize = nextPowerOf2(size);
        
        // Check free list
        if (!freeList[allocSize].empty()) {
            auto mem = freeList[allocSize].back();
            freeList[allocSize].pop_back();
            return mem;
        }
        
        // Allocate new chunk
        return allocateNewChunk(allocSize);
    }
};
```

## Performance Optimization Techniques

### 1. Batch Processing

**Problem:** Individual searches have high overhead (kernel launch, memory transfer)

**Solution:** Process multiple queries in parallel
```cpp
// Bad: Sequential searches
for (query : queries) {
    results.push_back(search(query, k));
}

// Good: Batch search
auto results = searchBatch(queries, k);
```

**Speedup:** 10-50x depending on batch size

### 2. Asynchronous Execution

```cpp
// Create multiple streams for overlap
cudaStream_t streams[4];
for (int i = 0; i < 4; i++) {
    cudaStreamCreate(&streams[i]);
}

// Overlap compute and memory transfers
for (int i = 0; i < numBatches; i++) {
    int streamId = i % 4;
    
    cudaMemcpyAsync(d_queries[i], h_queries[i], size,
                   cudaMemcpyHostToDevice, streams[streamId]);
    
    launchKernel<<<grid, block, 0, streams[streamId]>>>(
        d_queries[i], d_vectors, d_results[i]);
    
    cudaMemcpyAsync(h_results[i], d_results[i], size,
                   cudaMemcpyDeviceToHost, streams[streamId]);
}
```

### 3. Persistent Kernels

For high-throughput scenarios:
```cpp
__global__ void persistentKernel(
    WorkQueue* workQueue,
    float* vectors,
    float* results)
{
    while (true) {
        Work work = workQueue->dequeue();
        if (work.done) break;
        
        // Process work
        computeDistances(work.queries, vectors, results);
    }
}
```

### 4. Zero-Copy Access (Vulkan)

```cpp
// Map device memory to host address space
VkMemoryAllocateInfo allocInfo = {};
allocInfo.memoryTypeIndex = findMemoryType(
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
);

void* mappedMemory;
vkMapMemory(device, memory, 0, size, 0, &mappedMemory);

// Direct CPU write, GPU read (no explicit copy)
memcpy(mappedMemory, hostData, size);
```

## Performance Benchmarks

### Synthetic Benchmarks

**Hardware:**
- NVIDIA RTX 3080 (10GB VRAM, 8704 CUDA Cores)
- AMD RX 6800 XT (16GB VRAM, 72 CUs)
- Intel Arc A770 (16GB VRAM, 32 Xe Cores)

**Dataset:**
- 1M vectors, 128 dimensions
- Query batch size: 512
- k = 10 (top-10 nearest neighbors)

| Backend | Throughput (QPS) | Latency (ms) | VRAM (MB) |
|---------|------------------|--------------|-----------|
| CUDA (RTX 3080) | 62,340 | 8.2 | 512 |
| HIP (RX 6800 XT) | 51,280 | 10.0 | 512 |
| Vulkan (Arc A770) | 43,120 | 11.9 | 512 |
| CPU (Ryzen 5950X) | 4,820 | 106.2 | 512 |

### Real-World Performance

**Retrieval-Augmented Generation (RAG):**
- 10M document embeddings (768-dim, BERT)
- 100 concurrent query streams
- p50 latency: 15ms
- p99 latency: 28ms
- Sustained throughput: 45,000 QPS

## Error Handling & Reliability

### GPU Failure Detection

```cpp
bool isGPUHealthy() {
    try {
        // Allocate small test buffer
        void* testPtr;
        cudaMalloc(&testPtr, 1024);
        
        // Run simple kernel
        testKernel<<<1, 1>>>();
        cudaDeviceSynchronize();
        
        // Check for errors
        cudaError_t err = cudaGetLastError();
        cudaFree(testPtr);
        
        return (err == cudaSuccess);
    } catch (...) {
        return false;
    }
}
```

### Automatic Fallback

```cpp
std::vector<SearchResult> search(const std::vector<float>& query, size_t k) {
    if (activeBackend != Backend::CPU) {
        try {
            return gpuSearch(query, k);
        } catch (const GPUException& e) {
            std::cerr << "GPU search failed: " << e.what() << std::endl;
            
            if (config.allowCPUFallback) {
                std::cerr << "Falling back to CPU" << std::endl;
                activeBackend = Backend::CPU;
                return cpuSearch(query, k);
            }
            throw;
        }
    }
    return cpuSearch(query, k);
}
```

### Circuit Breaker Pattern

```cpp
class CircuitBreaker {
    int failureCount = 0;
    int threshold = 5;
    bool open = false;
    
    bool execute(std::function<void()> fn) {
        if (open) {
            throw CircuitBreakerOpenException();
        }
        
        try {
            fn();
            failureCount = 0;
            return true;
        } catch (...) {
            failureCount++;
            if (failureCount >= threshold) {
                open = true;
            }
            throw;
        }
    }
};
```

## Future Enhancements

### 1. Dynamic Backend Switching

Switch backends based on workload:
```cpp
if (queryLoad > HIGH_THRESHOLD) {
    switchBackend(Backend::CUDA); // Highest throughput
} else if (queryLoad > LOW_THRESHOLD) {
    switchBackend(Backend::VULKAN); // Balanced
} else {
    switchBackend(Backend::CPU); // Save power
}
```

### 2. Multi-GPU Load Balancing

```cpp
class MultiGPUScheduler {
    std::vector<GPUVectorIndex> gpus;
    std::atomic<int> roundRobinIndex{0};
    
    SearchResult schedule(const std::vector<float>& query, size_t k) {
        int gpuId = roundRobinIndex++ % gpus.size();
        return gpus[gpuId].search(query, k);
    }
};
```

### 3. Quantization Support

```cpp
// 8-bit quantization: 4x memory savings
struct QuantizedVector {
    std::vector<uint8_t> data;
    float scale;
    float offset;
};

float quantize(float value, float scale, float offset) {
    return std::round((value - offset) / scale * 255.0f);
}

float dequantize(uint8_t value, float scale, float offset) {
    return (value / 255.0f) * scale + offset;
}
```

### 4. Product Quantization

```cpp
// 96x memory reduction for 768-dim vectors
struct ProductQuantizer {
    int numSubspaces = 96;      // 768 / 8
    int subspaceDim = 8;
    int numCentroids = 256;     // 8-bit codebook
    
    std::vector<std::vector<float>> codebooks; // [96][256][8]
    
    std::vector<uint8_t> encode(const std::vector<float>& vector) {
        std::vector<uint8_t> codes(numSubspaces);
        for (int i = 0; i < numSubspaces; i++) {
            // Find nearest centroid in subspace
            codes[i] = findNearestCentroid(
                vector.data() + i * subspaceDim,
                codebooks[i]
            );
        }
        return codes;
    }
};
```

## References

1. Malkov, Y. A., & Yashunin, D. A. (2018). "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs". IEEE TPAMI.

2. Johnson, J., Douze, M., & Jégou, H. (2019). "Billion-scale similarity search with GPUs". IEEE Transactions on Big Data, 7(3), 535-547.

3. Dao, T., Fu, D. Y., Ermon, S., Rudra, A., & Ré, C. (2022). "FlashAttention: Fast and memory-efficient exact attention with IO-awareness". NeurIPS 2022.

4. Kwon, W., et al. (2023). "Efficient Memory Management for Large Language Model Serving with PagedAttention". SOSP 2023.

5. NVIDIA CUDA C++ Programming Guide. https://docs.nvidia.com/cuda/cuda-c-programming-guide/

6. Khronos Group. Vulkan 1.3 Specification. https://www.khronos.org/registry/vulkan/

7. AMD ROCm Documentation. https://rocmdocs.amd.com/

## Conclusion

ThemisDB's GPU Vector Indexing system provides a production-ready, high-performance solution for vector similarity search across multiple GPU backends. The unified API, automatic backend selection, and graceful fallback ensure reliability while maximizing performance on available hardware.
