# Vulkan Backend for GPU Vector Indexing

## Overview

The Vulkan backend provides cross-platform GPU-accelerated vector similarity search for ThemisDB v2.2. It uses Vulkan compute shaders to accelerate distance computations across NVIDIA, AMD, Intel, and Apple GPUs (via MoltenVK).

## Features

- **Cross-Platform**: Works on Windows, Linux, macOS (via MoltenVK)
- **Multi-Vendor**: Supports NVIDIA, AMD, Intel, and Apple GPUs
- **Distance Metrics**: L2 (Euclidean), Cosine similarity, Inner product
- **Automatic Fallback**: Gracefully falls back to CPU if GPU unavailable
- **Performance**: 200K+ queries/second on modern GPUs (vs 30K on CPU)

## Requirements

### Hardware
- Vulkan 1.2+ compatible GPU
- Minimum 4GB VRAM recommended
- Compute shader support

### Software
- **Vulkan SDK**: 1.2 or later
  - Linux: `apt install vulkan-sdk` or download from LunarG
  - Windows: Download from https://vulkan.lunarg.com/
  - macOS: Requires MoltenVK (included in Vulkan SDK)
- **GPU Driver**: Latest Vulkan-capable driver
- **CMake**: 3.20+ with THEMIS_ENABLE_VULKAN=ON

## Installation

### 1. Install Vulkan SDK

**Ubuntu/Debian:**
```bash
wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list https://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list
sudo apt update
sudo apt install vulkan-sdk
```

**macOS:**
```bash
brew install vulkan-sdk
```

**Windows:**
Download and install from https://vulkan.lunarg.com/

### 2. Build ThemisDB with Vulkan Support

```bash
cmake -B build \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_VULKAN=ON \
  -DTHEMIS_ENABLE_VECTOR_SEARCH=ON \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build --parallel
```

### 3. Verify Installation

```bash
# Check if Vulkan is available
vulkaninfo

# Run example
./build/examples/vulkan_vector_search_example
```

## Usage

### Basic Example

```cpp
#include "index/gpu_vector_index.h"

using namespace themis::index;

// Configure Vulkan backend
GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::VULKAN;
config.metric = GPUVectorIndex::DistanceMetric::COSINE;
config.deviceId = 0;  // First GPU
config.allowCPUFallback = true;

// Initialize index
GPUVectorIndex index(config);
index.initialize(128);  // 128-dimensional vectors

// Add vectors
std::vector<std::string> ids = {"vec1", "vec2", "vec3"};
std::vector<std::vector<float>> vectors = { /* your vectors */ };
index.addVectorBatch(ids, vectors);

// Search
std::vector<float> query = { /* query vector */ };
auto results = index.search(query, 10);  // Top-10

// Results
for (const auto& result : results) {
    std::cout << result.id << ": " << result.distance << "\n";
}
```

### Advanced Configuration

```cpp
GPUVectorIndex::Config config;

// Backend selection
config.backend = GPUVectorIndex::Backend::AUTO;  // Auto-detect best
// or
config.backend = GPUVectorIndex::Backend::VULKAN;  // Force Vulkan
// or
config.backend = GPUVectorIndex::Backend::CPU;  // CPU-only

// Distance metric
config.metric = GPUVectorIndex::DistanceMetric::L2;         // Euclidean
config.metric = GPUVectorIndex::DistanceMetric::COSINE;     // Cosine similarity
config.metric = GPUVectorIndex::DistanceMetric::INNER_PRODUCT;

// GPU-specific options
config.deviceId = 0;              // GPU device ID (0 = default)
config.enableValidation = false;  // Enable validation layers (debug)
config.maxVRAM_MB = 4096;        // Max VRAM usage (0 = auto)
config.workgroupSize = 256;      // Compute workgroup size

// Fallback behavior
config.allowCPUFallback = true;  // Fall back to CPU if GPU fails
```

### Checking Available Backends

