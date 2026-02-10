# QLoRA GPU Kernel Optimization Guide

## Overview

This document describes the GPU kernel optimizations implemented for QLoRA (Quantized Low-Rank Adaptation) in ThemisDB. These optimizations reduce the performance overhead of quantization from 15-20% to less than 5% through CUDA and Vulkan compute shaders.

## Architecture

### GPU Acceleration Stack

```
┌─────────────────────────────────────────────┐
│         Application Layer                   │
│  (LoRA Training, Inference)                 │
└──────────────┬──────────────────────────────┘
               │
┌──────────────┴──────────────────────────────┐
│         Quantization API                    │
│  (NF4, INT8, Block-wise)                    │
└──────────────┬──────────────────────────────┘
               │
       ┌───────┴────────┐
       │                │
┌──────▼──────┐  ┌──────▼──────┐
│ CUDA Kernels│  │Vulkan Shaders│
│  (NVIDIA)   │  │(Cross-platform)│
└─────────────┘  └──────────────┘
```

### Kernel Types

1. **Quantization Kernels**: Convert FP32 → NF4/INT8
2. **Dequantization Kernels**: Convert NF4/INT8 → FP32
3. **Fused Kernels**: Dequantize + MatMul (saves bandwidth)
4. **Mixed Precision**: FP16/BF16 compute for speed

## CUDA Implementation

### File Structure

```
include/llm/lora_framework/
  └── quantization_kernels.h       # CUDA kernel API

src/llm/lora_framework/kernels/
  └── quantization_kernels.cu      # CUDA kernel implementation

tests/
  └── test_qlora_gpu_kernels.cpp   # Correctness tests

benchmarks/
  └── bench_qlora_gpu_kernels.cpp  # Performance benchmarks
```

### NF4 Quantization Kernel

**Algorithm**:
1. Parallel min/max reduction per block
2. Compute scale and zero-point
3. Normalize values to [-1, 1]
4. Find nearest NF4 bin
5. Pack 2 values per byte

**Performance**:
- Block-wise parallelism (64-256 elements/block)
- Shared memory for reduction
- Coalesced memory access
- Target: 5x speedup vs CPU

**Code Example**:
```cpp
#include "llm/lora_framework/quantization_kernels.h"

// Allocate device memory
float* d_input;
uint8_t* d_output;
float* d_scales;
float* d_zeros;

cudaMalloc(&d_input, num_elements * sizeof(float));
cudaMalloc(&d_output, (num_elements + 1) / 2);
cudaMalloc(&d_scales, num_blocks * sizeof(float));
cudaMalloc(&d_zeros, num_blocks * sizeof(float));

// Copy data to GPU
cudaMemcpy(d_input, host_input, num_elements * sizeof(float), 
           cudaMemcpyHostToDevice);

// Launch quantization kernel
cudaError_t err = launch_quantize_nf4_kernel(
    d_input, d_output, d_scales, d_zeros, 
    num_elements, block_size);

// Wait for completion
cudaDeviceSynchronize();
```

### INT8 Quantization Kernel

**Algorithm**:
1. Parallel absmax reduction per block
2. Compute symmetric scale (absmax / 127)
3. Quantize: round(value / scale)
4. Clamp to [-127, 127]

**Performance**:
- Simpler than NF4 (no bin lookup)
- Better accuracy (MSE < 0.0001)
- Target: 5x speedup vs CPU

### Fused Dequantize + MatMul

**Optimization**: On-the-fly dequantization during matrix multiplication saves memory bandwidth by avoiding intermediate storage.

**Memory Savings**:
```
Traditional:  Dequantize → Store → MatMul
              (K*N*4 bytes stored)

Fused:        Dequantize+MatMul (no storage)
              (30% bandwidth reduction)
```

**Code Example**:
```cpp
// Fused kernel performs: output = input @ dequantize(weights)
cudaError_t err = launch_fused_dequant_matmul_kernel(
    quantized_weights,  // NF4 packed weights
    scales,             // Block scales
    zeros,              // Block zeros
    input,              // Input matrix (M, K)
    output,             // Output matrix (M, N)
    M, K, N,           // Matrix dimensions
    block_size,         // Quantization block size
    true               // use_nf4
);
```

### Mixed Precision (FP16)

**Benefit**: Volta+ GPUs have 2-4x faster FP16 compute than FP32.

**Code Example**:
```cpp
// Automatically converts FP32 → FP16 → FP32
cudaError_t err = launch_fp16_matmul_kernel(
    d_A, d_B, d_C,  // FP32 pointers
    M, K, N,
    alpha
);
```

**Compatibility**:
- Volta (V100): 2x speedup
- Ampere (A100): 4x speedup with Tensor Cores
- Ada Lovelace (RTX 4090): 4x speedup

## Vulkan Implementation

### Compute Shaders

**Files**:
```
src/acceleration/vulkan/shaders/lora/
  ├── quantization_nf4.comp      # NF4 quantization
  └── dequantization_nf4.comp    # NF4 dequantization
```

**Advantages**:
- Cross-platform (NVIDIA, AMD, Intel, Apple)
- Works on Windows, Linux, macOS
- Fallback when CUDA unavailable

