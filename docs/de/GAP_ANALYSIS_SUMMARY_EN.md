# GPU/VRAM Gap Analysis Summary
## Documentation vs. Actual Implementation

**Date:** January 15, 2026  
**Version:** 1.0  
**Status:** ⚠️ Critical Gap Identified

---

## Executive Summary

**Central Question:** Is GPU/VRAM actually used in all ThemisDB editions as documented?

**Answer:** **NO** - GPU/VRAM is **NOT enabled by default** in any edition except as a compile-time option.

### Critical Finding

Documentation in `docs/de/performance/*` suggests GPU acceleration is:
- ✅ Implemented (TRUE)
- ✅ Functional (TRUE when enabled)
- ❌ Available by default (FALSE)
- ❌ Automatically used (FALSE)

**Reality:** Without explicit build flags (`-DTHEMIS_ENABLE_CUDA=ON`, `-DTHEMIS_ENABLE_VULKAN=ON`), **no GPU acceleration** occurs, even in ENTERPRISE edition with 320 GB VRAM limit.

---

## GPU Usage by Edition

| Edition | Documented VRAM Limit | GPU Backends (Default) | Actual GPU Usage | Gap |
|---------|----------------------|------------------------|------------------|-----|
| **MINIMAL** | 0 GB | Explicitly OFF | ❌ No GPU | ✅ Correct |
| **COMMUNITY** | 16 GB | OFF (must enable) | ❌ No GPU (unless manually built) | 🔴 HIGH |
| **ENTERPRISE** | 320 GB | OFF (must enable) | ❌ No GPU (unless manually built) | 🔴 HIGH |
| **HYPERSCALER** | Unlimited | OFF (must enable) | ❌ No GPU (unless manually built) | 🔴 HIGH |

---

## Backend Status Comparison

| Backend | Documentation | Implementation | Default | Gap |
|---------|---------------|----------------|---------|-----|
| **CUDA** | ✅ "Fully functional" | ✅ Implemented | ❌ OFF | 🔴 HIGH |
| **Vulkan** | 🚧 "Partial" | ✅ Mostly done | ❌ OFF | 🟡 MEDIUM |
| **FAISS GPU** | ✅ "Native support" | ✅ Implemented | ❌ OFF | 🔴 HIGH |
| **HIP/OpenCL/Metal** | ❓ Undocumented | ✅ Implemented | ❌ OFF | 🟡 MEDIUM |

---

## Key Examples

### Example 1: CUDA Backend

**Documentation claims:**
```markdown
## Status: ✅ Implemented (Functional)
CUDA backend is now fully functional with custom CUDA kernels
```

**Reality (CMakeLists.txt):**
```cmake
option(THEMIS_ENABLE_CUDA "Enable CUDA backend" OFF)  # ← OFF by default!
```

**Reality (cuda_backend.cpp):**
```cpp
bool CUDAVectorBackend::isAvailable() const noexcept {
#ifdef THEMIS_ENABLE_CUDA
    // ... actual implementation
#else
    return false;  // ← Always false unless built with -DTHEMIS_ENABLE_CUDA=ON
#endif
}
```

**Gap:** Documentation says "fully functional" but doesn't mention:
- CUDA is OFF by default
- Requires explicit `-DTHEMIS_ENABLE_CUDA=ON` to build
- Without it, `isAvailable()` always returns `false`
- Automatic fallback to CPU

---

### Example 2: Edition VRAM Limits

**Documentation (edition.h):**
```cpp
// COMMUNITY: 16 GB (consumer-grade GPU like RTX 4090)
// ENTERPRISE: 320 GB (data center GPU like A100/H100)
constexpr int GPU_MAX_VRAM_GB = THEMIS_GPU_MAX_VRAM_GB;
```

**Reality (CMakeLists.txt):**
```cmake
elseif(THEMIS_EDITION STREQUAL "COMMUNITY")
    set(THEMIS_GPU_MAX_VRAM_GB 24)
    # BUT: CUDA/Vulkan are still OFF by default!
```

**Gap:** VRAM limit is set to 16 GB, but **no GPU backends are enabled**, making the limit meaningless.

---

### Example 3: Performance Claims

**Documentation:**
```markdown
Performance: ~550ms for 1000 queries = 1818 queries/sec
VRAM Usage: ~18 GB
```

**Missing context:**
- These numbers require `-DTHEMIS_ENABLE_CUDA=ON`
- Without CUDA: CPU fallback (~95s for 1000 queries = 10 q/s, **181x slower**)
- Benchmark hardware not always specified

---

