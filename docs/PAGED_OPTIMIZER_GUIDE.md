# Paged Optimizers Implementation Guide

## Overview

Paged optimizers enable training of larger models by offloading optimizer states (momentum, variance) between CPU and GPU memory. This reduces peak GPU memory usage by 30-50% while maintaining training accuracy.

**Integration with ThemisDB Infrastructure**: The paged optimizer leverages ThemisDB's existing memory management infrastructure (`VRAMAllocator`, `GPUMemoryManager`) for production-quality memory pooling and multi-backend support (CUDA/HIP/Vulkan/DirectX).

## Architecture

### Components

1. **PagedMemoryManager** - Manages memory paging using existing VRAMAllocator
2. **PagedAdamWOptimizer** - AdamW optimizer with automatic state paging
3. **LRU Cache** - Eviction policy for managing active GPU pages
4. **VRAMAllocator Integration** - Leverages existing <5% overhead memory pools
5. **GPUMemoryManager Integration** - Multi-backend device management

### Memory Flow

```
Training Step:
┌─────────────────────────────────────────┐
│ 1. Pre-Step: Page-in optimizer states  │
│    - VRAMAllocator: CPU → GPU transfer  │
│    - Pinned memory for fast DMA         │
└─────────────────┬───────────────────────┘
                  ▼
┌─────────────────────────────────────────┐
│ 2. Optimizer Step: Update on GPU       │
│    - States already on GPU              │
│    - Fast AdamW update                  │
└─────────────────┬───────────────────────┘
                  ▼
┌─────────────────────────────────────────┐
│ 3. Post-Step: Evict unused states      │
│    - VRAMAllocator: GPU → CPU transfer  │
│    - Free GPU memory for activations    │
└─────────────────────────────────────────┘
```

### Integration Benefits

- ✅ **Multi-Backend Support**: Automatically works with CUDA/HIP/Vulkan/DirectX
- ✅ **Production Memory Management**: <5% overhead via VRAMAllocator
- ✅ **Multi-GPU Ready**: Can leverage GPUMemoryManager's distributed memory
- ✅ **Consistent API**: Uses standard Device type throughout ThemisDB
- ✅ **Less Code**: ~200 lines removed by leveraging existing infrastructure

## Usage

### Basic Example

```cpp
#include "llm/lora_framework/paged_optimizer.h"

// Configure paging
PagedOptimizerConfig config;
config.enable_paging = true;
config.active_set_size = 512;  // Keep 512 states on GPU

// Create paged optimizer
PagedAdamWOptimizer optimizer(
    0.001f,   // learning_rate
    0.9f,     // beta1
    0.999f,   // beta2
    0.01f,    // weight_decay
    config
);

// Register parameters
std::vector<Tensor*> params = model->parameters();
optimizer.add_parameters(params);

// Training loop
for (int epoch = 0; epoch < num_epochs; ++epoch) {
    for (auto& batch : dataloader) {
        // Forward pass
        auto output = model->forward(batch.input);
        auto loss = compute_loss(output, batch.target);
        
        // Backward pass
        loss.backward();
        
        // Optimizer step (automatic paging)
        optimizer.step();
        optimizer.zero_grad();
    }
}

// Check metrics
auto metrics = optimizer.get_metrics();
std::cout << "GPU memory: " << metrics.gpu_memory_used << " bytes\n";
std::cout << "CPU memory: " << metrics.cpu_memory_used << " bytes\n";
std::cout << "Page-ins: " << metrics.num_page_ins << "\n";
std::cout << "Page-outs: " << metrics.num_page_outs << "\n";
```

### Advanced Configuration

```cpp
PagedOptimizerConfig config;

// Enable/disable paging
config.enable_paging = true;

// Page size (larger = fewer transfers, more memory per transfer)
config.page_size_bytes = 64 * 1024 * 1024;  // 64 MB

// Active set size (states to keep on GPU)
config.active_set_size = 1024;

// Prefetch distance (batches ahead)
config.prefetch_distance = 1;

// Unified memory (if supported by GPU)
config.use_unified_memory = false;

// Eviction policy
config.eviction_policy = EvictionPolicy::LRU;  // or LFU, FIFO, ADAPTIVE
```

## Memory Savings

### Expected Savings

| Model      | Standard QLoRA | Paged QLoRA | Savings |
|------------|----------------|-------------|---------|
| Llama-7B   | 5-6 GB        | 4-5 GB      | 20%     |
| Llama-13B  | 9-10 GB       | 7-8 GB      | 22%     |
| Llama-30B  | 20-22 GB      | 15-17 GB    | 25%     |
| Llama-65B  | 40-45 GB      | 30-35 GB    | 25%     |

