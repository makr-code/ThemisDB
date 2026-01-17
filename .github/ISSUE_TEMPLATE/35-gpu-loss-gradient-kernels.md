---
name: "⚡ Implement GPU Kernels for Loss and Gradient Computation"
about: Replace CPU-based loss/gradient computation with GPU kernels (High Priority - P1)
title: "[GPU Training] Implement GPU Kernels for MSE Loss and Gradients"
labels: priority:P1, type:performance, area:llm, area:gpu, effort:high, phase:production
assignees: ''

---

## 📋 Beschreibung / Description

**DE**: Loss- und Gradienten-Berechnung erfolgt aktuell auf der CPU mit mehreren GPU↔CPU Transfers pro Trainingsschritt. Dies ist ein massiver Performance-Bottleneck und verhindert echte GPU-Beschleunigung.

**EN**: Loss and gradient computation currently happens on CPU with multiple GPU↔CPU transfers per training step. This is a major performance bottleneck and prevents true GPU acceleration.

**Related Analysis**: `LORA_TRAINING_REVIEW.md` §2.2b & §2.2c (HIGH Priority)  
**Current Status**: `src/llm/lora_framework/gpu_training_loop.cpp:459-503` - TODO comments  
**Impact**: 🔥 **MAJOR Performance Bottleneck** - 5 GPU↔CPU transfers per step!

## 🎯 Ziele / Goals

- [ ] GPU Kernel für MSE Loss Berechnung implementieren
- [ ] GPU Kernel für MSE Gradient Berechnung implementieren
- [ ] Alle CPU↔GPU Transfers eliminieren
- [ ] Kernel Fusion für Loss+Gradient erwägen
- [ ] Performance-Tests (erwartete 3-5x Beschleunigung)

## 📝 Aufgaben / Tasks

### 1. GPU MSE Loss Kernel Implementation
**Priorität**: P1 - Critical

**Current Code** (Lines 459-475):
```cpp
float computeMSELossGPU(const GPUTensor& predictions, const GPUTensor& targets) {
    // TODO: Implement as GPU kernel for true GPU acceleration
    // Current implementation downloads to CPU which creates a performance bottleneck:
    // 1. GPU → CPU transfer for predictions (expensive)  ⚠️
    // 2. GPU → CPU transfer for targets (expensive)      ⚠️
    // 3. CPU computation (slower than GPU)
    
    auto pred_data = predictions.cpu_data();  // ⚠️ BOTTLENECK!
    auto target_data = targets.cpu_data();    // ⚠️ BOTTLENECK!
    
    float sum = 0.0f;
    for (size_t i = 0; i < pred_data.size(); ++i) {
        float diff = pred_data[i] - target_data[i];
        sum += diff * diff;
    }
    
    return sum / pred_data.size();
}
```

**Proposed GPU Implementation**:
```cpp
// File: src/llm/lora_framework/cuda_loss_kernels.cu

__global__ void mseLossReductionKernel(
    const float* predictions,
    const float* targets,
    float* partial_sums,
    int n
) {
    __shared__ float shared_sum[256];
    
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int tid = threadIdx.x;
    
    // Compute partial sum for this thread
    float local_sum = 0.0f;
    for (int i = idx; i < n; i += blockDim.x * gridDim.x) {
        float diff = predictions[i] - targets[i];
        local_sum += diff * diff;
    }
    
    // Reduce within block
    shared_sum[tid] = local_sum;
    __syncthreads();
    
    // Tree reduction in shared memory
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            shared_sum[tid] += shared_sum[tid + s];
        }
        __syncthreads();
    }
    
    // Write block result
    if (tid == 0) {
        partial_sums[blockIdx.x] = shared_sum[0];
    }
}

float computeMSELossGPU(const GPUTensor& predictions, const GPUTensor& targets) {
    size_t n = predictions.size();
    
    // Step 1: Parallel reduction on GPU
    int threads = 256;
    int blocks = min(1024, (n + threads - 1) / threads);
    
    GPUTensor partial_sums({static_cast<size_t>(blocks)}, predictions.device());
    
    mseLossReductionKernel<<<blocks, threads>>>(
        static_cast<const float*>(predictions.gpu_ptr()),
        static_cast<const float*>(targets.gpu_ptr()),
        static_cast<float*>(partial_sums.gpu_ptr()),
        n
    );
    
    // Step 2: Final reduction (small array, can do on CPU)
    auto partial_data = partial_sums.cpu_data();  // Only 1KB transfer!
    float sum = 0.0f;
    for (float val : partial_data) {
        sum += val;
    }
    
    return sum / n;
}
```

**Tasks**:
- [ ] Implement CUDA MSE loss kernel with reduction
- [ ] Implement HIP version (rocm)
- [ ] Implement Vulkan compute shader
- [ ] Implement DirectX compute shader
- [ ] Optimize shared memory usage
- [ ] Use warp shuffle for reduction (CUDA)
- [ ] Benchmark against CPU version
- [ ] Add error handling

**File**: `src/llm/lora_framework/cuda_loss_kernels.cu`

