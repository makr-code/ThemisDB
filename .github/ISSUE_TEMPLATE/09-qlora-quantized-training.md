---
name: "🔢 QLoRA (Quantized LoRA) Implementation"
about: Implement QLoRA with 4-bit/8-bit quantization for memory-efficient training (Phase 2)
title: "[QLoRA] Implement Quantized LoRA Training"
labels: priority:P1, type:feature, area:llm, area:performance, effort:large, phase:2
assignees: ''

---

## 📋 Description

Implement QLoRA (Quantized Low-Rank Adaptation) for memory-efficient fine-tuning with 4-bit and 8-bit quantization. QLoRA enables training larger models (e.g., Llama-70B) on consumer GPUs by quantizing base model weights while keeping LoRA adapters in full precision.

**Prerequisites**: Phase 1 complete (CPU-based LoRA training), GPU acceleration PR merged  
**Related Paper**: https://arxiv.org/abs/2305.14314 (QLoRA: Efficient Finetuning of Quantized LLMs)  
**Status Document**: `LORA_TRAINING_IMPLEMENTATION_STATUS.md`

## 🎯 Goals

- [ ] 4-bit NormalFloat (NF4) quantization for base models
- [ ] 8-bit integer quantization (INT8)
- [ ] Double quantization for quantization constants
- [ ] Paged optimizers for memory efficiency
- [ ] Full precision LoRA adapters on quantized base
- [ ] Memory usage: ~30-50% of full LoRA
- [ ] Maintain accuracy within 1-2% of full precision

## 📝 Tasks

### 1. Quantization Infrastructure
- [ ] Implement NF4 (4-bit NormalFloat) data type
- [ ] Implement INT8 quantization
- [ ] Quantization/dequantization kernels (CUDA, Vulkan)
- [ ] Block-wise quantization (64-128 elements per block)
- [ ] Double quantization for constants
- [ ] Calibration for quantization ranges

**Files**:
- `include/llm/lora_framework/quantization.h`
- `src/llm/lora_framework/quantization.cpp`
- `src/llm/lora_framework/kernels/quantization_kernels.cu`

**NF4 Data Type**:
```cpp
// 4-bit NormalFloat: optimized for normally distributed weights
// 16 bins: [-1.0, -0.6962, -0.5251, -0.3949, -0.2844, -0.1848, -0.0911, 0.0,
//           0.0796, 0.1609, 0.2461, 0.3379, 0.4407, 0.5626, 0.7230, 1.0]
struct NF4 {
    uint8_t data;  // 2 values per byte
    float scale;   // per-block scaling factor
    float zero;    // per-block zero point
};
```

### 2. Quantized Base Model Loading
- [ ] Load llama.cpp models in quantized format
- [ ] Support GGUF quantized models (Q4_K_M, Q8_0)
- [ ] On-the-fly quantization of full precision models
- [ ] Quantize weights layer-by-layer
- [ ] Memory-mapped quantized weights

**Files**:
- `include/llm/lora_framework/quantized_model.h`
- `src/llm/lora_framework/quantized_model.cpp`

**Supported Formats**:
- NF4 (4-bit NormalFloat) - QLoRA paper
- INT8 (8-bit integer)
- Q4_K_M (GGUF 4-bit with K-means)
- Q8_0 (GGUF 8-bit)

### 3. Mixed Precision Forward Pass
- [ ] Dequantize base weights on-the-fly
- [ ] Compute base model output in FP16/BF16
- [ ] LoRA computation in full precision (FP32 or FP16)
- [ ] Combine quantized base + full precision LoRA
- [ ] Optimize memory bandwidth

**Formula**: `output = dequantize(W_base) @ x + (B @ A) @ x * scaling`

**Memory Optimization**:
- Dequantize only active layers (layer-by-layer)
- Discard dequantized weights after computation
- Keep LoRA in GPU memory

### 4. Backward Pass Through Quantized Model
- [ ] Straight-through estimator for quantization
- [ ] Gradient computation only for LoRA (no base gradients)
- [ ] Handle dequantization in backward pass
- [ ] Memory-efficient gradient computation

**Gradient Flow**:
```
Forward: x → dequant(W_q) → base_out → LoRA → final_out
Backward: grad_out → LoRA_grad → [STOP] (no base_grad)
```

