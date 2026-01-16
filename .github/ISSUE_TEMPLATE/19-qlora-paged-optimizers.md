---
name: "💾 QLoRA Paged Optimizers"
about: Implement CPU-GPU paged optimizers for additional memory savings
title: "[QLoRA] Paged Optimizers Implementation"
labels: priority:P2, type:feature, area:llm, area:performance, effort:medium, phase:2-advanced
assignees: ''

---

## 📋 Description

Implement paged optimizers that offload optimizer states (momentum, variance) between CPU and GPU memory, enabling training of even larger models by reducing peak GPU memory usage.

**Prerequisites**: 
- ✅ QLoRA Infrastructure Complete
- ✅ Training service integration
- ⏳ Basic optimizer (SGD/Adam) working

**Related Documents**: 
- `QLORA_IMPLEMENTATION_SUMMARY.md`
- QLoRA Paper: https://arxiv.org/abs/2305.14314 (Section 3.3)

## 🎯 Goals

- [ ] Paged AdamW optimizer
- [ ] CPU ↔ GPU memory paging for optimizer states
- [ ] Automatic page-in/page-out based on usage
- [ ] Additional 30-50% memory savings vs standard QLoRA
- [ ] Minimal performance impact (<10% overhead)
- [ ] Enable training of 70B+ models on consumer GPUs

## 📝 Tasks

### 1. Paged Memory Manager
- [ ] Page-based memory allocation
- [ ] CPU memory pool for paged-out states
- [ ] GPU memory pool for active states
- [ ] Asynchronous page transfers
- [ ] LRU eviction policy

**Files**:
- `include/llm/lora_framework/paged_memory_manager.h`
- `src/llm/lora_framework/paged_memory_manager.cpp`

**Architecture**:
```cpp
class PagedMemoryManager {
public:
    // Allocate paged buffer
    PagedBuffer allocate(size_t size, DeviceType device);
    
    // Page in from CPU to GPU
    void pageIn(const PagedBuffer& buffer, cudaStream_t stream);
    
    // Page out from GPU to CPU
    void pageOut(const PagedBuffer& buffer, cudaStream_t stream);
    
    // Check if buffer is on GPU
    bool isOnGPU(const PagedBuffer& buffer) const;
    
private:
    // CPU memory pool (pinned for fast transfers)
    std::unique_ptr<PinnedMemoryPool> cpu_pool_;
    
    // GPU memory pool
    std::unique_ptr<GPUMemoryPool> gpu_pool_;
    
    // LRU cache for eviction
    LRUCache<PageID, PageInfo> page_cache_;
};
```

### 2. Paged Optimizer States
- [ ] Store optimizer states in paged memory
- [ ] Track which states are on GPU vs CPU
- [ ] Automatic paging based on access patterns
- [ ] Synchronization management

**State Management**:
```cpp
struct OptimizerState {
    PagedBuffer momentum;      // First moment (Adam)
    PagedBuffer variance;      // Second moment (Adam)
    PagedBuffer gradient;      // Current gradient
    
    bool momentum_on_gpu = false;
    bool variance_on_gpu = false;
};

class PagedOptimizerStateManager {
    // Ensure state is on GPU for optimizer step
    void ensureOnGPU(OptimizerState& state, cudaStream_t stream);
    
    // Page out unused states
    void evictUnused(cudaStream_t stream);
};
```

### 3. Paged AdamW Optimizer
- [ ] AdamW update rule with paged states
- [ ] Automatic state paging during step
- [ ] Prefetch states for next iteration
- [ ] Asynchronous transfers

**Files**:
- `include/llm/lora_framework/paged_optimizer.h`
- `src/llm/lora_framework/paged_optimizer.cpp`

**Implementation**:
```cpp
class PagedAdamWOptimizer {
public:
    PagedAdamWOptimizer(
        float learning_rate = 0.001f,
        float beta1 = 0.9f,
        float beta2 = 0.999f,
        float weight_decay = 0.01f,
        bool enable_paging = true
    );
    
    void step() {
        // For each parameter:
        // 1. Ensure optimizer state is on GPU
        for (auto* param : parameters_) {
            auto& state = states_[param];
            
            // Page in if needed
            if (!state.momentum_on_gpu) {
                memory_manager_->pageIn(
                    state.momentum, 
                    compute_stream_
                );
            }
            
            // Perform Adam update on GPU
            adam_update_kernel<<<...>>>(
                param->data(),
                param->grad(),
                state.momentum.gpu_ptr(),
                state.variance.gpu_ptr(),
                learning_rate_,
                beta1_,
                beta2_
            );
        }
        
        // 2. Page out least recently used states
        memory_manager_->evictUnused(compute_stream_);
    }
    
private:
    std::unique_ptr<PagedMemoryManager> memory_manager_;
    std::unordered_map<Tensor*, OptimizerState> states_;
    cudaStream_t compute_stream_;
};
```

### 4. Memory Paging Strategy
- [ ] Active set: Keep frequently used states on GPU
- [ ] Prefetching: Page in states before needed
- [ ] Batched transfers: Transfer multiple pages at once
- [ ] Priority-based eviction

**Paging Strategy**:
```
Training Step Phases:

1. Pre-Step (Prefetch):
   - Page in optimizer states for current batch
   - Asynchronous transfer while GPU is busy
   
2. Optimizer Step:
   - States should already be on GPU
   - Fast optimizer update
   
3. Post-Step (Eviction):
   - Page out states not needed for next batch
   - Free GPU memory for activations
   
Result: GPU memory oscillates:
  - Peak: During forward/backward (activations)
  - Low: During optimizer (states temporarily on CPU)
```

