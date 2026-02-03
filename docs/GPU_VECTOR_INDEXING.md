# GPU Vector Indexing Implementation

## Overview

ThemisDB's GPU Vector Indexing provides high-performance vector similarity search across multiple GPU backends:

- **Vulkan**: Cross-platform GPU acceleration (Windows, Linux, macOS, Android)
- **CUDA**: NVIDIA GPU acceleration with advanced optimizations
- **HIP**: AMD ROCm acceleration for AMD GPUs

## Architecture

### Unified Interface

The `GPUVectorIndex` class provides a unified API for vector operations across all backends:

```cpp
#include "index/gpu_vector_index.h"

using namespace themis::index;

// Create index with automatic backend selection
GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::AUTO;
config.metric = GPUVectorIndex::DistanceMetric::COSINE;

GPUVectorIndex index(config);
index.initialize(128); // 128-dimensional vectors

// Add vectors
index.addVector("id1", vector1);
index.addVectorBatch(ids, vectors);

// Search
auto results = index.search(query, 10); // Top-10 nearest neighbors
```

### Backend Selection

The system automatically selects the best available backend:

1. **Vulkan** (if available and enabled)
2. **CUDA** (if NVIDIA GPU available)
3. **HIP** (if AMD GPU available)
4. **CPU** (fallback)

You can also manually select a specific backend:

```cpp
config.backend = GPUVectorIndex::Backend::CUDA;
```

### Distance Metrics

Three distance metrics are supported:

- **L2 (Euclidean)**: `||a - b||²`
- **Cosine**: `1 - (a·b)/(||a|| ||b||)`
- **Inner Product**: `max(0, -a·b)`

## Features

### Vulkan Backend

- Cross-platform compute shaders
- Efficient memory management with Vulkan buffers
- Multi-GPU support via device selection
- Zero-copy transfers where possible

### CUDA Backend

- **Mixed Precision**: FP16/TF32/INT8 support
- **Tensor Core Acceleration**: Automatic on Ampere+ GPUs
- **Flash Attention Optimization**: Tiled computation for memory efficiency
- **Memory Coalescing**: Optimized for sequential access
- **Unified Memory**: Optional for large datasets
- **Graph Execution**: Kernel fusion for better performance

### HIP Backend

- **rocBLAS Integration**: Optimized GEMM operations
- **RDNA Optimization**: Wave32/Wave64 kernel tuning
- **Shared Memory**: Efficient use of LDS (Local Data Share)
- **RCCL Support**: Multi-GPU collective operations
- **Device-to-Device Transfers**: Minimal host involvement

## Build Configuration

### CMake Options

```bash
cmake -DTHEMIS_ENABLE_GPU=ON \
      -DTHEMIS_ENABLE_VULKAN=ON \
      -DTHEMIS_ENABLE_CUDA=ON \
      -DTHEMIS_ENABLE_HIP=ON \
      -DTHEMIS_ENABLE_VECTOR_SEARCH=ON
```

### Individual Backend Control

```bash
# Vulkan only
cmake -DTHEMIS_ENABLE_GPU=ON -DTHEMIS_ENABLE_VULKAN=ON

# CUDA only
cmake -DTHEMIS_ENABLE_GPU=ON -DTHEMIS_ENABLE_CUDA=ON

# HIP only
cmake -DTHEMIS_ENABLE_GPU=ON -DTHEMIS_ENABLE_HIP=ON
```

### Requirements

**Vulkan:**
- Vulkan SDK 1.2+
- Vulkan-capable GPU driver

**CUDA:**
- CUDA Toolkit 11.0+
- NVIDIA GPU with Compute Capability 6.0+ (Pascal or newer)

**HIP:**
- ROCm 5.0+
- AMD GPU with GCN 4.0+ or RDNA architecture

## Performance

### Expected Throughput

| Backend | GPU | Vectors | Dimension | QPS (Queries/Sec) |
|---------|-----|---------|-----------|-------------------|
| Vulkan  | RTX 3080 | 1M | 128 | 45,000+ |
| CUDA    | RTX 3080 | 1M | 128 | 60,000+ |
| HIP     | RX 6800 XT | 1M | 128 | 50,000+ |
| CPU     | Ryzen 5950X | 1M | 128 | 5,000+ |

### Memory Usage

| Vectors | Dimension | VRAM Usage |
|---------|-----------|------------|
| 100K | 128 | ~50 MB |
| 1M | 128 | ~500 MB |
| 10M | 128 | ~5 GB |
| 100M | 128 | ~50 GB |

### Optimization Tips

1. **Batch Queries**: Use `searchBatch()` instead of multiple `search()` calls
2. **Mixed Precision**: Enable for 2-3x speedup with minimal accuracy loss
3. **Pre-allocation**: Build index once, reuse for multiple searches
4. **GPU Selection**: Use dedicated GPU, avoid integrated graphics
5. **Memory Coalescing**: Ensure vectors are contiguous in memory

## API Reference

### GPUVectorIndex

#### Constructor

```cpp
GPUVectorIndex(const Config& config = Config{});
```

#### Configuration

```cpp
struct Config {
    Backend backend = Backend::AUTO;
    DistanceMetric metric = DistanceMetric::COSINE;
    int M = 16;                    // HNSW connections per layer
    int efConstruction = 200;      // Construction accuracy
    int efSearch = 64;             // Query accuracy
    int batchSize = 512;           // Batch size
    size_t maxVRAM_MB = 8192;      // Max VRAM
    int deviceId = 0;              // GPU device ID
    bool enableMultiGPU = false;   // Multi-GPU support
    bool useMixedPrecision = true; // FP16/TF32
    bool allowCPUFallback = true;  // CPU fallback
};
```