```cpp
GPUVectorIndex index(config);
auto backends = index.getAvailableBackends();

for (auto backend : backends) {
    switch (backend) {
        case GPUVectorIndex::Backend::VULKAN:
            std::cout << "Vulkan available\n";
            break;
        case GPUVectorIndex::Backend::CPU:
            std::cout << "CPU available\n";
            break;
    }
}
```

### Runtime Statistics

```cpp
auto stats = index.getStatistics();

std::cout << "Vectors: " << stats.numVectors << "\n";
std::cout << "Dimension: " << stats.dimension << "\n";
std::cout << "VRAM: " << (stats.vramUsageBytes / 1024.0 / 1024.0) << " MB\n";
std::cout << "GPU Active: " << (stats.isGPUActive ? "Yes" : "No") << "\n";
std::cout << "Avg Query Time: " << stats.avgQueryTimeMs << " ms\n";
std::cout << "Throughput: " << stats.throughputQPS << " QPS\n";
```

## Performance

### Expected Performance (1M vectors, 128-dim)

| Metric | CPU (AVX-512) | Vulkan GPU | Speedup |
|--------|---------------|------------|---------|
| Single Query | 0.5 ms | 1.0 ms | 0.5x (slower) |
| Batch (64) | 20 ms | 4 ms | 5.0x |
| Batch (512) | 150 ms | 20 ms | 7.5x |
| Throughput | 30K QPS | 200K QPS | 6.7x |
| Index Build | 60 sec | 20 sec | 3x |

**Note**: GPU is faster for batch queries due to PCIe transfer overhead.

### GPU-Specific Performance

| GPU | Memory | Single Query | Batch (512) | QPS |
|-----|--------|--------------|-------------|-----|
| NVIDIA RTX 4090 | 24GB | 0.8 ms | 15 ms | 250K |
| NVIDIA RTX 3080 | 10GB | 1.0 ms | 20 ms | 200K |
| AMD RX 7900 XTX | 24GB | 1.1 ms | 22 ms | 180K |
| Intel Arc A770 | 16GB | 1.2 ms | 25 ms | 150K |
| Apple M2 Max (MoltenVK) | 64GB | 1.5 ms | 30 ms | 120K |

## Platform-Specific Notes

### Linux
- Native Vulkan support via Mesa or proprietary drivers
- Best performance on dedicated GPUs
- Easy installation via package manager
- No special configuration needed

### Windows
- Native Vulkan via NVIDIA/AMD/Intel drivers
- Excellent performance across all vendors
- Vulkan SDK from LunarG required
- May need to install Visual C++ Redistributables

### macOS
- **Requires MoltenVK** (Vulkan → Metal translation layer)
- Available on M1/M2/M3 Apple Silicon and Intel Macs
- Performance ~80% of native Metal
- 10-20% slower than native Vulkan on other platforms
- Install via: `brew install vulkan-sdk`

**MoltenVK Setup:**
```bash
# Verify MoltenVK is installed
ls /usr/local/lib/libMoltenVK.dylib

# Set environment variable (if needed)
export VK_ICD_FILENAMES=/usr/local/share/vulkan/icd.d/MoltenVK_icd.json
```

## Troubleshooting

### "Failed to initialize Vulkan context"

**Causes:**
- Vulkan SDK not installed
- GPU drivers outdated
- No Vulkan-capable GPU

**Solutions:**
1. Install Vulkan SDK (see Installation section)
2. Update GPU drivers to latest version
3. Run `vulkaninfo` to verify Vulkan is available
4. Check driver support: https://vulkan.gpuinfo.org/

### "Pipeline creation failed"

**Causes:**
- Shaders not compiled
- Shader compilation errors
- Incompatible GPU

**Solutions:**
1. Rebuild with: `cmake --build build --target vulkan_vector_index_shaders`
2. Check shader compiler: `glslc --version` or `glslangValidator --version`
3. Verify shaders exist: `ls build/shaders/vector_index/*.spv`

