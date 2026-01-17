---
name: "🔌 LoRa Adapter Application Implementation"
about: Apply loaded LoRa adapters to inference models (Kritisch - P0)
title: "[LoRa] Implement LoRa Adapter Application to Loaded Models"
labels: priority:P0, type:feature, area:llm, effort:medium, phase:production
assignees: ''

---

## 📋 Beschreibung / Description

**DE**: Geladene LoRa Adapter werden aktuell nicht auf Modelle angewendet. Fine-tuned Modelle verhalten sich identisch zu Base Models, da die Adapter-Gewichte nicht in die Inferenz einfließen.

**EN**: Loaded LoRa adapters are currently not applied to models. Fine-tuned models behave identically to base models because adapter weights are not used in inference.

**Related Analysis**: `REMAINING_GAPS_SUMMARY.md` §3 (Priority 0)  
**Current Status**: `src/llm/llamacpp_inference_engine.cpp:205` - TODO comment  
**Blocker**: ❌ **PRODUKTIONSBLOCKER** - LoRa Adapter funktionieren nicht

## 🎯 Ziele / Goals

- [ ] LoRa Adapter auf geladene Modelle anwenden
- [ ] Integration mit llama.cpp LoRa API
- [ ] Validierung dass Adapter korrekt angewendet wurden
- [ ] Support für multiple Adapter gleichzeitig
- [ ] Tests mit bekannten Adaptern

## 📝 Aufgaben / Tasks

### 1. llama.cpp LoRa API Integration
**Priorität**: P0 - Kritisch

**Current Code** (Line 205):
```cpp
// 3. TODO: Apply adapter to loaded model
```

**Implementation Steps**:
- [ ] Research llama.cpp LoRa adapter API
- [ ] Identify correct functions for adapter application
- [ ] Understand adapter format requirements (safetensors vs GGUF)
- [ ] Document API usage patterns

**llama.cpp API Reference**:
```cpp
// Research these llama.cpp functions:
// - llama_lora_adapter_init()
// - llama_lora_adapter_set()
// - llama_lora_adapter_remove()
// - llama_lora_adapter_clear()
```

**File**: `src/llm/llamacpp_inference_engine.cpp`

---

### 2. Adapter Loading from Storage
**Priorität**: P0 - Kritisch

**Implementation**:
```cpp
bool LlamaCppInferenceEngine::loadAndApplyLoRAAdapter(
    const std::string& adapter_id,
    float scale = 1.0f
) {
    spdlog::info("Loading and applying LoRA adapter: {}", adapter_id);
    
    // 1. Load adapter weights from storage
    auto storage = getLoRAStorageService();
    auto weights_opt = storage->loadAdapter(adapter_id);
    
    if (!weights_opt) {
        spdlog::error("Failed to load adapter: {}", adapter_id);
        return false;
    }
    
    auto& weights = *weights_opt;
    spdlog::info("Adapter loaded: {} bytes, rank={}", 
                 weights.size_bytes, weights.hyperparameters.rank);
    
    // 2. Convert to llama.cpp format if needed
    std::string adapter_path = convertToLlamaCppFormat(weights);
    
    // 3. Apply adapter to model
    int adapter_id_int = llama_lora_adapter_init(model_, adapter_path.c_str());
    if (adapter_id_int < 0) {
        spdlog::error("Failed to initialize adapter: {}", adapter_id);
        return false;
    }
    
    // 4. Set adapter with scale
    int result = llama_lora_adapter_set(context_, adapter_id_int, scale);
    if (result != 0) {
        spdlog::error("Failed to set adapter: {}", adapter_id);
        llama_lora_adapter_remove(adapter_id_int);
        return false;
    }
    
    // 5. Track active adapters
    active_adapters_[adapter_id] = adapter_id_int;
    
    spdlog::info("✓ LoRA adapter applied successfully: {} (scale={})", 
                 adapter_id, scale);
    return true;
}
```

**Tasks**:
- [ ] Implement `loadAndApplyLoRAAdapter()` method
- [ ] Handle adapter format conversion (safetensors → llama.cpp)
- [ ] Add error handling for each step
- [ ] Track active adapters in map
- [ ] Add logging for debugging

---

### 3. Format Conversion
**Priorität**: P0 - Kritisch

**Implementation**:
```cpp
std::string LlamaCppInferenceEngine::convertToLlamaCppFormat(
    const LoRAAdapterWeights& weights
) {
    // Check if already in correct format
    if (weights.format == "gguf" || weights.format == "llama.cpp") {
        // Save directly to temp file
        return saveTempAdapter(weights);
    }
    
    // Convert safetensors to llama.cpp format
    if (weights.format == "safetensors") {
        spdlog::debug("Converting adapter from safetensors to llama.cpp format");
        
        // 1. Parse safetensors format
        auto tensors = parseSafetensors(weights.data);
        
        // 2. Convert to llama.cpp GGUF format
        auto gguf_data = convertToGGUF(tensors, weights.hyperparameters);
        
        // 3. Save to temp file
        std::string temp_path = getTempAdapterPath();
        saveToFile(temp_path, gguf_data);
        
        return temp_path;
    }
    
    throw std::runtime_error("Unsupported adapter format: " + weights.format);
}
```

**Tasks**:
- [ ] Implement format detection
- [ ] Add safetensors parser (if needed)
- [ ] Implement GGUF conversion
- [ ] Handle temporary file management
- [ ] Add cleanup for temp files

---

### 4. Multi-Adapter Support
**Priorität**: P1 - High

