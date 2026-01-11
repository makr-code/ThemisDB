---
name: Define Additional Error Codes for Uncovered Cases
about: Add new error codes to the Error Registry for cases not currently covered
title: '[TASK] Define Additional Error Codes for Uncovered Cases'
labels: 'enhancement, error-handling, priority-medium'
assignees: ''
---

# Define Additional Error Codes for Uncovered Cases

## 📋 Summary

Add 10 new error codes to the Error Registry (`include/utils/error_registry.h` and `src/utils/error_registry.cpp`) to cover error cases that were identified in the migration analysis but don't have corresponding error codes yet.

## 🎯 Objectives

Define and register metadata for the following new error codes:

### LLM Error Codes (2005-2011)

1. **ERR_LLM_INVALID_HANDLE** (2005) - Model/context handle is null
2. **ERR_LLM_VISION_INFERENCE_FAILED** (2006) - Vision inference error
3. **ERR_LLM_DRAFT_MODEL_LOAD_FAILED** (2007) - Failed to load draft model
4. **ERR_LLM_RAM_OOM** (2008) - Insufficient RAM
5. **ERR_LLM_GPU_NOT_AVAILABLE** (2009) - GPU is not available
6. **ERR_LLM_GPU_ALLOC_FAILED** (2010) - GPU allocation failed (cudaMalloc)
7. **ERR_LLM_GPU_PEER_ACCESS_FAILED** (2011) - Failed to enable GPU peer access

### LoRA Error Codes (2105-2106)

8. **ERR_LORA_MODEL_MISMATCH** (2105) - LoRAs from different base models
9. **ERR_LORA_GPU_LOAD_FAILED** (2106) - Failed to load LoRA on GPU

### MCP Error Codes (3004)

10. **ERR_MCP_STDIO_INIT_FAILED** (3004) - Failed to initialize stdio transport

## 📝 Implementation Details

### Step 1: Add Error Codes to Enum

**File: `include/utils/error_registry.h`**

Add to the `ErrorCode` enum:

```cpp
enum class ErrorCode {
    // ... existing codes ...
    
    // LLM Errors (2000-2099)
    ERR_LLM_MODEL_NOT_FOUND = 2000,
    ERR_LLM_MODEL_LOAD_FAILED = 2001,
    ERR_LLM_CONTEXT_CREATION_FAILED = 2002,
    ERR_LLM_INFERENCE_TIMEOUT = 2003,
    ERR_LLM_GPU_OOM = 2004,
    // NEW:
    ERR_LLM_INVALID_HANDLE = 2005,
    ERR_LLM_VISION_INFERENCE_FAILED = 2006,
    ERR_LLM_DRAFT_MODEL_LOAD_FAILED = 2007,
    ERR_LLM_RAM_OOM = 2008,
    ERR_LLM_GPU_NOT_AVAILABLE = 2009,
    ERR_LLM_GPU_ALLOC_FAILED = 2010,
    ERR_LLM_GPU_PEER_ACCESS_FAILED = 2011,
    
    // LoRA Errors (2100-2199)
    ERR_LORA_NOT_LOADED = 2100,
    ERR_LORA_BATCHING_DISABLED = 2101,
    ERR_LORA_WEIGHT_MISMATCH = 2102,
    ERR_LORA_FUSION_FAILED = 2103,
    ERR_LORA_INVALID_DATA = 2104,
    // NEW:
    ERR_LORA_MODEL_MISMATCH = 2105,
    ERR_LORA_GPU_LOAD_FAILED = 2106,
    
    // MCP Errors (3000-3999)
    ERR_MCP_TRANSPORT_FAILED = 3000,
    ERR_MCP_INVALID_REQUEST = 3001,
    ERR_MCP_TOOL_NOT_FOUND = 3002,
    ERR_MCP_SCHEMA_UNAVAILABLE = 3003,
    // NEW:
    ERR_MCP_STDIO_INIT_FAILED = 3004,
    
    // ... rest of codes ...
};
```

### Step 2: Register Error Metadata

**File: `src/utils/error_registry.cpp`**

Add to the `registerDefaultErrors()` method:

