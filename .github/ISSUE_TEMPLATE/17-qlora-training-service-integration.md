---
name: "🔗 QLoRA Training Service Integration"
about: Integrate QLoRA infrastructure with training service for end-to-end training
title: "[QLoRA] Training Service Integration"
labels: priority:P1, type:feature, area:llm, effort:medium, phase:2-integration
assignees: ''

---

## 📋 Description

Integrate the completed QLoRA infrastructure (quantization, quantized model, QLoRALayer) with the LoRA training service to enable end-to-end quantized LoRA training.

**Prerequisites**: 
- ✅ QLoRA Infrastructure Complete (PR #[number])
- ✅ NF4/INT8 quantization implemented
- ✅ QLoRALayer with mixed precision
- ⏳ Training service exists

**Related Documents**: 
- `QLORA_IMPLEMENTATION_SUMMARY.md`
- `docs/QLORA_GUIDE.md`
- `LORA_TRAINING_IMPLEMENTATION_STATUS.md`

## 🎯 Goals

- [ ] Wire QLoRA infrastructure into training service
- [ ] Add configuration options for quantization
- [ ] Support loading quantized base models
- [ ] Enable QLoRA training via API
- [ ] End-to-end training tests on real datasets
- [ ] Convergence validation (accuracy < 2% degradation)

## 📝 Tasks

### 1. Training Service Updates
- [ ] Add QLoRA mode to training service
- [ ] Support quantized model loading
- [ ] Configure quantization type (NF4, INT8)
- [ ] Handle mixed precision training flow
- [ ] Memory profiling integration

**Files to Update**:
- `src/llm/lora_framework/lora_training_service.cpp`
- `include/llm/lora_framework/lora_training_service.h`

**Implementation**:
```cpp
// Add QLoRA support to training service
class LoRATrainingService {
    // New: QLoRA training mode
    void trainWithQuantization(
        const QuantizedModel& base_model,
        const LoRATrainingConfig& config
    );
    
    // Helper: Create QLoRA layers
    std::vector<std::unique_ptr<QLoRALayer>> createQLoRALayers(
        const QuantizedModel& model,
        size_t rank
    );
};
```

### 2. Configuration System
- [ ] Add QLoRA config to LoRATrainingConfig
- [ ] Quantization type selection
- [ ] Block size configuration
- [ ] Double quantization toggle
- [ ] Layer-by-layer mode settings

**Files**:
- `include/llm/lora_framework/lora_training_config.h`
- `src/llm/lora_framework/lora_training_config.cpp`

**Config Structure**:
```cpp
struct QLoRAConfig {
    bool enabled = false;
    QuantizationType quantization_type = QuantizationType::NF4;
    size_t block_size = 64;
    bool use_double_quantization = false;
    bool layer_by_layer = true;
    
    // Future: paged optimizer settings
    bool use_paged_optimizer = false;
    std::string optimizer_offload = "none";  // "cpu", "none"
};

// Add to LoRATrainingConfig
struct LoRATrainingConfig {
    // ... existing fields ...
    
    // NEW: QLoRA configuration
    QLoRAConfig qlora;
};
```

### 3. Model Loading Integration
- [ ] Load base model in quantized format
- [ ] Support both pre-quantized and on-the-fly quantization
- [ ] Validate model size vs available memory
- [ ] Automatic fallback to full precision if needed
- [ ] Memory usage monitoring

**Functions**:
```cpp
// Load and optionally quantize model
std::unique_ptr<QuantizedModel> loadBaseModel(
    const std::string& model_path,
    const QLoRAConfig& config
);

// Estimate memory requirements
size_t estimateMemoryUsage(
    const std::string& model_path,
    const QLoRAConfig& config
);
```

### 4. Training Loop Integration
- [ ] Use QLoRALayer instead of LoRALayer when quantization enabled
- [ ] Handle forward/backward passes correctly
- [ ] Update optimizer to work with QLoRA parameters
- [ ] Track quantization-specific metrics
- [ ] Memory usage tracking

**Training Flow**:
```
1. Load base model → Quantize (if needed)
2. Create QLoRA layers with quantized base
3. Initialize optimizer (only LoRA parameters)
4. Training loop:
   - Forward: dequantize base, apply LoRA
   - Backward: gradients only for LoRA
   - Update: optimizer step
5. Save LoRA adapters (not base model)
```

### 5. End-to-End Testing
- [ ] Test on small dataset (Alpaca-100)
- [ ] Verify training converges
- [ ] Measure actual memory usage
- [ ] Compare accuracy vs full-precision LoRA
- [ ] Benchmark training speed

**Test Cases**:
1. Small model (Llama-7B) with NF4
2. Small model with INT8
3. Compare QLoRA vs LoRA accuracy
4. Memory usage validation
5. Training speed benchmarks

**Files**:
- `tests/test_qlora_training_integration.cpp`

### 6. API Endpoints
- [ ] Add QLoRA config to training API
- [ ] Expose quantization options
- [ ] Return memory usage statistics
- [ ] Show quantization metrics

**API Changes**:
```json
// POST /api/lora/train
{
  "model_path": "llama-7b.gguf",
  "dataset": "alpaca-100",
  "config": {
    "rank": 8,
    "alpha": 16,
    "learning_rate": 0.0001,
    "qlora": {
      "enabled": true,
      "quantization_type": "nf4",
      "block_size": 64,
      "use_double_quantization": true
    }
  }
}
```

### 7. Documentation Updates
- [ ] Update training guide with QLoRA examples
- [ ] Add troubleshooting section
- [ ] Document configuration options
- [ ] Provide memory estimation guide

**Files**:
- `docs/LORA_TRAINING_GUIDE.md` (update)
- `docs/QLORA_GUIDE.md` (already exists)

## ✅ Acceptance Criteria

- [ ] Can train with QLoRA via training service API
- [ ] Supports NF4 and INT8 quantization
- [ ] Memory usage reduced by 60-80% vs full LoRA
- [ ] Accuracy within 2% of full-precision LoRA
- [ ] Training converges on test dataset
- [ ] All integration tests pass
- [ ] Documentation complete
- [ ] Ready for production use

## 🔗 Dependencies

- ✅ QLoRA Infrastructure (Phase 1-3 complete)
- ✅ Quantization module
- ✅ QLoRALayer implementation
- ⏳ Training service (modify)
- ⏳ Real dataset support (optional, can use synthetic)

## 📊 Estimated Effort

**Time**: 2-3 weeks  
**Priority**: 🟡 High (P1 - enables QLoRA usage)  
**Complexity**: Medium (integration work)

## 🧪 Test Strategy

1. **Unit Tests**: QLoRA-specific training functions
2. **Integration Tests**: End-to-end training flow
3. **Convergence Tests**: Verify training works on real data
4. **Memory Tests**: Measure actual vs theoretical usage
5. **Performance Tests**: Compare speed vs full LoRA

### Expected Results

```
Metric              Full LoRA    QLoRA (NF4)    Status
-----------------------------------------------------------
Memory (7B)         ~14 GB       ~5-6 GB        Target: 60-70% reduction
Accuracy            100%         98-99%         Target: < 2% degradation
Speed               1x           0.8-0.9x       Acceptable tradeoff
Training Steps      1000         1000           Should converge
```

## 📚 References

- QLoRA Paper: https://arxiv.org/abs/2305.14314
- Implementation Summary: `QLORA_IMPLEMENTATION_SUMMARY.md`
- User Guide: `docs/QLORA_GUIDE.md`
- Quantization Formats: `docs/QUANTIZATION_FORMATS.md`

## 💡 Implementation Notes

### Integration Points

1. **Training Service**: Add `trainWithQLoRA()` method
2. **Configuration**: Extend `LoRATrainingConfig` with `QLoRAConfig`
3. **Model Loading**: Support quantized model loading
4. **Training Loop**: Use `QLoRALayer` when quantization enabled
5. **API**: Add QLoRA options to training endpoints

### Challenges

- **Memory Estimation**: Need accurate estimates before training starts
- **Error Handling**: Graceful fallback if quantization fails
- **Testing**: Need real datasets for convergence validation

### Success Metrics

- ✅ Can train Llama-7B on 8GB GPU (impossible with full LoRA)
- ✅ Accuracy within 2% of full precision
- ✅ Training time acceptable (<20% overhead)
- ✅ Memory savings match theoretical estimates

## 🏁 Definition of Done

- [ ] QLoRA training works end-to-end
- [ ] Configuration system in place
- [ ] All tests passing
- [ ] Memory reduction validated
- [ ] Accuracy meets targets
- [ ] Documentation complete
- [ ] Code reviewed and merged
- [ ] Ready for production deployment
