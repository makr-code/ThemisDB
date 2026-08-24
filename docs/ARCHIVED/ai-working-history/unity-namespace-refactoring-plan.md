# Unity Build Namespace Refactoring Plan - ThemisDB Graph Module

**Date:** 2026-06-19
**Objective:** Fix namespace structure for 21 .cpp files to compile correctly with Unity build (MSVC_UNITY_BUILD=ON)

## Problem Analysis

When files are concatenated in Unity build mode, namespace declarations from different files create compilation errors:
- Some files open `namespace themis { namespace acceleration {`
- Others open `namespace themis { namespace index {`
- Others open `namespace themis { namespace graph {`
- Others open just `namespace themis {`
- Some files are missing closing braces

**Solution:** Standardize all files to use `namespace themis { namespace graph {` by:
1. File #1 ONLY opens namespaces
2. Files #2-20 remove ALL namespace declarations
3. File #21 ONLY closes namespaces

## File-by-File Refactoring Specification

### FILE 1: src/acceleration/ai_hardware_dispatcher.cpp
**Current:** `namespace themis { namespace acceleration {`
**Action:** Change namespace from `acceleration` to `graph`
**Line Changes:**
- Line 123: Keep `namespace themis {`
- Line 124: Change `namespace acceleration {` → `namespace graph {`

**Replacement:**
```cpp
namespace themis {
namespace graph {
```

---

### FILE 2: src/index/graph_auto_buffer.cpp
**Current:** `namespace themis { ... } // namespace themis`
**Action:** Remove all namespace opens/closes
**Line Changes:**
- Line 28: Remove `namespace themis {`
- Line 408: Remove `} // namespace themis`

**Replacements:**
```
1. Remove line 28: "namespace themis {"
2. Remove line 408: "} // namespace themis"
```

---

### FILE 3: src/index/spatial_index.cpp
**Current:** `namespace themis { namespace index { ... } // namespace index } // namespace themis`
**Action:** Remove all namespace opens/closes
**Line Changes:**
- Lines 30-31: Remove `namespace themis { namespace index {`
- End of file: Remove `}  // namespace index` and `}  // namespace themis`

---

### FILE 4: src/index/temporal_graph.cpp
**Current:** `namespace themis { ... } // namespace themis`
**Action:** Remove all namespace opens/closes

---

### FILE 5: src/index/property_graph.cpp
**Current:** `namespace themis { ... } // namespace themis`
**Action:** Remove all namespace opens/closes

---

### FILE 6: src/index/edge_types.cpp
**Current:** `namespace themis { ... } // namespace themis`
**Action:** Remove all namespace opens/closes

---

### FILE 7: src/index/process_graph.cpp
**Current:** `namespace themis { ... } // namespace themis`
**Action:** Remove all namespace opens/closes

---

### FILE 8: src/index/gnn_embeddings.cpp
**Current:** `namespace themis { ... } // namespace themis`
**Action:** Remove all namespace opens/closes

---

### FILE 9: src/index/graph_analytics.cpp
**Current:** `namespace themis { ... } // namespace themis`
**Action:** Remove all namespace opens/closes

---

### FILE 10: src/graph/graph_query_optimizer.cpp
**Current:** `namespace themis { namespace graph { ... } // namespace graph } // namespace themis`
**Action:** Remove namespace opens/closes (file #1 will open them)

---

### FILE 11: src/graph/explain_plan.cpp
**Current:** `namespace themis { namespace graph { ... } // namespace graph } // namespace themis`
**Action:** Remove namespace opens/closes

---

### FILE 12: src/graph/ontology_manager.cpp
**Current:** `namespace themis { namespace graph { ... [BUG: MISSING CLOSING BRACES]`
**Action:** Remove namespace opens/closes (will be closed by file #21)
**NOTE:** This file is currently missing `} // namespace graph` and `} // namespace themis`

---

### FILE 13: src/graph/knowledge_graph_reasoner.cpp
**Current:** `namespace themis { namespace graph { ... } // namespace graph } // namespace themis`
**Action:** Remove namespace opens/closes

---

### FILE 14: src/graph/rotate_completion.cpp
**Current:** `namespace themis { namespace graph { ... } // namespace graph } // namespace themis`
**Action:** Remove namespace opens/closes

---

### FILE 15: src/query/result_stream.cpp
**Current:** `namespace themis { namespace query { ... } // namespace query } // namespace themis`
**Action:** Remove ALL namespace opens/closes (change query to graph is not needed for Unity)
**NOTE:** This file is in wrong namespace but will be corrected by removing opens/closes

---

### FILE 16: src/graph/path_constraints.cpp
**Current:** `namespace themis { namespace graph { ... } // namespace graph } // namespace themis`
**Action:** Remove namespace opens/closes

---

### FILE 17: src/graph/distributed_graph.cpp
**Current:** `namespace themis { namespace graph { ... } // namespace graph } // namespace themis`
**Action:** Remove namespace opens/closes

---

### FILE 18: src/graph/gpu_traversal.cpp
**Current:** `namespace themis { namespace graph { ... [COMPLEX: has multiple anonymous namespaces and cuda_impl namespace]`
**Action:** Remove only the themis/graph namespace opens/closes, preserve anonymous namespaces and cuda_impl

---

### FILE 19: src/graph/parallel_traversal.cpp
**Current:** `namespace themis { namespace graph { ... } // namespace graph } // namespace themis`
**Action:** Remove namespace opens/closes

---

### FILE 20: src/graph/scheduled_edge_refresh.cpp
**Current:** `namespace themis { namespace graph { ... } // namespace graph } // namespace themis`
**Action:** Remove namespace opens/closes

---

### FILE 21: src/graph/graph_query_rewriter.cpp
**Current:** `namespace themis { namespace graph { ... } // namespace graph } // namespace themis`
**Action:** KEEP AS IS - This is the ONLY file that should have closing braces

---

## Verification Steps

After applying changes:

1. **Build verification:**
   ```bash
   cmake --build --preset windows-release --target themis_graph --parallel 1
   ```

2. **Namespace validation:** Verify compilation unit has proper scope

3. **Test execution:**
   ```bash
   ctest --preset windows-release --filter "graph" --verbose
   ```

## Risk Assessment

**High Risk Items:**
- FILE 12 (ontology_manager.cpp): Currently has missing closing braces - this fix corrects existing bug
- FILE 15 (result_stream.cpp): In wrong namespace - removal puts it in correct context

**Medium Risk Items:**
- FILE 18 (gpu_traversal.cpp): Multiple internal namespaces - ensure we don't accidentally remove cuda_impl
- FILE 1 (ai_hardware_dispatcher.cpp): Changing from acceleration to graph namespace

**Low Risk Items:**
- FILES 2-20 (except 12,15,18): Simple removal of consistent namespace opens/closes

## Implementation Order

1. Apply all replacements using multi_replace_string_in_file (parallel execution)
2. Run compilation test
3. If compilation fails, diagnose and fix incrementally
4. Run full test suite
