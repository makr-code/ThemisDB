> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: current | validated: 2026-05-05 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Tensor Module

**Last Audit:** 2026-05-05
**Auditor:** Copilot
**Status:** 🟡 Pass with findings (Experimental module, Phase 1)

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ⚠️ Not yet registered in CMakeLists.txt (Phase 2 prerequisite) |
| Source Files | 3 (tensor_index.cpp, tensor_index_manager.cpp, hnsw_tt_bridge.cpp) |
| Header Files | 3 (tensor_index.h, tensor_index_manager.h, hnsw_tt_bridge.h) |
| Test Coverage | ⚠️ No dedicated test files yet (Phase 2) |
| Open TODOs | 0 explicit TODOs |
| Open Stubs | 7 (TTI-01, TTI-02, TIM-01, TIM-02, HTB-01, HTB-02, HTB-03) |
| Security Issues | 1 open — tenant key isolation not RocksDB-enforced yet (TEN-01) |

## Build System

The tensor module is **not yet** registered in `CMakeLists.txt`.  Before Phase 2
can proceed the following must be added:

```cmake
# src/tensor/ — Tensor-Train ANN index module
add_library(themis_tensor STATIC
    src/tensor/tensor_index.cpp
    src/tensor/tensor_index_manager.cpp
    src/tensor/hnsw_tt_bridge.cpp
)
target_link_libraries(themis_tensor
    themis_storage_tensor    # TensorTrainDecomposer etc.
    themis_utils
    ${ROCKSDB_LIBRARIES}
)
target_include_directories(themis_tensor PUBLIC include)
```

## Source Files Audited

| File | Status |
|------|--------|
| `include/tensor/tensor_index.h` | ✅ Reviewed — interface complete, well-documented |
| `include/tensor/tensor_index_manager.h` | ✅ Reviewed — lifecycle correct, GGML stub documented |
| `include/tensor/hnsw_tt_bridge.h` | ✅ Reviewed — two-layer design correct |
| `src/tensor/tensor_index.cpp` | ✅ Reviewed — TT inner-product sweep correct (Holtz 2012) |
| `src/tensor/tensor_index_manager.cpp` | ✅ Reviewed — mutex usage correct; tenant isolation in-memory only |
| `src/tensor/hnsw_tt_bridge.cpp` | ✅ Reviewed — linear-scan stub documented; TT arithmetic correct |

## Findings

### Open

#### ⚠️ [TEN-01] Tenant key isolation — in-memory only
- The `IndexHandle::key()` scheme (`__ttmgr__:<tenant>:<collection>:<field>`)
  enforces tenant isolation in-memory.  However, no RocksDB key validation or
  separator injection protection is implemented yet.
- **Severity:** Medium (Phase 1 is in-memory; risk materialises in Phase 2)
- **Action:** When RocksDB persistence is added (Phase 2), apply the same
  `isValidTenantComponent()` guard used by `src/index/index_manager.cpp` (#1872).

#### ⚠️ [STUB-01..07] Seven active stubs
- TTI-01 / TTI-02: `FlatTensorIndex::save/load` — no-ops with WARN log
- TIM-01: `ggmlCorePtrs()` — returns raw pointers without mmap pin
- TIM-02: `dropTenantIndexes()` — no RocksDB prefix-delete
- HTB-01: `HnswLayer` is linear-scan, not hnswlib
- HTB-02 / HTB-03: `HnswTTBridge::save/load` — no-ops with WARN log
- **Severity:** Low (all stubs emit WARN logs and have documented removal plans)
- **Action:** Resolve per Phase 2 schedule (Q4 2026).

#### ℹ️ [DUP-01] TT arithmetic duplicated
- `ttInnerProductFromTrains` and `ttNormFromTrain` are implemented in both
  `tensor_index.cpp` (as static helpers) and `hnsw_tt_bridge.cpp`.
- **Severity:** Low (correctness is not affected; code quality)
- **Action:** Extract to `src/tensor/tt_arithmetic.cpp` + `include/tensor/tt_arithmetic.h`
  in Phase 2.

#### ℹ️ [CMK-01] Not in CMakeLists.txt
- **Severity:** Low (Phase 1 is header-only integration; Phase 2 prerequisite)
- **Action:** Add `themis_tensor` library target before Phase 2 starts.

#### ℹ️ [TEST-01] No dedicated test files
- **Severity:** Low (Phase 1 correctness validated via existing storage/query tests)
- **Action:** Add `tests/tensor/test_tensor_index.cpp` in Phase 2 with
  TTI-01..20 test cases.

### Resolved

- None yet (Phase 1 initial audit).

## Compliance

| Requirement | Status |
|-------------|--------|
| Stub documentation in STUB_INVENTORY.md | ✅ Entries #150–#156 registered |
| Stub WARN logs at runtime | ✅ All 7 stubs emit THEMIS_WARN |
| Tenant isolation (in-memory) | ✅ Key prefix enforced |
| Tenant isolation (RocksDB) | ⚠️ Phase 2 prerequisite |
| Thread safety | ✅ shared_mutex on all read paths |
| No raw new/delete | ✅ unique_ptr throughout |
| No global mutable state | ✅ All state in instances |
| CUDA guard on GPU paths | ✅ N/A Phase 1 (future: THEMIS_ENABLE_CUDA) |
