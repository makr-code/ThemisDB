# chimera — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **chimera** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

## Marker-Validierung 2026-08-31

- Quelle: `audit/MARKER_LOCATIONS_2026-08-31.md`
- Ergebnis: **38 reale Gaps**, **1 Doku-Leaks**
- Klassifikation: Doku-Leaks kommen aus auto-generierten `@note Gap Summary`-Headerzeilen und sind keine fehlende Produktionslogik.
- Real-Beispiel: `GAP-0364` → `src/chimera/mongodb_adapter.cpp:67` (// TODO: Actual mongocxx client creation)
- Doku-Leak-Beispiel: `GAP-0402` → `src/chimera/themisdb_adapter.cpp:7` (* @note Gap Summary: total=19; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=11, Debt=0, C=6, H=18, M=28, L=0)
- Korrespondierende Gesamtliste: `audit/MARKER_GAP_CLASSIFICATION_2026-08-31.md`

- **Total Gaps**: 491
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 9
- **HIGH**: 181
- **MEDIUM**: 298
- **LOW**: 3

### By Type

- braces_imbalance: 4
- braces_imbalance_midfile: 133
- cast_to_smaller_type: 1
- chimera_retry_duplication: 29
- circular_lock_ordering: 26
- db_connection_leak: 4
- iterator_invalidation: 6
- legacy_or_compat_path: 1
- manual_cleanup: 1
- missing_adr_reference: 1
- missing_noexcept_on_move: 1
- missing_volatile: 2
- module_doc_linkset_drift: 2
- no_retry_logic: 4
- null_dereference: 2
- o_n_squared: 5
- pointer_arithmetic_unbounded: 1
- repeated_search: 2
- scope_mismatch: 262
- sensitive_data_logging: 1
- string_concat_loop: 2
- todo_as_productionlogic: 1

## Top 20 Gaps

- [braces_imbalance] mongodb_adapter.cpp:1 (CRITICAL)
- [braces_imbalance] qdrant_adapter.cpp:1 (CRITICAL)
- [braces_imbalance] themisdb_adapter.cpp:1 (CRITICAL)
- [iterator_invalidation] themisdb_adapter.cpp:520 (CRITICAL)
- [iterator_invalidation] themisdb_adapter.cpp:570 (CRITICAL)
- [iterator_invalidation] themisdb_adapter.cpp:579 (CRITICAL)
- [iterator_invalidation] themisdb_adapter.cpp:588 (CRITICAL)
- [iterator_invalidation] themisdb_adapter.cpp:625 (CRITICAL)
- [iterator_invalidation] themisdb_adapter.cpp:686 (CRITICAL)
- [braces_imbalance] neo4j_adapter.cpp:1 (HIGH)
- [chimera_retry_duplication] retry_executor.cpp:21 (HIGH)
- [no_retry_logic] neo4j_adapter.cpp:44 (HIGH)
- [no_retry_logic] qdrant_adapter.cpp:44 (HIGH)
- [no_retry_logic] mongodb_adapter.cpp:46 (HIGH)
- [chimera_retry_duplication] retry_executor.cpp:56 (HIGH)
- [no_retry_logic] themisdb_adapter.cpp:74 (HIGH)
- [repeated_search] themisdb_adapter.cpp:138 (HIGH)
- [circular_lock_ordering] themisdb_adapter.cpp:185 (HIGH)
- [repeated_search] themisdb_adapter.cpp:190 (HIGH)
- [circular_lock_ordering] themisdb_adapter.cpp:214 (HIGH)

... and 471 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
