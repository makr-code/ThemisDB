# ThemisDB - CUDA & GPU Optimization Guide

## Overview

This guide covers GPU optimization patterns for ThemisDB, focusing on CUDA kernel
design, memory hierarchy utilization, and ThemisDB-specific acceleration use cases.

## CUDA Kernel Design Best Practices

### Optimal Block and Grid Sizing

Choose thread block sizes that maximize occupancy:

- **Default:** 256 threads/block for RTX-class GPUs (good register pressure balance)
- **Memory-bound kernels:** 128 threads/block to increase L2 reuse
- **Compute-bound kernels:** 512 threads/block if register count permits

```cuda
// Calculate optimal grid dimensions
int threadsPerBlock = 256;
int blocksPerGrid = (n + threadsPerBlock - 1) / threadsPerBlock;

// Account for multiprocessor count for large workloads
int smCount;
cudaDeviceGetAttribute(&smCount, cudaDevAttrMultiProcessorCount, 0);
blocksPerGrid = std::max(blocksPerGrid, smCount * 4);  // 4 waves minimum

myKernel<<<blocksPerGrid, threadsPerBlock>>>(args...);
```

### Warp-Aware Programming

All 32 threads in a warp execute the same instruction simultaneously. Design
kernels so threads in a warp follow the same execution path:

```cuda
// ✅ Good: All threads in a warp take the same branch
__global__ void processVectors(const float* data, float* out, int n) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= n) return;  // Divergence only at boundary warp

    // Uniform computation for all active threads
    out[tid] = data[tid] * 2.0f;
}

// ❌ Bad: Per-element branching causes warp divergence
__global__ void processWithDivergence(const float* data, float* out, int n) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= n) return;

    // Different threads take different paths → serialized execution
    if (data[tid] > 0.5f) {
        out[tid] = expf(data[tid]);   // Half the warp here
    } else {
        // Note: logf(-data[tid]) is NaN for data[tid] ∈ (0, 0.5].
        // This intentionally unrealistic math keeps the focus on the
        // divergence anti-pattern; do not copy this arithmetic.
        out[tid] = logf(-data[tid]);  // Other half here
    }
}
```

### Divergence Handling

When divergence is unavoidable, minimize its scope:

```cuda
// ✅ Good: Divergent section is short and infrequent
__global__ void classifyVectors(const float* norms, int* labels, int n) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= n) return;

    float norm = norms[tid];
    // Short divergent section — warp reconverges quickly
    int label = (norm > 1.0f) ? 1 : (norm > 0.5f) ? 2 : 0;
    labels[tid] = label;
}
```

## Memory Optimization

### Coalesced Global Memory Access

GPU memory controllers fetch 128-byte cache lines. Ensure adjacent threads
access adjacent memory addresses:

```cuda
// ✅ Good: Thread i accesses data[i] → coalesced (128-byte line serves 32 floats)
__global__ void scaleVectors(float* data, float scale, int n) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) data[tid] *= scale;
}

// ❌ Bad: Strided access pattern causes cache line waste
__global__ void scaleStrided(float* data, float scale, int stride, int n) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    int idx = tid * stride;  // stride > 1 → non-coalesced
    if (idx < n) data[idx] *= scale;
}
```

For AoS (Array of Structs) data, prefer SoA (Struct of Arrays) layout:

```cpp
// ❌ Bad: AoS — per-field access is strided
struct Vector { float x, y, z, w; };
Vector* vectors;  // vectors[i].x, vectors[i+1].x are 16 bytes apart

// ✅ Good: SoA — field arrays are contiguous
float* xs;  // xs[i], xs[i+1] are 4 bytes apart → coalesced
float* ys;
float* zs;
float* ws;
```

### Shared Memory and Bank Conflicts

Shared memory is divided into 32 banks (4-byte wide). Simultaneous access to
the same bank by multiple threads in a warp is serialized:

```cuda
// ❌ Bad: All threads access bank 0 (stride = 32 × sizeof(float))
__shared__ float tile[32][32];
float val = tile[threadIdx.x][0];  // Column 0 → all in bank 0

// ✅ Good: Pad shared memory to avoid bank conflicts
__shared__ float tile[32][33];  // Extra column breaks conflict pattern
float val = tile[threadIdx.x][threadIdx.y];
```

