# GPU Module

GPU utility functions and memory management for ThemisDB.

## Components

- **GPU Memory Manager**: Edition-aware GPU VRAM allocation and limits
- **Kernel Management**: GPU kernel loading and execution
- **Edition Constraints**: Runtime enforcement of GPU limits per edition
- **Resource Tracking**: Monitor GPU memory usage and allocation

## Features

### Edition-Based GPU Limits
- **Community Edition**: No GPU support (0 GB VRAM)
- **Professional Edition**: Up to 8 GB VRAM
- **Enterprise Edition**: Up to 24 GB VRAM
- **Unlimited Edition**: No GPU VRAM limits
- **Runtime Enforcement**: Prevents exceeding edition limits

### Memory Management
- **Allocation Tracking**: Track all GPU memory allocations
- **Limit Enforcement**: Reject allocations exceeding limits
- **Memory Statistics**: Monitor usage and available memory
- **Automatic Cleanup**: Free GPU resources on shutdown

### GPU Operations
- **Vector Operations**: GPU-accelerated vector operations
- **Matrix Multiplication**: Fast matrix operations for ML
- **Index Building**: GPU-accelerated index construction
- **Query Processing**: GPU query acceleration (future)

## Architecture

```
GPUModule
├─→ GPUMemoryManager (Edition-aware allocation)
├─→ KernelManager (GPU kernel loading)
├─→ ResourceTracker (Usage monitoring)
└─→ EditionChecker (Runtime limit enforcement)
```

## Use Cases

### Vector Search Acceleration
- Accelerate HNSW index construction
- Fast vector similarity computations
- Batch embedding generation
- Real-time vector search

### ML Model Inference
- GPU-accelerated inference for embeddings
- Model quantization with GPU
- Batch prediction
- LoRA adapter loading

### Index Operations
- Parallel index building
- Concurrent index updates
- Index compression with GPU
- Bulk operations

## Configuration

### GPU Memory Manager
```cpp
#include "themis/gpu/memory_manager.h"

using namespace themis::gpu;

// Get singleton instance
auto& gpu_mgr = GPUMemoryManager::GetInstance();

// Check GPU limits
uint64_t max_vram = GPUMemoryManager::GetMaxGPUVRAMBytes();
std::cout << "Max GPU VRAM: " 
          << (max_vram / (1024*1024*1024)) << " GB" << std::endl;

// Allocate GPU memory
size_t allocation_size = 1024 * 1024 * 1024; // 1 GB
if (gpu_mgr.TryAllocateGPU(allocation_size, "Vector Index")) {
    std::cout << "GPU allocation succeeded" << std::endl;
    // Use GPU memory...
    gpu_mgr.DeallocateGPU(allocation_size);
} else {
    std::cout << "GPU allocation failed (would exceed limit)" << std::endl;
}

// Get allocation statistics
auto stats = gpu_mgr.GetStats();
std::cout << "Allocated: " << (stats.allocated_bytes / (1024*1024)) << " MB" << std::endl;
std::cout << "Peak: " << (stats.peak_bytes / (1024*1024)) << " MB" << std::endl;
```

## Performance Characteristics

- **Allocation overhead**: <1μs per allocation
- **Limit checking**: O(1) constant time
- **Memory tracking**: Minimal overhead
- **Thread safety**: Lock-protected for concurrent access

## Edition Limits

| Edition        | GPU VRAM Limit | Use Case                          |
|----------------|----------------|-----------------------------------|
| Community      | 0 GB           | CPU-only, no GPU support          |
| Professional   | 8 GB           | Small models, limited acceleration|
| Enterprise     | 24 GB          | Medium models, production use     |
| Unlimited      | No limit       | Large models, research            |

## Integration Points

- **LLM Module**: GPU memory for model inference
- **Vector Index**: GPU-accelerated index building
- **Edition Module**: Edition-specific limits
- **Monitoring**: GPU usage metrics

## Thread Safety

- Thread-safe allocation and deallocation
- Mutex-protected statistics
- Safe for concurrent GPU operations

## Dependencies

- **Edition Module**: Edition-specific configuration
- **CUDA/ROCm**: Optional GPU runtime (not required for Community)

## Documentation

For detailed implementation documentation, see:
- [GPU Memory Manager](../../docs/gpu/memory_manager.md)
- [Edition Limits](../../docs/editions/gpu_limits.md)
- [Future Enhancements](FUTURE_ENHANCEMENTS.md)

## Version History

- **v1.0.0**: Edition-aware GPU memory manager
- **v1.1.0**: Planned - CUDA kernel support
- **v1.2.0**: Planned - GPU query acceleration

## Examples

### Check GPU Availability
```cpp
auto& gpu_mgr = GPUMemoryManager::GetInstance();

if (GPUMemoryManager::GetMaxGPUVRAMGB() > 0) {
    std::cout << "GPU support enabled" << std::endl;
} else {
    std::cout << "GPU not available (Community Edition)" << std::endl;
}
```

### Allocate GPU Memory
```cpp
// Try to allocate 2 GB for vector index
size_t size = 2ULL * 1024 * 1024 * 1024;

if (gpu_mgr.TryAllocateGPU(size, "HNSW Index")) {
    // Success - proceed with GPU operations
    buildIndexOnGPU();
    gpu_mgr.DeallocateGPU(size);
} else {
    // Fallback to CPU
    buildIndexOnCPU();
}
```

### Monitor GPU Usage
```cpp
auto stats = gpu_mgr.GetStats();

std::cout << "GPU Memory Usage:" << std::endl;
std::cout << "  Allocated: " << (stats.allocated_bytes >> 30) << " GB" << std::endl;
std::cout << "  Peak: " << (stats.peak_bytes >> 30) << " GB" << std::endl;
std::cout << "  Allocations: " << stats.allocation_count << std::endl;
std::cout << "  Deallocations: " << stats.deallocation_count << std::endl;
```

## Best Practices

### Memory Management
1. **Check limits before allocation**: Use `TryAllocateGPU()` instead of assuming success
2. **Free memory promptly**: Call `DeallocateGPU()` when done
3. **Monitor peak usage**: Track peak usage for capacity planning
4. **Provide descriptive reasons**: Help debug allocation failures

### Edition Awareness
1. **Handle missing GPU gracefully**: Always have CPU fallback
2. **Respect edition limits**: Don't try to circumvent limits
3. **Test on target edition**: Verify behavior on actual edition

## See Also

- [Edition Module](../edition/README.md) - Edition configuration
- [LLM Module](../llm/README.md) - GPU model inference
- [Vector Index](../index/README.md) - GPU-accelerated indexing
