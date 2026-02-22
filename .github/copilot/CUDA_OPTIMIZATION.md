# ThemisDB - CUDA & GPU Optimization Guide

## GPU Architecture Basics

### NVIDIA Terminology

| Term | Description |
|------|-------------|
| **Thread** | Single execution unit; executes one instance of the kernel |
| **Warp** | 32 threads that execute in lock-step (SIMT model) |
| **Block** | Group of threads (up to 1024) sharing Shared Memory and synchronization |
| **Grid** | Collection of blocks dispatched for one kernel launch |
| **SM (Streaming Multiprocessor)** | Physical execution unit; executes multiple warps concurrently |
| **Compute Capability** | Hardware feature version (e.g., 8.6 = Ampere, 9.0 = Hopper) |

### Memory Hierarchy

```
┌──────────────────────────────────────────────┐
│  Global Memory  (DRAM, ~80 GB/s, high latency)│
│  ┌────────────────────────────────────────┐   │
│  │  L2 Cache  (per-device, ~3 MB typical) │   │
│  │  ┌──────────────────────────────────┐  │   │
│  │  │  Shared Memory / L1  (per SM)    │  │   │
│  │  │  ┌────────────────────────────┐  │  │   │
│  │  │  │  Register File (per thread)│  │  │   │
│  │  │  └────────────────────────────┘  │  │   │
│  │  └──────────────────────────────────┘  │   │
│  └────────────────────────────────────────┘   │
└──────────────────────────────────────────────┘
```

| Memory Type | Scope | Latency | Size |
|-------------|-------|---------|------|
| Register | Per thread | ~1 cycle | ~256 KB per SM |
| Shared | Per block | ~5 cycles | 48–96 KB per block |
| L1/Texture Cache | Per SM | ~30 cycles | 32–128 KB per SM |
| L2 Cache | Per device | ~200 cycles | 2–40 MB |
| Global Memory | All threads | ~600 cycles | GBs |
| Constant Memory | Read-only, all threads | ~5 cycles (cached) | 64 KB |

### Compute Capability Impact

```cpp
// Detect compute capability at runtime
cudaDeviceProp prop;
cudaGetDeviceProperties(&prop, 0);
int major = prop.major;  // e.g., 8 for Ampere
int minor = prop.minor;  // e.g., 6 for RTX 3090

// Feature availability:
// >= 7.0 (Volta)  : Tensor Cores, independent thread scheduling
// >= 8.0 (Ampere) : Structured sparsity, TF32, BF16
// >= 9.0 (Hopper) : FP8, Transformer Engine
```

---

## Kernel Design Best Practices

### Optimal Block & Grid Sizes

```cpp
// ✅ Good: 256 threads per block (common optimal choice)
// Range: 64–1024, must be multiple of 32 (warp size)
constexpr int BLOCK_SIZE = 256;

// Grid size: cover all elements with at least 2× SM coverage
int num_sms;
cudaDeviceGetAttribute(&num_sms, cudaDevAttrMultiProcessorCount, 0);
int grid_size = std::max(
    (n + BLOCK_SIZE - 1) / BLOCK_SIZE,
    2 * num_sms  // Minimum occupancy target
);

myKernel<<<grid_size, BLOCK_SIZE>>>(data, n);
```

### Occupancy-Driven Tuning

```cpp
// ✅ Good: Use occupancy API to find optimal block size
int block_size;
int min_grid_size;
cudaOccupancyMaxPotentialBlockSize(
    &min_grid_size, &block_size, myKernel, 0, 0
);

int grid_size = (n + block_size - 1) / block_size;
myKernel<<<grid_size, block_size>>>(data, n);
```

### Shared Memory Usage

```cpp
// ✅ Good: Declare shared memory statically (< 96 KB per block)
__global__ void tiledKernel(const float* __restrict__ input,
                             float* __restrict__ output,
                             int n) {
    __shared__ float tile[BLOCK_SIZE];  // Fast on-chip storage

    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    tile[threadIdx.x] = (idx < n) ? input[idx] : 0.0f;
    __syncthreads();  // Barrier before using shared data

    // Process tile...
    output[idx] = tile[threadIdx.x] * 2.0f;
}

// ✅ Good: Dynamic shared memory when size is runtime-dependent
__global__ void dynamicSharedKernel(const float* input, float* output, int n) {
    extern __shared__ float smem[];  // Allocated at launch time
    // ...
}
// Launch: kernel<<<grid, block, shared_bytes>>>(...)
```