Typical shared memory pattern for matrix/vector tiles:

```cuda
__global__ void vectorSearchKernel(
    const float* __restrict__ queries,   // Global: coalesced read
    const float* __restrict__ vectors,   // Global: texture cache via __ldg
    float* results,                       // Global: coalesced write
    int dimension, int numVectors
) {
    extern __shared__ float sharedQuery[];  // One query per block

    // Cooperatively load query into shared memory
    for (int d = threadIdx.x; d < dimension; d += blockDim.x) {
        sharedQuery[d] = queries[blockIdx.y * dimension + d];
    }
    __syncthreads();

    // Each thread computes dot product for one database vector
    int vid = blockIdx.x * blockDim.x + threadIdx.x;
    if (vid >= numVectors) return;

    float dot = 0.0f;
    for (int d = 0; d < dimension; d++) {
        dot += sharedQuery[d] * __ldg(&vectors[vid * dimension + d]);
    }
    results[blockIdx.y * numVectors + vid] = dot;
}
```

### Global Memory Hierarchy

| Level        | Latency  | Bandwidth      | Use for                      |
|--------------|----------|----------------|------------------------------|
| Registers    | ~1 cycle | N/A            | Per-thread scalars           |
| L1 / Shared  | ~20 cy   | ~100 TB/s      | Frequently reused tile data  |
| L2 Cache     | ~200 cy  | ~7 TB/s        | Cross-SM reuse               |
| Global DRAM  | ~600 cy  | ~1-2 TB/s      | One-time reads/writes        |
| Host (PCIe)  | ~us      | ~64 GB/s       | Infrequent transfers         |

Use `__ldg()` (load through read-only data cache) for read-only arrays:

```cuda
// Read-only vector data benefits from texture cache
float val = __ldg(&vectors[idx]);
```

### Pinned Memory for Host-Device Transfer

Use pinned (page-locked) host memory to enable DMA transfers and overlap
computation with data movement:

```cpp
// ✅ Good: Pinned memory for async transfers
float* hostData;
cudaMallocHost(&hostData, size);  // Page-locked allocation

cudaMemcpyAsync(deviceData, hostData, size,
                cudaMemcpyHostToDevice, stream);

// Launch kernel while transfer completes
processKernel<<<grid, block, 0, stream>>>(deviceData, ...);

cudaStreamSynchronize(stream);
cudaFreeHost(hostData);

// ❌ Bad: Pageable memory forces synchronous transfer
float* pageableData = new float[n];
cudaMemcpy(deviceData, pageableData, size, cudaMemcpyHostToDevice);
```

## SIMD and Vector Operations

### x86 SSE/AVX Intrinsics

Use SIMD for CPU-side preprocessing, distance computations, and result merging:

```cpp
#include <immintrin.h>

// AVX2: Compute dot product of two 256-bit float vectors (8 floats)
float dotProductAVX(const float* a, const float* b, int n) {
    __m256 sum = _mm256_setzero_ps();
    int i = 0;

    // Process 8 floats at a time
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        sum = _mm256_fmadd_ps(va, vb, sum);  // FMA: sum += va * vb
    }

    // Horizontal sum of 8 lanes
    __m128 lo = _mm256_castps256_ps128(sum);
    __m128 hi = _mm256_extractf128_ps(sum, 1);
    lo = _mm_add_ps(lo, hi);
    lo = _mm_hadd_ps(lo, lo);
    lo = _mm_hadd_ps(lo, lo);
    float result = _mm_cvtss_f32(lo);

    // Scalar remainder
    for (; i < n; ++i) result += a[i] * b[i];
    return result;
}
```

### ARM NEON Patterns