```cpp
void ErrorRegistry::registerDefaultErrors() {
    // ... existing registrations ...
    
    // NEW LLM Errors
    registerError({
        ErrorCode::ERR_LLM_INVALID_HANDLE,
        "LLM",
        "Error",
        "Invalid model or context handle: {}",
        "The model or context handle is null, indicating improper initialization.",
        "1. Verify model was loaded successfully before use\n"
        "2. Check for previous load errors\n"
        "3. Ensure context creation completed without errors\n"
        "4. Review model initialization sequence",
        {"/docs/llm/initialization.md"},
        {"llm", "handle", "null", "invalid"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_VISION_INFERENCE_FAILED,
        "LLM",
        "Error",
        "Vision inference failed: {}",
        "The vision/multimodal inference operation failed.",
        "1. Verify image format is supported\n"
        "2. Check image size and resolution limits\n"
        "3. Ensure vision model is properly loaded\n"
        "4. Review CLIP integration status",
        {"/docs/llm/vision.md"},
        {"llm", "vision", "inference", "failed", "clip"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_DRAFT_MODEL_LOAD_FAILED,
        "LLM",
        "Error",
        "Failed to load draft model: {}",
        "The draft model for speculative decoding could not be loaded.",
        "1. Verify draft model path in configuration\n"
        "2. Ensure draft model is compatible with base model\n"
        "3. Check draft model file is not corrupted\n"
        "4. Verify sufficient memory for draft model",
        {"/docs/llm/speculative_decoding.md"},
        {"llm", "draft", "model", "speculative"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_RAM_OOM,
        "LLM",
        "Critical",
        "Out of RAM: {} bytes required, {} bytes available",
        "Insufficient system RAM to load the model or complete the operation.",
        "1. Use a smaller model or quantized version\n"
        "2. Close other applications to free RAM\n"
        "3. Increase system swap space\n"
        "4. Consider GPU offloading if available\n"
        "5. Enable model streaming from disk",
        {"/docs/llm/memory_management.md"},
        {"ram", "oom", "out of memory", "llm"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_GPU_NOT_AVAILABLE,
        "LLM",
        "Error",
        "GPU {} is not available",
        "The specified GPU device is not available or accessible.",
        "1. Verify GPU is detected: nvidia-smi or rocm-smi\n"
        "2. Check GPU drivers are installed\n"
        "3. Ensure CUDA/ROCm runtime is available\n"
        "4. Verify GPU device ID is correct\n"
        "5. Check GPU is not in exclusive compute mode",
        {"/docs/llm/gpu_setup.md"},
        {"gpu", "not available", "device", "llm"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_GPU_ALLOC_FAILED,
        "LLM",
        "Critical",
        "GPU memory allocation failed: {}",
        "cudaMalloc or hipMalloc failed to allocate GPU memory.",
        "1. Check GPU has available VRAM\n"
        "2. Reduce model size or batch size\n"
        "3. Close other GPU applications\n"
        "4. Reset GPU: nvidia-smi -r\n"
        "5. Check for GPU hardware issues",
        {"/docs/llm/gpu_troubleshooting.md"},
        {"gpu", "allocation", "failed", "cudamalloc", "llm"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_GPU_PEER_ACCESS_FAILED,
        "LLM",
        "Warning",
        "Failed to enable peer access from GPU {} to GPU {}: {}",
        "Could not enable direct GPU-to-GPU memory access (peer access).",
        "1. Verify GPUs support peer access\n"
        "2. Check GPUs are on same PCIe root complex\n"
        "3. Review NVIDIA/AMD documentation for peer access requirements\n"
        "4. Model will still work but with reduced performance\n"
        "5. Consider using single GPU or NVLINK/Infinity Fabric",
        {"/docs/llm/multi_gpu.md"},
        {"gpu", "peer access", "failed", "multi-gpu"}
    });
    
    // NEW LoRA Errors
    registerError({
        ErrorCode::ERR_LORA_MODEL_MISMATCH,
        "LoRA",
        "Error",
        "Cannot fuse LoRAs from different base models: {} vs {}",
        "LoRA adapters must be trained on the same base model to be fused.",
        "1. Verify all LoRA adapters are from the same base model\n"
        "2. Check LoRA metadata for base model information\n"
        "3. Re-train adapters if necessary\n"
        "4. Use adapters separately instead of fusing",
        {"/docs/llm/lora_fusion.md"},
        {"lora", "model", "mismatch", "fusion"}
    });
    
    registerError({
        ErrorCode::ERR_LORA_GPU_LOAD_FAILED,
        "LoRA",
        "Error",
        "Failed to load LoRA on GPU {}: {}",
        "Could not load LoRA adapter onto the specified GPU.",
        "1. Verify GPU has sufficient VRAM\n"
        "2. Check LoRA adapter file is valid\n"
        "3. Ensure GPU is available and accessible\n"
        "4. Try loading on different GPU or CPU",
        {"/docs/llm/lora_gpu.md"},
        {"lora", "gpu", "load", "failed"}
    });
    
    // NEW MCP Errors
    registerError({
        ErrorCode::ERR_MCP_STDIO_INIT_FAILED,
        "MCP",
        "Error",
        "Failed to initialize MCP stdio transport: {}",
        "Could not initialize stdin/stdout transport for MCP communication.",
        "1. Check if stdin/stdout are available\n"
        "2. Verify process has proper file descriptor access\n"
        "3. Ensure not running in detached/daemon mode\n"
        "4. Check platform-specific stdio requirements",
        {"/docs/mcp/stdio_transport.md"},
        {"mcp", "stdio", "init", "failed", "transport"}
    });
}
```

## ✅ Acceptance Criteria

- [ ] All 10 new error codes added to `ErrorCode` enum
- [ ] Metadata registered for all 10 codes in `registerDefaultErrors()`
- [ ] Each error code has:
  - Appropriate category
  - Correct severity level
  - Clear message template
  - Detailed cause explanation
  - Step-by-step solution
  - Relevant documentation links
  - Searchable keywords
- [ ] Code compiles without errors
- [ ] Existing tests pass
- [ ] New error codes are queryable via MCP tools
- [ ] Error codes appear in `get_error_info` and `search_errors` results
- [ ] JSON serialization works correctly

## 🧪 Testing

```bash
# Test error code retrieval
curl http://localhost:8080/api/v1/errors/2005
curl http://localhost:8080/api/v1/errors/2106

# Test MCP tools
# get_error_info with new code
# search_errors for "vision" should find 2006
```

## 📚 Related Documents

- **Migration Guide:** `docs/ERROR_CODE_MIGRATION_LIST.md` (section "NEUE ERROR CODES ERFORDERLICH")
- **Error Registry:** `include/utils/error_registry.h`, `src/utils/error_registry.cpp`
- **Design Doc:** `docs/research/ERROR_AWARENESS_AND_INTROSPECTION.md`

## 🔄 Dependencies

- Error Registry PR must be merged first
- Should be completed before Phase 3 migration

## 📊 Estimated Effort

- **Development:** 1-2 hours
- **Testing:** 30 minutes
- **Code Review:** 30 minutes

**Total:** ~2-3 hours

---

**Priority:** 🟡 MEDIUM  
**Blocked by:** Error Registry PR merge  
**Blocks:** Phase 3 migration (partially)