### Breakdown

```
Memory Usage (Llama-7B):
- Base weights (quantized): 3.5 GB
- LoRA adapters: 0.05 GB
- Optimizer states:
  * Standard: 2.0 GB (on GPU)
  * Paged: 0.5 GB GPU + 1.5 GB CPU
- Activations: ~2.0 GB

Total GPU:
- Standard: ~7.5 GB
- Paged: ~6.0 GB (20% savings)
```

## Performance

### Overhead

With proper configuration:
- **CPU-GPU bandwidth**: PCIe 3.0 (16 GB/s) or better
- **Transfer overhead**: < 10% of training time
- **Throughput**: 90-95% of non-paged optimizer

### Optimization Tips

1. **Active Set Size**: Balance memory vs transfers
   - Larger = less paging, more GPU memory
   - Smaller = more paging, less GPU memory
   - Recommended: 25-50% of total parameters

2. **Prefetching**: Enable async transfers
   - Transfer states while GPU is busy
   - Hides latency of memory transfers
   - Requires fast CPU-GPU link

3. **Batch Size**: Larger batches amortize overhead
   - More computation per optimizer step
   - Reduces relative transfer time
   - Limited by available memory

## Configuration Guidelines

### When to Use Paging

**Use paged optimizers when:**
- Training large models (>30B parameters)
- GPU memory is constrained
- Have fast CPU-GPU interconnect (PCIe 4.0+)
- Willing to accept small performance overhead

**Don't use paging when:**
- Small models (<13B parameters)
- Plenty of GPU memory available
- Need maximum training speed
- Simple setup preferred

### Recommended Settings

#### Small Models (7-13B)
```cpp
config.enable_paging = false;  // Not needed
```

#### Medium Models (13-30B)
```cpp
config.enable_paging = true;
config.active_set_size = 1024;
config.page_size_bytes = 64 * 1024 * 1024;  // 64 MB
```

#### Large Models (30-70B)
```cpp
config.enable_paging = true;
config.active_set_size = 512;   // Smaller for memory
config.page_size_bytes = 128 * 1024 * 1024;  // 128 MB
config.prefetch_distance = 2;    // More prefetching
```

#### Very Large Models (70B+)
```cpp
config.enable_paging = true;
config.active_set_size = 256;   // Minimal GPU footprint
config.page_size_bytes = 128 * 1024 * 1024;
config.prefetch_distance = 2;
config.use_unified_memory = true;  // If supported
```

## Monitoring

### Metrics

```cpp
auto metrics = optimizer.get_metrics();

// Memory usage
std::cout << "GPU memory: " << metrics.gpu_memory_used / (1024*1024) << " MB\n";
std::cout << "CPU memory: " << metrics.cpu_memory_used / (1024*1024) << " MB\n";
std::cout << "Peak GPU: " << metrics.peak_gpu_memory / (1024*1024) << " MB\n";

// Transfer statistics
std::cout << "Page-ins: " << metrics.num_page_ins << "\n";
std::cout << "Page-outs: " << metrics.num_page_outs << "\n";
std::cout << "Bytes transferred: " << metrics.bytes_transferred / (1024*1024) << " MB\n";
std::cout << "Transfer time: " << metrics.transfer_time_ms << " ms\n";
std::cout << "Bandwidth: " << metrics.avg_transfer_bandwidth << " GB/s\n";
```

### Performance Analysis

```cpp
// Reset metrics at start of epoch
optimizer.reset_metrics();

// ... train for one epoch ...

auto metrics = optimizer.get_metrics();

// Calculate overhead
double transfer_overhead = metrics.transfer_time_ms / total_epoch_time_ms * 100.0;
std::cout << "Transfer overhead: " << transfer_overhead << "%\n";

// Check if paging is effective
if (transfer_overhead > 10.0) {
    std::cout << "Warning: High paging overhead. Consider:\n";
    std::cout << "  1. Increase active_set_size\n";
    std::cout << "  2. Increase batch_size\n";
    std::cout << "  3. Check CPU-GPU bandwidth\n";
}
```

## Troubleshooting

### High Overhead (>10%)

**Symptoms:** Training is significantly slower with paging

**Solutions:**
1. Increase `active_set_size` to reduce paging frequency
2. Increase batch size to amortize transfer cost
3. Check CPU-GPU bandwidth (use `nvidia-smi`)
4. Enable prefetching if not already enabled

### Out of Memory (GPU)

**Symptoms:** Still running out of GPU memory

**Solutions:**
1. Decrease `active_set_size` to free GPU memory
2. Enable gradient checkpointing
3. Reduce batch size
4. Consider model parallelism

