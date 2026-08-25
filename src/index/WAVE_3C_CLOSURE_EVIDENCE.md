# Wave 3-C Closure Evidence — `src/index/`

**Date:** 2026-08-25  
**Scope:** Six CRITICAL gap-fix items assigned to Wave 3-C in the index module.  
**Method:** Read every reported source line, inspect actual code, then fix or record disposition.

---

## 1. Fix 2 — CRITICAL FIXED: `vector_auto_buffer.cpp` — Exception in Destructor

### Evidence

| | Before | After |
|---|---|---|
| **File** | `src/index/vector_auto_buffer.cpp` | `src/index/vector_auto_buffer.cpp` |
| **Destructor** | `~VectorAutoBuffer()` — not noexcept, bare `stop()` call | `~VectorAutoBuffer() noexcept` — `stop()` wrapped in try/catch |
| **Header** | `include/index/vector_auto_buffer.h:173` — `~VectorAutoBuffer()` | `~VectorAutoBuffer() noexcept` |

**Root cause:** `stop()` calls `flush()`, which calls downstream VectorIndexManager operations. Any exception thrown during an in-progress flush would propagate out of the destructor. If the destructor was called during stack unwinding (e.g. a vector of VectorAutoBuffer objects during scope exit), this invokes `std::terminate`.

**Fix applied:**
```cpp
// BEFORE
VectorAutoBuffer::~VectorAutoBuffer() {
    if (running_.load()) {
        stop();
    }
}

// AFTER
VectorAutoBuffer::~VectorAutoBuffer() noexcept {
    if (running_.load()) {
        try {
            stop();
        } catch (const std::exception& e) {
            THEMIS_WARN("VectorAutoBuffer::~VectorAutoBuffer: exception during stop (ignored): {}",
                        e.what());
        } catch (...) {
            THEMIS_WARN("VectorAutoBuffer::~VectorAutoBuffer: unknown exception during stop (ignored)");
        }
    }
}
```

**C++ standard reference:** `[except.terminate]` — if an exception escapes a destructor called during stack unwinding, `std::terminate` is called.

---

## 2. Fix 1 — Pre-existing: `graph_auto_buffer.cpp:44` — Already noexcept

### Evidence

Actual code at line 44 (inspected 2026-08-25):

```cpp
GraphAutoBuffer::~GraphAutoBuffer() noexcept {
    // Gap: exception_in_destructor — wrap stop() to prevent exception propagation
    if (running_.load()) {
        try {
            stop();
        } catch (const std::exception& e) {
            THEMIS_ERROR("GraphAutoBuffer::~GraphAutoBuffer: exception during stop (ignored): {}",
                         e.what());
        } catch (...) {
            THEMIS_ERROR("GraphAutoBuffer::~GraphAutoBuffer: unknown exception during stop (ignored)");
        }
    }
}
```

**Disposition:** ✅ Already fixed before Wave 3-C. No change required. Scanner reported line 52 (inside the catch body) — the fix comment was at line 45; the scanner was reporting the wrong granularity.

---

## 3. Fix 3 — Confirmed FP: `gpu_memory_oversubscription.cpp:53` — GPU Memory Leak

### Evidence

Actual code at lines 94–106 (inspected 2026-08-25):

```cpp
~Impl() noexcept {
    // RAII cleanup: evict all VRAM-resident partitions on destruction
    for (auto& [id, p] : partitions) {
        if (p.in_vram && p.vram_ptr &&
            p.vram_ptr != static_cast<void*>(p.host_data.data())) {
#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_HIP)
            themis::gpu::GPUUnifiedMemoryAllocator::GetInstance().free(p.vram_ptr);
#endif
            p.vram_ptr = nullptr;
            p.in_vram  = false;
        }
    }
}
```

**Disposition:** ✅ False positive. The RAII cleanup loop in `~Impl() noexcept` correctly frees all VRAM-resident partitions. The scanner flagged line 53 (the struct `Partition` opening brace) — not actual leaked memory. The outer class destructor `~GPUMemoryOversubscriptionManager::Impl()` handles teardown properly.

---

