# tensor — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **tensor** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

## Marker-Validierung 2026-08-31

- Quelle: `audit/MARKER_LOCATIONS_2026-08-31.md`
- Ergebnis: **25 reale Gaps**, **14 Doku-Leaks**
- Klassifikation: Doku-Leaks kommen aus auto-generierten `@note Gap Summary`-Headerzeilen und sind keine fehlende Produktionslogik.
- Real-Beispiel: `GAP-1551` → `src/tensor/compression_strategy.cpp:40` (// TODO: Wire to actual TensorTrainDecomposer)
- Doku-Leak-Beispiel: `GAP-1550` → `src/tensor/adapter_repository.cpp:7` (* @note Gap Summary: total=19; TODO=1, Stub=13, Unimpl=0, Mock=1, Sim=4, Debt=0, C=0, H=6, M=3, L=0)
- Korrespondierende Gesamtliste: `audit/MARKER_GAP_CLASSIFICATION_2026-08-31.md`

- **Total Gaps**: 787
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 11
- **HIGH**: 58
- **MEDIUM**: 716
- **LOW**: 2

### By Type

- braces_imbalance: 3
- braces_imbalance_midfile: 1
- db_connection_leak: 1
- delete_without_nullptr: 1
- hardcoded_path: 2
- iterator_invalidation: 3
- legacy_or_compat_path: 3
- manual_cleanup: 2
- missing_dtor: 2
- module_doc_linkset_drift: 2
- new_without_raii: 1
- no_timeout: 1
- o_n_squared: 2
- pointer_arithmetic_unbounded: 8
- resource_leaked_in_exception: 1
- scope_mismatch: 697
- shift_overflow: 4
- silent_error_swallow: 3
- smart_ptr_misuse: 2
- todo_as_productionlogic: 19
- uncaught_exception: 4
- unchecked_array_index: 3
- unchecked_malloc: 1
- unchecked_result: 6
- uninitialized_access: 4
- uninitialized_variable: 11

## Top 20 Gaps

- [braces_imbalance] adapter_repository.cpp:1 (CRITICAL)
- [braces_imbalance] tensor_mmap_bridge.cpp:1 (CRITICAL)
- [missing_dtor] tensor_index_manager.cpp:32 (CRITICAL)
- [missing_dtor] hnsw_tt_bridge.cpp:47 (CRITICAL)
- [new_without_raii] hnsw_tt_bridge.cpp:72 (CRITICAL)
- [smart_ptr_misuse] hnsw_tt_bridge.cpp:72 (CRITICAL)
- [smart_ptr_misuse] hnsw_tt_bridge.cpp:74 (CRITICAL)
- [no_timeout] tensor_core_bridge.cpp:131 (CRITICAL)
- [iterator_invalidation] hnsw_tt_bridge.cpp:171 (CRITICAL)
- [iterator_invalidation] utr_converter.cpp:181 (CRITICAL)
- [iterator_invalidation] adapter_repository.cpp:295 (CRITICAL)
- [braces_imbalance] tensor_index_manager.cpp:1 (HIGH)
- [scope_mismatch] tensor_fingerprint_graph.cpp:57 (HIGH)
- [resource_leaked_in_exception] hnsw_tt_bridge.cpp:72 (HIGH)
- [unchecked_malloc] tensor_mmap_bridge.cpp:82 (HIGH)
- [db_connection_leak] tnsr_task.cpp:83 (HIGH)
- [uninitialized_access] hiss_structural_search.cpp:85 (HIGH)
- [uninitialized_variable] ht_index.cpp:114 (HIGH)
- [scope_mismatch] tensor_index.cpp:118 (HIGH)
- [silent_error_swallow] tnsr_task.cpp:119 (HIGH)

... and 767 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
