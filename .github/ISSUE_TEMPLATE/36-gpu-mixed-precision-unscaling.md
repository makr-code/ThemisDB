---
name: "🔬 Implement Mixed Precision Gradient Unscaling for GPU Tensors"
about: Add GPU tensor support for mixed precision gradient unscaling (Medium Priority - P2)
title: "[GPU Training] Implement Mixed Precision Gradient Unscaling for GPUTensor"
labels: priority:P2, type:feature, area:llm, area:gpu, effort:medium, phase:production
assignees: ''

---

## 📋 Beschreibung / Description

**DE**: Mixed Precision Training kann aktuell keine Gradienten-Unscaling für GPU-Tensoren durchführen, da `MixedPrecisionTrainer::unscale_gradients()` nur `std::vector<Tensor*>` akzeptiert, aber wir `std::vector<GPUTensor*>` haben. Dies kann zu Gradient-Overflow in FP16 Mode führen.

**EN**: Mixed precision training currently cannot perform gradient unscaling for GPU tensors because `MixedPrecisionTrainer::unscale_gradients()` only accepts `std::vector<Tensor*>` but we have `std::vector<GPUTensor*>`. This may lead to gradient overflow in FP16 mode.

**Related Analysis**: `LORA_TRAINING_REVIEW.md` §2.2d (MEDIUM Priority)  
**Current Status**: `src/llm/lora_framework/gpu_training_loop.cpp:361-373` - TODO comment  
**Impact**: ⚠️ **Potential Gradient Overflow in FP16** - Limits mixed precision effectiveness

## 🎯 Ziele / Goals

- [ ] GPU-kompatible Gradient Unscaling Methode implementieren
- [ ] Adapter-Pattern für GPUTensor→Tensor Konvertierung
- [ ] Overflow Detection für GPU-Tensoren
- [ ] FP16 Training Stabilität sicherstellen
- [ ] Tests für Mixed Precision mit GPU

## 📝 Aufgaben / Tasks

### 1. GPU Tensor Adapter Implementation
**Priorität**: P2 - Medium

**Current Code** (Lines 361-373):
```cpp
// Unscale gradients if mixed precision
bool should_step = true;
if (mixed_precision_trainer_ && mixed_precision_trainer_->is_enabled()) {
    // TODO: Implement proper gradient unscaling for GPU tensors
    // Current limitation: MixedPrecisionTrainer::unscale_gradients expects std::vector<Tensor*>
    // but we have std::vector<GPUTensor*>. Need to either:
    // 1. Create an adapter/wrapper to convert GPUTensor* to Tensor*
    // 2. Add a GPU-specific unscale_gradients method to MixedPrecisionTrainer
    // 3. Implement gradient unscaling directly in GPU training loop
    
    spdlog::debug("Mixed precision gradient unscaling skipped (not yet implemented for GPU tensors)");
    should_step = true;  // Proceed with optimizer step despite skipped unscaling
}
```

**Proposed Solution 1: GPU-Native Unscaling**:
```cpp
// File: include/llm/lora_framework/gpu_mixed_precision.h

class GPUMixedPrecisionTrainer {
public:
    /**
     * @brief Unscale gradients on GPU (no CPU transfer)
     * @param gradients Vector of GPU gradient tensors
     * @return true if no overflow detected, false otherwise
     */
    bool unscale_gradients(std::vector<GPUTensor*>& gradients);
    
    /**
     * @brief Check for overflow in GPU tensors
     * @param gradients Vector of GPU gradient tensors
     * @return true if overflow/underflow detected
     */
    bool has_overflow(const std::vector<GPUTensor*>& gradients);
    
    /**
     * @brief Scale loss on GPU
     */
    float scale_loss(float loss) const { return loss * current_loss_scale_; }
    
    /**
     * @brief Update loss scale based on overflow
     */
    void update_loss_scale(bool had_overflow);
    
private:
    float current_loss_scale_;
    int steps_since_overflow_;
    MixedPrecisionConfig config_;
};
```

