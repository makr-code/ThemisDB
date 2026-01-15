---
name: "🧮 Adam Optimizer Implementation"
about: Implement Adam optimizer for LoRA training (Phase 2)
title: "[LoRA] Implement Adam Optimizer"
labels: priority:P1, type:feature, area:llm, effort:medium, phase:2
assignees: ''

---

## 📋 Description

Implement Adam (Adaptive Moment Estimation) optimizer for LoRA training, replacing the simple SGD optimizer from Phase 1. Adam provides adaptive learning rates and better convergence.

**Prerequisites**: Phase 1 complete (SGD optimizer working)  
**Related Issue**: #[Phase 1 Issue Number]  
**Reference**: https://arxiv.org/abs/1412.6980

## 🎯 Goals

- [ ] Implement Adam update rule with β1, β2, ε
- [ ] First and second moment estimates
- [ ] Bias correction
- [ ] Weight decay (AdamW variant)
- [ ] Learning rate scheduling support
- [ ] GPU-accelerated parameter updates (if GPU PR merged)

## 📝 Tasks

### 1. Adam Optimizer Core
- [ ] Implement first moment (momentum) estimation
- [ ] Implement second moment (RMSprop) estimation
- [ ] Bias correction for moments
- [ ] Adaptive learning rate computation
- [ ] Parameter update rule

**File**: `src/llm/lora_framework/lora_layers.cpp`

**Algorithm**:
```
m_t = β1 * m_{t-1} + (1 - β1) * g_t        // First moment
v_t = β2 * v_{t-1} + (1 - β2) * g_t²       // Second moment
m̂_t = m_t / (1 - β1^t)                     // Bias-corrected first moment
v̂_t = v_t / (1 - β2^t)                     // Bias-corrected second moment
θ_t = θ_{t-1} - α * m̂_t / (√v̂_t + ε)     // Parameter update
```

### 2. AdamW Variant
- [ ] Decouple weight decay from gradient
- [ ] Implement proper L2 regularization
- [ ] Compare AdamW vs Adam performance

**Formula**: `θ_t = θ_{t-1} - α * (m̂_t / (√v̂_t + ε) + λ * θ_{t-1})`

### 3. Hyperparameters
- [ ] Learning rate (α): configurable, default 1e-4
- [ ] β1 (momentum): default 0.9
- [ ] β2 (RMSprop): default 0.999
- [ ] ε (numerical stability): default 1e-8
- [ ] Weight decay (λ): default 0.01 for AdamW

### 4. Learning Rate Scheduling
- [ ] Constant learning rate
- [ ] Linear warmup
- [ ] Cosine annealing
- [ ] Step decay
- [ ] Exponential decay

**File**: `include/llm/lora_framework/lr_scheduler.h`

### 5. Integration
- [ ] Update optimizer interface in header
- [ ] Replace SGD in training loop (keep SGD as option)
- [ ] Add optimizer selection config
- [ ] Update hyperparameters struct

**Files**:
- `include/llm/lora_framework/lora_layers.h` (update)
- `src/llm/lora_framework/lora_training_service.cpp` (update)

### 6. Testing
- [ ] Unit tests for Adam update rule
- [ ] Bias correction tests
- [ ] Convergence test on toy problem
- [ ] Compare convergence: Adam vs SGD
- [ ] Test with different learning rates
- [ ] Test with different β values

**File**: `tests/test_lora_optimizer.cpp`

## ✅ Acceptance Criteria

- [ ] Adam optimizer correctly implements update rule
- [ ] Bias correction works properly (early training steps)
- [ ] Converges faster than SGD on toy problem
- [ ] Memory overhead acceptable (2x parameter memory for moments)
- [ ] Hyperparameters configurable via API
- [ ] Learning rate scheduling works
- [ ] All tests pass
- [ ] Documentation updated

## 🔗 Dependencies

- Phase 1 SGD optimizer implementation
- Tensor operations from Phase 1
- (Optional) GPU support from GPU acceleration PR

## 📊 Estimated Effort

**Time**: 1-2 weeks  
**Priority**: 🟡 High (Phase 2)  
**Complexity**: Medium (well-defined algorithm)

## 🧪 Test Strategy

1. **Unit Tests**: Test update rule components individually
2. **Convergence Test**: Train on toy problem (XOR, quadratic function)
3. **Comparison**: Adam vs SGD on same problem
4. **Hyperparameter Sensitivity**: Test different α, β1, β2
5. **Edge Cases**: Very small/large gradients, zero initialization

### Expected Convergence Improvement

```
Problem          SGD (steps)    Adam (steps)    Improvement
--------------------------------------------------------------
XOR              1000           200            5x faster
Quadratic        500            100            5x faster
LoRA Toy Problem 100            30             3x faster
```

## 📚 References

- Adam Paper: https://arxiv.org/abs/1412.6980
- AdamW: https://arxiv.org/abs/1711.05101
- PyTorch Adam Implementation: https://pytorch.org/docs/stable/generated/torch.optim.Adam.html
- LoRA Paper (optimizer recommendations): https://arxiv.org/abs/2106.09685

## 💡 Implementation Notes

### Adam vs SGD
- **SGD**: Simple, reliable, requires careful learning rate tuning
- **Adam**: Adaptive, faster convergence, less sensitive to learning rate
- **AdamW**: Better generalization, recommended for LLM fine-tuning

### Typical Hyperparameters for LoRA
```cpp
AdamOptimizer optimizer(
    learning_rate = 1e-4,   // 3e-4 for larger models
    beta1 = 0.9,            // Standard momentum
    beta2 = 0.999,          // Standard RMSprop
    epsilon = 1e-8,         // Numerical stability
    weight_decay = 0.01     // L2 regularization (AdamW)
);
```

### Memory Overhead
- **Per Parameter**: 2x memory (first + second moments)
- **Example**: 12,288 params → 24,576 moment values
- **Memory**: ~96 KB additional (FP32) for typical LoRA layer

### Learning Rate Schedules
1. **Warmup**: Linear increase from 0 to α over N steps
2. **Cosine**: Smooth decay to 0: `α * 0.5 * (1 + cos(π * t / T))`
3. **Step**: Multiply by γ every N steps
4. **Constant**: Keep α fixed (simplest)

## 🏁 Definition of Done

- [ ] Adam optimizer implemented and tested
- [ ] AdamW variant available
- [ ] Learning rate scheduling works
- [ ] Converges faster than SGD on benchmarks
- [ ] Memory usage acceptable
- [ ] Code reviewed and approved
- [ ] Tests pass
- [ ] Documentation complete
- [ ] Ready for use in production training
