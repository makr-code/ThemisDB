# GPU-Accelerated Rotary Position Embeddings (RoPE)

## Overview

ThemisDB now supports GPU-accelerated rotation operations for Rotary Position Embeddings (RoPE), enabling 10-100x speedup for large batch operations through CUDA and HIP kernels.

## Features

- **CUDA Support**: NVIDIA GPU acceleration via CUDA kernels
- **HIP Support**: AMD GPU acceleration via ROCm HIP
- **Automatic Fallback**: Graceful degradation to CPU when GPU unavailable
- **Batch Optimization**: Configurable threshold for GPU vs CPU selection
- **Streaming API**: Direct device memory access for advanced users

## Performance Targets

| Batch Size | CPU Latency | GPU Target | Speedup |
|------------|-------------|------------|---------|
| 100        | ~200 µs     | ~10 µs     | 20x     |
| 1,000      | ~2 ms       | ~50 µs     | 40x     |
| 10,000     | ~20 ms      | ~200 µs    | 100x    |

## Building with GPU Support

### CUDA (NVIDIA)

```bash
cmake -B build -DTHEMIS_ENABLE_CUDA=ON
cmake --build build
```

### HIP (AMD ROCm)

```bash
cmake -B build -DTHEMIS_ENABLE_HIP=ON
cmake --build build
```

### CPU-Only (No GPU)

```bash
cmake -B build -DTHEMIS_ENABLE_CUDA=OFF -DTHEMIS_ENABLE_HIP=OFF
cmake --build build
```

## Usage Examples

### Basic Usage

```cpp
#include "index/rotary_embeddings_gpu.h"

using namespace themis;

// Configure RoPE
RotationConfig config;
config.hidden_dim = 768;
config.num_rotation_pairs = 384;
config.base_theta = 10000.0;
config.computeThetaCache();

// Create GPU-accelerated instance
auto gpu_rope = std::make_unique<RotaryEmbeddingGPU>(config, GPUBackend::CUDA);

// Check if GPU is available
if (gpu_rope->isGPUAvailable()) {
    std::cout << "GPU acceleration enabled!" << std::endl;
} else {
    std::cout << "Falling back to CPU" << std::endl;
}
```

### Batch Rotation

```cpp
// Prepare large batch of embeddings
size_t batch_size = 1000;
std::vector<std::vector<float>> embeddings(batch_size, std::vector<float>(768));
std::vector<size_t> positions(batch_size);
std::iota(positions.begin(), positions.end(), 0);

// Automatic GPU/CPU selection based on batch size
auto rotated = gpu_rope->rotateBatch(embeddings, positions);
// Uses GPU if batch_size >= threshold (default: 100)
```

### Explicit GPU Usage

```cpp
// Force GPU usage (throws if GPU unavailable)
auto rotated_gpu = gpu_rope->rotateBatchGPU(embeddings, positions);
```

### Configure Batch Threshold

```cpp
// Set custom threshold for GPU activation
gpu_rope->setGPUBatchThreshold(500);  // Use GPU for batches >= 500

// Small batches use CPU automatically
std::vector<std::vector<float>> small_batch(100, std::vector<float>(768));
std::vector<size_t> small_positions(100);
auto result = gpu_rope->rotateBatch(small_batch, small_positions);
// Uses CPU (below threshold)

// Large batches use GPU automatically
std::vector<std::vector<float>> large_batch(1000, std::vector<float>(768));
std::vector<size_t> large_positions(1000);
auto gpu_result = gpu_rope->rotateBatch(large_batch, large_positions);
// Uses GPU (above threshold)
```

### Advanced: Streaming API

For maximum performance with very large datasets, use the streaming API with device memory:

```cpp
// Allocate device memory
size_t batch_size = 10000;
size_t hidden_dim = 768;

float* d_embeddings;
size_t* d_positions;
float* d_output;

cudaMalloc(&d_embeddings, batch_size * hidden_dim * sizeof(float));
cudaMalloc(&d_positions, batch_size * sizeof(size_t));
cudaMalloc(&d_output, batch_size * hidden_dim * sizeof(float));

// Copy data to device
cudaMemcpy(d_embeddings, host_embeddings.data(), 
           batch_size * hidden_dim * sizeof(float), 
           cudaMemcpyHostToDevice);
cudaMemcpy(d_positions, host_positions.data(), 
           batch_size * sizeof(size_t), 
           cudaMemcpyHostToDevice);

// Perform rotation on GPU
gpu_rope->rotateBatchStreamGPU(d_embeddings, d_positions, d_output, batch_size);

// Copy results back
cudaMemcpy(host_output.data(), d_output, 
           batch_size * hidden_dim * sizeof(float), 
           cudaMemcpyDeviceToHost);

// Clean up
cudaFree(d_embeddings);
cudaFree(d_positions);
cudaFree(d_output);
```