---

### 2. GPU MSE Gradient Kernel Implementation
**Priorität**: P1 - Critical

**Current Code** (Lines 477-503):
```cpp
GPUTensor computeMSEGradientGPU(const GPUTensor& predictions, const GPUTensor& targets) {
    // TODO: Implement as GPU kernel
    // Current: 3 CPU↔GPU transfers per call!
    // 1. GPU → CPU: predictions download     ⚠️
    // 2. GPU → CPU: targets download         ⚠️
    // 3. CPU → GPU: gradient upload          ⚠️
    
    auto pred_data = predictions.cpu_data();    // ⚠️ BOTTLENECK!
    auto target_data = targets.cpu_data();      // ⚠️ BOTTLENECK!
    
    std::vector<float> grad_data(pred_data.size());
    float scale = 2.0f / pred_data.size();
    
    for (size_t i = 0; i < pred_data.size(); ++i) {
        grad_data[i] = scale * (pred_data[i] - target_data[i]);
    }
    
    GPUTensor grad(predictions.shape(), predictions.device());
    grad.upload(grad_data);  // ⚠️ BOTTLENECK!
    return grad;
}
```

**Proposed GPU Implementation**:
```cpp
// File: src/llm/lora_framework/cuda_gradient_kernels.cu

__global__ void mseGradientKernel(
    float* grad_output,
    const float* predictions,
    const float* targets,
    float scale,
    int n
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;
    
    for (int i = idx; i < n; i += stride) {
        grad_output[i] = scale * (predictions[i] - targets[i]);
    }
}

GPUTensor computeMSEGradientGPU(const GPUTensor& predictions, const GPUTensor& targets) {
    // ✅ All computation on GPU, no CPU transfers!
    
    GPUTensor grad(predictions.shape(), predictions.device());
    
    size_t n = predictions.size();
    float scale = 2.0f / n;
    
    int threads = 256;
    int blocks = (n + threads - 1) / threads;
    
    mseGradientKernel<<<blocks, threads>>>(
        static_cast<float*>(grad.gpu_ptr()),
        static_cast<const float*>(predictions.gpu_ptr()),
        static_cast<const float*>(targets.gpu_ptr()),
        scale,
        n
    );
    
    cudaDeviceSynchronize();
    
    return grad;
}
```

**Tasks**:
- [ ] Implement CUDA MSE gradient kernel
- [ ] Implement HIP version
- [ ] Implement Vulkan compute shader
- [ ] Implement DirectX compute shader
- [ ] Optimize memory access patterns (coalescing)
- [ ] Use vectorized loads/stores (float4)
- [ ] Benchmark performance
- [ ] Verify gradient correctness

**File**: `src/llm/lora_framework/cuda_gradient_kernels.cu`

---

### 3. Kernel Fusion Optimization
**Priorität**: P2 - Medium (after basic kernels work)

**Fused Loss + Gradient Kernel**:
```cpp
// Compute loss AND gradient in single kernel pass
// Saves memory bandwidth (read predictions/targets once)

__global__ void mseLossGradientFusedKernel(
    float* grad_output,
    float* partial_loss,
    const float* predictions,
    const float* targets,
    int n
) {
    __shared__ float shared_loss[256];
    
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int tid = threadIdx.x;
    
    // Compute gradient AND accumulate loss
    float local_loss = 0.0f;
    for (int i = idx; i < n; i += blockDim.x * gridDim.x) {
        float diff = predictions[i] - targets[i];
        
        // Gradient
        grad_output[i] = (2.0f / n) * diff;
        
        // Loss
        local_loss += diff * diff;
    }
    
    // Reduce loss within block
    shared_loss[tid] = local_loss;
    __syncthreads();
    
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            shared_loss[tid] += shared_loss[tid + s];
        }
        __syncthreads();
    }
    
    if (tid == 0) {
        partial_loss[blockIdx.x] = shared_loss[0];
    }
}
```

**Tasks**:
- [ ] Implement fused loss+gradient kernel
- [ ] Benchmark vs separate kernels
- [ ] Verify numerical stability
- [ ] Add to GPUTrainingLoop as optimization

---

### 4. Multi-Backend Support
**Priorität**: P1 - High

**Backend Abstraction**:
```cpp
// File: include/llm/lora_framework/gpu_loss_kernels.h

namespace gpu_kernels {

/**
 * @brief Compute MSE loss on GPU (backend-agnostic)
 */
float computeMSELoss(
    const GPUTensor& predictions,
    const GPUTensor& targets
);

/**
 * @brief Compute MSE gradient on GPU (backend-agnostic)
 */
GPUTensor computeMSEGradient(
    const GPUTensor& predictions,
    const GPUTensor& targets
);

// Backend-specific implementations
namespace cuda {
    float computeMSELoss(...);
    GPUTensor computeMSEGradient(...);
}

namespace hip {
    float computeMSELoss(...);
    GPUTensor computeMSEGradient(...);
}

namespace vulkan {
    float computeMSELoss(...);
    GPUTensor computeMSEGradient(...);
}

namespace directx {
    float computeMSELoss(...);
    GPUTensor computeMSEGradient(...);
}

} // namespace gpu_kernels
```