**Implementation**:
```cpp
// File: src/llm/lora_framework/gpu_mixed_precision.cpp

bool GPUMixedPrecisionTrainer::unscale_gradients(std::vector<GPUTensor*>& gradients) {
    if (current_loss_scale_ == 1.0f) {
        return true;  // No scaling applied
    }
    
    float inv_scale = 1.0f / current_loss_scale_;
    
    // Launch GPU kernel to unscale all gradients
    for (auto* grad : gradients) {
        if (!grad || grad->size() == 0) continue;
        
        // GPU kernel: grad = grad * inv_scale
        gpu_kernels::scalarMultiply(*grad, inv_scale);
    }
    
    // Check for overflow (detect NaN/Inf in GPU tensors)
    return !has_overflow(gradients);
}

bool GPUMixedPrecisionTrainer::has_overflow(const std::vector<GPUTensor*>& gradients) {
    for (auto* grad : gradients) {
        if (!grad) continue;
        
        // GPU kernel: check for NaN/Inf
        if (gpu_kernels::hasInfOrNaN(*grad)) {
            spdlog::warn("Gradient overflow detected in mixed precision training");
            return true;
        }
    }
    return false;
}
```

**Tasks**:
- [ ] Create GPUMixedPrecisionTrainer class
- [ ] Implement GPU-native unscale_gradients
- [ ] Implement GPU-native overflow detection
- [ ] Add scalar multiply GPU kernel
- [ ] Add NaN/Inf detection GPU kernel
- [ ] Integrate with GPUTrainingLoop

**File**: `include/llm/lora_framework/gpu_mixed_precision.h`

---

### 2. GPU Overflow Detection Kernel
**Priorität**: P2 - Medium

**CUDA Kernel Implementation**:
```cuda
// File: src/llm/lora_framework/cuda_mixed_precision_kernels.cu

__device__ bool isInfOrNaN(float val) {
    return isinf(val) || isnan(val);
}

__global__ void checkInfNaNKernel(
    const float* data,
    int n,
    int* has_overflow  // 1 if overflow, 0 otherwise
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx < n) {
        if (isInfOrNaN(data[idx])) {
            atomicExch(has_overflow, 1);
        }
    }
}

bool hasInfOrNaN(const GPUTensor& tensor) {
    // Allocate device flag
    int* d_overflow;
    cudaMalloc(&d_overflow, sizeof(int));
    cudaMemset(d_overflow, 0, sizeof(int));
    
    // Launch kernel
    int threads = 256;
    int blocks = (tensor.size() + threads - 1) / threads;
    
    checkInfNaNKernel<<<blocks, threads>>>(
        static_cast<const float*>(tensor.gpu_ptr()),
        tensor.size(),
        d_overflow
    );
    
    // Copy result back
    int h_overflow;
    cudaMemcpy(&h_overflow, d_overflow, sizeof(int), cudaMemcpyDeviceToHost);
    cudaFree(d_overflow);
    
    return h_overflow == 1;
}
```

**Tasks**:
- [ ] Implement CUDA overflow detection kernel
- [ ] Implement HIP version
- [ ] Implement Vulkan compute shader
- [ ] Optimize for early exit (atomic operations)
- [ ] Benchmark performance
- [ ] Add to gpu_kernels namespace

---

### 3. Scalar Multiply GPU Kernel
**Priorität**: P2 - Medium

**CUDA Kernel Implementation**:
```cuda
// File: src/llm/lora_framework/cuda_mixed_precision_kernels.cu

__global__ void scalarMultiplyKernel(
    float* data,
    float scalar,
    int n
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;
    
    for (int i = idx; i < n; i += stride) {
        data[i] *= scalar;
    }
}

void scalarMultiply(GPUTensor& tensor, float scalar) {
    int threads = 256;
    int blocks = (tensor.size() + threads - 1) / threads;
    
    scalarMultiplyKernel<<<blocks, threads>>>(
        static_cast<float*>(tensor.gpu_ptr()),
        scalar,
        tensor.size()
    );
    
    cudaDeviceSynchronize();
}
```