### 5. Unified Memory Support
- [ ] Use CUDA Unified Memory when available
- [ ] Automatic migration managed by driver
- [ ] Simplified code path
- [ ] Best for newer GPUs (Pascal+)

**Unified Memory Mode**:
```cpp
class UnifiedMemoryOptimizer {
    // Allocate with cudaMallocManaged
    void* allocate(size_t size) {
        void* ptr;
        cudaMallocManaged(&ptr, size);
        return ptr;
    }
    
    // Driver handles paging automatically
    // No explicit page-in/page-out needed
};
```

### 6. Performance Monitoring
- [ ] Track page-in/page-out frequency
- [ ] Measure transfer overhead
- [ ] Monitor GPU memory pressure
- [ ] Adaptive paging based on metrics

**Metrics**:
```cpp
struct PagingMetrics {
    size_t num_page_ins = 0;
    size_t num_page_outs = 0;
    size_t bytes_transferred = 0;
    double transfer_time_ms = 0.0;
    double avg_transfer_bandwidth = 0.0;  // GB/s
    
    // Memory usage
    size_t gpu_memory_used = 0;
    size_t cpu_memory_used = 0;
    size_t peak_gpu_memory = 0;
};
```

### 7. Configuration & Tuning
- [ ] Configure paging parameters
- [ ] Auto-tune based on hardware
- [ ] Manual override for advanced users
- [ ] Profile-guided optimization

**Configuration**:
```cpp
struct PagedOptimizerConfig {
    // Enable paging (default: true if memory constrained)
    bool enable_paging = true;
    
    // Page size (default: 64 MB)
    size_t page_size_bytes = 64 * 1024 * 1024;
    
    // Active set size (states to keep on GPU)
    size_t active_set_size = 1024;
    
    // Prefetch distance (batches ahead)
    size_t prefetch_distance = 1;
    
    // Use unified memory (if available)
    bool use_unified_memory = false;
    
    // Eviction policy
    enum class EvictionPolicy {
        LRU,        // Least Recently Used
        LFU,        // Least Frequently Used
        FIFO,       // First In First Out
        ADAPTIVE    // Adaptive based on access pattern
    } eviction_policy = EvictionPolicy::LRU;
};
```

### 8. Testing & Validation
- [ ] Memory usage tests
- [ ] Performance benchmarks
- [ ] Correctness validation (vs non-paged)
- [ ] Stress tests (large models)

**Test Cases**:
1. Training with paging enabled vs disabled
2. Memory usage under different configurations
3. Performance overhead measurement
4. Convergence validation (same accuracy)
5. Large model tests (>30B parameters)

**Files**:
- `tests/test_paged_optimizer.cpp`
- `benchmarks/bench_paged_memory.cpp`

## ✅ Acceptance Criteria

- [ ] Paged AdamW optimizer working
- [ ] Additional 30-50% memory savings
- [ ] Performance overhead < 10%
- [ ] Accuracy unchanged vs non-paged
- [ ] Can train 70B model on 48GB GPU
- [ ] All tests passing
- [ ] Documentation complete

## 🔗 Dependencies

- ✅ QLoRA Infrastructure
- ✅ Basic optimizer (Adam/SGD)
- ⏳ CUDA 11.8+ (for async memcpy)
- ⏳ Pinned memory support

## 📊 Estimated Effort

**Time**: 2-3 weeks  
**Priority**: 🟢 Medium (P2 - advanced optimization)  
**Complexity**: Medium-High (memory management)

## 🧪 Test Strategy

1. **Correctness**: Paged optimizer produces same results as standard
2. **Memory**: Measure actual vs theoretical savings
3. **Performance**: Benchmark overhead
4. **Stability**: Long training runs without OOM
5. **Scaling**: Test on progressively larger models

### Memory Savings Targets

```
Model       | Standard QLoRA | Paged QLoRA | Savings
--------|----------------|-------------|--------
Llama-7B  | 5-6 GB        | 4-5 GB      | 20%
Llama-13B | 9-10 GB       | 7-8 GB      | 22%
Llama-30B | 20-22 GB      | 15-17 GB    | 25%
Llama-65B | 40-45 GB      | 30-35 GB    | 25%

Enables: Llama-65B on 40GB GPU (vs 48GB needed without paging)
```

## 📚 References

- QLoRA Paper (Section 3.3): https://arxiv.org/abs/2305.14314
- CUDA Unified Memory: https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#um
- Memory Management Best Practices: https://developer.nvidia.com/blog/unified-memory-cuda-beginners/

## 💡 Implementation Notes

### Paging Strategy Trade-offs

**Advantages**:
- ✅ Significant memory savings (30-50%)
- ✅ Enables training of larger models
- ✅ Transparent to training loop

**Disadvantages**:
- ⚠️ Adds complexity
- ⚠️ Transfer overhead (if not well-tuned)
- ⚠️ Requires fast CPU-GPU interconnect (PCIe 4.0+)

### When to Use Paged Optimizers

**Use when**:
- Training very large models (>30B)
- GPU memory is constrained
- Have fast CPU-GPU interconnect
- Willing to accept small perf overhead

**Don't use when**:
- Small models (< 13B)
- Plenty of GPU memory
- Need maximum speed
- Simple setup preferred

## 🏁 Definition of Done

- [ ] Paged memory manager implemented
- [ ] Paged AdamW optimizer working
- [ ] Memory savings validated
- [ ] Performance overhead acceptable
- [ ] All tests passing
- [ ] Benchmarks complete
- [ ] Documentation ready
- [ ] Code reviewed and merged
