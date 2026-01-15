---
name: "🦙 llama.cpp Base Model Integration"
about: Integrate LoRA training with llama.cpp base models (Phase 2)
title: "[LoRA] Integrate LoRA Training with llama.cpp Base Models"
labels: priority:P1, type:feature, area:llm, effort:large, phase:2
assignees: ''

---

## 📋 Description

Integrate LoRA training with llama.cpp base models, enabling fine-tuning of actual LLM models (Llama, Mistral, etc.) instead of training standalone LoRA layers. This connects the training system with real language models.

**Prerequisites**: Phase 1 complete (LoRA training working)  
**Related Issue**: #[Phase 1 Issue Number]  
**Reference**: https://github.com/ggerganov/llama.cpp

## 🎯 Goals

- [ ] Load frozen llama.cpp base model weights
- [ ] Inject LoRA adapters into model layers
- [ ] Forward pass: base model + LoRA
- [ ] Backward pass: only through LoRA (freeze base)
- [ ] Weight merging (optional)
- [ ] Test with actual Llama/Mistral models

## 📝 Tasks

### 1. Base Model Loading
- [ ] Load GGUF model files via llama.cpp
- [ ] Extract base model weights (frozen)
- [ ] Identify attention/linear layers for LoRA
- [ ] Create layer mapping (model → LoRA)
- [ ] Support different model architectures

**Files**:
- `include/llm/lora_framework/base_model_adapter.h`
- `src/llm/lora_framework/base_model_adapter.cpp`

**Supported Models**:
- Llama 2 (7B, 13B, 70B)
- Llama 3 (8B, 70B)
- Mistral (7B)
- CodeLlama
- Any llama.cpp compatible model

### 2. LoRA Injection
- [ ] Identify target layers (attention, MLP)
- [ ] Create LoRA adapters for each layer
- [ ] Inject adapters without modifying base weights
- [ ] Support selective layer adaptation (e.g., Q+V only)
- [ ] Memory efficient injection

**Target Layers**:
- `attention.wq` (Query projection)
- `attention.wk` (Key projection)
- `attention.wv` (Value projection)
- `attention.wo` (Output projection)
- `feed_forward.w1`, `w2`, `w3` (MLP layers)

### 3. Forward Pass Integration
- [ ] Base model forward pass (frozen)
- [ ] LoRA forward pass on top
- [ ] Combine outputs: `output = base_output + lora_output * scaling`
- [ ] Cache activations for backward pass
- [ ] Efficient tensor passing between base and LoRA

**Formula**: `h' = h + ΔW * h` where `ΔW = B @ A * α/r`

### 4. Backward Pass (LoRA Only)
- [ ] Gradient computation only through LoRA layers
- [ ] Freeze base model (no gradient computation)
- [ ] Efficient gradient flow
- [ ] Memory optimization (no base model gradients)

### 5. Model Configuration
- [ ] LoRA rank per layer
- [ ] Alpha scaling factor
- [ ] Target layers selection
- [ ] Quantization support (4-bit, 8-bit base)
- [ ] Configuration via YAML/JSON

**Example Config**:
```yaml
base_model: "models/llama-2-7b.gguf"
lora:
  rank: 16
  alpha: 32
  target_modules:
    - "attention.wq"
    - "attention.wv"
  dropout: 0.05
```

### 6. Weight Merging (Optional)
- [ ] Merge LoRA weights into base model
- [ ] Export merged model
- [ ] Quantization-aware merging
- [ ] Verify merged model accuracy

**Formula**: `W_merged = W_base + B @ A * α/r`

### 7. Text Data Processing
- [ ] Tokenization via llama.cpp tokenizer
- [ ] Dataset loading (JSONL, Alpaca, ShareGPT formats)
- [ ] Prompt formatting
- [ ] Batch collation
- [ ] Data augmentation

**Files**:
- `include/llm/lora_framework/data_loader.h`
- `src/llm/lora_framework/data_loader.cpp`

### 8. Testing
- [ ] Unit tests for model loading
- [ ] Unit tests for LoRA injection
- [ ] Integration test: train on toy dataset
- [ ] Test with actual Llama-2-7B
- [ ] Verify loss decrease on real data
- [ ] Compare with reference implementations