**Tasks**:
- [ ] Implement CUDA scalar multiply kernel
- [ ] Implement HIP version
- [ ] Implement Vulkan compute shader
- [ ] Optimize memory access (vectorized loads/stores)
- [ ] Add to gpu_kernels namespace
- [ ] Benchmark performance

---

### 4. Integration with GPU Training Loop
**Priorität**: P2 - Medium

**Update trainStep Method**:
```cpp
// File: src/llm/lora_framework/gpu_training_loop.cpp

float GPUTrainingLoop::trainStep(const GPUBatch& batch) {
    // ... forward pass ...
    
    // Compute loss
    float loss = computeMSELossGPU(predictions, target_embeddings);
    
    // Scale loss for mixed precision
    if (gpu_mixed_precision_trainer_ && gpu_mixed_precision_trainer_->is_enabled()) {
        loss = gpu_mixed_precision_trainer_->scale_loss(loss);
    }
    
    // Backward pass
    GPUTensor grad_output = computeMSEGradientGPU(predictions, target_embeddings);
    
    if (multi_gpu_layer_) {
        multi_gpu_layer_->backward({grad_output});
        multi_gpu_layer_->synchronize_gradients();
    } else {
        layers_[0]->backward(grad_output);
    }
    
    // ✅ NEW: GPU-native gradient unscaling
    bool should_step = true;
    if (gpu_mixed_precision_trainer_ && gpu_mixed_precision_trainer_->is_enabled()) {
        std::vector<GPUTensor*> gradients;
        if (multi_gpu_layer_) {
            gradients = multi_gpu_layer_->get_layer(0).gradients();
        } else {
            gradients = layers_[0]->gradients();
        }
        
        // Unscale gradients on GPU
        bool no_overflow = gpu_mixed_precision_trainer_->unscale_gradients(gradients);
        
        // Update loss scale
        gpu_mixed_precision_trainer_->update_loss_scale(!no_overflow);
        
        if (!no_overflow) {
            spdlog::warn("Gradient overflow detected, skipping optimizer step");
            should_step = false;
        }
    }
    
    // Optimizer step
    if (should_step) {
        optimizer_->step();
    }
    
    return loss;
}
```

**Tasks**:
- [ ] Add gpu_mixed_precision_trainer_ member
- [ ] Update trainStep to use GPU gradient unscaling
- [ ] Remove skipped unscaling TODO comment
- [ ] Add overflow detection logging
- [ ] Test FP16 training stability

---

### 5. Alternative: Tensor Adapter Pattern
**Priorität**: P3 - Low (if GPU-native solution not preferred)

**Adapter Implementation**:
```cpp
// File: include/llm/lora_framework/gpu_tensor_adapter.h

class GPUTensorAdapter : public Tensor {
public:
    explicit GPUTensorAdapter(GPUTensor* gpu_tensor)
        : gpu_tensor_(gpu_tensor) {}
    
    // Implement Tensor interface delegating to GPUTensor
    float* data() override {
        // Return CPU data (triggers download)
        cpu_cache_ = gpu_tensor_->cpu_data();
        return cpu_cache_.data();
    }
    
    const float* data() const override {
        cpu_cache_ = gpu_tensor_->cpu_data();
        return cpu_cache_.data();
    }
    
    // ... other Tensor methods ...
    
    void sync_to_gpu() {
        gpu_tensor_->upload(cpu_cache_);
    }
    
private:
    GPUTensor* gpu_tensor_;
    mutable std::vector<float> cpu_cache_;
};

// Usage in training loop:
std::vector<Tensor*> tensor_adapters;
std::vector<std::unique_ptr<GPUTensorAdapter>> adapter_storage;

for (auto* gpu_grad : gpu_gradients) {
    auto adapter = std::make_unique<GPUTensorAdapter>(gpu_grad);
    tensor_adapters.push_back(adapter.get());
    adapter_storage.push_back(std::move(adapter));
}

// Now can use existing MixedPrecisionTrainer
mixed_precision_trainer_->unscale_gradients(tensor_adapters);

// Sync back to GPU
for (auto& adapter : adapter_storage) {
    adapter->sync_to_gpu();
}
```