### 5. Double Quantization
- [ ] Quantize quantization constants (scales, zeros)
- [ ] 8-bit quantization for FP32 constants
- [ ] Block-wise constant quantization
- [ ] Memory savings: additional 0.37 bits per param

**Example**:
```
Without: 4-bit weights + 32-bit constants = 4.125 bits/param
With: 4-bit weights + 8-bit constants = 4.03 bits/param
Savings: ~2% additional memory reduction
```

### 6. Paged Optimizers
- [ ] Implement paged AdamW optimizer
- [ ] CPU ↔ GPU paging for optimizer states
- [ ] Offload optimizer states to CPU when not needed
- [ ] Gradient checkpointing integration
- [ ] Unified memory support (if available)

**Files**:
- `include/llm/lora_framework/paged_optimizer.h`
- `src/llm/lora_framework/paged_optimizer.cpp`

**Memory Management**:
```
GPU: Active optimizer states (current batch)
CPU: Inactive optimizer states (paged out)
Transfer: Only when needed (async)
```

### 7. QLoRA Training Loop Integration
- [ ] Update training service for quantized models
- [ ] Configuration for quantization settings
- [ ] Monitor memory usage (VRAM, RAM)
- [ ] Automatic quantization selection based on GPU
- [ ] Fallback to full precision if needed

**Files**:
- `src/llm/lora_framework/lora_training_service.cpp` (update)
- `include/llm/lora_framework/qlora_config.h`

**Config Example**:
```yaml
qlora:
  enabled: true
  base_quantization: "nf4"  # nf4, int8, q4_k_m, q8_0
  compute_dtype: "fp16"     # fp16, bf16, fp32
  double_quant: true
  use_paged_optimizer: true
  optimizer_offload: "cpu"  # cpu, none
```

### 8. Memory Profiling & Optimization
- [ ] Memory usage tracking
- [ ] Peak memory analysis
- [ ] Activation checkpointing
- [ ] Gradient accumulation for large batches
- [ ] Memory benchmarks vs full LoRA

**Target Memory Usage**:
```
Model         Full LoRA    QLoRA (4-bit)    Reduction
--------------------------------------------------------
Llama-7B      ~14 GB      ~5-6 GB          60-65%
Llama-13B     ~26 GB      ~9-10 GB         62-65%
Llama-30B     ~60 GB      ~20-22 GB        63-67%
Llama-65B     ~130 GB     ~40-45 GB        65-69%
```

### 9. Testing
- [ ] Unit tests for quantization/dequantization
- [ ] Numerical accuracy tests (quantized vs full precision)
- [ ] Memory usage tests
- [ ] Training convergence tests
- [ ] Compare QLoRA vs full LoRA accuracy
- [ ] Benchmark training speed

**Files**:
- `tests/test_qlora.cpp`
- `tests/test_quantization.cpp`
- `benchmarks/bench_qlora_memory.cpp`

**Test Scenarios**:
1. Quantization accuracy (reconstruction error < 1%)
2. Training convergence (QLoRA vs LoRA < 2% accuracy difference)
3. Memory savings (measured vs theoretical)
4. Performance (throughput vs memory tradeoff)

### 10. Documentation
- [ ] QLoRA configuration guide
- [ ] Memory requirements calculator
- [ ] Performance tuning guide
- [ ] Quantization format comparison
- [ ] Migration guide from LoRA to QLoRA

**Files**:
- `docs/QLORA_GUIDE.md`
- `docs/QUANTIZATION_FORMATS.md`

## ✅ Acceptance Criteria

- [ ] 4-bit NF4 quantization works correctly
- [ ] 8-bit INT8 quantization works correctly
- [ ] Can load and train on quantized llama.cpp models
- [ ] Memory usage reduced by 60-70% vs full LoRA
- [ ] Accuracy within 1-2% of full precision LoRA
- [ ] Training converges on real datasets
- [ ] Supports Llama-7B to Llama-70B models
- [ ] Paged optimizers work correctly
- [ ] All tests pass
- [ ] Documentation complete

## 🔗 Dependencies

- Phase 1: CPU-based LoRA training
- Phase 2: GPU acceleration (CUDA/Vulkan)
- Phase 2: llama.cpp integration
- CUDA 11.8+ for efficient quantization kernels
- bitsandbytes library (reference) - optional

## 📊 Estimated Effort

**Time**: 4-6 weeks  
**Priority**: 🟡 High (Phase 2, enables large model training)  
**Complexity**: High (quantization, memory management, numerical precision)

