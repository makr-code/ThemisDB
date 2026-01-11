---
name: Error Code Migration - Phase 2 (High Priority)
about: Migrate high priority error logging locations to structured error codes
title: '[TASK] Error Code Migration - Phase 2: High Priority Errors'
labels: 'enhancement, error-handling, refactoring, priority-medium'
assignees: ''
---

# Error Code Migration - Phase 2: High Priority

## 📋 Summary

Migrate high priority error logging locations from `spdlog::error()` to the new structured error code system. This is Phase 2 focusing on errors that significantly impact functionality but are not immediately blocking.

## 🎯 Objectives

Migrate the following high priority error locations:

### 🟡 HIGH (Short-term - Phase 2)

1. **Context Creation Failed** - Impacts inference (2 locations)
2. **LoRA Weight Mismatch** - Prevents correct fusion (1 location)
3. **LoRA Invalid Data** - Data consistency (4 locations)
4. **LoRA Batching Disabled** - Feature availability (1 location)
5. **LoRA Fusion Failed** - Multi-LoRA operations (1 location)

**Total locations in Phase 2:** ~9 error logging statements

## 📝 Detailed Migration List

### 1. Context Creation Failed (ERR_LLM_CONTEXT_CREATION_FAILED - 2002)

**Files:**

| File | Line | Current Code | Target Code |
|------|------|--------------|-------------|
| `src/llm/model_loader.cpp` | 415 | `spdlog::error("Failed to create context for model: {}", model_id);` | `errors::logError(ErrorCode::ERR_LLM_CONTEXT_CREATION_FAILED, model_id);` |
| `src/llm/llama_wrapper.cpp` | 1391 | `spdlog::error("Failed to create context for draft model");` | `errors::logError(ErrorCode::ERR_LLM_CONTEXT_CREATION_FAILED, "draft model");` |

### 2. LoRA Weight Mismatch (ERR_LORA_WEIGHT_MISMATCH - 2102)

**File: `src/llm/multi_lora_manager.cpp`**

| Line | Current Code | Target Code |
|------|--------------|-------------|
| 341 | `spdlog::error("Number of LoRAs ({}) doesn't match number of weights ({})", ...)` | `errors::logError(ErrorCode::ERR_LORA_WEIGHT_MISMATCH, num_loras, num_weights);` |

### 3. LoRA Invalid Data (ERR_LORA_INVALID_DATA - 2104)

**File: `src/llm/multi_lora_manager.cpp`**

| Line | Current Code | Target Code |
|------|--------------|-------------|
| 707 | `spdlog::error("Empty LoRA data");` | `errors::logError(ErrorCode::ERR_LORA_INVALID_DATA, "empty data");` |
| 718 | `spdlog::error("Invalid LoRA data: too small");` | `errors::logError(ErrorCode::ERR_LORA_INVALID_DATA, "too small");` |
| 726 | `spdlog::error("Invalid LoRA data: invalid id_len");` | `errors::logError(ErrorCode::ERR_LORA_INVALID_DATA, "invalid id_len");` |
| 737 | `spdlog::error("Invalid LoRA data: invalid path_len");` | `errors::logError(ErrorCode::ERR_LORA_INVALID_DATA, "invalid path_len");` |

### 4. LoRA Batching Disabled (ERR_LORA_BATCHING_DISABLED - 2101)

**File: `src/llm/multi_lora_manager.cpp`**

| Line | Current Code | Target Code |
|------|--------------|-------------|
| 257 | `spdlog::error("Multi-LoRA batching is disabled");` | `errors::logError(ErrorCode::ERR_LORA_BATCHING_DISABLED);` |

### 5. LoRA Fusion Failed (ERR_LORA_FUSION_FAILED - 2103)

**File: `src/llm/multi_lora_manager.cpp`**

| Line | Current Code | Target Code |
|------|--------------|-------------|
| 336 | `spdlog::error("No LoRAs provided for fusion");` | `errors::logError(ErrorCode::ERR_LORA_FUSION_FAILED, "no LoRAs provided");` |

## 🔧 Implementation Steps

### Step 1: Add Required Include

Add to affected files:
```cpp
#include "utils/error_registry.h"
```

### Step 2: Replace spdlog::error Calls

Follow the migration pattern from Phase 1.

### Step 3: Testing

For each migrated file:
1. Compile and verify no errors
2. Run relevant unit tests
3. Trigger the error condition and verify error code logging

## ✅ Acceptance Criteria

- [ ] All 9 high priority error locations migrated
- [ ] Include statements added to all affected files
- [ ] Code compiles without errors or warnings
- [ ] Existing unit tests pass
- [ ] Error codes are visible in logs with format `[CODE] message`
- [ ] MCP tool `get_error_info` returns correct metadata for all migrated codes

## 📚 Related Documents

- **Migration Guide:** `docs/ERROR_CODE_MIGRATION_LIST.md`
- **Error Registry:** `include/utils/error_registry.h`
- **Phase 1 Issue:** Link to Phase 1 issue

## 🔄 Dependencies

- Phase 1 must be completed and merged

## 📊 Estimated Effort

- **Development:** 1-2 hours
- **Testing:** 1 hour
- **Code Review:** 30 minutes

**Total:** ~2-4 hours

---

**Priority:** 🟡 HIGH  
**Phase:** 2 of 4  
**Blocked by:** Phase 1 completion
