> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · SECURITY.md · CHANGELOG.md · PERFORMANCE_EXPECTATIONS.md -->

# Audit Report — Tensor Module

**Last Audit:** 2026-05-13
**Auditor:** Copilot
**Status:** 🟡 Pass with findings (Experimental module, Phases 1–7 skeleton)

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ⚠️ Not yet registered in CMakeLists.txt (Phase 2 prerequisite) |
| Source Files | 14 (adapter_repository, hiss_structural_search, hnsw_tt_bridge, ht_index, hyper_index_builder, tensor_butterfly_operator, tensor_core_bridge, tensor_fingerprint_graph, tensor_index, tensor_index_manager, tensor_ingestion_bridge, tensor_mmap_bridge, tnsr_task, utr_converter) |
| Header Files | 15 (adapter_repository, hiss_structural_search, hnsw_tt_bridge, ht_index, ht_train, hyper_index_builder, tensor_butterfly_operator, tensor_core_bridge, tensor_fingerprint_graph, tensor_index, tensor_index_manager, tensor_ingestion_bridge, tensor_mmap_bridge, tnsr_task, utr_converter) |
| Test Coverage | ⚠️ tests/test_tensor_ingestion_bridge.cpp (TIB-01..TIB-20); no unit tests for index backends yet (Phase 2) |
| Open TODOs | 0 explicit TODOs |
| Open Stubs | 16 (TTI-01/02, TIM-01/02, HTB-01/02/03, #160, #172, #174, #176, #178, #252, #254, #257, #258) |
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
| `include/tensor/tensor_ingestion_bridge.h` | ✅ Reviewed — κ-gate, pilot decomp, provenance documented |
| `include/tensor/tensor_core_bridge.h` | ✅ Reviewed — upsert semantics, fail-closed validation |
| `include/tensor/tensor_mmap_bridge.h` | ✅ Reviewed — MAP_ANONYMOUS stub (#176) documented |
| `include/tensor/adapter_repository.h` | ✅ Reviewed — heap-copy stub (#172) documented |
| `include/tensor/tensor_butterfly_operator.h` | ✅ Reviewed — FOURIER/WHT complete; RADON/GREENS_FUNCTION stubs |
| `include/tensor/tensor_fingerprint_graph.h` | ✅ Reviewed — column-mean cosine stub (#174) documented |
| `include/tensor/ht_train.h` | ✅ Reviewed — HT decomposition types |
| `include/tensor/ht_index.h` | ✅ Reviewed — HT linear-scan index interface |
| `include/tensor/hiss_structural_search.h` | ✅ Reviewed — QTTMappingDescriptor and TemplateCatalog topology |
| `include/tensor/tnsr_task.h` | ✅ Reviewed — background topology stub (#252) documented |
| `include/tensor/utr_converter.h` | ✅ Reviewed — FNV-1a (#257) and raw-pixel (#258) stubs documented |
| `include/tensor/hyper_index_builder.h` | ✅ Reviewed — tabular co-occurrence TT index |
| `src/tensor/tensor_index.cpp` | ✅ Reviewed — TT inner-product sweep correct (Holtz 2012) |
| `src/tensor/tensor_index_manager.cpp` | ✅ Reviewed — mutex usage correct; tenant isolation in-memory only |
| `src/tensor/hnsw_tt_bridge.cpp` | ✅ Reviewed — linear-scan stub documented; TT arithmetic correct |
| `src/tensor/tensor_ingestion_bridge.cpp` | ✅ Reviewed — TIB-01..TIB-20 tests pass |
| `src/tensor/tensor_core_bridge.cpp` | ✅ Reviewed — InMemoryTensorBackend stub (#160) documented |
| `src/tensor/tensor_mmap_bridge.cpp` | ✅ Reviewed — RAII correct; MAP_ANONYMOUS stub |
| `src/tensor/adapter_repository.cpp` | ✅ Reviewed — heap-copy path; mmap deferred Q1 2027 |
| `src/tensor/tensor_butterfly_operator.cpp` | ✅ Reviewed — FOURIER/WHT implemented |
| `src/tensor/tensor_fingerprint_graph.cpp` | ✅ Reviewed — LSH+MinHash; column-mean cosine stub |
| `src/tensor/ht_index.cpp` | ✅ Reviewed — FlatHTIndex Phase-5 skeleton |
| `src/tensor/hiss_structural_search.cpp` | ✅ Reviewed — stochastic sub-network sampling |
| `src/tensor/tnsr_task.cpp` | ✅ Reviewed — background task stub; persistence deferred |
| `src/tensor/utr_converter.cpp` | ✅ Reviewed — multi-modal fallback stubs documented |
| `src/tensor/hyper_index_builder.cpp` | ✅ Reviewed — tabular co-occurrence builder |

## Findings

### Open

#### ⚠️ [TEN-01] Tenant key isolation — in-memory only
- The `IndexHandle::key()` scheme (`__ttmgr__:<tenant>:<collection>:<field>`)
  enforces tenant isolation in-memory.  However, no RocksDB key validation or
  separator injection protection is implemented yet.
- **Severity:** Medium (Phase 1 is in-memory; risk materialises in Phase 2)
- **Action:** When RocksDB persistence is added (Phase 2), apply the same
  `isValidTenantComponent()` guard used by `src/index/index_manager.cpp` (#1872).

#### ⚠️ [STUB-01..16] Sixteen active stubs
- TTI-01 / TTI-02: `FlatTensorIndex::save/load` — no-ops with WARN log
- TIM-01: `ggmlCorePtrs()` — returns raw pointers without mmap pin
- TIM-02: `dropTenantIndexes()` — no RocksDB prefix-delete
- HTB-01: `HnswLayer` is linear-scan, not hnswlib
- HTB-02 / HTB-03: `HnswTTBridge::save/load` — no-ops with WARN log
- #160: `TensorCoreStorageBridge` — `InMemoryTensorBackend` until RocksDB wired
- #172: `AdapterRepository::loadAdapter()` — heap-copy cores; no mmap
- #174: `TensorFingerprintGraph` — column-mean cosine; not full TT inner-product
- #176: `TensorMmapBridge` — `MAP_ANONYMOUS` + memcpy; no `MAP_SHARED` on SST
- #178: `HTTrain::toTTTrain()` — full reconstruction for TT compatibility
- #252: `TNSRTask` — topology mutations not persisted without bridge
- #254: `HissReshaper::exposeQuantics()` — residual-factor fallback; not pure-binary QTT
- #257: `UTRConverter::fromDocument()` — FNV-1a hash embedding fallback
- #258: `UTRConverter::fromImage()` — raw-pixel TT; no semantic encoder
- **Severity:** Low (all stubs emit WARN logs and have documented removal plans)
- **Action:** Resolve per phase schedule; see `README.md` stub table for targets.

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
| Stub documentation in STUB_INVENTORY.md | ✅ Entries registered for all 16 stubs |
| Stub WARN logs at runtime | ✅ All 16 stubs emit THEMIS_WARN |
| Tenant isolation (in-memory) | ✅ Key prefix enforced |
| Tenant isolation (RocksDB) | ⚠️ Phase 2 prerequisite |
| Thread safety | ✅ shared_mutex on all read paths |
| No raw new/delete | ✅ unique_ptr throughout |
| No global mutable state | ✅ All state in instances |
| CUDA guard on GPU paths | ✅ N/A Phase 1 (future: THEMIS_ENABLE_CUDA) |
| SECURITY.md present | ✅ `src/tensor/SECURITY.md` — threat model + controls |
| CHANGELOG.md present | ✅ `src/tensor/CHANGELOG.md` — Keep a Changelog format |
| PERFORMANCE_EXPECTATIONS.md present | ✅ `src/tensor/PERFORMANCE_EXPECTATIONS.md` — benchmark targets |