**Files**:
- `tests/test_lora_llama_integration.cpp`
- `tests/test_data_loader.cpp`

## ✅ Acceptance Criteria

- [ ] Can load llama.cpp models (GGUF format)
- [ ] LoRA adapters correctly injected
- [ ] Forward pass produces valid outputs
- [ ] Backward pass computes gradients only for LoRA
- [ ] Training converges on real text dataset
- [ ] Memory efficient (base model shared, only LoRA parameters trained)
- [ ] Supports multiple model architectures
- [ ] Can export trained adapters
- [ ] All tests pass

## 🔗 Dependencies

- llama.cpp (already integrated as submodule)
- Phase 1 LoRA training implementation
- Tokenization infrastructure
- (Optional) GPU support for faster training

## 📊 Estimated Effort

**Time**: 3-4 weeks  
**Priority**: 🟡 High (Phase 2, Week 15-18)  
**Complexity**: High (model integration, architecture-specific code)

## 🧪 Test Strategy

1. **Model Loading**: Test GGUF loading for various models
2. **Injection**: Verify LoRA adapters in correct layers
3. **Toy Dataset**: Train on small synthetic dataset (10-100 samples)
4. **Real Dataset**: Train on Alpaca/ShareGPT subset (1k samples)
5. **Convergence**: Verify loss decreases over epochs
6. **Quality**: Generate text samples, verify improvement

### Test Datasets

```
1. Toy Dataset (testing):
   - 10 instruction-response pairs
   - Simple patterns
   - Quick convergence check

2. Alpaca Subset (validation):
   - 1,000 high-quality samples
   - Diverse instructions
   - ~1 hour training on GPU

3. Full Training (optional):
   - 50,000+ samples
   - Multi-epoch training
   - Production-quality adapter
```

## 📚 References

- llama.cpp: https://github.com/ggerganov/llama.cpp
- LoRA Paper: https://arxiv.org/abs/2106.09685
- Llama 2 Paper: https://arxiv.org/abs/2307.09288
- PEFT Library (reference): https://github.com/huggingface/peft
- Alpaca Dataset: https://github.com/tatsu-lab/stanford_alpaca

## 💡 Implementation Notes

### Base Model Memory Usage
```
Model          Precision  Memory   LoRA Overhead
------------------------------------------------
Llama-2-7B     FP16       ~14 GB   ~50 MB
Llama-2-7B     Q4_K_M     ~4 GB    ~50 MB
Llama-2-13B    FP16       ~26 GB   ~80 MB
Llama-2-70B    FP16       ~140 GB  ~200 MB
```

### LoRA Configuration Examples

**Light Fine-tuning** (quick experiments):
```cpp
rank = 4, alpha = 8, target = ["attention.wq", "attention.wv"]
Parameters: ~10 MB
Training time: ~30 min (1k samples)
```

**Standard Fine-tuning** (recommended):
```cpp
rank = 16, alpha = 32, target = ["attention.wq", "attention.wk", "attention.wv", "attention.wo"]
Parameters: ~50 MB
Training time: ~2 hours (10k samples)
```

**Comprehensive Fine-tuning** (best quality):
```cpp
rank = 64, alpha = 128, target = all attention + MLP layers
Parameters: ~200 MB
Training time: ~8 hours (50k samples)
```

### Architecture Mapping

**Llama/Mistral**:
- `layers[i].attention.wq/wk/wv/wo` → Query/Key/Value/Output
- `layers[i].feed_forward.w1/w2/w3` → MLP gates/down/up

**GPT-NeoX**:
- `layers[i].attention.query_key_value` → Combined QKV
- `layers[i].mlp.dense_h_to_4h/dense_4h_to_h` → MLP

## 🏁 Definition of Done

- [ ] All tasks completed and checked off
- [ ] llama.cpp models load correctly
- [ ] LoRA injection works for multiple architectures
- [ ] Training works on real text data
- [ ] Loss decreases consistently
- [ ] Generated text shows quality improvement
- [ ] Code reviewed and approved
- [ ] Tests pass
- [ ] Documentation complete
- [ ] Ready for production fine-tuning