```cpp
#include <arm_neon.h>

// NEON: L2 distance between two 128-bit float vectors (4 floats)
float l2DistanceNEON(const float* a, const float* b, int n) {
    float32x4_t sum = vdupq_n_f32(0.0f);
    int i = 0;

    for (; i + 4 <= n; i += 4) {
        float32x4_t va = vld1q_f32(a + i);
        float32x4_t vb = vld1q_f32(b + i);
        float32x4_t diff = vsubq_f32(va, vb);
        sum = vmlaq_f32(sum, diff, diff);  // sum += diff * diff
    }

    // Horizontal sum
    float32x2_t lo = vget_low_f32(sum);
    float32x2_t hi = vget_high_f32(sum);
    float32x2_t pair = vadd_f32(lo, hi);
    float result = vget_lane_f32(vpadd_f32(pair, pair), 0);

    for (; i < n; ++i) {
        float d = a[i] - b[i];
        result += d * d;
    }
    return result;
}
```

### Vectorization Guidelines

- Align data to 32-byte boundaries for AVX2, 16-byte for SSE/NEON
- Prefer `-O3 -march=native` for auto-vectorization; verify with `-fopt-info-vec`
- Use `__builtin_assume_aligned` or `alignas` to hint the compiler:

```cpp
// Hint: pointer is 32-byte aligned → enables AVX auto-vectorization
void processAligned(float* __restrict__ out,
                    const float* __restrict__ in, int n) {
    out = static_cast<float*>(__builtin_assume_aligned(out, 32));
    in  = static_cast<float*>(__builtin_assume_aligned(in,  32));
    for (int i = 0; i < n; ++i) out[i] = in[i] * 2.0f;
}
```

## ThemisDB-Specific GPU Optimizations

### Vector Index Acceleration

HNSW / IVF vector search kernel outline:

```cuda
// Batch cosine similarity: queries[B x D] × vectors[N x D] → scores[B x N]
__global__ void batchCosineSimilarity(
    const float* __restrict__ queries,   // [batchSize × dimension]
    const float* __restrict__ vectors,   // [numVectors × dimension]
    const float* __restrict__ queryNorms,
    const float* __restrict__ vectorNorms,
    float* scores,                        // [batchSize × numVectors]
    int dimension, int numVectors, int batchSize
) {
    int vid = blockIdx.x * blockDim.x + threadIdx.x;
    int qid = blockIdx.y;
    if (vid >= numVectors || qid >= batchSize) return;

    const float* q = queries + qid * dimension;
    const float* v = vectors + vid * dimension;

    float dot = 0.0f;
    // Process 4 elements per iteration; dimension must be padded to a multiple
    // of 4 by the caller (pad with zeros) to avoid out-of-bounds accesses.
    for (int d = 0; d < dimension; d += 4) {
        dot += q[d]   * __ldg(v + d);
        dot += q[d+1] * __ldg(v + d + 1);
        dot += q[d+2] * __ldg(v + d + 2);
        dot += q[d+3] * __ldg(v + d + 3);
    }

    scores[qid * numVectors + vid] =
        dot / (queryNorms[qid] * __ldg(&vectorNorms[vid]) + 1e-8f);
}
```

### Geospatial Distance Kernels

Haversine distance batch computation:

```cuda
__device__ float haversineGPU(float lat1, float lon1,
                               float lat2, float lon2) {
    constexpr float R = 6371000.0f;           // Earth radius in meters
    constexpr float DEG_TO_RAD = M_PI / 180.0f;
    float dlat = (lat2 - lat1) * DEG_TO_RAD;
    float dlon = (lon2 - lon1) * DEG_TO_RAD;
    float a = sinf(dlat * 0.5f) * sinf(dlat * 0.5f)
            + cosf(lat1 * DEG_TO_RAD)
            * cosf(lat2 * DEG_TO_RAD)
            * sinf(dlon * 0.5f) * sinf(dlon * 0.5f);
    return R * 2.0f * asinf(sqrtf(a));
}

__global__ void batchHaversine(
    const float* queryLat, const float* queryLon,
    const float* dbLat,    const float* dbLon,
    float* distances, int numQuery, int numDB
) {
    int did = blockIdx.x * blockDim.x + threadIdx.x;
    int qid = blockIdx.y;
    if (did >= numDB || qid >= numQuery) return;

    distances[qid * numDB + did] =
        haversineGPU(queryLat[qid], queryLon[qid],
                     dbLat[did],    dbLon[did]);
}
```

