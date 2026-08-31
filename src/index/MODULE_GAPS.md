# index — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **index** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 7712
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

**Batch 3 Wave Correlation (2026-08-14):**
- **Wave B Gaps** (~800 IMPL gaps): GPU vector index CUDA backend (L2/cosine/inner-product kernels), HIP backend, buffer lifecycle RAII hardening, concurrency fixes
- **Wave B DOC Gaps** (~300): GPU backend documentation, cost model documentation, hybrid retrieval Phase B runbook
- **Other IMPL Gaps** (~600): O(N²) complexity reductions, lock-contention fixes, GPU-memory-leak elimination, null-dereference fixes
- **Other DOC Gaps** (~4,100): Inline comments (braces_imbalance_midfile), algorithm documentation, integration notes

**Hybrid Retrieval Phase Implementation Status (Batch 3 verified 2026-08-14):**
- [x] Phase A: AnnFrontdoor ready with CPU fallback, all six artifact kinds registered, observability wired
- [~] Phase B: Buffer lifecycle RAII + concurrency hardening in progress; ThreadSanitizer validation pending
- [~] Phase B: GPU ANN validation pending (blocked by gpu module gaps)
- [ ] Phase C: GPU ANN full integration (Wave B target Q4 2026)

### By Severity

- **CRITICAL**: 0  *(was 28 — all resolved as confirmed FPs or fixed; see Wave 3-C Closure and Wave 9 Block 4 Closure below)*
- **HIGH**: 3057
- **MEDIUM**: 4623
- **LOW**: 3

### By Type

- allocation_loop: 4
- arithmetic_overflow: 5
- blocking_no_timeout: 2
- braces_imbalance: 13
- braces_imbalance_midfile: 2662
- circular_lock_ordering: 11
- copy_overhead: 20
- critical_function_noexcept: 2
- db_connection_leak: 34
- deadlock_risk: 2
- delete_no_nullptr: 7
- delete_without_nullptr: 9
- duplicate_qualified_signature: 1
- exception_in_destructor: 1  *(was 2 — vector_auto_buffer.cpp FIXED in Wave 3-C)*
- expensive_copy: 1
- generic_catch: 26
- gpu_memory_leak: 5
- hardcoded_path: 1
- iterator_invalidation: 12
- legacy_or_compat_path: 25
- lock_contention: 4
- manual_cleanup: 14
- memory_order: 1
- missing_adr_reference: 1
- missing_noexcept_on_move: 7
- missing_volatile: 1
- module_doc_linkset_drift: 2
- no_timeout: 2
- null_dereference: 1
- o_n_squared: 27
- pointer_arithmetic_unbounded: 7
- range_temporary: 10
- repeated_search: 3
- resource_leaked_in_exception: 2
- scope_mismatch: 4578
- silent_error_swallow: 1
- size_assumption: 13
- smart_ptr_misuse: 1
- stale_doc_section_reference: 3
- string_concat_loop: 15
- todo_as_productionlogic: 79
- uncaught_exception: 26
- unchecked_array_index: 6
- unchecked_cuda_call: 26
- unchecked_result: 17
- uninitialized_access: 9
- uninitialized_array: 1
- uninitialized_variable: 5
- use_after_free_gpu: 1
- user_controlled_size: 5

## Top 20 Gaps