## 4. Fix 4 — Confirmed FP: `cuda_hnsw_graph_traversal.cpp:362,370,381` — GPU Memory Leaks

### Evidence

Three reported cudaMalloc call sites (inspected 2026-08-25):

**Line 359** (`d_offsets`):
```cpp
if (cudaMalloc(&impl_->d_offsets, off_bytes) != cudaSuccess) {
    THEMIS_ERROR("CudaHnswTraversalEngine::buildIndex: cudaMalloc(offsets) failed");
    impl_->freeDevice();        // frees d_vectors (already allocated)
    impl_->cuda_available = false;
    impl_->index_built = true;
    return true;  // CPU fallback
}
```

**Line 366** (`d_neighbours`):
```cpp
if (cudaMalloc(&impl_->d_neighbours, nb_bytes) != cudaSuccess) {
    THEMIS_ERROR("CudaHnswTraversalEngine::buildIndex: cudaMalloc(neighbours) failed");
    impl_->freeDevice();        // frees d_vectors + d_offsets
    ...
}
```

**Lines 394** (`d_visited_pool` — non-fatal):
```cpp
cudaError_t ve = cudaMalloc(&impl_->d_visited_pool, new_pool_sz);
if (ve == cudaSuccess) {
    impl_->visited_pool_bytes = new_pool_sz;
    ...
} else {
    impl_->d_visited_pool   = nullptr;
    impl_->visited_pool_bytes = 0;
    THEMIS_WARN("... per-invocation fallback allocation will be used");
}
```

`freeDevice()` is defined (inspected lines 259–271):
```cpp
void freeDevice() {
    if (d_vectors)       { cudaFree(d_vectors);       d_vectors       = nullptr; }
    if (d_offsets)       { cudaFree(d_offsets);       d_offsets       = nullptr; }
    if (d_neighbours)    { cudaFree(d_neighbours);    d_neighbours    = nullptr; }
    if (d_result_ids)    { cudaFree(d_result_ids);    d_result_ids    = nullptr; }
    if (d_result_scores) { cudaFree(d_result_scores); d_result_scores = nullptr; }
    if (d_visited_pool)  { cudaFree(d_visited_pool);  d_visited_pool  = nullptr;
                                                       visited_pool_bytes = 0;  }
    if (stream)          { cudaStreamDestroy(stream); stream          = nullptr; }
}
```

**Disposition:** ✅ False positive. All three `cudaMalloc` calls are checked. `freeDevice()` is called on every failure path and frees all previously allocated device pointers. The `buildIndex()` function also has `freeDevice()` at line 334 (unconditional reset before any new allocation attempt). No VRAM is leaked.

---

## 5. Fix 5 — All Confirmed FP: Iterator Invalidation Sites

### 5.1 `vector_index.cpp:80`

```cpp
// Lines 75-89
auto it = pkToId.find(pk);
if (it == pkToId.end()) {
    const size_t id = idToPk.size();
    pkToId.emplace(pk, id);   // ← line 78: fresh insertion, no active iterator
    idToPk.push_back(pk);
    return id;
}
```

`pkToId.emplace()` is safe — the iterator `it` was just checked as `end()` (not found), so no active iterator into `pkToId` is live when `emplace` is called.

### 5.2 `multi_vector_search.cpp:224` and `:406`

Both sites use read-only `.find()` on local `score_map` / `vector_score_by_doc` / `keyword_scores` maps inside a range-for over `all_doc_ids`. The outer loop iterates `all_doc_ids` (a `std::vector`); the inner `.find()` operates on completely independent maps. No container mutation occurs during either loop.

### 5.3 `graph_index.cpp:244,247,248`

Index-based string parsing (A-2.6 annotated in the file):
```cpp
size_t start = 0;
while (start < s.size()) {
    auto pos = s.find(',', start);               // string operation, no container iterator
    ...
    encryptList.push_back(part.substr(...));      // push_back to local vector by index
    start = pos + 1;
}
```

No iterator into `encryptList` is held across the `push_back`. The only active iterator is the implicit position in the string-scan loop (`start` is an integer index).

### 5.4 `edge_types.cpp:364`

