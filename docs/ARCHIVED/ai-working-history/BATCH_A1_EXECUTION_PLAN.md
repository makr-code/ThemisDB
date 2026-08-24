# Batch A-1 Through A-4 Execution Plan

**Target:** Index Module CRITICAL Gap Remediation Phase 2  
**Date:** 2026-08-15  
**Repository:** ThemisDB  
**Total Issues:** 29 CRITICAL gaps  

## Batch Overview

### Batch A-1: exception_in_destructor (2 files)
**Files:**
- `src/index/graph_auto_buffer.cpp` (line 44-48)
- `src/index/vector_auto_buffer.cpp` (line 58-62)

**Issue:** Destructors call stop() which can throw; violates noexcept contract.

**Fix Strategy:**
1. Add `noexcept` to destructor declarations in headers
2. Wrap stop() calls in try-catch blocks in destructor implementations
3. Use THEMIS_ERROR logging fallback on exception
4. Ensure no exception propagation from destructor

**Status:** 🟡 PENDING

---

### Batch A-2: iterator_invalidation (6-8 files)
**Files (from top-20):**
- `src/index/vector_index.cpp` (line 80)
- `src/index/multi_vector_search.cpp` (lines 224, 406)
- `src/index/gpu_memory_oversubscription.cpp` (line 230)
- `src/index/graph_index.cpp` (lines 244, 247, 248)
- `src/index/edge_types.cpp` (line 364)

**Issue:** Iterators invalidated after container mutation; use-after-free risk.

**Fix Strategy:**
1. Identify operations causing invalidation (resize, erase, clear)
2. Use index-based access or re-fetch iterator after mutation
3. Cache size before loop or use references
4. Add focused test case

**Status:** 🟡 PENDING

---

### Batch A-3: gpu_memory_leak (3-5 files, 3 CRITICAL)
**Files (CRITICAL subset):**
- `src/index/cuda_hnsw_graph_traversal.cpp` (lines 362, 370, 381)
- `src/index/gpu_memory_oversubscription.cpp` (line 53)

**Issue:** GPU memory allocated via cuMalloc/cudaMalloc not freed.

**Fix Strategy:**
1. Audit all GPU allocation calls
2. Ensure paired free calls (cuMemFree, cudaFree)
3. Use RAII wrappers if available
4. Add error handling for OOM
5. Add test for GPU memory pressure

**Status:** 🟡 PENDING

---

### Batch A-4: braces_imbalance CRITICAL (6 files)
**Files:**
- `src/index/cuda_hnsw_graph_traversal.cpp` (line 1)
- `src/index/graph_index.cpp` (line 1)
- `src/index/hnsw_production_defaults.cpp` (line 1)
- `src/index/property_graph.cpp` (line 1)
- `src/index/secondary_index.cpp` (line 1)
- `src/index/spatial_index.cpp` (line 1)

**Issue:** Structural brace imbalance; compiler errors.

**Fix Strategy:**
1. Audit file structure (namespace, class, function braces)
2. Fix unclosed braces or dangling closures
3. Use clang-format for consistency
4. Validate compilation

**Status:** 🟡 PENDING

---

## Build & Test Commands

```bash
# Configure (strict mode)
cmake --preset develop-strict

# Build index module only
cmake --build <build_dir> --target index_tests

# Test (focused)
ctest -L index --output-on-failure -j 1

# Memory checks (ASan/MSan)
ctest -L index --output-on-failure -j 1 -E "perf"
```

## Execution Order
1. **A-1** (1-2 days): Exception safety
2. **A-2** (2-3 days): Iterator fixes (parallel with A-3)
3. **A-3** (1-2 days): GPU memory (parallel with A-2)
4. **A-4** (1 day): Brace fixes

## Success Criteria
- ✅ 0 exceptions from destructors
- ✅ 0 use-after-free on iterators (ASan confirms)
- ✅ 0 GPU memory leaks (validated)
- ✅ All files compile without brace errors
- ✅ All focused tests PASS
- ✅ ROADMAP.md Phase 2 updated

---