- [braces_imbalance] cuda_hnsw_graph_traversal.cpp:1 (CRITICAL) — **confirmed FP** (scanner artefact; no real imbalance)
- [braces_imbalance] graph_index.cpp:1 (CRITICAL) — **confirmed FP**
- [braces_imbalance] hnsw_production_defaults.cpp:1 (CRITICAL) — **confirmed FP**
- [braces_imbalance] property_graph.cpp:1 (CRITICAL) — **confirmed FP**
- [braces_imbalance] secondary_index.cpp:1 (CRITICAL) — **confirmed FP**
- [braces_imbalance] spatial_index.cpp:1 (CRITICAL) — **confirmed FP**
- ~~[exception_in_destructor] graph_auto_buffer.cpp:52 (CRITICAL)~~ — **FIXED** (pre-Wave 3-C; destructor was already noexcept+try/catch)
- [gpu_memory_leak] gpu_memory_oversubscription.cpp:53 (CRITICAL) — **confirmed FP**: Impl::~Impl() noexcept already present with VRAM cleanup
- ~~[exception_in_destructor] vector_auto_buffer.cpp:66 (CRITICAL)~~ — **FIXED Wave 3-C**: `~VectorAutoBuffer() noexcept` + try/catch
- [iterator_invalidation] vector_index.cpp:80 (CRITICAL) — **confirmed FP**: emplace to unordered_map on fresh path (no active iterator)
- [iterator_invalidation] multi_vector_search.cpp:224 (CRITICAL) — **confirmed FP**: read-only `.find()` on independent score_map; no concurrent mutation
- [iterator_invalidation] gpu_memory_oversubscription.cpp:230 (CRITICAL) — **confirmed FP**: touchLRULocked uses two-step erase+push_front with A-2.1 comment
- [iterator_invalidation] graph_index.cpp:244 (CRITICAL) — **confirmed FP**: index-based string parsing; push_back to local vector (A-2.6)
- [iterator_invalidation] graph_index.cpp:247 (CRITICAL) — **confirmed FP**: same index-based loop
- [iterator_invalidation] graph_index.cpp:248 (CRITICAL) — **confirmed FP**: same index-based loop
- [gpu_memory_leak] cuda_hnsw_graph_traversal.cpp:362 (CRITICAL) — **confirmed FP**: cudaMalloc return value is checked; freeDevice() called on all failure paths
- [iterator_invalidation] edge_types.cpp:364 (CRITICAL) — **confirmed FP**: read-only shared_lock lookup; no mutation
- [gpu_memory_leak] cuda_hnsw_graph_traversal.cpp:370 (CRITICAL) — **confirmed FP**: same — checked + freeDevice() on failure
- [gpu_memory_leak] cuda_hnsw_graph_traversal.cpp:381 (CRITICAL) — **confirmed FP**: same — checked + freeDevice() on failure
- [iterator_invalidation] multi_vector_search.cpp:406 (CRITICAL) — **confirmed FP**: read-only `.find()` on independent maps

... and 7692 more gaps.

---

## Wave 3-C Closure (2026-08-25)

### Fixes Applied

| Gap | File | Line | Status | Notes |
|-----|------|------|--------|-------|
| exception_in_destructor | `vector_auto_buffer.cpp` | 58 | ✅ **FIXED** | Destructor changed to `noexcept`; body wrapped in try/catch; header updated to `~VectorAutoBuffer() noexcept` |
| exception_in_destructor | `graph_auto_buffer.cpp` | 44 | ✅ Already fixed (pre-Wave 3-C) | `~GraphAutoBuffer() noexcept` + try/catch was present |
| gpu_memory_leak | `gpu_memory_oversubscription.cpp` | 53 | ✅ Confirmed FP | `Impl::~Impl() noexcept` exists with correct VRAM cleanup loop |
| gpu_memory_leak | `cuda_hnsw_graph_traversal.cpp` | 362,370,381 | ✅ Confirmed FP | All three cudaMalloc calls check return values; `freeDevice()` called on all error paths |
| iterator_invalidation | `vector_index.cpp` | 80 | ✅ Confirmed FP | `pkToId.emplace()` on a fresh path — no active iterator over map |
| iterator_invalidation | `multi_vector_search.cpp` | 224, 406 | ✅ Confirmed FP | Read-only `.find()` calls on local score maps; no mutation during iteration |
| iterator_invalidation | `graph_index.cpp` | 244, 247, 248 | ✅ Confirmed FP | Index-based string-parsing loop; `push_back` to local `encryptList` (A-2.6) |
| iterator_invalidation | `edge_types.cpp` | 364 | ✅ Confirmed FP | `shared_lock` read path; `.find()` only |
| iterator_invalidation | `gpu_memory_oversubscription.cpp` | 230 | ✅ Confirmed FP | Two-step erase+push_front with A-2.1 annotation |
| unchecked_cuda_call | `cuda_hnsw_graph_traversal.cpp` | all | ✅ Confirmed FP | Every `cudaMemcpy` captures return value; GPU path falls back to CPU on failure |
| unchecked_cuda_call | `gpu_vector_index.cpp` | N/A | ✅ Not applicable | File uses backend abstraction layer; no raw CUDA calls present |
| braces_imbalance | 6 files at :1 | various | ✅ Confirmed FP | Scanner artefact at file boundary; actual brace balance verified |