**Shader Structure**:
```glsl
#version 450
layout(local_size_x = 256) in;

layout(binding = 0) buffer InputBuffer { float input_data[]; };
layout(binding = 1) buffer OutputBuffer { uint output_data[]; };
layout(binding = 2) buffer ScalesBuffer { float scales[]; };
layout(binding = 3) buffer ZerosBuffer { float zeros[]; };

layout(push_constant) uniform PushConstants {
    uint num_elements;
    uint block_size;
};

void main() {
    uint gid = gl_GlobalInvocationID.x;
    // Quantization logic...
}
```

## GPU Memory Manager

### Features

1. **Efficient Allocation**: Pre-allocate GPU buffers
2. **Pinned Host Memory**: Fast CPU↔GPU transfers
3. **Async Transfers**: Overlap compute and I/O
4. **Memory Pooling**: Reuse allocations

### Usage

```cpp
#include "llm/lora_framework/quantization_kernels.h"

GPUMemoryManager manager;

// Allocate quantized buffer (NF4 = 4 bits per value)
void* buffer = manager.allocateQuantizedBuffer(num_params, true);

// Allocate pinned host memory for fast transfers
void* pinned = manager.allocatePinnedHost(size);

// Async transfer to GPU
cudaStream_t stream;
cudaStreamCreate(&stream);
manager.transferToGPUAsync(d_ptr, h_ptr, size, stream);

// Cleanup
manager.freeDevice(buffer);
manager.freePinned(pinned);
```

## Performance Targets

### Quantization (1M parameters)

| Operation           | CPU    | GPU (CUDA) | Speedup |
|---------------------|--------|------------|---------|
| NF4 Quantization    | 10 ms  | 2 ms       | 5x      |
| INT8 Quantization   | 8 ms   | 1.6 ms     | 5x      |
| NF4 Dequantization  | 8 ms   | 1 ms       | 8x      |
| INT8 Dequantization | 6 ms   | 0.8 ms     | 7.5x    |

### Matrix Multiplication (768×768)

| Operation               | Baseline | GPU      | Speedup |
|------------------------|----------|----------|---------|
| FP32 MatMul            | 50 ms    | 10 ms    | 5x      |
| Fused Dequant+MatMul   | 58 ms    | 12 ms    | 4.8x    |
| FP16 MatMul (Ampere+)  | 50 ms    | 5 ms     | 10x     |

### Training Step (End-to-End)

| Scenario          | CPU       | GPU       | Speedup |
|-------------------|-----------|-----------|---------|
| Full Training     | 100-120ms | 30-40ms   | 3-3.3x  |
| Overhead          | 15-20%    | < 5%      | 3-4x    |

## Building with GPU Support

### Prerequisites

**CUDA**:
```bash
# CUDA Toolkit 11.8+ required
# Ubuntu/Debian:
sudo apt install nvidia-cuda-toolkit

# Windows:
# Download from https://developer.nvidia.com/cuda-downloads
```

**Vulkan**:
```bash
# Vulkan SDK 1.3+ required
# Ubuntu/Debian:
sudo apt install vulkan-sdk

# Windows:
# Download from https://vulkan.lunarg.com/
```

### CMake Configuration

```bash
# Enable CUDA support
cmake -B build \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_BUILD_TESTS=ON

# Build
cmake --build build --config Release

# Run tests
./build/tests/test_qlora_gpu_kernels

# Run benchmarks
./build/benchmarks/bench_qlora_gpu_kernels
```

### Windows (MSVC)

```powershell
# Configure
cmake -B build -G "Visual Studio 17 2022" `
  -DTHEMIS_ENABLE_CUDA=ON `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

# Build
cmake --build build --config Release
```

## Testing

### Correctness Tests

```bash
# Run all GPU kernel tests
./build/tests/test_qlora_gpu_kernels

# Specific tests
./build/tests/test_qlora_gpu_kernels --gtest_filter="QLoRAGPUKernels.NF4*"
```

**Test Categories**:
1. **Basic Operations**: Quantize/dequantize correctness
2. **Round-Trip Accuracy**: MSE < threshold
3. **Edge Cases**: Empty, single value, uniform data
4. **Memory Management**: Allocation, transfer, cleanup
5. **Performance**: Speedup vs CPU baseline

### Benchmarks

```bash
# Run all benchmarks
./build/benchmarks/bench_qlora_gpu_kernels

# Specific benchmarks
./build/benchmarks/bench_qlora_gpu_kernels --benchmark_filter="NF4"
```

**Benchmark Suites**:
1. Quantization (1K - 10M parameters)
2. Dequantization (1K - 10M parameters)
3. Fused kernels (256×256 - 1024×1024)
4. Mixed precision (FP16 vs FP32)
5. Memory transfers (async vs sync)

## Integration with Existing Code

### CPU Fallback

The GPU kernels are **optional**. If `THEMIS_ENABLE_CUDA` is not defined, the system automatically falls back to CPU implementation:

```cpp
#ifdef THEMIS_ENABLE_CUDA
    // Use GPU kernels
    launch_quantize_nf4_kernel(...);
#else
    // Use CPU implementation
    quantization::quantize_nf4(...);
#endif
```

