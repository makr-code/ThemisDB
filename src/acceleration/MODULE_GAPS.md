# acceleration — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **acceleration** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

## Marker-Validierung 2026-08-31

- Quelle: `audit/MARKER_LOCATIONS_2026-08-31.md`
- Ergebnis: **16 reale Gaps**, **26 Doku-Leaks**
- Klassifikation: Doku-Leaks kommen aus auto-generierten `@note Gap Summary`-Headerzeilen und sind keine fehlende Produktionslogik.
- Real-Beispiel: `GAP-0200` → `src/acceleration/break_even_validator.cc:184` (// Export metrics (TODO: integrate with Prometheus))
- Doku-Leak-Beispiel: `GAP-0198` → `src/acceleration/ai_hardware_dispatcher.cpp:7` (* @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=28, H=49, M=2, L=0)
- Korrespondierende Gesamtliste: `audit/MARKER_GAP_CLASSIFICATION_2026-08-31.md`

- **Total Gaps**: 2558
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 37
- **HIGH**: 221
- **MEDIUM**: 2299
- **LOW**: 1

### By Type

- blocking_no_timeout: 2
- braces_imbalance: 7
- braces_imbalance_midfile: 8
- cast_to_smaller_type: 2
- circular_lock_ordering: 4
- copy_overhead: 2
- crypto_weakness: 2
- data_race: 1
- db_connection_leak: 11
- delete_no_nullptr: 19
- delete_without_nullptr: 19
- duplicate_qualified_signature: 6
- expensive_inner_op: 12
- hardcoded_path: 1
- legacy_or_compat_path: 3
- manual_cleanup: 43
- missing_noexcept_on_move: 9
- missing_resource_limits: 1
- mock_return_value: 1
- module_doc_linkset_drift: 1
- new_without_raii: 12
- no_timeout: 3
- path_traversal: 1
- range_temporary: 4
- repeated_search: 1
- resource_leaked_in_exception: 8
- scope_mismatch: 2237
- size_assumption: 6
- smart_ptr_misuse: 12
- stale_doc_section_reference: 12
- stub_temporary_comment: 1
- todo_as_productionlogic: 52
- uncaught_exception: 1
- unchecked_cuda_call: 14
- unchecked_result: 17
- uninitialized_access: 13
- uninitialized_array: 1
- uninitialized_variable: 9

## Top 20 Gaps

- [braces_imbalance] cpu_backend.cpp:1 (CRITICAL)
- [braces_imbalance] cuda_backend.cpp:1 (CRITICAL)
- [braces_imbalance] faiss_gpu_backend.cpp:1 (CRITICAL)
- [braces_imbalance] geo_acceleration_bridge.cpp:1 (CRITICAL)
- [braces_imbalance] vec_knn.cpp:1 (CRITICAL)
- [new_without_raii] oneapi_backend.cpp:83 (CRITICAL)
- [smart_ptr_misuse] oneapi_backend.cpp:83 (CRITICAL)
- [new_without_raii] oneapi_backend.cpp:87 (CRITICAL)
- [smart_ptr_misuse] oneapi_backend.cpp:87 (CRITICAL)
- [new_without_raii] oneapi_backend.cpp:90 (CRITICAL)
- [smart_ptr_misuse] oneapi_backend.cpp:90 (CRITICAL)
- [new_without_raii] oneapi_backend.cpp:94 (CRITICAL)
- [smart_ptr_misuse] oneapi_backend.cpp:94 (CRITICAL)
- [data_race] oneapi_backend.cpp:97 (CRITICAL)
- [new_without_raii] faiss_gpu_backend.cpp:175 (CRITICAL)
- [smart_ptr_misuse] faiss_gpu_backend.cpp:175 (CRITICAL)
- [blocking_no_timeout] oneapi_backend.cpp:182 (CRITICAL)
- [no_timeout] oneapi_backend.cpp:182 (CRITICAL)
- [new_without_raii] faiss_gpu_backend.cpp:184 (CRITICAL)
- [smart_ptr_misuse] faiss_gpu_backend.cpp:184 (CRITICAL)

... and 2538 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
