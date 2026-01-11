---
name: Error Code Migration - Phase 1 (Critical Priority)
about: Migrate critical error logging locations to use structured error codes
title: '[TASK] Error Code Migration - Phase 1: Critical Priority Errors'
labels: 'enhancement, error-handling, refactoring, priority-high'
assignees: ''
---

# Error Code Migration - Phase 1: Critical Priority

## 📋 Summary

Migrate critical error logging locations from `spdlog::error()` to the new structured error code system using `errors::logError()`. This is Phase 1 focusing on **CRITICAL** priority errors that affect core functionality.

## 🎯 Objectives

Migrate the following critical error locations to use structured error codes from the Error Registry:

### 🔴 CRITICAL (Immediate - Phase 1)

1. **GPU OOM Errors** - Most frequent error source in LLM operations (8 locations)
2. **Model Load Failed** - Blocks basic functionality (4 locations)
3. **LoRA Not Loaded** - Important for Multi-LoRA features (5 locations)

**Total locations in Phase 1:** ~17 error logging statements

## 📝 Detailed Migration List

### 1. GPU Out of Memory Errors (ERR_LLM_GPU_OOM - 2004)

**File: `src/llm/gpu_memory_manager.cpp`**

| Line | Current Code | Target Code |
|------|--------------|-------------|
| 229 | `spdlog::error("Cannot allocate {} bytes VRAM for model {}: insufficient memory", ...)` | `errors::logError(ErrorCode::ERR_LLM_GPU_OOM, bytes, available_memory);` |
| 968 | `spdlog::error("Cannot allocate {} bytes on GPU {}: insufficient memory (used: {}, max: {})", ...)` | `errors::logError(ErrorCode::ERR_LLM_GPU_OOM, bytes, available_memory);` |
| 248 | `spdlog::error("Failed to allocate {} bytes for model {} (simulation)", bytes, model_id);` | `errors::logError(ErrorCode::ERR_LLM_GPU_OOM, bytes, 0);` |
| 256 | `spdlog::error("Failed to allocate {} bytes for model {} (simulation)", bytes, model_id);` | `errors::logError(ErrorCode::ERR_LLM_GPU_OOM, bytes, 0);` |
| 990 | `spdlog::error("Failed to allocate {} bytes on GPU {} for model {} (simulation)", ...)` | `errors::logError(ErrorCode::ERR_LLM_GPU_OOM, bytes, 0);` |
| 999 | `spdlog::error("Failed to allocate {} bytes on GPU {} for model {} (simulation)", ...)` | `errors::logError(ErrorCode::ERR_LLM_GPU_OOM, bytes, 0);` |

### 2. Model Load Failed (ERR_LLM_MODEL_LOAD_FAILED - 2001)

**Files:**

| File | Line | Current Code | Target Code |
|------|------|--------------|-------------|
| `src/llm/model_loader.cpp` | 341 | `spdlog::error("Failed to load model from file: {}", model_path);` | `errors::logError(ErrorCode::ERR_LLM_MODEL_LOAD_FAILED, model_path);` |
| `src/llm/llama_wrapper.cpp` | 292 | `spdlog::error("Failed to load model: {}", model_path);` | `errors::logError(ErrorCode::ERR_LLM_MODEL_LOAD_FAILED, model_path);` |
| `src/llm/embedded_llm.cpp` | 41 | `spdlog::error("Failed to load model: {}", config.model_path);` | `errors::logError(ErrorCode::ERR_LLM_MODEL_LOAD_FAILED, config.model_path);` |
| `src/llm/llm_plugin_manager.cpp` | 426 | `spdlog::error("Failed to load model: {}", model_path);` | `errors::logError(ErrorCode::ERR_LLM_MODEL_LOAD_FAILED, model_path);` |

### 3. Model Not Found (ERR_LLM_MODEL_NOT_FOUND - 2000)

**File: `src/llm/model_loader.cpp`**

| Line | Current Code | Target Code |
|------|--------------|-------------|
| 307 | `spdlog::error("Model file not found: {}", model_path);` | `errors::logError(ErrorCode::ERR_LLM_MODEL_NOT_FOUND, model_path);` |

### 4. LoRA Not Loaded (ERR_LORA_NOT_LOADED - 2100)

**Files:**

| File | Line | Current Code | Target Code |
|------|------|--------------|-------------|
| `src/llm/multi_lora_manager.cpp` | 206 | `spdlog::error("LoRA not loaded: {}", lora_id);` | `errors::logError(ErrorCode::ERR_LORA_NOT_LOADED, lora_id);` |
| `src/llm/multi_lora_manager.cpp` | 359 | `spdlog::error("LoRA {} not loaded", lora_ids[i]);` | `errors::logError(ErrorCode::ERR_LORA_NOT_LOADED, lora_ids[i]);` |
| `src/llm/multi_lora_manager.cpp` | 655 | `spdlog::error("Cannot export LoRA: {} not loaded", lora_id);` | `errors::logError(ErrorCode::ERR_LORA_NOT_LOADED, lora_id);` |
| `src/llm/llama_wrapper.cpp` | 378 | `spdlog::error("Cannot load LoRA: no model loaded");` | `errors::logError(ErrorCode::ERR_LORA_NOT_LOADED, "no model");` |
| `src/llm/llama_wrapper.cpp` | 853 | `spdlog::error("Cannot import LoRA: no model loaded");` | `errors::logError(ErrorCode::ERR_LORA_NOT_LOADED, "no model");` |

## 🔧 Implementation Steps

### Step 1: Add Required Include

Add to affected files:
```cpp
#include "utils/error_registry.h"
```

### Step 2: Replace spdlog::error Calls

**Before:**
```cpp
spdlog::error("Model file not found: {}", model_path);
```

**After:**
```cpp
errors::logError(errors::ErrorCode::ERR_LLM_MODEL_NOT_FOUND, model_path);
```

### Step 3: Testing

For each migrated file:
1. Compile and verify no errors
2. Run relevant unit tests
3. Trigger the error condition and verify:
   - Error code is logged correctly
   - Error message includes code number
   - Error details are accessible via MCP tools

## ✅ Acceptance Criteria

- [ ] All 17 critical error locations migrated
- [ ] Include statements added to all affected files
- [ ] Code compiles without errors or warnings
- [ ] Existing unit tests pass
- [ ] Error codes are visible in logs with format `[CODE] message`
- [ ] MCP tool `get_error_info` returns correct metadata for all migrated codes
- [ ] Documentation updated if needed

## 📚 Related Documents

- **Migration Guide:** `docs/ERROR_CODE_MIGRATION_LIST.md`
- **Error Registry:** `include/utils/error_registry.h`
- **Design Doc:** `docs/research/ERROR_AWARENESS_AND_INTROSPECTION.md`

## 🔄 Dependencies

- PR #[NUMBER]: Integrate AI-Explained Error Handling System (must be merged first)

## 📊 Estimated Effort

- **Development:** 2-3 hours
- **Testing:** 1-2 hours
- **Code Review:** 30 minutes

**Total:** ~4-6 hours

## 🎯 Success Metrics

- Zero regressions in existing functionality
- Error codes visible in production logs
- Reduced time to diagnose issues (measurable via support metrics)
- AI can explain all migrated errors via MCP tools

---

**Priority:** 🔴 CRITICAL  
**Phase:** 1 of 4  
**Blocked by:** Error Registry PR merge
