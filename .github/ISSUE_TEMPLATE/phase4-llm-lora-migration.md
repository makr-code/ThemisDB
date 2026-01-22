---
name: Phase 4 - LLM/LoRA Migration
about: Track LLM and LoRA modules error handling migration to Result<T> pattern
title: '[Phase 4] LLM/LoRA Migration'
labels: ['error-handling', 'phase-4', 'llm', 'lora', 'refactoring']
assignees: ''
---

## 📋 Module: LLM/LoRA

**Priority:** P0 (Critical)  
**Estimated Effort:** 3-4 weeks  
**Complexity:** High  
**Dependencies:** Phase 4 Foundation PR must be merged

## 🎯 Objective

Migrate LLM inference engine and LoRA framework error handling from legacy patterns (`return nullptr`, exception-based) to unified `Result<T>` pattern using `tl::expected`.

## 📊 Scope

### Files to Migrate

**Model Loader** (`src/llm/model_loader.cpp`):
- [ ] 7 nullptr returns → `Result<Model*>` or `Result<unique_ptr<Model>>`
- [ ] Model file loading errors
- [ ] GPU memory allocation failures
- [ ] Model validation errors

**LlamaCpp Inference Engine** (`src/llm/llamacpp_inference_engine.cpp`):
- [ ] Inference execution errors
- [ ] Batch processing failures
- [ ] Context window overflow handling

**LoRA Framework** (51 files in `src/llm/lora_framework/*.cpp`):
- [ ] 41 nullptr returns → `Result<T*>`
- [ ] 103 Status returns → `Result<T>` pattern
- [ ] Adapter loading failures
- [ ] Hot-swap error scenarios
- [ ] Multi-adapter conflict detection

**Distributed Training Coordinator**:
- [ ] Multi-GPU error propagation
- [ ] Training divergence detection
- [ ] Checkpoint save/load failures

**Total:** 41 nullptr sites + 103 Status returns = **144 migration points**

## 📚 Resources

**Foundation Documentation:**
- Phase 4 Migration Matrix: `docs/error_handling/phase4_migration_matrix.md`
- Migration Example: `docs/error_handling/phase4_week2_getOrCreateColumnFamily_example.md`

**Error Codes Available:**
- `ERR_LLM_MODEL_NOT_FOUND` (4000)
- `ERR_LLM_INFERENCE_FAILED` (4001)
- `ERR_LLM_CONTEXT_OVERFLOW` (4002)
- `ERR_LLM_GPU_OOM` (4003)
- `ERR_LLM_INVALID_CONFIG` (4004)
- `ERR_LLM_TOKENIZATION_FAILED` (4005)
- `ERR_LLM_INCOMPATIBLE_MODEL` (4006)
- `ERR_LLM_CHECKPOINT_FAILED` (4007)
- `ERR_LLM_QUANTIZATION_FAILED` (4008)
- `ERR_LLM_UNSUPPORTED_ARCHITECTURE` (4009)
- `ERR_LLM_INITIALIZATION_FAILED` (4010)
- `ERR_LORA_ADAPTER_NOT_FOUND` (4100)
- `ERR_LORA_INVALID_CONFIG` (4101)
- `ERR_LORA_INCOMPATIBLE_BASE` (4102)
- `ERR_LORA_MERGE_FAILED` (4103)
- `ERR_LORA_LOAD_FAILED` (4104)
- `ERR_LORA_SAVE_FAILED` (4105)
- `ERR_LORA_TRAINING_FAILED` (4106)

**Error Codes to Add:**
- [ ] `ERR_LLM_BATCH_SIZE_EXCEEDED` (4011)
- [ ] `ERR_LORA_ADAPTER_CONFLICT` (4107)
- [ ] `ERR_LORA_TRAINING_DIVERGED` (4108)

## 🔧 Implementation Steps

### Phase 1: Error Code Addition (Week 1 Day 1-2)
- [ ] Add 3 new error codes to error registry
- [ ] Register with detailed metadata
- [ ] Update error documentation

### Phase 2: Model Loader (Week 1)
- [ ] Migrate model file loading (3 nullptr)
- [ ] Migrate GPU allocation (2 nullptr)
- [ ] Migrate model validation (2 nullptr)
- [ ] Update call sites across inference engine
- [ ] Add unit tests for loading failures
- [ ] Add GPU OOM simulation tests
- [ ] Build verification

### Phase 3: LoRA Adapter Management (Week 1-2)
- [ ] Migrate adapter loading functions (15 nullptr + 30 Status)
- [ ] Migrate adapter hot-swap logic (10 nullptr + 25 Status)
- [ ] Migrate adapter conflict detection
- [ ] Migrate adapter merging operations (5 nullptr + 20 Status)
- [ ] Update call sites
- [ ] Add unit tests for adapter conflicts
- [ ] Add multi-adapter error tests
- [ ] Build verification