### Bank Conflict Avoidance

```cpp
// ❌ Bad: 32-wide float array → stride-1 access causes bank conflicts
//         when all 32 threads in a warp access the same column
__shared__ float mat_bad[32][32];
float val = mat_bad[threadIdx.y][threadIdx.x];  // OK for row-major

// ✅ Good: Padding avoids bank conflicts on transpose
__shared__ float mat_good[32][33];  // +1 column padding
float val = mat_good[threadIdx.x][threadIdx.y];  // Conflict-free transpose
```

---

## Memory Access Patterns

### Coalesced Global Memory Access

```cpp
// ✅ Good: Consecutive threads access consecutive addresses (128-byte alignment)
// Thread 0 → addr 0, Thread 1 → addr 4, ..., Thread 31 → addr 124
__global__ void coalescedKernel(const float* __restrict__ data, float* out, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        out[idx] = data[idx] * 2.0f;  // ✅ Coalesced
    }
}

// ❌ Bad: Strided access → multiple memory transactions per warp
__global__ void stridedKernel(const float* data, float* out, int n, int stride) {
    int idx = (blockIdx.x * blockDim.x + threadIdx.x) * stride;
    if (idx < n) {
        out[idx] = data[idx];  // ❌ Strided: poor bandwidth utilization
    }
}
```

### Read-Only Data with __restrict__ and __ldg

```cpp
// ✅ Good: Use __restrict__ and __ldg for read-only arrays
__global__ void readOnlyKernel(const float* __restrict__ a,
                                const float* __restrict__ b,
                                float* __restrict__ c, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        // __ldg uses texture/L1 read-only cache path
        c[idx] = __ldg(&a[idx]) + __ldg(&b[idx]);
    }
}
```

### Memory Bandwidth Optimization

```cpp
// ✅ Good: Vectorized loads for higher memory bandwidth
__global__ void vectorizedLoad(const float4* __restrict__ input,
                                float4* __restrict__ output, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n / 4) {
        float4 val = input[idx];  // Load 128 bits in one transaction
        val.x *= 2.0f;
        val.y *= 2.0f;
        val.z *= 2.0f;
        val.w *= 2.0f;
        output[idx] = val;
    }
}
```

---

## SIMD Optimization (x86 & ARM)

### AVX2 (256-bit) for x86

```cpp
#ifdef __AVX2__
#include <immintrin.h>

// Process 8 floats per iteration with AVX2
void addVectorsAVX2(const float* a, const float* b, float* c, int n) {
    int i = 0;
    for (; i <= n - 8; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 vc = _mm256_add_ps(va, vb);
        _mm256_storeu_ps(c + i, vc);
    }
    // Scalar tail
    for (; i < n; ++i) c[i] = a[i] + b[i];
}
#endif
```

### AVX-512 (512-bit) for x86

```cpp
#ifdef __AVX512F__
#include <immintrin.h>

// Process 16 floats per iteration with AVX-512
void addVectorsAVX512(const float* a, const float* b, float* c, int n) {
    int i = 0;
    for (; i <= n - 16; i += 16) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        _mm512_storeu_ps(c + i, _mm512_add_ps(va, vb));
    }
    for (; i < n; ++i) c[i] = a[i] + b[i];
}
#endif
```

### ARM NEON (128-bit)

```cpp
#ifdef __ARM_NEON
#include <arm_neon.h>

// Process 4 floats per iteration with NEON
void addVectorsNEON(const float* a, const float* b, float* c, int n) {
    int i = 0;
    for (; i <= n - 4; i += 4) {
        float32x4_t va = vld1q_f32(a + i);
        float32x4_t vb = vld1q_f32(b + i);
        vst1q_f32(c + i, vaddq_f32(va, vb));
    }
    for (; i < n; ++i) c[i] = a[i] + b[i];
}
#endif
```

### Portable SIMD Fallback Strategy

```cpp
// ✅ Good: Compile-time dispatch with scalar fallback
void addVectors(const float* a, const float* b, float* c, int n) {
#if defined(__AVX512F__)
    addVectorsAVX512(a, b, c, n);
#elif defined(__AVX2__)
    addVectorsAVX2(a, b, c, n);
#elif defined(__ARM_NEON)
    addVectorsNEON(a, b, c, n);
#else
    for (int i = 0; i < n; ++i) c[i] = a[i] + b[i];
#endif
}
```