```cpp
std::optional<EdgeCategory> EdgeTypeRegistry::getCategoryForType(std::string_view type_name) const {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    auto it = types_.find(std::string(type_name));
    if (it != types_.end()) {
        return it->second.category;
    }
    return std::nullopt;
}
```

Read-only `shared_lock` path. `.find()` does not modify the container. `it` is not invalidated because no write happens.

### 5.5 `gpu_memory_oversubscription.cpp:230`

A-2.1 annotated (touchLRULocked):
```cpp
auto it = lru_map.find(partition_id);
if (it != lru_map.end()) {
    lru_list.erase(it->second);    // invalidates only the erased list iterator
}
lru_list.push_front(partition_id); // does not invalidate other list iterators
lru_map[partition_id] = lru_list.begin();
```

`std::list::erase` only invalidates the iterator to the erased element. `std::list::push_front` does not invalidate any existing iterators. The map is updated after the mutation. This is the canonical safe two-step pattern.

---

## 6. Fix 6 — Confirmed FP: Unchecked CUDA Calls

### `cuda_hnsw_graph_traversal.cpp`

Every `cudaMemcpy` in the file captures the return value in a boolean (`all_ok`, `queries_ok`) or directly checks `!= cudaSuccess`. Failures fall back to CPU. Verified at lines 346, 373–376, 523–525, 586–591, 650–652, 703–708.

### `gpu_vector_index.cpp`

Zero raw CUDA calls present (grep confirmed). The file delegates to `CUDAVectorBackend` (an abstraction layer that handles its own error checking), `HIPVectorBackend`, or the `GPUMemoryOversubscriptionManager`.

---

## Summary Table

| Fix ID | Gap Type | File | Line | Real Gap? | Action |
|--------|----------|------|------|-----------|--------|
| Fix 1 | exception_in_destructor | graph_auto_buffer.cpp | 44 | Pre-fixed | No change |
| **Fix 2** | **exception_in_destructor** | **vector_auto_buffer.cpp** | **58** | **YES** | **FIXED** |
| Fix 3 | gpu_memory_leak | gpu_memory_oversubscription.cpp | 53 | FP | Documented |
| Fix 4a | gpu_memory_leak | cuda_hnsw_graph_traversal.cpp | 362 | FP | Documented |
| Fix 4b | gpu_memory_leak | cuda_hnsw_graph_traversal.cpp | 370 | FP | Documented |
| Fix 4c | gpu_memory_leak | cuda_hnsw_graph_traversal.cpp | 381 | FP | Documented |
| Fix 5a | iterator_invalidation | vector_index.cpp | 80 | FP | Documented |
| Fix 5b | iterator_invalidation | multi_vector_search.cpp | 224 | FP | Documented |
| Fix 5c | iterator_invalidation | multi_vector_search.cpp | 406 | FP | Documented |
| Fix 5d | iterator_invalidation | graph_index.cpp | 244,247,248 | FP | Documented |
| Fix 5e | iterator_invalidation | edge_types.cpp | 364 | FP | Documented |
| Fix 5f | iterator_invalidation | gpu_memory_oversubscription.cpp | 230 | FP | Documented |
| Fix 6a | unchecked_cuda_call | cuda_hnsw_graph_traversal.cpp | all | FP | Documented |
| Fix 6b | unchecked_cuda_call | gpu_vector_index.cpp | N/A | FP | Documented |

**Net CRITICAL reduction: 29 → 28 (−1 real fix; 6 braces_imbalance FPs remain in scanner but are not real)**

---

## Files Changed

| File | Change |
|------|--------|
| `src/index/vector_auto_buffer.cpp` | Destructor changed to `noexcept`; try/catch added around `stop()` |
| `include/index/vector_auto_buffer.h` | Declaration updated to `~VectorAutoBuffer() noexcept` |
| `tests/index/test_wave3c_index_raii.cpp` | **New** — compile-time + runtime tests |
| `src/index/MODULE_GAPS.md` | Wave 3-C section added; CRITICAL count updated; FPs annotated |
| `src/index/WAVE_3C_CLOSURE_EVIDENCE.md` | **This file** |
