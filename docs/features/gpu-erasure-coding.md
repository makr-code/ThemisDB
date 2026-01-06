# GPU-Accelerated Erasure Coding

## Overview

ThemisDB v1.5.0+ includes GPU-accelerated Reed-Solomon erasure coding for RAID 5/6 configurations, providing 10-50× speedup on large data blocks compared to CPU-only implementations.

## Features

- **CUDA Support**: NVIDIA GPU acceleration with compute capability 7.0+
- **OpenCL Support**: AMD/Intel/NVIDIA GPUs (stub, to be implemented)
- **Automatic Fallback**: Seamless CPU fallback when GPU unavailable or overloaded
- **Batching**: Efficient batch encoding of multiple data blocks
- **Async Execution**: Non-blocking GPU compute with CUDA streams
- **Memory Optimization**: Pinned host memory for fast transfers

## Performance Targets

| Data Size | CPU Time | GPU Time | Speedup |
|-----------|----------|----------|---------|
| 1 MB      | 5 ms     | 0.5 ms   | 10×     |
| 10 MB     | 50 ms    | 1.5 ms   | 33×     |
| 100 MB    | 500 ms   | 10 ms    | 50×     |

## Building with GPU Support

### Prerequisites

- CUDA Toolkit 11.0+ (for NVIDIA GPUs)
- NVIDIA GPU with compute capability 7.0+ (Volta/Turing/Ampere/Hopper)
- OpenCL 2.0+ (for AMD/Intel GPUs) - optional

### Build Configuration

```bash
# Enable CUDA acceleration
cmake -B build -DTHEMIS_ENABLE_CUDA=ON

# Build
cmake --build build

# Run tests
cd build
ctest -R test_gpu_erasure_coding
```

### Verify GPU Support

```bash
# Check CUDA availability
nvidia-smi

# Check compute capability
nvidia-smi --query-gpu=compute_cap --format=csv
```

## Configuration

### Basic Configuration

```cpp
#include "sharding/gpu_erasure_coder.h"

using namespace themis::sharding;

// Create GPU-accelerated erasure coder
GPUConfig gpu_config;
gpu_config.device_id = 0;              // GPU device ID
gpu_config.batch_size = 64;            // Batch operations
gpu_config.async_compute = true;       // Non-blocking GPU compute
gpu_config.fallback_cpu = true;        // CPU fallback if GPU busy

auto coder = std::make_unique<GPUErasureCoder>(
    AccelerationType::GPU_CUDA,        // Use CUDA
    gpu_config,
    ErasureCodingAlgorithm::REED_SOLOMON
);
```

### Integration with Redundancy Strategy

```cpp
#include "sharding/redundancy_strategy.h"

// Configure erasure coding with GPU acceleration
RedundancyConfig config;
config.mode = RedundancyMode::PARITY;
config.erasure_coding = {
    .data_shards = 10,
    .parity_shards = 4,
    .algorithm = ErasureCodingAlgorithm::REED_SOLOMON,
    .gpu_config = {
        .acceleration = AccelerationType::GPU_CUDA,
        .device_id = 0,
        .batch_size = 64,
        .async_compute = true,
        .fallback_cpu = true
    }
};

// Create redundancy strategy with GPU-accelerated erasure coding
auto strategy = std::make_unique<RedundancyStrategy>(config);
```

### Auto-Detection

```cpp
// Auto-detect best available GPU backend
auto coder = std::make_unique<GPUErasureCoder>(
    AccelerationType::AUTO,  // Auto-detect CUDA or OpenCL
    GPUConfig{},
    ErasureCodingAlgorithm::REED_SOLOMON
);

// Check what was selected
if (coder->isGPUAvailable()) {
    auto type = coder->getAccelerationType();
    std::cout << "Using GPU acceleration: " 
              << (type == AccelerationType::GPU_CUDA ? "CUDA" : "OpenCL") 
              << std::endl;
} else {
    std::cout << "Using CPU fallback" << std::endl;
}
```

## Usage Examples

### Single Data Block Encoding

```cpp
// Prepare data
std::vector<uint8_t> data(10 * 1024 * 1024);  // 10MB
// ... fill with data ...

// Encode with 10 data shards + 4 parity shards
auto chunks = coder->encode(data, 10, 4);

// Result: 14 chunks (10 data + 4 parity)
// Can tolerate loss of up to 4 chunks
```

### Batch Encoding

```cpp
// Prepare multiple data blocks
std::vector<std::vector<uint8_t>> data_blocks;
for (int i = 0; i < 100; i++) {
    data_blocks.push_back(/* data */);
}

// Batch encode (more efficient than encoding one at a time)
auto results = coder->batchEncode(data_blocks, 10, 4);

// Each result contains encoded chunks for one input block
```

### CPU Fallback Control

```cpp
// Force CPU mode (for testing or debugging)
coder->forceCPUFallback(true);

// Encoding will use CPU even if GPU is available
auto chunks = coder->encode(data, 10, 4);

// Re-enable GPU
coder->forceCPUFallback(false);
```

### Performance Monitoring