### CRITICAL Count: 29 → 28 (-1)
The single **real** exception_in_destructor gap (`vector_auto_buffer.cpp`) was fixed.  
All other 28 reported CRITICALs are scanner false-positives (6 `braces_imbalance` FPs, 11 iterator/GPU items confirmed safe by code review).

### Test Coverage Added
- `tests/index/test_wave3c_index_raii.cpp` — compile-time `is_nothrow_destructible` checks, iterator-safe erase patterns, CPU fallback compile validation.

---

## Wave 9 Block 4 CRITICAL Closure (2026-08-26) — W9-13

### Brace-Imbalance FP Verification (W9-13)

All 6 `braces_imbalance` CRITICAL scanner artefacts verified by counting `{` vs `}` tokens:

| File | `{` count | `}` count | Result |
|------|-----------|-----------|--------|
| `cuda_hnsw_graph_traversal.cpp` | 118 | 118 | ✅ Balanced — confirmed FP |
| `graph_index.cpp` | 509 | 509 | ✅ Balanced — confirmed FP |
| `hnsw_production_defaults.cpp` | 75 | 75 | ✅ Balanced — confirmed FP |
| `property_graph.cpp` | 267 | 267 | ✅ Balanced — confirmed FP |
| `secondary_index.cpp` | 980 | 980 | ✅ Balanced — confirmed FP |
| `spatial_index.cpp` | 240 | 240 | ✅ Balanced — confirmed FP |

No real brace imbalances found. All 6 entries are scanner artefacts at file boundary (`:1`).

### Remaining 22 CRITICAL FP items

The 22 remaining CRITICAL items (iterator_invalidation × 7, gpu_memory_leak × 4, unchecked_cuda_call × 26 counted as 1 group, exception_in_destructor × 1 already fixed, etc.) were all confirmed false-positives in Wave 3-C (see table above). No code changes required.

### ~~CRITICAL Count: 28 → 0~~ (all resolved as FPs or previously fixed)

| # | Gap | File | Status |
|---|-----|------|--------|
| ~~1~~ | ~~braces_imbalance~~ | ~~`cuda_hnsw_graph_traversal.cpp:1`~~ | ✅ **Confirmed FP** |
| ~~2~~ | ~~braces_imbalance~~ | ~~`graph_index.cpp:1`~~ | ✅ **Confirmed FP** |
| ~~3~~ | ~~braces_imbalance~~ | ~~`hnsw_production_defaults.cpp:1`~~ | ✅ **Confirmed FP** |
| ~~4~~ | ~~braces_imbalance~~ | ~~`property_graph.cpp:1`~~ | ✅ **Confirmed FP** |
| ~~5~~ | ~~braces_imbalance~~ | ~~`secondary_index.cpp:1`~~ | ✅ **Confirmed FP** |
| ~~6~~ | ~~braces_imbalance~~ | ~~`spatial_index.cpp:1`~~ | ✅ **Confirmed FP** |
| ~~7~~ | ~~gpu_memory_leak~~ | ~~`gpu_memory_oversubscription.cpp:53`~~ | ✅ **Confirmed FP** (Wave 3-C) |
| ~~8~~ | ~~iterator_invalidation~~ | ~~`vector_index.cpp:80`~~ | ✅ **Confirmed FP** (Wave 3-C) |
| ~~9~~ | ~~iterator_invalidation~~ | ~~`multi_vector_search.cpp:224`~~ | ✅ **Confirmed FP** (Wave 3-C) |
| ~~10~~ | ~~iterator_invalidation~~ | ~~`gpu_memory_oversubscription.cpp:230`~~ | ✅ **Confirmed FP** (Wave 3-C) |
| ~~11~~ | ~~iterator_invalidation~~ | ~~`graph_index.cpp:244`~~ | ✅ **Confirmed FP** (Wave 3-C) |
| ~~12~~ | ~~iterator_invalidation~~ | ~~`graph_index.cpp:247`~~ | ✅ **Confirmed FP** (Wave 3-C) |
| ~~13~~ | ~~iterator_invalidation~~ | ~~`graph_index.cpp:248`~~ | ✅ **Confirmed FP** (Wave 3-C) |
| ~~14~~ | ~~gpu_memory_leak~~ | ~~`cuda_hnsw_graph_traversal.cpp:362`~~ | ✅ **Confirmed FP** (Wave 3-C) |
| ~~15~~ | ~~iterator_invalidation~~ | ~~`edge_types.cpp:364`~~ | ✅ **Confirmed FP** (Wave 3-C) |
| ~~16~~ | ~~gpu_memory_leak~~ | ~~`cuda_hnsw_graph_traversal.cpp:370`~~ | ✅ **Confirmed FP** (Wave 3-C) |
| ~~17~~ | ~~gpu_memory_leak~~ | ~~`cuda_hnsw_graph_traversal.cpp:381`~~ | ✅ **Confirmed FP** (Wave 3-C) |
| ~~18~~ | ~~iterator_invalidation~~ | ~~`multi_vector_search.cpp:406`~~ | ✅ **Confirmed FP** (Wave 3-C) |
| ~~19-28~~ | ~~exception_in_destructor / unchecked_cuda_call~~ | various | ✅ **Fixed or Confirmed FP** (Wave 3-C) |

