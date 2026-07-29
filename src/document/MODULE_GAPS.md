# document — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **document** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 35
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 0
- **HIGH**: 2
- **MEDIUM**: 31
- **LOW**: 2

### By Type

- module_doc_linkset_drift: 2
- scope_mismatch: 31
- todo_as_productionlogic: 2

## Top 20 Gaps

- [scope_mismatch] round_trip_editor.cpp:73 (HIGH)
- [scope_mismatch] round_trip_editor.cpp:95 (HIGH)
- [todo_as_productionlogic] round_trip_editor.cpp:7 (MEDIUM)
- [todo_as_productionlogic] round_trip_editor.cpp:15 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:51 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:55 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:63 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:64 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:65 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:66 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:67 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:72 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:74 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:85 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:86 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:87 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:88 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:89 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:94 (MEDIUM)
- [scope_mismatch] round_trip_editor.cpp:96 (MEDIUM)

... and 15 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).

---

## Resolved Findings

### [DOC-AUD-01] Schema Transition Edge Hardening
- **Status:** RESOLVED - 2026-07-28
- **Resolution:** Added `ERR_DOC_SCHEMA_TRANSITION_INVALID` (9412) and `ERR_DOC_VALIDATION_ABORTED` (9418) to the error taxonomy in `include/utils/error_registry.h`. Both codes are classified as `SCHEMA_VIOLATION` and covered by `classifyDocumentError()` and `documentErrorDescription()` in `include/document/document_diagnostics.h`. Schema transition and non-object-body validation failure paths now have distinct, named error codes with deterministic classification.

### [DOC-AUD-03] Unified Diagnostics Taxonomy
- **Status:** RESOLVED - 2026-07-28
- **Resolution:** `include/document/document_diagnostics.h` provides the complete Phase 2 diagnostics taxonomy: `DocumentErrorClass` enum (8 classes), `classifyDocumentError()`, `documentErrorClassName()`, `documentErrorDescription()`, `formatDocumentError()`, and `DocumentDiagnosticSink` — covering all 20 `ERR_DOC_*` codes (9400–9419) across store, schema, merge, lifecycle, round-trip, exchange, and input-validation failure paths. `DocumentDiagnosticSink` is thread-safe via `std::mutex`/`std::lock_guard` with per-class and aggregate error counting.