#### Methods

```cpp
// Initialization
bool initialize(int dimension);
void shutdown();

// Vector operations
bool addVector(const std::string& id, const std::vector<float>& vector);
bool addVectorBatch(const std::vector<std::string>& ids, 
                   const std::vector<std::vector<float>>& vectors);
bool removeVector(const std::string& id);
bool updateVector(const std::string& id, const std::vector<float>& vector);

// Search operations
std::vector<SearchResult> search(const std::vector<float>& query, size_t k);
std::vector<std::vector<SearchResult>> searchBatch(
    const std::vector<std::vector<float>>& queries, size_t k);

// Index management
bool buildIndex();
bool saveIndex(const std::string& path);
bool loadIndex(const std::string& path);

// Configuration
void setEfSearch(int ef);
void setBatchSize(int size);
Backend getActiveBackend() const;
Statistics getStatistics() const;

// Backend control
bool switchBackend(Backend backend);
std::vector<Backend> getAvailableBackends() const;
```

### Backend-Specific Features

#### CUDA Backend

```cpp
CUDAVectorIndexBackend backend;

// Mixed precision
backend.enableMixedPrecision(true, true, false); // FP16, TF32, no INT8

// Flash Attention optimization
backend.enableFlashAttentionOptimization(true);

// Tensor Cores
if (backend.hasTensorCoreSupport()) {
    backend.enableTensorCores(true);
}

// Unified memory (for datasets larger than VRAM)
backend.enableUnifiedMemory(true);
```

#### HIP Backend

```cpp
HIPVectorIndexBackend backend;

// rocBLAS for GEMM
backend.enableRocBLAS(true);

// Architecture-specific optimization
backend.optimizeForRDNA3();

// Wave size tuning
backend.setWaveSize(32); // or 64

// Multi-GPU (RCCL)
backend.enableRCCL(4); // 4 GPUs
```

#### Vulkan Backend

```cpp
VulkanVectorIndexBackend backend;

// Multi-GPU support
backend.enableMultiGPU(2); // 2 GPUs

// Load distribution
backend.distributeLoad(vectors, deviceId);
```

## Troubleshooting

### Common Issues

**"Backend not available"**
- Ensure GPU drivers are installed and up-to-date
- Check that the backend is enabled in CMake configuration
- Verify GPU is not in use by another process

**"Out of memory"**
- Reduce `maxVRAM_MB` in config
- Enable `useUnifiedMemory` (CUDA only)
- Use smaller batch sizes
- Consider CPU fallback

**"Slow performance"**
- Check GPU utilization with `nvidia-smi` or `rocm-smi`
- Enable mixed precision
- Use batch search instead of individual queries
- Verify GPU is not thermal throttling

### Debugging

Enable verbose logging:

```cpp
// Set log level before initialization
std::cout << "Available backends: ";
for (auto backend : index.getAvailableBackends()) {
    std::cout << static_cast<int>(backend) << " ";
}
std::cout << std::endl;

// Check statistics
auto stats = index.getStatistics();
std::cout << "Active backend: " << static_cast<int>(stats.activeBackend) << std::endl;
std::cout << "VRAM usage: " << (stats.vramUsageBytes / (1024*1024)) << " MB" << std::endl;
std::cout << "Avg query time: " << stats.avgQueryTimeMs << " ms" << std::endl;
std::cout << "Throughput: " << stats.throughputQPS << " QPS" << std::endl;
```

## Examples

### Basic Usage

```cpp
#include "index/gpu_vector_index.h"

int main() {
    // Initialize index
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::AUTO;
    config.metric = GPUVectorIndex::DistanceMetric::COSINE;
    
    GPUVectorIndex index(config);
    index.initialize(128);
    
    // Add vectors
    std::vector<float> vec(128, 0.5f);
    index.addVector("doc1", vec);
    
    // Search
    std::vector<float> query(128, 0.3f);
    auto results = index.search(query, 10);
    
    for (const auto& result : results) {
        std::cout << "ID: " << result.id 
                  << ", Distance: " << result.distance << std::endl;
    }
    
    index.shutdown();
    return 0;
}
```

### Batch Processing

```cpp
// Prepare batch of queries
std::vector<std::vector<float>> queries;
for (int i = 0; i < 100; ++i) {
    std::vector<float> query(128);
    // ... fill query
    queries.push_back(query);
}

// Batch search (much faster than individual searches)
auto batchResults = index.searchBatch(queries, 10);

for (size_t i = 0; i < batchResults.size(); ++i) {
    std::cout << "Query " << i << " results:\n";
    for (const auto& result : batchResults[i]) {
        std::cout << "  " << result.id << ": " << result.distance << "\n";
    }
}
```

## References

- HNSW Algorithm: Malkov & Yashunin (2018) - IEEE TPAMI
- FAISS GPU: Johnson et al. (2019) - IEEE Transactions on Big Data
- Flash Attention: Dao et al. (2022) - NeurIPS
- vLLM Paged Attention: Kwon et al. (2023) - SOSP
- Vulkan Compute: Khronos Vulkan Specification 1.3
- ROCm Documentation: https://rocmdocs.amd.com/
- CUDA Best Practices: https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/