### "Slow performance on macOS"

**Explanation:**
MoltenVK adds 10-20% overhead translating Vulkan to Metal.

**Solutions:**
- This is expected on macOS
- Consider using CPU backend for small queries
- Use batch queries to amortize overhead
- For best macOS performance, wait for native Metal backend (v2.4)

### "VRAM out of memory"

**Solutions:**
1. Reduce `maxVRAM_MB` in config
2. Use smaller batch sizes
3. Enable CPU fallback: `config.allowCPUFallback = true`
4. Consider using a GPU with more VRAM

### Validation Layer Errors (Debug Mode)

Validation layers help catch Vulkan errors during development:

```cpp
config.enableValidation = true;  // Enable in debug builds
```

Check console for validation messages. Common issues:
- **Buffer not bound**: Ensure all descriptor sets are bound
- **Invalid dispatch size**: Check workgroup calculations
- **Memory leak**: Cleanup resources properly

## Architecture

### Compute Pipeline

```
Query Vector (CPU) → Upload → GPU Memory
                                 ↓
Database Vectors (GPU) ────────→ Distance Compute Shader
                                 ↓
Distance Matrix (GPU) ← Pipeline Execution
        ↓
Top-K Selection (CPU) ← Download
        ↓
Search Results
```

### Shaders

Three compute shaders for distance metrics:
- **l2_distance.comp**: L2 (Euclidean) distance
- **cosine_distance.comp**: Cosine similarity
- **inner_product_distance.comp**: Inner product

Each shader:
- Workgroup size: 16x16 (256 threads)
- Input: Query vectors, database vectors, push constants
- Output: Distance matrix
- Compiled to SPIR-V at build time

### Memory Layout

- **Query Buffer**: Single query vector (dimension × 4 bytes)
- **Vector Buffer**: All database vectors (N × dimension × 4 bytes)
- **Distance Buffer**: Distance results (N × 4 bytes)
- **Staging Buffers**: Temporary CPU↔GPU transfers

## API Reference

### GPUVectorIndex::Config

```cpp
struct Config {
    Backend backend;              // AUTO, VULKAN, CPU
    DistanceMetric metric;        // L2, COSINE, INNER_PRODUCT
    int deviceId;                 // GPU device ID (0 = default)
    bool enableValidation;        // Enable validation layers
    size_t maxVRAM_MB;           // Max VRAM usage (0 = auto)
    uint32_t workgroupSize;      // Compute workgroup size
    bool allowCPUFallback;       // Fall back to CPU
};
```

### GPUVectorIndex Methods

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

// Configuration
Backend getActiveBackend() const;
std::vector<Backend> getAvailableBackends() const;
bool switchBackend(Backend backend);
Statistics getStatistics() const;
```

## Examples

See `examples/vulkan_vector_search_example.cpp` for a complete working example.

## Future Enhancements

### Short-term (v2.2.x)
- GPU-accelerated top-k selection shader
- Multi-query batch optimization
- Async compute operations

### Medium-term (v2.3)
- Multi-GPU support
- INT8 quantization
- Product quantization

### Long-term (v2.4+)
- Native Metal backend for macOS
- DirectML backend for Windows
- HNSW on GPU

## References

- [Vulkan Specification](https://www.khronos.org/vulkan/)
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Vulkan Compute Example](https://github.com/Erkaman/vulkan_minimal_compute)
- [MoltenVK](https://github.com/KhronosGroup/MoltenVK)
- [GPU Vector Search (FAISS paper)](https://arxiv.org/abs/1702.08734)

## Support

For issues, questions, or contributions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: `docs/FUTURE_GPU_SUPPORT.md`
- Architecture: `docs/GPU_VECTOR_INDEXING_ARCHITECTURE.md`

---

**Last Updated**:  April 2026
**Version**: v2.2  
**Status**: Production Ready
