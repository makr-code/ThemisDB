---
name: "🧠 LoRA Training Implementation"
about: Implement LoRA training with GPU acceleration (Phase 1)
title: "[LoRA] Implement LoRA Training System with GPU Kernels"
labels: priority:P0, type:feature, area:llm, area:performance, effort:x-large, phase:1
assignees: ''

---

## 📋 Description

Implement production-ready LoRA (Low-Rank Adaptation) training system with GPU-accelerated forward/backward passes. This replaces the current simulated training (sleep calls) with actual gradient computation.

**Related Analysis**: `docs/analysis/IMPLEMENTATION_GUIDE.md` §3
**Current Issue**: `src/llm/lora_framework/lora_training_service.cpp:69-78` (simulation)
**Infrastructure Files**:
- `include/llm/lora_framework/lora_layers.h`
- `src/llm/lora_framework/lora_layers.cpp`

## 🎯 Goals

- [ ] Implement actual tensor operations (not stubs)
- [ ] Implement forward/backward passes for LoRA layers
- [ ] GPU-accelerated training with Vulkan/CUDA kernels
- [ ] Adam optimizer implementation
- [ ] Integration with llama.cpp base models
- [ ] Comprehensive training pipeline

## 📝 Tasks

### 1. Tensor Class Implementation
- [ ] Implement `Tensor` class with data storage
- [ ] Add shape, dtype, device management
- [ ] Implement basic operations (add, mul, matmul)
- [ ] GPU memory allocation
- [ ] CPU ↔ GPU data transfer
- [ ] Test tensor operations

**File**: `src/llm/lora_framework/lora_layers.cpp`
**Lines**: To be expanded from stub
**Backends**: CPU, CUDA, Vulkan

### 2. LoRALayer Forward Pass
- [ ] Implement matrix multiplication (B @ A)
- [ ] Apply scaling factor
- [ ] Add to base model output (W + BA * scaling)
- [ ] GPU kernel implementation (CUDA/Vulkan)
- [ ] Cache activations for backward pass
- [ ] Test correctness vs reference implementation
- [ ] Benchmark performance

**File**: `src/llm/lora_framework/lora_layers.cpp`
**Lines**: 25-45
**Formula**: `output = input @ (B @ A) * scaling`

### 3. LoRALayer Backward Pass
- [ ] Compute gradients w.r.t. B matrix
- [ ] Compute gradients w.r.t. A matrix
- [ ] Compute gradients w.r.t. input
- [ ] GPU kernel implementation
- [ ] Test gradient correctness (numerical gradient check)
- [ ] Benchmark performance

**File**: `src/llm/lora_framework/lora_layers.cpp`
**Lines**: 50-75
**Gradient Check**: Compare with numerical gradients (finite differences)

### 4. AttentionLoRA Implementation
- [ ] Implement Q, K, V, O projection LoRA layers
- [ ] Forward pass through all projections
- [ ] Backward pass with gradient flow
- [ ] Test with various configurations (Q-only, QKV, QKVO)
- [ ] Benchmark performance

**File**: `src/llm/lora_framework/lora_layers.cpp`
**Lines**: 100-180

### 5. Sequential Container Implementation
- [ ] Forward pass (sequential application)
- [ ] Backward pass (reverse order gradient flow)
- [ ] Parameter collection from all layers
- [ ] Test with nested containers
- [ ] Test with mixed layer types

**File**: `src/llm/lora_framework/lora_layers.cpp`
**Lines**: 200-260

### 6. Adam Optimizer Implementation
- [ ] Implement Adam update rule
- [ ] Add momentum and adaptive learning rate
- [ ] Support weight decay
- [ ] GPU-accelerated parameter updates
- [ ] Test convergence on toy problem
- [ ] Benchmark performance

**New File**: `src/llm/lora_framework/lora_optimizer.cpp`
**Algorithm**: Adam with β1=0.9, β2=0.999, ε=1e-8

### 7. Training Loop Implementation
- [ ] Replace `sleep()` with actual training
- [ ] Implement data loading and batching
- [ ] Forward pass through model + LoRA
- [ ] Loss computation
- [ ] Backward pass (gradient computation)
- [ ] Optimizer step
- [ ] Metrics tracking (loss, learning rate)
- [ ] Checkpoint saving

**File**: `src/llm/lora_framework/lora_training_service.cpp`
**Lines**: 69-78 (to be replaced) and expanded

### 8. GPU Kernel Implementation
- [ ] **Vulkan** compute shaders for matrix ops (PRIORITIZED)
- [ ] CUDA kernels for NVIDIA GPUs
- [ ] HIP kernels for AMD GPUs
- [ ] Kernel fusion for performance
- [ ] Test each kernel independently
- [ ] Benchmark kernel performance

**New Files**:
- `src/llm/lora_framework/kernels/vulkan_kernels.cpp`
- `src/llm/lora_framework/kernels/cuda_kernels.cu`
- `src/llm/lora_framework/kernels/hip_kernels.cpp`

### 9. Integration with llama.cpp Base Model
- [ ] Load frozen base model weights
- [ ] Add LoRA layers on top
- [ ] Forward pass: base + LoRA
- [ ] Backward pass: only through LoRA (freeze base)
- [ ] Test with actual llama models
- [ ] Verify weight merging (optional)