---

## Atomic Operations

### Use Atomics Only in Critical Sections

```cpp
// ✅ Good: Atomic counter in a reduction scenario
__global__ void countKernel(const int* data, int* count, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n && data[idx] > 0) {
        atomicAdd(count, 1);  // Only when necessary
    }
}

// ✅ Better: Block-level reduction first, then one atomic per block
__global__ void countKernelOptimized(const int* data, int* count, int n) {
    __shared__ int block_count;
    if (threadIdx.x == 0) block_count = 0;
    __syncthreads();

    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n && data[idx] > 0) {
        atomicAdd(&block_count, 1);  // Shared memory atomic (fast)
    }
    __syncthreads();

    if (threadIdx.x == 0) {
        atomicAdd(count, block_count);  // Only one global atomic per block
    }
}
```

### Compare-and-Swap (CAS) Pattern

```cpp
// ✅ Good: Lock-free maximum update using CAS
__device__ void atomicMaxFloat(float* addr, float val) {
    int* addr_as_int = reinterpret_cast<int*>(addr);
    int old_val = *addr_as_int;
    int new_val = __float_as_int(val);
    while (__int_as_float(old_val) < val) {
        int prev = atomicCAS(addr_as_int, old_val, new_val);
        if (prev == old_val) break;
        old_val = prev;
    }
}
```

---

## Code Examples

### Example 1: Vector Addition Kernel (Simple)

```cpp
__global__ void vectorAdd(const float* __restrict__ a,
                           const float* __restrict__ b,
                           float* __restrict__ c, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        c[idx] = a[idx] + b[idx];
    }
}

// Launch helper
void launchVectorAdd(const float* a, const float* b, float* c, int n) {
    constexpr int BLOCK = 256;
    int grid = (n + BLOCK - 1) / BLOCK;
    vectorAdd<<<grid, BLOCK>>>(a, b, c, n);
    cudaCheckError(cudaGetLastError());
}
```

### Example 2: Parallel Reduction Kernel (Intermediate)

```cpp
template <int BLOCK_SIZE>
__global__ void reduceSum(const float* __restrict__ input,
                           float* __restrict__ output, int n) {
    __shared__ float smem[BLOCK_SIZE];

    int tid = threadIdx.x;
    int idx = blockIdx.x * BLOCK_SIZE * 2 + tid;

    // Load two elements and add (reduces global memory transactions)
    float val = 0.0f;
    if (idx < n)          val += input[idx];
    if (idx + BLOCK_SIZE < n) val += input[idx + BLOCK_SIZE];
    smem[tid] = val;
    __syncthreads();

    // Tree reduction in shared memory
    for (int s = BLOCK_SIZE / 2; s > 32; s >>= 1) {
        if (tid < s) smem[tid] += smem[tid + s];
        __syncthreads();
    }
    // Warp-level reduction (no sync needed within a warp)
    if (tid < 32) {
        volatile float* vs = smem;
        vs[tid] += vs[tid + 32];
        vs[tid] += vs[tid + 16];
        vs[tid] += vs[tid + 8];
        vs[tid] += vs[tid + 4];
        vs[tid] += vs[tid + 2];
        vs[tid] += vs[tid + 1];
    }

    if (tid == 0) output[blockIdx.x] = smem[0];
}
```

### Example 3: Custom Operator Kernel (Advanced)

