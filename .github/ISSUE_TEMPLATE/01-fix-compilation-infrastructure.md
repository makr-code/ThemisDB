---
name: "🔴 Fix Compilation Infrastructure"
about: Critical blocker - Fix build system to enable LLM features
title: "[P0] Fix Compilation Infrastructure for LLM Integration"
labels: ["priority: critical", "type: bug", "component: build", "llm"]
assignees: []
---

## Priority
🔴 **CRITICAL** - P0 Blocker for all LLM features

## Problem Statement

ThemisDB cannot currently compile with LLM features enabled. This blocks testing and deployment of all Phase 1 and Phase 2 features that have already been implemented.

**Affected Features:**
- ✅ Phase 1: Flash Attention, KV-Cache Reuse, Embeddings (implemented, cannot test)
- ✅ Phase 2: Speculative Decoding, Continuous Batching (implemented, cannot test)
- ⏳ Phase 3: Grammar Constraints, Vision, RoPE Scaling (pending implementation)

## Current Status

```bash
# Attempted build command
cmake -B build -DLLAMA_CUDA=ON -DTHEMIS_ENABLE_LLM=ON
make -j$(nproc)

# Result: Build fails
```

## Expected Behavior

```bash
# Should successfully build
cmake -B build \
  -DLLAMA_CUDA=ON \
  -DTHEMIS_ENABLE_LLM=ON \
  -DCMAKE_BUILD_TYPE=Release

make -j$(nproc)

# Should produce working binary
./build/themisdb --version
# ThemisDB v1.3.1 with LLM support
```

## Tasks

### Build System
- [ ] Verify CMake configuration
  - [ ] Check THEMIS_ENABLE_LLM flag handling
  - [ ] Verify llama.cpp dependency detection
  - [ ] Fix include paths
  - [ ] Fix library linking

### Dependencies
- [ ] Ensure llama.cpp is properly integrated
  - [ ] Check llama.cpp submodule/external
  - [ ] Verify CUDA toolkit detection
  - [ ] Check required libraries (spdlog, nlohmann/json, etc.)

### Compilation
- [ ] Fix header includes
- [ ] Resolve undefined references
- [ ] Fix template instantiation errors
- [ ] Resolve linking errors

### Validation
- [ ] Build succeeds without errors
- [ ] Binary starts successfully
- [ ] LLM features are available
- [ ] Basic inference test works

## Investigation Checklist

1. **Check Build Logs**
   ```bash
   cmake -B build -DLLAMA_CUDA=ON -DTHEMIS_ENABLE_LLM=ON 2>&1 | tee cmake.log
   make -j$(nproc) 2>&1 | tee make.log
   ```

2. **Verify Dependencies**
   ```bash
   # CUDA
   nvcc --version
   
   # llama.cpp
   ls -la external/llama.cpp/ || ls -la lib/llama.cpp/
   
   # Other libraries
   pkg-config --list-all | grep -E 'spdlog|protobuf|grpc'
   ```

3. **Check CMakeLists.txt**
   - Is THEMIS_ENABLE_LLM option defined?
   - Are LLM sources conditionally included?
   - Are llama.cpp headers found?

## Acceptance Criteria

- [ ] Build completes without errors
- [ ] All tests pass
- [ ] Binary runs and shows LLM support in version info
- [ ] Can load a model and run basic inference
- [ ] CI/CD pipeline passes

## Estimated Effort

**Time:** 1-3 days
**Complexity:** High (build system debugging)
**Blockers:** None (this IS the blocker)

## Related Issues

- Blocks: #2 (Test Phase 1)
- Blocks: #3 (Test Phase 2)
- Blocks: #4 (Implement Phase 3.1)
- Blocks: #5 (Implement Phase 3.2)
- Blocks: #6 (Implement Phase 3.3)

## References

- PR #XXX: Phase 1 & 2 Implementation
- Docs: `docs/en/llm/FLASH_ATTENTION_IMPLEMENTATION.md`
- Docs: `docs/de/llm/LLAMA_CPP_INTEGRATION.md`

## Notes

This is the **most critical issue** blocking all LLM functionality. All other Phase 1-3 work is blocked until compilation succeeds.

Once resolved, we can immediately test and deploy Phase 1 & 2 features that provide **50-100x performance improvement**.