**Tasks**:
- [ ] Create backend abstraction layer
- [ ] Implement CUDA kernels
- [ ] Implement HIP kernels
- [ ] Implement Vulkan compute shaders
- [ ] Implement DirectX compute shaders
- [ ] Runtime backend selection
- [ ] Fallback to CPU if GPU unavailable

---

### 5. Integration with GPU Training Loop
**Priorität**: P1 - High

**Update trainStep Method**:
```cpp
// File: src/llm/lora_framework/gpu_training_loop.cpp

float GPUTrainingLoop::trainStep(const GPUBatch& batch) {
    // ... forward pass ...
    
    // ✅ NEW: GPU kernel for loss (no CPU transfer)
    float loss = gpu_kernels::computeMSELoss(predictions, target_embeddings);
    
    // Scale loss for mixed precision
    if (mixed_precision_trainer_ && mixed_precision_trainer_->is_enabled()) {
        loss = mixed_precision_trainer_->scale_loss(loss);
    }
    
    // ✅ NEW: GPU kernel for gradient (no CPU transfer)
    GPUTensor grad_output = gpu_kernels::computeMSEGradient(
        predictions, target_embeddings
    );
    
    // ... backward pass ...
}
```

**Tasks**:
- [ ] Replace computeMSELossGPU with GPU kernel version
- [ ] Replace computeMSEGradientGPU with GPU kernel version
- [ ] Remove all CPU↔GPU transfers
- [ ] Add performance logging
- [ ] Verify training still converges

---

### 6. Performance Testing and Validation
**Priorität**: P1 - High

**Benchmarking Strategy**:
```cpp
// Test file: tests/test_gpu_loss_kernels.cpp

TEST(GPULossKernelsTest, MSELossCUDA) {
    // Create large tensors on GPU
    // Compute loss with CPU version (baseline)
    // Compute loss with GPU kernel
    // Verify numerical accuracy (< 1e-5 difference)
    // Verify GPU version is faster (>10x speedup)
}

TEST(GPULossKernelsTest, MSEGradientCUDA) {
    // Similar to loss test
    // Verify gradient correctness
    // Verify GPU version is faster
}

TEST(GPULossKernelsTest, NoTransfersProfile) {
    // Use CUDA profiler (nsys)
    // Verify no cudaMemcpy calls in training loop
    // Verify all computation on GPU
}

TEST(GPULossKernelsTest, EndToEndPerformance) {
    // Train for 100 steps with CPU version
    // Train for 100 steps with GPU kernel version
    // GPU should be 3-5x faster overall
}
```

**Performance Targets**:
- MSE Loss computation: >10x speedup vs CPU
- MSE Gradient computation: >10x speedup vs CPU
- End-to-end training: 3-5x speedup vs current implementation
- Zero CPU↔GPU transfers during training loop

**Tasks**:
- [ ] Create comprehensive benchmark suite
- [ ] Profile with nsys/rocprof/PIX
- [ ] Verify no CPU↔GPU transfers
- [ ] Measure end-to-end speedup
- [ ] Compare against PyTorch/TensorFlow baselines
- [ ] Add performance regression tests

---

## ✅ Akzeptanzkriterien / Acceptance Criteria

- [ ] GPU kernels for MSE loss implemented (all backends)
- [ ] GPU kernels for MSE gradient implemented (all backends)
- [ ] All CPU↔GPU transfers eliminated from training loop
- [ ] Loss computation >10x faster than CPU version
- [ ] Gradient computation >10x faster than CPU version
- [ ] End-to-end training 3-5x faster than current implementation
- [ ] Numerical accuracy maintained (<1e-5 error)
- [ ] All backends work (CUDA, HIP, Vulkan, DirectX)
- [ ] Comprehensive tests pass (>95% coverage)
- [ ] Performance profiling confirms no CPU transfers
- [ ] Training still converges to same quality

## 📊 Effort Estimation

- **Aufwand / Effort**: 1-2 weeks (High)
- **Komplexität / Complexity**: High (GPU kernel optimization)
- **Risiko / Risk**: Low-Medium (well-defined optimization)

## 🔗 Related Issues

- Issue #34: GPU Real Embeddings
- Issue #36: Mixed Precision Gradient Unscaling
- Code Review: `LORA_TRAINING_REVIEW.md` §2.2b & §2.2c

## 📚 References

- Code location: `src/llm/lora_framework/gpu_training_loop.cpp:459-503`
- Review analysis: `LORA_TRAINING_REVIEW.md` Section 2.2
- CUDA programming guide: https://docs.nvidia.com/cuda/
- Vulkan compute: https://www.khronos.org/vulkan/
- Performance analysis: `LORA_TRAINING_REVIEW.md` Section 4

---

**Priority**: P1 - Critical for production performance  
**Impact**: 🔥 Major performance improvement (3-5x speedup expected)  
**Status**: Ready to implement