**Implementation**:
```cpp
bool LlamaCppInferenceEngine::applyMultipleAdapters(
    const std::vector<std::pair<std::string, float>>& adapters
) {
    // adapters: vector of (adapter_id, scale) pairs
    
    spdlog::info("Applying {} LoRA adapters", adapters.size());
    
    bool all_success = true;
    for (const auto& [adapter_id, scale] : adapters) {
        if (!loadAndApplyLoRAAdapter(adapter_id, scale)) {
            spdlog::error("Failed to apply adapter: {}", adapter_id);
            all_success = false;
            // Continue trying other adapters
        }
    }
    
    if (all_success) {
        spdlog::info("✓ All {} adapters applied successfully", adapters.size());
    } else {
        spdlog::warn("⚠️ Some adapters failed to apply");
    }
    
    return all_success;
}

bool LlamaCppInferenceEngine::removeAdapter(const std::string& adapter_id) {
    auto it = active_adapters_.find(adapter_id);
    if (it == active_adapters_.end()) {
        spdlog::warn("Adapter not active: {}", adapter_id);
        return false;
    }
    
    int adapter_id_int = it->second;
    llama_lora_adapter_remove(adapter_id_int);
    active_adapters_.erase(it);
    
    spdlog::info("Adapter removed: {}", adapter_id);
    return true;
}

void LlamaCppInferenceEngine::clearAllAdapters() {
    spdlog::info("Clearing all {} active adapters", active_adapters_.size());
    
    llama_lora_adapter_clear(context_);
    active_adapters_.clear();
}
```

**Tasks**:
- [ ] Implement multiple adapter application
- [ ] Add adapter removal functionality
- [ ] Implement clear all adapters
- [ ] Handle adapter conflicts
- [ ] Add adapter listing/info methods

---

### 5. Validation and Testing
**Priorität**: P0 - Kritisch

**Validation Strategy**:
```cpp
bool LlamaCppInferenceEngine::validateAdapterApplication(
    const std::string& adapter_id
) {
    // 1. Check if adapter is in active list
    if (active_adapters_.find(adapter_id) == active_adapters_.end()) {
        spdlog::error("Adapter not in active list: {}", adapter_id);
        return false;
    }
    
    // 2. Run inference with and without adapter
    std::string test_prompt = "Hello, world!";
    
    // Generate with adapter
    auto result_with = generateSync(test_prompt);
    
    // Temporarily remove adapter
    removeAdapter(adapter_id);
    
    // Generate without adapter
    auto result_without = generateSync(test_prompt);
    
    // Re-apply adapter
    loadAndApplyLoRAAdapter(adapter_id);
    
    // 3. Compare results (should be different)
    bool outputs_differ = (result_with != result_without);
    
    if (outputs_differ) {
        spdlog::info("✓ Adapter validation passed: outputs differ");
    } else {
        spdlog::warn("⚠️ Adapter validation: outputs identical (adapter may not be applied)");
    }
    
    return outputs_differ;
}
```

**Test Cases**:
```cpp
// Test file: tests/test_lora_adapter_application.cpp

TEST(LoRAAdapterApplicationTest, BasicAdapterApplication) {
    // Load model, load adapter, apply, verify different output
}

TEST(LoRAAdapterApplicationTest, MultipleAdapters) {
    // Apply multiple adapters with different scales
}

TEST(LoRAAdapterApplicationTest, AdapterRemoval) {
    // Apply adapter, remove, verify output reverts
}

TEST(LoRAAdapterApplicationTest, AdapterScaling) {
    // Test different scale values (0.0, 0.5, 1.0, 2.0)
}

TEST(LoRAAdapterApplicationTest, InvalidAdapter) {
    // Try to apply non-existent adapter, should fail gracefully
}

TEST(LoRAAdapterApplicationTest, FormatConversion) {
    // Load safetensors adapter, verify conversion works
}
```

**Tasks**:
- [ ] Create comprehensive test suite
- [ ] Test with known adapters (e.g., Alpaca, ShareGPT)
- [ ] Verify output changes with adapter
- [ ] Test error cases
- [ ] Add performance benchmarks

---

## ✅ Akzeptanzkriterien / Acceptance Criteria

- [ ] LoRA adapters can be loaded from storage
- [ ] Adapters are correctly applied to loaded models
- [ ] Model output changes when adapter is applied (validation passes)
- [ ] Multiple adapters can be applied simultaneously
- [ ] Adapter removal/clearing works correctly
- [ ] Format conversion works for safetensors and GGUF
- [ ] Comprehensive tests pass (>90% coverage)
- [ ] Documentation updated with usage examples
- [ ] No regressions in existing functionality

## 📊 Effort Estimation

- **Aufwand / Effort**: 1 week (Medium)
- **Komplexität / Complexity**: Medium-High
- **Risiko / Risk**: Medium (depends on llama.cpp API stability)

## 🔗 Related Issues

- Issue #11: LoRa Storage Backend (must be complete first)
- Issue #07: LoRa llama.cpp Integration
- Original analysis: `REMAINING_GAPS_SUMMARY.md` §3

## 📚 References

- llama.cpp LoRA documentation: https://github.com/ggerganov/llama.cpp/tree/master/examples/lora
- Code location: `src/llm/llamacpp_inference_engine.cpp:205`
- Storage service: `src/llm/lora_framework/lora_storage_service.cpp`
- Adapter format: `include/llm/lora_framework/lora_config.h`

---

**Priority**: P0 - Must fix before production  
**Impact**: LoRa fine-tuning completely non-functional without this  
**Status**: Ready to implement (after storage backend complete)