### Phase 4: Inference Engine (Week 2-3)
- [ ] Migrate batch processing (remaining nullptr + Status)
- [ ] Migrate async inference error handling
- [ ] Migrate context management
- [ ] Convert exception-based to Result<T>
- [ ] Update call sites
- [ ] Add inference failure tests
- [ ] Add batch size exceeded tests
- [ ] Build verification

### Phase 5: Distributed Training (Week 3)
- [ ] Migrate multi-GPU error propagation
- [ ] Migrate training divergence detection
- [ ] Migrate checkpoint operations
- [ ] Update call sites
- [ ] Add GPU failure simulation
- [ ] Add training divergence tests
- [ ] Build verification

### Phase 6: Testing & Validation (Week 3-4)
- [ ] Update ~20 existing test files
- [ ] Add GPU failure simulation tests
- [ ] Add model loading stress tests
- [ ] Add multi-adapter conflict tests
- [ ] Add async inference error tests
- [ ] Performance benchmarking (ensure <5% inference overhead)
- [ ] Code review and refinement
- [ ] Documentation updates

## ✅ Acceptance Criteria

- [ ] All 144 LLM/LoRA functions migrated to `Result<T>` pattern
- [ ] All call sites updated to use Result<T> checks
- [ ] 3 new error codes added and registered
- [ ] Exception-based error handling converted to Result<T>
- [ ] Zero build warnings or errors
- [ ] All unit tests passing
- [ ] GPU simulation tests passing
- [ ] Inference performance regression <5%
- [ ] Code review approved
- [ ] Documentation updated

## 📝 Migration Pattern

```cpp
// BEFORE: nullptr + exception pattern
Model* loadModel(const std::string& path) {
    try {
        if (!fs::exists(path)) return nullptr;
        
        auto* model = new Model();
        model->load(path);
        
        if (!validateModel(model)) {
            delete model;
            return nullptr;
        }
        
        return model;
    } catch (const std::exception& e) {
        LOG_ERROR("Model loading failed: {}", e.what());
        return nullptr;
    }
}

// AFTER: Result<T> pattern
Result<std::unique_ptr<Model>> loadModel(const std::string& path) {
    if (!fs::exists(path)) {
        return Err<std::unique_ptr<Model>>(
            ERR_LLM_MODEL_NOT_FOUND,
            fmt::format("Model file not found: {}", path)
        );
    }
    
    auto model = std::make_unique<Model>();
    
    auto load_result = model->load(path);
    if (!load_result) {
        return Err<std::unique_ptr<Model>>(
            ERR_LLM_INITIALIZATION_FAILED,
            fmt::format("Model load failed: {}", load_result.error().message())
        );
    }
    
    auto valid_result = validateModel(model.get());
    if (!valid_result) {
        return Err<std::unique_ptr<Model>>(
            ERR_LLM_INCOMPATIBLE_MODEL,
            fmt::format("Model validation failed: {}", valid_result.error().message())
        );
    }
    
    return Ok(std::move(model));
}

// Call site update with RAII
auto model_result = loadModel(model_path);
if (model_result) {
    auto model = std::move(*model_result);
    // use model (automatically cleaned up)
} else {
    LOG_ERROR("Failed to load model: {}", model_result.error().message());
    return model_result.error();
}
```

## 🔗 Related Issues

- Depends on: Phase 4 Foundation PR
- Coordinates with: Query Engine Migration (inference integration)
- High priority due to GPU resource management

## 📊 Progress Tracking

**Week 1:** ⬜⬜⬜⬜⬜ 0%  
**Week 2:** ⬜⬜⬜⬜⬜ 0%  
**Week 3:** ⬜⬜⬜⬜⬜ 0%  
**Week 4:** ⬜⬜⬜⬜⬜ 0%

**Overall:** 0 of 144 functions migrated (0%)

**Breakdown:**
- Model Loader: 0 / 7 (0%)
- LoRA Framework: 0 / 103 (0%)
- Inference Engine: 0 / 20 (0%)
- Distributed Training: 0 / 14 (0%)

## ⚠️ High Risk Areas

- **GPU Memory:** Critical to preserve proper GPU resource cleanup
- **Multi-GPU:** Error propagation across GPUs is complex
- **Async Operations:** Inference errors must propagate through async boundaries
- **Performance Critical:** Inference hot path must maintain performance
- **51 LoRA Files:** Large surface area requiring careful coordination

## 💬 Notes

- Most complex module in Phase 4 (High complexity rating)
- GPU resource management is critical - use RAII with Result<unique_ptr<T>>
- Exception-based code requires careful conversion to avoid resource leaks
- Async error handling needs special attention
- LoRA hot-swap is a critical feature - test thoroughly
- Coordinate with infrastructure team on GPU simulation testing

---
**Assigned to:** TBD  
**Started:** TBD  
**Target Completion:** TBD  
**Actual Completion:** TBD