### Graph Traversal on GPU

BFS/DFS for graph index traversal using frontier-based parallelism:

```cuda
// BFS kernel: one thread per frontier node
__global__ void bfsKernel(
    const int* __restrict__ adjOffsets,  // CSR row offsets
    const int* __restrict__ adjIndices,  // CSR column indices
    const float* __restrict__ edgeWeights,
    int* frontier, int frontierSize,
    int* nextFrontier, int* nextSize,
    float* distances  // initialized to +FLT_MAX; source node set to 0 by caller
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= frontierSize) return;

    int node = frontier[idx];
    int start = adjOffsets[node];
    int end   = adjOffsets[node + 1];

    for (int e = start; e < end; ++e) {
        int neighbor = adjIndices[e];
        float newDist = distances[node] + edgeWeights[e];
        // Atomic min update for non-negative floats only.
        // IEEE 754 positive floats maintain their ordering when reinterpreted
        // as signed 32-bit integers (same bit pattern, same order for >= 0),
        // so atomicMin on int* is safe here.
        // distances[] must be initialized to +FLT_MAX (not negative values).
        if (atomicMin(reinterpret_cast<int*>(&distances[neighbor]),
                      __float_as_int(newDist)) >
            __float_as_int(newDist)) {
            int pos = atomicAdd(nextSize, 1);
            nextFrontier[pos] = neighbor;
        }
    }
}
```

### Batch Query Processing

Use CUDA streams to overlap data transfer and computation for multiple queries:

```cpp
void batchQueryGPU(const QueryBatch& batch, ResultBatch& results) {
    constexpr int NUM_STREAMS = 4;
    cudaStream_t streams[NUM_STREAMS];
    for (auto& s : streams) cudaStreamCreate(&s);

    int chunkSize = (batch.size() + NUM_STREAMS - 1) / NUM_STREAMS;

    for (int i = 0; i < NUM_STREAMS; ++i) {
        int offset = i * chunkSize;
        int count  = std::min(chunkSize, (int)batch.size() - offset);
        if (count <= 0) break;

        // Async transfer
        cudaMemcpyAsync(d_queries[i], batch.data() + offset,
                        count * queryStride_, cudaMemcpyHostToDevice,
                        streams[i]);

        // Launch kernel on this stream
        int blocks = (numVectors_ + 255) / 256;
        searchKernel<<<blocks, 256, sharedMemSize_, streams[i]>>>(
            d_queries[i], d_vectors_, d_results[i], dimension_, count);

        // Async copy back
        cudaMemcpyAsync(results.data() + offset, d_results[i],
                        count * resultStride_, cudaMemcpyDeviceToHost,
                        streams[i]);
    }

    for (auto& s : streams) {
        cudaStreamSynchronize(s);
        cudaStreamDestroy(s);
    }
}
```

## Performance Checklist

- [ ] Block size is multiple of 32 (warp size), typically 128, 256, or 512
- [ ] Global memory accesses are coalesced (adjacent threads → adjacent addresses)
- [ ] Shared memory bank conflicts avoided (pad arrays when needed)
- [ ] Read-only data accessed via `__ldg()` or `const __restrict__`
- [ ] Pinned host memory used for large or frequent H2D/D2H transfers
- [ ] Multiple CUDA streams used for concurrent transfers and kernels
- [ ] Occupancy checked with `cudaOccupancyMaxPotentialBlockSize`
- [ ] Warp divergence minimized within hot loops
- [ ] Kernel profiled with Nsight Compute (see `PERFORMANCE_PROFILING.md`)

## Additional Resources

- Nvidia CUDA Programming Guide: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
- Project Performance Profiling: [PERFORMANCE_PROFILING.md](PERFORMANCE_PROFILING.md)
- Cross-Compilation Context: [CROSS_COMPILATION_CONTEXT.md](CROSS_COMPILATION_CONTEXT.md)
- GPU CI Workflow: `../../.github/workflows/gpu-ci.yml`