### Out of Memory (CPU)

**Symptoms:** System runs out of CPU memory

**Solutions:**
1. Increase `active_set_size` (keep more on GPU)
2. Use smaller model
3. Add more system RAM

### Slow Transfers

**Symptoms:** Low bandwidth reported in metrics

**Solutions:**
1. Use pinned memory (automatic if CUDA available)
2. Check PCIe link speed (`lspci -vv`)
3. Verify no thermal throttling
4. Consider NVLink if available

## Implementation Details

### Memory Manager

The `PagedMemoryManager` class handles all memory allocation and transfers:

- **CPU Pool**: Uses pinned (page-locked) memory for fast DMA
- **GPU Pool**: Standard device memory allocation
- **LRU Cache**: Tracks page access patterns for eviction

### Optimizer States

Each parameter has associated optimizer states:

```cpp
struct PagedOptimizerState {
    PagedBuffer momentum;      // First moment (β1)
    PagedBuffer variance;      // Second moment (β2)
    
    bool momentum_on_gpu;      // Current location
    bool variance_on_gpu;
};
```

### Update Algorithm

1. **Ensure states on GPU**: Page in if needed
2. **Compute updates**: Standard AdamW algorithm
3. **Apply updates**: Update parameters in-place
4. **Evict LRU states**: Free GPU memory for next batch

## API Reference

### PagedMemoryManager

```cpp
class PagedMemoryManager {
public:
    // Allocate paged buffer
    PagedBuffer allocate(size_t size, DeviceType device = DeviceType::CPU);
    
    // Deallocate buffer
    void deallocate(PagedBuffer& buffer);
    
    // Page in from CPU to GPU
    bool pageIn(PagedBuffer& buffer, void* stream = nullptr);
    
    // Page out from GPU to CPU
    bool pageOut(PagedBuffer& buffer, void* stream = nullptr);
    
    // Check if on GPU
    bool isOnGPU(const PagedBuffer& buffer) const;
    
    // Evict LRU pages
    size_t evictLRU(size_t num_pages, void* stream = nullptr);
    
    // Memory usage
    size_t gpu_memory_used() const;
    size_t cpu_memory_used() const;
    bool is_cuda_available() const;
};
```

### PagedAdamWOptimizer

```cpp
class PagedAdamWOptimizer {
public:
    // Constructor
    PagedAdamWOptimizer(
        float learning_rate = 1e-3f,
        float beta1 = 0.9f,
        float beta2 = 0.999f,
        float weight_decay = 0.01f,
        const PagedOptimizerConfig& config = PagedOptimizerConfig()
    );
    
    // Register parameters
    void add_parameters(const std::vector<Tensor*>& params);
    
    // Optimization step
    void step();
    
    // Zero gradients
    void zero_grad();
    
    // Getters/Setters
    float learning_rate() const;
    void set_learning_rate(float lr);
    int step_count() const;
    
    // Metrics
    const PagingMetrics& get_metrics() const;
    void reset_metrics();
    
    // Status
    bool is_paging_enabled() const;
    bool is_cuda_available() const;
};
```

## Testing

Run the paged optimizer tests:

```bash
# Build tests
cmake -B build -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_ENABLE_LORA_TESTS=ON
cmake --build build --target test_paged_optimizer

# Run tests
./build/tests/test_paged_optimizer
```

Test coverage:
- Memory manager allocation/deallocation
- Page-in/page-out operations
- LRU eviction
- Optimizer correctness vs non-paged
- Memory savings validation
- Performance overhead measurement
- Edge cases and error handling

## Future Enhancements

### Planned Features

1. **GPU Kernels**: CUDA/HIP kernels for faster updates
2. **Unified Memory**: Automatic paging by CUDA driver
3. **Async Streams**: Multiple concurrent transfers
4. **Adaptive Paging**: Smart prefetching based on access patterns
5. **Multi-GPU**: Distribute optimizer states across GPUs

### Experimental

- **Compression**: Compress states in CPU memory
- **NVMe Offload**: Ultra-large models using SSD storage
- **Mixed Precision States**: FP16 states for 2x memory savings

## References

- QLoRA Paper: https://arxiv.org/abs/2305.14314
- CUDA Unified Memory: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
- Memory Management Best Practices: https://developer.nvidia.com/blog/unified-memory-cuda-beginners/

## Support

For issues or questions:
1. Check the troubleshooting section
2. Review test cases in `tests/test_paged_optimizer.cpp`
3. Consult `QLORA_IMPLEMENTATION_SUMMARY.md`
4. Open an issue on GitHub

---

*Last Updated: January 16, 2026*
