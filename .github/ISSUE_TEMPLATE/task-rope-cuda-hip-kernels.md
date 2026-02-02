---
name: "RoPE Enhancement: CUDA/HIP Kernels for GPU Acceleration"
about: Implement GPU-accelerated rotation for large batches using CUDA/HIP kernels
title: '[RoPE] GPU-Accelerated Rotation with CUDA/HIP Kernels'
labels: 'enhancement, priority:P2, area:vector-index, component:rotary-embeddings, effort:medium, performance'
assignees: ''
---

## Feature Description

Implement CUDA and HIP kernels to enable GPU-accelerated rotation operations for Rotary Position Embeddings (RoPE). This enhancement will significantly improve throughput for large batch operations and high-dimensional embeddings.

## Problem Statement

Current RoPE implementation runs on CPU with O(d) complexity per rotation. For large batches (>1000 vectors) or high dimensions (>2048), CPU performance becomes a bottleneck. GPU acceleration can provide 10-100x speedup for these workloads.

## Proposed Solution

### Core Components

1. **CUDA Kernel Implementation** (`src/index/rotary_embeddings_cuda.cu`)
   - Parallel rotation kernel for batch operations
   - Optimized memory coalescing and shared memory usage
   - Support for multiple CUDA compute capabilities

2. **HIP Kernel Implementation** (`src/index/rotary_embeddings_hip.cpp`)
   - AMD GPU support via HIP
   - Portable implementation compatible with CUDA kernels
   - Support for ROCm platform

3. **Kernel Launcher** (`include/index/rotary_embeddings_gpu.h`)
   - Unified C++ interface for both CUDA and HIP
   - Automatic device selection and memory management
   - Fallback to CPU for small batches

### Technical Design

```cpp
class RotaryEmbeddingGPU : public RotaryEmbedding {
public:
    explicit RotaryEmbeddingGPU(const RotationConfig& config, GPUBackend backend);
    
    // GPU-accelerated batch rotation
    std::vector<std::vector<float>> rotateBatchGPU(
        const std::vector<std::vector<float>>& embeddings,
        const std::vector<size_t>& positions
    ) const override;
    
    // Streaming API for large datasets
    void rotateBatchStreamGPU(
        const float* d_embeddings,  // Device memory
        const size_t* d_positions,
        float* d_output,
        size_t batch_size,
        cudaStream_t stream = 0
    ) const;
    
private:
    GPUBackend backend_;  // CUDA or HIP
    void* gpu_theta_cache_;  // Device memory for theta cache
};
```

### Kernel Pseudocode

```cuda
__global__ void rotateKernel(
    const float* embeddings,
    const size_t* positions,
    const double* theta_cache,
    float* output,
    size_t batch_size,
    size_t hidden_dim,
    size_t num_pairs
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= batch_size * num_pairs) return;
    
    int batch_idx = idx / num_pairs;
    int pair_idx = idx % num_pairs;
    
    size_t pos = positions[batch_idx];
    double theta = theta_cache[pair_idx];
    double angle = pos * theta;
    
    float cos_val = cosf(angle);
    float sin_val = sinf(angle);
    
    int offset = batch_idx * hidden_dim + pair_idx * 2;
    float x = embeddings[offset];
    float y = embeddings[offset + 1];
    
    output[offset] = x * cos_val - y * sin_val;
    output[offset + 1] = x * sin_val + y * cos_val;
}
```

## Performance Targets

| Operation | Batch Size | CPU Latency | GPU Target | Speedup |
|-----------|------------|-------------|------------|---------|
| Rotation | 100 | ~200 µs | ~10 µs | 20x |
| Rotation | 1,000 | ~2 ms | ~50 µs | 40x |
| Rotation | 10,000 | ~20 ms | ~200 µs | 100x |

**Memory Overhead:**
- Theta cache: ~512 bytes (for 768-dim embeddings)
- Batch buffer: `batch_size * hidden_dim * sizeof(float)`

## Implementation Considerations

### Dependencies
- CUDA Toolkit 11.0+ or ROCm 5.0+ (HIP)
- CMake CUDA/HIP language support
- vcpkg or system-installed GPU libraries

### CMakeLists.txt Changes
```cmake
if(THEMIS_ENABLE_CUDA)
    enable_language(CUDA)
    add_library(rotary_embeddings_cuda
        src/index/rotary_embeddings_cuda.cu
    )
    target_compile_options(rotary_embeddings_cuda PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:-arch=sm_70>  # Volta+
    )
endif()

if(THEMIS_ENABLE_HIP)
    find_package(hip REQUIRED)
    add_library(rotary_embeddings_hip
        src/index/rotary_embeddings_hip.cpp
    )
    target_link_libraries(rotary_embeddings_hip PRIVATE hip::host)
endif()
```

### Automatic Fallback Strategy
1. Try GPU if available and batch_size > threshold (default: 100)
2. Fall back to CPU for small batches (overhead not justified)
3. Handle GPU OOM gracefully with CPU fallback

### Testing Requirements
- Unit tests with mock GPU or CPU fallback
- Correctness validation against CPU implementation
- Performance benchmarks on different GPUs (V100, A100, MI100)
- Multi-GPU support (optional)

## Use Cases

1. **Large-Scale Document Ingestion**: Rotate 10,000+ document embeddings per second
2. **Real-Time Knowledge Graph Updates**: Process 1,000+ entity embeddings in <100ms
3. **Temporal Data Streams**: Handle high-throughput time-series embedding rotation

## Example Usage

```cpp
// Enable GPU acceleration
RotationConfig config;
config.hidden_dim = 768;
config.num_rotation_pairs = 384;
config.computeThetaCache();

auto gpu_rope = std::make_unique<RotaryEmbeddingGPU>(config, GPUBackend::CUDA);

// Batch rotation with GPU
std::vector<std::vector<float>> large_batch(10000, std::vector<float>(768));
std::vector<size_t> positions(10000);
std::iota(positions.begin(), positions.end(), 0);

auto rotated = gpu_rope->rotateBatchGPU(large_batch, positions);
// ~200 µs instead of ~20 ms on CPU
```

## Alternative Solutions

1. **OpenCL**: More portable but lower performance than CUDA/HIP
2. **Vulkan Compute**: Gaming GPU support but less mature ecosystem
3. **CPU SIMD**: AVX-512 can achieve 4-8x speedup without GPU

## Related Features

- Vector Index GPU Acceleration ([#existing_issue])
- HNSW GPU Build ([#existing_issue])
- LoRA Training GPU Support ([#existing_issue])

## Additional Context

**References:**
- NVIDIA CUDA Best Practices: https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/
- AMD ROCm HIP Guide: https://rocmdocs.amd.com/en/latest/Programming_Guides/HIP-GUIDE.html
- ThemisDB GPU Backend Registry: `src/acceleration/backend_registry.cpp`

**Priority:** P2 (Medium) - Nice to have for large-scale deployments  
**Effort:** 2-3 weeks  
**Complexity:** Medium (requires GPU expertise)

---

**Checklist:**
- [ ] I have searched existing issues to ensure this is not a duplicate
- [ ] I have clearly described the problem this feature solves
- [ ] I have provided a detailed description of the proposed solution
- [ ] I have considered the impact on existing functionality
- [ ] I have identified performance targets and benchmarks