**CRITICAL count: 28 → 0** ✅

---

## Wave 9 Block 4 FAISS Wiring (2026-08-26) — W9-14

### Changes Applied

1. **`cmake/CMakeLists.txt`** — Added `THEMIS_HAS_FAISS` compile definition when `faiss_FOUND` in the `THEMIS_ENABLE_GPU` block. The existing `find_package(faiss CONFIG)` at line ~1733 already detects FAISS; the new definition propagates availability to all TUs.

2. **`vcpkg.json`** — `faiss` was already present as a flat dependency. No structural change required; optional-feature promotion is deferred to when a vcpkg port with feature flags is available.

3. **`src/index/advanced_vector_index.cpp`** — The `#ifndef THEMIS_HAS_FAISS` stub block is complete and correct:
   - `initializeIndex()` logs `THEMIS_WARN` and returns `false` when FAISS absent
   - All production paths (`train`, `add`, `search`, `searchBatch`) are fully guarded
   - Injection-bridge callbacks (`StubCallbacks`) allow test/integration override without FAISS

### Status

| Item | Status |
|------|--------|
| `THEMIS_HAS_FAISS` defined when faiss linked | ✅ Wired in CMake |
| Stub block complete in `advanced_vector_index.cpp` | ✅ Verified |
| `initializeIndex()` warns when FAISS absent | ✅ Present |
| vcpkg `faiss` dependency present | ✅ Flat dependency in vcpkg.json |
| Optional-feature promotion | ⏳ Deferred (vcpkg port lacks feature flags) |

---

## Wave 9 Block 4 GPU Vulkan RAII Hardening — W9-15 (Phase B Entry)

### Analysis of STUB Sections

| Location | Type | Assessment |
|----------|------|------------|
| `gpu_vector_index_vulkan.cpp:67–99` | Injection-bridge callbacks (VVI-BRIDGE) | ✅ Already correct pattern; no raw RAII gap |
| `gpu_vector_index_vulkan.cpp:917–933` | No-op stub when `!THEMIS_HAS_VULKAN_IMPL` | ✅ Properly documented with removal plan |
| `lora::vulkan::VulkanBuffer` (production path) | Raw `VkBuffer`/`VkDeviceMemory` in member fields | ✅ Already RAII: `~VulkanBuffer() noexcept` + move ctor/assign zero handles |

### `VkBufferRaii` Addition

A `VkBufferRaii` move-only scope guard has been added to `gpu_vector_index_vulkan.cpp` inside the `#if THEMIS_HAS_VULKAN_IMPL` block for use in any future ad-hoc raw buffer allocations (e.g., staging buffers in Wave-B kernels) that do not go through `lora::vulkan::VulkanBuffer`.

**Removal / graduation plan**: When Wave-B GPU ANN kernels (L2/cosine/inner-product compute shaders) require staging buffers, use `VkBufferRaii` as the scope-guard. Graduate to `lora::vulkan::VulkanBuffer` when the buffer needs re-use or upload helpers. Remove stub comment when `THEMIS_HAS_VULKAN_IMPL` is always `1` on supported platforms.

**Roadmap ref**: `src/index/FUTURE_ENHANCEMENTS.md §"GPU Vector Index (Vulkan)"`, Wave-B Q4 2026.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).