## 🧪 Test Strategy

1. **Quantization Tests**: Verify NF4/INT8 accuracy (reconstruction error)
2. **Memory Tests**: Measure actual vs theoretical memory usage
3. **Accuracy Tests**: Compare QLoRA vs full LoRA on benchmarks
4. **Convergence Tests**: Train on Alpaca, verify loss decreases
5. **Large Model Tests**: Test on Llama-30B, Llama-65B (if hardware available)

### Expected Results

```
Metric              Full LoRA    QLoRA (4-bit)    Difference
--------------------------------------------------------------
Memory (7B)         ~14 GB       ~5-6 GB          60% reduction
Accuracy            100%         98-99%           1-2% drop
Speed               1x           0.8-0.9x         10-20% slower
Largest Model       7B (16GB)    65B (40GB)       9x larger
```

## 📚 References

- QLoRA Paper: https://arxiv.org/abs/2305.14314
- bitsandbytes: https://github.com/TimDettmers/bitsandbytes
- GPTQ: https://arxiv.org/abs/2210.17323
- llama.cpp quantization: https://github.com/ggerganov/llama.cpp
- Hugging Face PEFT: https://github.com/huggingface/peft

## 💡 Implementation Notes

### QLoRA vs LoRA

**LoRA**:
- Base model: Full precision (FP16/FP32)
- Memory: ~14 GB for Llama-7B
- Largest model on 16GB GPU: ~7B

**QLoRA**:
- Base model: 4-bit quantized (NF4)
- Memory: ~5-6 GB for Llama-7B
- Largest model on 16GB GPU: ~30B
- Accuracy: 98-99% of full LoRA

### Quantization Formats Comparison

```
Format    Bits    Precision    Speed    Memory    Use Case
---------------------------------------------------------------
FP32      32      Highest      1x       1x        Baseline
FP16      16      High         2x       0.5x      Standard training
INT8      8       Good         3x       0.25x     Inference
NF4       4       Acceptable   4x       0.125x    QLoRA training
```

### NF4 (4-bit NormalFloat)

Optimal for normally distributed weights:
- 16 quantization bins
- Non-uniform spacing (dense near 0)
- Minimal information loss for neural nets
- Better than uniform 4-bit INT

### Memory Breakdown (Llama-7B)

```
Component              Full LoRA    QLoRA        Savings
-------------------------------------------------------------
Base Model Weights     13 GB        3.5 GB       73%
LoRA Adapters          50 MB        50 MB        0%
Optimizer States       50 MB        50 MB*       0%
Activations            2 GB         2 GB         0%
-------------------------------------------------------------
Total                  ~15 GB       ~5.6 GB      63%

* Can be paged to CPU for additional savings
```

### Double Quantization Example

```cpp
// Single quantization
float weights[128];  // Original weights
uint8_t quant[64];   // 4-bit quantized (2 per byte)
float scale;         // 32-bit scale factor
float zero;          // 32-bit zero point

// Double quantization
float weights[128];   // Original weights
uint8_t quant[64];    // 4-bit quantized weights
uint8_t quant_scale;  // 8-bit quantized scale
uint8_t quant_zero;   // 8-bit quantized zero point
float global_scale;   // One 32-bit scale for all blocks
```

### Paged Optimizer Strategy

```
1. Training starts → Optimizer states in GPU
2. Forward pass → Move states to CPU (async)
3. Backward pass → States still in CPU
4. Optimizer step → Move states back to GPU (async)
5. Repeat

Result: ~50% optimizer memory on GPU at any time
```

## 🏁 Definition of Done

- [ ] All tasks completed and checked off
- [ ] NF4 and INT8 quantization implemented
- [ ] QLoRA training works on quantized models
- [ ] Memory savings verified (60-70% reduction)
- [ ] Accuracy maintained (< 2% degradation)
- [ ] Can train Llama-30B on 24GB GPU
- [ ] Paged optimizers functional
- [ ] Code reviewed and approved
- [ ] Tests pass
- [ ] Documentation complete
- [ ] Ready for production use

## 🎯 Success Metrics

- **Memory Efficiency**: Train Llama-30B on 24GB GPU (impossible with full LoRA)
- **Accuracy**: Within 1-2% of full precision LoRA on benchmarks
- **Speed**: 80-90% throughput of full LoRA (acceptable tradeoff)
- **Scalability**: Enable training of 65B+ models on consumer hardware