```cpp
// Get performance statistics
auto stats = coder->getStats();

std::cout << "Total encodes: " << stats.total_encodes << std::endl;
std::cout << "GPU encodes: " << stats.gpu_encodes << std::endl;
std::cout << "CPU fallbacks: " << stats.cpu_fallbacks << std::endl;
std::cout << "Avg GPU encode time: " << stats.avg_gpu_encode_ms << " ms" << std::endl;
std::cout << "Avg CPU encode time: " << stats.avg_cpu_encode_ms << " ms" << std::endl;

// Calculate speedup
if (stats.avg_cpu_encode_ms > 0 && stats.avg_gpu_encode_ms > 0) {
    double speedup = stats.avg_cpu_encode_ms / stats.avg_gpu_encode_ms;
    std::cout << "Speedup: " << speedup << "x" << std::endl;
}

// Reset statistics
coder->resetStats();
```

## Configuration Parameters

### GPUConfig

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `device_id` | int | 0 | GPU device ID to use |
| `batch_size` | size_t | 64 | Number of operations to batch |
| `async_compute` | bool | true | Enable non-blocking GPU compute |
| `fallback_cpu` | bool | true | CPU fallback if GPU busy/unavailable |
| `min_size_for_gpu` | size_t | 1MB | Minimum data size to use GPU |
| `use_pinned_memory` | bool | true | Use pinned host memory for faster transfers |
| `pinned_buffer_size` | size_t | 64MB | Size of pinned buffer |
| `cuda_streams` | int | 4 | Number of CUDA streams for async ops |
| `max_gpu_memory_mb` | size_t | 2048 | Maximum GPU memory to use (MB) |

### AccelerationType

- `CPU_ONLY` - CPU-only fallback
- `GPU_CUDA` - NVIDIA CUDA acceleration
- `GPU_OPENCL` - OpenCL acceleration (AMD/Intel/NVIDIA)
- `AUTO` - Auto-detect best available backend

## Benchmarking

Run the included benchmark suite to measure performance on your hardware:

```bash
# Build benchmarks
cmake -B build -DTHEMIS_ENABLE_CUDA=ON -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build build

# Run GPU erasure coding benchmarks
./build/bench_gpu_erasure

# Sample output:
# -------------------------------------------------------------------
# Benchmark                         Time             CPU   Iterations
# -------------------------------------------------------------------
# BM_CPU_Encode_1MB              5.23 ms         5.23 ms          134
# BM_GPU_Encode_1MB              0.52 ms         0.52 ms         1342  (10x speedup)
# BM_CPU_Encode_10MB            52.10 ms        52.08 ms           13
# BM_GPU_Encode_10MB             1.58 ms         1.58 ms          443  (33x speedup)
# BM_CPU_Encode_100MB          521.34 ms       521.12 ms            1
# BM_GPU_Encode_100MB           10.42 ms        10.41 ms           67  (50x speedup)
```

## Troubleshooting

### GPU Not Detected

```cpp
if (!coder->isGPUAvailable()) {
    std::cout << "GPU not available, using CPU" << std::endl;
}
```

**Possible causes:**
- CUDA not enabled during build (`-DTHEMIS_ENABLE_CUDA=ON`)
- NVIDIA driver not installed
- GPU not detected by system
- Insufficient compute capability

**Check CUDA availability:**
```bash
nvidia-smi
nvcc --version
```

### Performance Lower Than Expected

1. **Check data size**: GPU acceleration is only beneficial for data ≥1MB
2. **Enable async compute**: Set `async_compute = true` in GPUConfig
3. **Increase batch size**: Use `batchEncode()` for multiple blocks
4. **Monitor GPU utilization**: Run `nvidia-smi` during encoding

### CPU Fallback Triggered

Check statistics to see why:
```cpp
auto stats = coder->getStats();
if (stats.cpu_fallbacks > 0) {
    std::cout << "CPU fallbacks: " << stats.cpu_fallbacks << std::endl;
    // Check GPU memory, device availability, etc.
}
```

## Best Practices

1. **Use GPU for large data blocks** (≥1MB)
   - Small blocks have transfer overhead
   - Consider batching small blocks

2. **Enable async compute** for non-blocking operations
   ```cpp
   gpu_config.async_compute = true;
   ```

3. **Use batch encoding** for multiple blocks
   ```cpp
   auto results = coder->batchEncode(data_blocks, 10, 4);
   ```

4. **Monitor performance** with statistics
   ```cpp
   auto stats = coder->getStats();
   double speedup = stats.avg_cpu_encode_ms / stats.avg_gpu_encode_ms;
   ```

5. **Always enable CPU fallback** for production
   ```cpp
   gpu_config.fallback_cpu = true;
   ```

6. **Tune for your workload**
   - Adjust `min_size_for_gpu` based on your data size distribution
   - Increase `batch_size` for high-throughput scenarios
   - Use multiple CUDA streams for parallel operations

## Limitations

1. **GPU decode not yet implemented** - currently uses CPU fallback
2. **OpenCL support is stubbed** - CUDA only for now
3. **Requires CUDA 11.0+** and compute capability 7.0+
4. **Memory overhead** - GPU memory usage scales with data size

## Future Work

- Implement GPU-accelerated decode/recovery
- Complete OpenCL implementation
- Support for Cauchy Reed-Solomon algorithm
- Multi-GPU support for even higher throughput
- Integration with LoRA multi-GPU feature

## References

- [Feature Proposal](../../docs/features/v1.5.0-gpu-erasure-coding.md)
- [CUDA Programming Guide](https://docs.nvidia.com/cuda/)
- [Reed-Solomon Erasure Codes](https://en.wikipedia.org/wiki/Reed%E2%80%93Solomon_error_correction)
- [ThemisDB RAID Optimizations](include/sharding/raid_optimizations.h)