**Integration**: `src/llm/llama_wrapper.cpp` + LoRA layers

### 10. Testing
- [ ] Unit tests for each layer type (`tests/test_lora_layers.cpp`)
- [ ] Gradient check tests (numerical vs analytical)
- [ ] Integration tests with llama.cpp models
- [ ] Test on toy problem (verify convergence)
- [ ] Test on real dataset (e.g., Alpaca, ShareGPT)
- [ ] Performance benchmarks (`benchmarks/bench_lora_training.cpp`)
- [ ] Memory usage tests

### 11. Training Pipeline Features
- [ ] Mixed precision training (FP16/BF16)
- [ ] Gradient accumulation
- [ ] Gradient clipping
- [ ] Learning rate scheduling (cosine, linear warmup)
- [ ] Checkpointing and resumption
- [ ] Distributed training (multi-GPU)
- [ ] Logging and visualization (Grafana integration)

### 12. Documentation
- [ ] Update training documentation
- [ ] Add training examples
- [ ] Document hyperparameters
- [ ] Document GPU requirements
- [ ] Add troubleshooting guide
- [ ] Update `INFRASTRUCTURE_README.md`

## ✅ Acceptance Criteria

- [ ] All TODO comments in lora_layers.cpp are resolved
- [ ] Tensor operations work correctly (CPU and GPU)
- [ ] Forward/backward passes are mathematically correct
- [ ] Training loop converges on toy problem
- [ ] Training loop trains real LoRA adapter successfully
- [ ] GPU acceleration works (Vulkan, CUDA, HIP)
- [ ] Adam optimizer updates parameters correctly
- [ ] All tests pass (unit, integration, gradient check)
- [ ] Code coverage > 80%
- [ ] Training is faster than CPU (with GPU)
- [ ] Memory usage is within budget (< 80% VRAM)
- [ ] Documentation is complete

## 🔗 Dependencies

- Tensor library (to be implemented or use existing: Eigen, xtensor)
- GPU backends (Vulkan, CUDA, HIP) - from existing `BackendRegistry`
- llama.cpp base model integration
- Adam optimizer implementation
- Training data loader

## 📊 Estimated Effort

**Time**: 6-8 weeks (1-2 FTE)
**Priority**: 🔴 Critical (Phase 1, Week 7-14)

## 🧪 Test Strategy

1. **Unit Tests**: Test each component independently
2. **Gradient Check**: Numerical gradient vs analytical gradient (< 1e-5 error)
3. **Toy Problem**: Train on XOR or simple function, verify convergence
4. **Real Dataset**: Train on Alpaca/ShareGPT, measure loss decrease
5. **GPU Tests**: Test each GPU backend (Vulkan, CUDA, HIP)
6. **Multi-GPU Tests**: Test distributed training
7. **Performance Tests**: Benchmark training speed, memory usage

### Toy Problem for Testing
```python
# XOR problem (classic non-linear test)
X = [[0, 0], [0, 1], [1, 0], [1, 1]]
y = [0, 1, 1, 0]

# Train LoRA layer to learn XOR
# Should converge to < 0.01 loss in < 1000 steps
```

## 📚 References

- `docs/analysis/IMPLEMENTATION_GUIDE.md` §3 - Training implementation
- `docs/analysis/GPU_ACCELERATION_ADDENDUM.md` - GPU kernels
- LoRA paper: https://arxiv.org/abs/2106.09685
- Adam optimizer paper: https://arxiv.org/abs/1412.6980
- llama.cpp: https://github.com/ggerganov/llama.cpp

## 💡 Implementation Notes

### LoRA Formula
Forward: `output = base_output + (input @ B @ A) * scaling`
Backward:
- `grad_A = B.T @ (grad_output * scaling) @ input.T`
- `grad_B = (grad_output * scaling) @ A.T @ input`
- `grad_input = (grad_output * scaling) @ A.T @ B.T`

### GPU Optimization
- Use kernel fusion (combine operations)
- Use tensor cores (NVIDIA) or FMA (AMD)
- Use mixed precision (FP16 for speed, FP32 for accumulation)
- Use gradient checkpointing (save memory)

### Typical Hyperparameters
- Rank: 4-64 (8-16 typical)
- Learning rate: 1e-4 to 3e-4
- Batch size: 1-8 (gradient accumulation for larger)
- Training steps: 1000-10000
- Warmup steps: 100-500

### Memory Requirements
- Base model: ~7 GB (Llama-7B in FP16)
- LoRA parameters: ~50 MB (rank=16, all layers)
- Gradients: ~50 MB
- Activations: ~2 GB (depends on sequence length)
- Total: ~10 GB VRAM

## 🏁 Definition of Done

- [ ] All tasks completed and checked off
- [ ] All acceptance criteria met
- [ ] Code reviewed and approved
- [ ] Tests pass in CI/CD
- [ ] Gradient check passes (< 1e-5 error)
- [ ] Training converges on toy problem
- [ ] Training works on real dataset
- [ ] GPU acceleration tested
- [ ] Performance benchmarks meet targets
- [ ] Memory usage within budget
- [ ] Documentation updated
- [ ] Ready for Phase 2 (Production Deployment)