## Integration with Vector Index

```cpp
#include "index/vector_index_manager.h"
#include "index/rotary_embeddings_gpu.h"

// Enable rotary embeddings for vector index
RotationConfig config;
config.hidden_dim = 768;
config.num_rotation_pairs = 384;
config.computeThetaCache();

auto vector_mgr = std::make_unique<VectorIndexManager>(*db);
vector_mgr->init("embeddings", 768, VectorIndexManager::Metric::COSINE);
vector_mgr->setRotaryEmbeddingConfig(config);

// Add entity with GPU-accelerated rotation
BaseEntity entity("doc1");
std::vector<float> embedding(768, 1.0f);
entity.setField("embedding", embedding);

// Position-aware rotation
vector_mgr->addEntityWithRotation(entity, "embedding", 42);

// Relational rotation (for knowledge graphs)
vector_mgr->addEntityWithRelationalRotation(entity, "embedding", "parent_of");
```

## Use Cases

### 1. Large-Scale Document Ingestion

```cpp
// Process 10,000+ documents with GPU acceleration
std::vector<std::vector<float>> doc_embeddings = loadDocuments();
std::vector<size_t> positions(doc_embeddings.size());
std::iota(positions.begin(), positions.end(), 0);

auto gpu_rope = std::make_unique<RotaryEmbeddingGPU>(config, GPUBackend::CUDA);
auto rotated = gpu_rope->rotateBatchGPU(doc_embeddings, positions);

// ~200 µs instead of ~20 ms on CPU
```

### 2. Real-Time Knowledge Graph Updates

```cpp
// Update 1,000+ entity embeddings in <100ms
for (const auto& entity : entities) {
    auto embedding = entity.getEmbedding();
    auto rotated = gpu_rope->rotate(embedding, entity.position);
    updateVectorIndex(entity.id, rotated);
}
```

### 3. Temporal Data Streams

```cpp
// Handle high-throughput time-series embedding rotation
while (stream.hasNext()) {
    auto batch = stream.readBatch(1000);
    auto rotated = gpu_rope->rotateBatchGPU(batch.embeddings, batch.timestamps);
    processBatch(rotated);
}
```

## Error Handling

```cpp
try {
    auto gpu_rope = std::make_unique<RotaryEmbeddingGPU>(config, GPUBackend::CUDA);
    
    if (!gpu_rope->isGPUAvailable()) {
        std::cerr << "Warning: GPU not available, using CPU fallback" << std::endl;
    }
    
    // This will throw if GPU is not available
    auto result = gpu_rope->rotateBatchGPU(embeddings, positions);
    
} catch (const std::runtime_error& e) {
    std::cerr << "GPU error: " << e.what() << std::endl;
    // Fall back to CPU implementation
    auto cpu_rope = std::make_unique<RotaryEmbedding>(config);
    auto result = cpu_rope->rotateBatch(embeddings, positions);
}
```

## Requirements

### CUDA (NVIDIA)
- CUDA Toolkit 11.0+
- NVIDIA GPU with compute capability 7.0+ (Volta or newer)
- Driver version 450.80.02+

### HIP (AMD)
- ROCm 5.0+
- AMD GPU supported by ROCm
- Compatible AMD driver

### CPU Fallback
- No additional requirements
- Works on any platform

## Performance Tips

1. **Use GPU for large batches**: GPU overhead makes it less efficient for small batches
2. **Adjust threshold**: Tune `setGPUBatchThreshold()` based on your hardware
3. **Batch operations**: Combine multiple rotations into batches for better GPU utilization
4. **Memory reuse**: The GPU implementation reuses device memory for repeated operations
5. **Streaming API**: Use direct device memory for very large datasets to avoid copy overhead

## Limitations

- GPU memory is limited - very large batches may require chunking
- Small batches (<100) are faster on CPU due to GPU overhead
- Thread safety: Create separate instances for multi-threaded access

## Testing

```bash
# Run GPU tests (requires GPU hardware)
./build/themis_tests --gtest_filter="RotaryEmbeddingGPUTest.*"

# Tests will skip gracefully if GPU unavailable
```

## References

- [RoFormer Paper](https://arxiv.org/abs/2104.09864) - Su et al. (2021)
- [CUDA Programming Guide](https://docs.nvidia.com/cuda/)
- [HIP Programming Guide](https://rocmdocs.amd.com/en/latest/Programming_Guides/HIP-GUIDE.html)