**Note**: This approach has performance cost (CPU↔GPU transfers) and is not recommended.

---

### 6. Testing and Validation
**Priorität**: P2 - Medium

**Test Cases**:
```cpp
// Test file: tests/test_gpu_mixed_precision.cpp

TEST(GPUMixedPrecisionTest, GradientUnscaling) {
    // Create GPU gradients with known values
    // Apply loss scaling
    // Unscale gradients
    // Verify values are correct
}

TEST(GPUMixedPrecisionTest, OverflowDetection) {
    // Create GPU tensor with NaN
    // Check overflow detection returns true
    // Create GPU tensor with Inf
    // Check overflow detection returns true
    // Create normal GPU tensor
    // Check overflow detection returns false
}

TEST(GPUMixedPrecisionTest, FP16TrainingStability) {
    // Train with FP16 for 100 steps
    // Should converge without gradient overflow
    // Loss should decrease smoothly
}

TEST(GPUMixedPrecisionTest, LossScaleUpdate) {
    // Simulate overflow
    // Verify loss scale decreases
    // Simulate successful steps
    // Verify loss scale increases
}

TEST(GPUMixedPrecisionTest, CompareWithCPUVersion) {
    // Train with CPU mixed precision
    // Train with GPU mixed precision
    // Final loss should be similar (<1% difference)
}
```

**Tasks**:
- [ ] Create comprehensive test suite
- [ ] Test overflow detection accuracy
- [ ] Test gradient unscaling correctness
- [ ] Validate FP16 training stability
- [ ] Compare with CPU mixed precision
- [ ] Add performance benchmarks

---

## ✅ Akzeptanzkriterien / Acceptance Criteria

- [ ] GPU-native gradient unscaling implemented
- [ ] Overflow detection works on GPU tensors
- [ ] FP16 training stable (no gradient explosions)
- [ ] Loss scale dynamically adjusted based on overflow
- [ ] Performance acceptable (<1ms overhead per step)
- [ ] Works with all GPU backends (CUDA, HIP, Vulkan, DirectX)
- [ ] Comprehensive tests pass (>90% coverage)
- [ ] Training quality matches CPU mixed precision
- [ ] No CPU↔GPU transfers for gradient unscaling
- [ ] Overflow detection is accurate (>99%)

## 📊 Effort Estimation

- **Aufwand / Effort**: 1 week (Medium)
- **Komplexität / Complexity**: Medium (GPU kernel development)
- **Risiko / Risk**: Low (well-defined feature)

## 🔗 Related Issues

- Issue #34: GPU Real Embeddings
- Issue #35: GPU Loss/Gradient Kernels
- Code Review: `LORA_TRAINING_REVIEW.md` §2.2d

## 📚 References

- Code location: `src/llm/lora_framework/gpu_training_loop.cpp:361-373`
- Review analysis: `LORA_TRAINING_REVIEW.md` Section 2.2
- Mixed precision implementation: `src/llm/lora_framework/mixed_precision.cpp`
- FP16 training guide: https://docs.nvidia.com/deeplearning/performance/mixed-precision-training/

---

**Priority**: P2 - Medium priority (after GPU kernels)  
**Impact**: FP16 training stability, gradient overflow prevention  
**Status**: Ready to implement (after Issue #35 completed)