## Specific Problems

### Problem 1: Misleading Status Labels

Documentation uses "✅ Implemented (Functional)" for features that are:
- Implemented: TRUE
- Functional: TRUE (when enabled)
- Available by default: FALSE (not mentioned)

**Users expect:** Working GPU acceleration out of the box
**Users get:** CPU fallback (unless they know to rebuild with GPU flags)

---

### Problem 2: Missing Build Instructions

Documentation shows usage:
```cpp
auto& registry = BackendRegistry::instance();
registry.autoDetect();  // Finds CUDA if available
```

**What's missing:** How to make CUDA "available"!

**Should include:**
```bash
# Build with CUDA support
cmake -S . -B build -DTHEMIS_ENABLE_CUDA=ON
cmake --build build
```

---

### Problem 3: Edition Limits Without Backend Activation

**Current situation:**
- COMMUNITY edition: `GPU_MAX_VRAM_GB = 24`
- All GPU backends: OFF
- Result: VRAM limit is **irrelevant** (no GPU usage)

**Options:**
1. Enable GPU backends by default in COMMUNITY/ENTERPRISE
2. Or: Update docs to clarify limits apply "when GPU backends are enabled"

---

### Problem 4: Performance Claims Without Prerequisites

**Current:** "Performance: 1818 queries/sec with GPU"

**Should be:**
```markdown
## Performance (with CUDA Backend Enabled)

⚠️ **Prerequisite:** Build with `-DTHEMIS_ENABLE_CUDA=ON`

**With CUDA:**
- 1818 queries/sec
- 18 GB VRAM

**Without CUDA (CPU fallback):**
- 10 queries/sec (**181x slower**)
- 128 GB RAM
```

---

## Recommendations

### Short-term (Update Documentation)

1. **Add warnings to all GPU backend docs:**
   - ⚠️ "OFF by default, requires `-DTHEMIS_ENABLE_*=ON`"
   
2. **Include in each backend doc:**
   - Build instructions
   - Dependencies (CUDA Toolkit, Vulkan SDK, etc.)
   - Verification commands
   - Fallback behavior

3. **Update edition docs:**
   - MINIMAL: "GPU disabled" ✅ (correct)
   - COMMUNITY: "GPU optional (16 GB limit **when enabled**)"
   - ENTERPRISE: "GPU optional (320 GB limit **when enabled**)"

4. **Mark performance benchmarks:**
   - Specify GPU backend used
   - Include CPU fallback performance
   - Note hardware configuration

---

### Medium-term (Build System)

5. **Consider enabling GPU backends by default:**
   ```cmake
   if(THEMIS_EDITION STREQUAL "COMMUNITY" OR THEMIS_EDITION STREQUAL "ENTERPRISE")
       set(THEMIS_ENABLE_CUDA ON CACHE BOOL "Enable CUDA in ${THEMIS_EDITION}")
   endif()
   ```

6. **Add GPU status to startup logs:**
   ```cpp
   if (registry.getBestVectorBackend()->type() == BackendType::CPU) {
       LOG_WARNING << "No GPU backend available. Using CPU (slower).";
       LOG_WARNING << "Rebuild with -DTHEMIS_ENABLE_CUDA=ON for GPU acceleration.";
   }
   ```

---

### Long-term (Architecture)

7. **Runtime GPU plugin system:**
   - Load GPU backends dynamically at runtime
   - No recompilation needed

8. **Unified GPU abstraction:**
   - Single interface for all GPU backends
   - Automatic hardware detection
   - Transparent fallback

---

## Detailed Analysis

For full analysis (in German), see: [`GAP_ANALYSE_GPU_VRAM_NUTZUNG.md`](./GAP_ANALYSE_GPU_VRAM_NUTZUNG.md)

**Contents:**
- Detailed gap analysis per backend (CUDA, Vulkan, FAISS GPU, etc.)
- Edition-specific comparison
- Code examples
- Comprehensive recommendations
- Build system analysis

---

## Conclusion

**GPU/VRAM is NOT used by default in any edition.**

The documentation creates an impression that GPU acceleration is:
- Available and working ✅ (when built with GPU flags)
- Ready to use ❌ (false - requires recompilation)

**Without explicit build flags, no GPU acceleration occurs**, even in ENTERPRISE edition with 320 GB VRAM limit.

**Action required:** Update documentation to clarify that GPU backends are compile-time optional and OFF by default.

---

**Created:** January 15, 2026  
**Priority:** P0 (Documentation misleading)  
**Impact:** Users may not realize GPU acceleration requires rebuild