```cpp
// Distance computation kernel for vector similarity search
__global__ void computeL2Distances(
    const float* __restrict__ queries,   // [num_queries × dim]
    const float* __restrict__ database,  // [num_vectors × dim]
    float* __restrict__ distances,       // [num_queries × num_vectors]
    int num_queries, int num_vectors, int dim) {

    int q = blockIdx.y * blockDim.y + threadIdx.y;
    int v = blockIdx.x * blockDim.x + threadIdx.x;

    if (q >= num_queries || v >= num_vectors) return;

    float dist = 0.0f;
    for (int d = 0; d < dim; d++) {
        float diff = queries[q * dim + d] - database[v * dim + d];
        dist += diff * diff;
    }
    distances[q * num_vectors + v] = dist;
}

// ✅ Better: Tiled version to exploit shared memory
__global__ void computeL2DistancesTiled(
    const float* __restrict__ queries,
    const float* __restrict__ database,
    float* __restrict__ distances,
    int num_queries, int num_vectors, int dim) {

    constexpr int TILE = 16;
    __shared__ float q_tile[TILE][TILE];
    __shared__ float d_tile[TILE][TILE];

    int q = blockIdx.y * TILE + threadIdx.y;
    int v = blockIdx.x * TILE + threadIdx.x;
    float dist = 0.0f;

    for (int t = 0; t < (dim + TILE - 1) / TILE; ++t) {
        int col = t * TILE;
        q_tile[threadIdx.y][threadIdx.x] =
            (q < num_queries && col + threadIdx.x < dim)
                ? queries[q * dim + col + threadIdx.x] : 0.0f;
        d_tile[threadIdx.y][threadIdx.x] =
            (v < num_vectors && col + threadIdx.y < dim)
                ? database[v * dim + col + threadIdx.y] : 0.0f;
        __syncthreads();

        for (int k = 0; k < TILE; ++k) {
            float diff = q_tile[threadIdx.y][k] - d_tile[k][threadIdx.x];
            dist += diff * diff;
        }
        __syncthreads();
    }

    if (q < num_queries && v < num_vectors)
        distances[q * num_vectors + v] = dist;
}
```

---

## Common Pitfalls

### 1. Underutilized Warps (Low Occupancy)

```
Problem:  Too few active warps → SM idles during memory latency
Symptom:  Low SM utilization in profiler (< 50%)
Fix:      Increase parallelism (more threads/blocks) or reduce register usage

// ❌ Bad: 32 threads per block = 1 warp → SM is 95% idle
kernel<<<grid, 32>>>(data);

// ✅ Good: 256+ threads per block
kernel<<<grid, 256>>>(data);
```

### 2. Bank Conflicts in Shared Memory

```
Problem:  Multiple threads in a warp access the same shared memory bank
Symptom:  ncu reports "shared memory bank conflicts" > 0
Fix:      Add padding or reorganize data layout (see Bank Conflict section)
```

### 3. Warp Divergence

```cpp
// ❌ Bad: Branches based on thread index → warp divergence
if (threadIdx.x % 2 == 0) {
    result = heavyComputation();  // Even threads compute
} else {
    result = 0.0f;                // Odd threads idle
}

// ✅ Good: Process uniform data; diverge only at boundaries
int idx = blockIdx.x * blockDim.x + threadIdx.x;
if (idx < n) {  // Boundary check only—usually affects just last block
    result = heavyComputation(data[idx]);
}
```

### 4. Global Memory Thrashing

```
Problem:  Repeated global memory accesses to same data without caching
Symptom:  Low L2 cache hit rate in profiler
Fix:      Load data into shared memory once, reuse from there
```

---

## CUDA Error Handling

```cpp
// ✅ Required: Always check CUDA API return codes
#define cudaCheckError(call)                                              \
    do {                                                                   \
        cudaError_t err = (call);                                         \
        if (err != cudaSuccess) {                                         \
            throw std::runtime_error(                                     \
                std::string("CUDA error at ") + __FILE__ + ":" +          \
                std::to_string(__LINE__) + " - " +                        \
                cudaGetErrorString(err));                                  \
        }                                                                  \
    } while (0)

// Usage
cudaCheckError(cudaMalloc(&d_ptr, size));
cudaCheckError(cudaMemcpy(d_ptr, h_ptr, size, cudaMemcpyHostToDevice));
myKernel<<<grid, block>>>(d_ptr, n);
cudaCheckError(cudaGetLastError());      // Catch launch errors
cudaCheckError(cudaDeviceSynchronize()); // Catch async execution errors
```

---

## Additional Resources

- [PERFORMANCE_PROFILING.md](PERFORMANCE_PROFILING.md) - GPU profiling workflow
- [CROSS_COMPILATION_CONTEXT.md](CROSS_COMPILATION_CONTEXT.md) - Platform-specific SIMD flags
- [CODE_STANDARDS.md](CODE_STANDARDS.md) - C++ coding conventions
- NVIDIA CUDA Best Practices Guide: https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/
- NVIDIA Nsight Compute: https://developer.nvidia.com/nsight-compute
