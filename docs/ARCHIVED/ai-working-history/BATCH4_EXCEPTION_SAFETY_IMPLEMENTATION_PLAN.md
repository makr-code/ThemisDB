# Batch 4: Exception-Safety & Error Handling Fixes - IMPLEMENTATION PLAN
**Date:** 2026-08-17  
**Status:** IN PROGRESS  
**Target:** Close 133 exception-safety IMPL gaps in LLM module (FINAL BATCH)

## Executive Summary
Batch 4 is the final IMPL gap closure for the LLM module, addressing:
- **13 exception_in_destructor** → Mark destructors noexcept, add try-catch
- **61 uncaught_exception** → Wrap exceptions with context using source_location
- **59 null_dereference** → Defensive null checks at API boundaries

## Gap Categories & Implementation Strategy

### Category 1: Exception-Safe Destructors (13 instances)
**Pattern:** All destructors must be marked `noexcept` and wrap non-noexcept cleanup in try-catch

**Target Files:**
- src/llm/model_loader.cpp (ScopedLlamaLogCapture, ModelLoaderImpl)
- src/llm/gpu_memory_manager.cpp (MemoryHolder, GPUMemoryPool)
- src/llm/llama_wrapper.cpp (LlamaWrapper, ConnectionPool)
- src/llm/multi_lora_manager.cpp (LoRAAdapter, LoRARegistry)
- src/llm/continuous_batch_scheduler.cpp (BatchQueue, Scheduler)
- src/llm/production_validator.cpp (Validator, ValidatorCache)

**Status:** [PENDING]

### Category 2: Exception Context Wrapping (61 instances)
**Pattern:** Use fmt::format with std::source_location for exception messages

**Key Paths:**
- Model loading and initialization
- GPU memory allocation/deallocation
- Inference execution
- LoRA adapter management
- Batch scheduling

**Status:** [PENDING]

### Category 3: Residual Null Validation (59 instances)
**Pattern:** Add defensive nullptr checks at public API entry points

**Status:** [PENDING]

## Implementation Checklist
- [ ] Fix ScopedLlamaLogCapture destructor in model_loader.cpp
- [ ] Fix MemoryHolder destructor in gpu_memory_manager.cpp
- [ ] Fix LlamaWrapper destructors in llama_wrapper.cpp
- [ ] Add exception context wrappers to model loading paths
- [ ] Add exception context wrappers to GPU memory operations
- [ ] Add exception context wrappers to inference operations
- [ ] Add residual null validation checks
- [ ] Verify build (0 errors, ≤5 warnings)
- [ ] Run LLM tests (expect 120+ pass)
- [ ] Run exception scenario tests
- [ ] Validate with UBSan (0 UB detected)

## Build Validation
```bash
cmake --preset windows-release
cmake --build --preset windows-release -j16
```

## Test Validation
```bash
ctest --preset windows-release -L llm -V
ctest --preset windows-release -L llm -R "exception|error" -V
```

## Next Actions
1. Begin with destructor fixes in model_loader.cpp
2. Progress through gpu_memory_manager.cpp
3. Add exception context wrappers to critical paths
4. Add residual null validation
5. Validate and document completion