### Automatic Selection

Future work will add automatic GPU/CPU selection based on:
- Data size (small batches → CPU, large → GPU)
- GPU availability
- Memory constraints

## Performance Tips

### 1. Batch Operations

**Bad**: Quantize one layer at a time
```cpp
for (auto& layer : layers) {
    quantize_layer(layer);  // GPU launch overhead per layer
}
```

**Good**: Batch multiple layers
```cpp
quantize_model(layers);  // Single GPU kernel launch
```

### 2. Use Async Transfers

**Bad**: Synchronous transfers block CPU
```cpp
cudaMemcpy(d_ptr, h_ptr, size, cudaMemcpyHostToDevice);
kernel<<<...>>>();
cudaDeviceSynchronize();
```

**Good**: Overlap transfers with compute
```cpp
cudaStream_t stream;
cudaStreamCreate(&stream);
cudaMemcpyAsync(d_ptr, h_ptr, size, cudaMemcpyHostToDevice, stream);
kernel<<<..., stream>>>();
cudaStreamSynchronize(stream);
```

### 3. Reuse GPU Memory

**Bad**: Allocate/free every call
```cpp
void quantize(...) {
    float* d_ptr;
    cudaMalloc(&d_ptr, size);
    // ...
    cudaFree(d_ptr);
}
```

**Good**: Use memory manager
```cpp
GPUMemoryManager manager;  // Create once
void* d_ptr = manager.allocateQuantizedBuffer(...);
// Reuse across calls
```

### 4. Choose Right Block Size

**Rule of Thumb**:
- Small models (< 1M params): block_size = 32-64
- Medium models (1M-10M): block_size = 64-128
- Large models (> 10M): block_size = 128-256

Larger blocks = less overhead, slightly lower accuracy.

## Hardware Compatibility

### NVIDIA GPUs

| Architecture | Compute Capability | FP16 | BF16 | TF32 | Notes                  |
|--------------|-------------------|------|------|------|------------------------|
| Pascal       | 6.0-6.2           | ✅   | ❌   | ❌   | Basic FP16             |
| Volta        | 7.0-7.2           | ✅   | ❌   | ❌   | Tensor Cores (FP16)    |
| Turing       | 7.5               | ✅   | ❌   | ❌   | INT8 Tensor Cores      |
| Ampere       | 8.0-8.6           | ✅   | ✅   | ✅   | Best performance       |
| Ada Lovelace | 8.9               | ✅   | ✅   | ✅   | Consumer RTX 4000      |
| Hopper       | 9.0               | ✅   | ✅   | ✅   | H100 (enterprise)      |

**Recommended**: Ampere (A100, RTX 3000) or newer for best performance.

### AMD GPUs (via ROCm/HIP)

Support planned but not yet implemented. Current CUDA kernels can be ported to HIP with minimal changes.

### Intel GPUs (via oneAPI)

Support planned via SYCL/DPC++ for Intel Xe GPUs.

### Apple Silicon (via Metal)

Metal compute shaders can be added similar to Vulkan shaders.

## Troubleshooting

### Common Issues

**1. CUDA out of memory**

**Solution**: Use smaller batch sizes or enable memory pooling:
```cpp
GPUMemoryManager manager;  // Manages memory efficiently
```

**2. Slow performance on small models**

**Solution**: Use CPU for models < 100K parameters (GPU launch overhead dominates).

**3. Incorrect results on older GPUs**

**Solution**: Check compute capability. Pascal (6.0+) minimum required.

**4. Build errors with CUDA**

**Solution**: Ensure CUDA toolkit version matches CMake requirements:
```bash
nvcc --version  # Should be 11.8+
```

## References

- [QLoRA Paper](https://arxiv.org/abs/2305.14314)
- [CUDA Programming Guide](https://docs.nvidia.com/cuda/cuda-c-programming-guide/)
- [Vulkan Compute Tutorial](https://www.khronos.org/vulkan/)
- [Mixed Precision Training](https://arxiv.org/abs/1710.03740)
- [Kernel Fusion Techniques](https://arxiv.org/abs/1904.06697)

## Future Work

### Planned Enhancements

1. **BF16 Support**: For Ampere+ GPUs
2. **Vulkan Pipeline**: Complete C++ wrapper
3. **Multi-GPU**: Distribute quantization across GPUs
4. **Paged Attention**: Integration with vLLM-style paging
5. **Dynamic Batching**: Automatic batch size selection
6. **ROCm/HIP Port**: AMD GPU support
7. **Metal Shaders**: Apple Silicon support

### Performance Goals

- **v1.0 (Current)**: 5x speedup on quantization
- **v2.0 (Planned)**: 10x speedup with Tensor Cores
- **v3.0 (Future)**: 20x speedup with multi-GPU

## Contributing

Contributions welcome! Areas of interest:
- Additional quantization formats (Q4_K_M, Q8_0)
- Optimization for specific GPU architectures
- Cross-platform testing
- Performance profiling

See `CONTRIBUTING.md` for guidelines.

---

*Last updated: January 16, 2026*
*Version: 1.0.0*
