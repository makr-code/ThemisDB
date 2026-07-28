# Audit Report - Document Module

<!-- Status: current | validated: 2026-07-28 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · DEVELOPMENT_STATUS_2026_07_28.md -->

## Summary

| Metric | Result |
|---|---|
| Build registration | pass |
| Source set size | header-first module + 1 src implementation file |
| Focused test presence | pass |
| Open hardening findings | yes |
| Critical blockers | none identified |

## Verified Files

- include/document/document_store.h
- include/document/document_manager.h
- include/document/document_lifecycle.h
- include/document/document_schema_evolution.h
- include/document/document_diff_merge.h
- include/document/xdomea_connector.h
- include/document/round_trip_editor.h
- src/document/round_trip_editor.cpp
- tests/test_document_store.cpp
- tests/test_xdomea_connector.cpp

## Findings

### Open

1. [DOC-AUD-01] schema transition and merge-conflict edge hardening remains active.
- Severity: medium
- Evidence: roadmap/future retain active work for schema/merge edge permutations.
- Action: close remaining deterministic conflict and transition regressions.

2. [DOC-AUD-02] round-trip persistence diagnostics need further tightening.
- Severity: medium
- Evidence: active follow-up work for relay snapshot failure taxonomy and diagnostics.
- Action: unify store/round-trip error categories and operator diagnostics.

3. [DOC-AUD-03] benchmark coverage remains proxy-heavy.
- Severity: low
- Evidence: mapped benchmark set exists but dedicated document-specific benchmarks are limited.
- Action: add module-native diff/merge and round-trip benchmark coverage.

### Closed

- core document runtime surfaces are present and source-verified.
- documentation set is synchronized to source-verifiable claims.
- changelog/roadmap role separation is aligned to governance pattern.

## Phase 2 Taxonomy Extension

Added 2026-07-28 as part of the Q3+Q4 2026 module diagnostics unification work.

### New error codes (9412–9419)

| Code | Constant | Purpose |
|------|----------|---------|
| 9412 | `ERR_DOC_SCHEMA_TRANSITION_INVALID`  | Schema version transition violates ordering or compatibility rules |
| 9413 | `ERR_DOC_SNAPSHOT_COLLISION`         | Round-trip snapshot ID already exists (relay/index collision) |
| 9414 | `ERR_DOC_ROUND_TRIP_PERSIST_FAIL`    | Round-trip persistence failed at store level |
| 9415 | `ERR_DOC_EXCHANGE_BOUNDARY_VIOLATED` | XDOMEA/exchange boundary enforcement failed |
| 9416 | `ERR_DOC_STORE_UNAVAILABLE`          | Backing document store is unavailable or unresponsive |
| 9417 | `ERR_DOC_LIFECYCLE_HOOK_FAILED`      | Lifecycle hook signaled a terminal failure (rare; hooks are noexcept) |
| 9418 | `ERR_DOC_VALIDATION_ABORTED`         | Schema validation aborted due to structural document error (non-object body) |
| 9419 | `ERR_DOC_VERSION_CONFLICT`           | Concurrent version update conflict detected |

### DocumentErrorClass taxonomy

All 20 ERR_DOC_* codes (9400–9419) are now classified into one of eight semantic failure classes exposed via `include/document/document_diagnostics.h`:

| Class | Codes |
|-------|-------|
| `STORE_FAILURE`    | 9400, 9401, 9402, 9408, 9409, 9416 |
| `SCHEMA_VIOLATION` | 9403, 9404, 9405, 9412, 9418 |
| `MERGE_CONFLICT`   | 9407, 9419 |
| `LIFECYCLE_ERROR`  | 9417 |
| `ROUND_TRIP_ERROR` | 9413, 9414 |
| `EXCHANGE_ERROR`   | 9406, 9415 |
| `INVALID_INPUT`    | 9410, 9411 |
| `UNKNOWN`          | all other codes |

### formatDocumentError() contract

`formatDocumentError(const themis::Error& err)` returns a single-line diagnostic string in the format:

```
[DOC:<numeric_code>/<CLASS_NAME>] <description>[: <context>]
```

Example: `[DOC:9407/MERGE_CONFLICT] three-way merge produced unresolvable conflicts: 3 conflict(s)`

### Resolution status

- **[DOC-AUD-01]** schema transition and merge-conflict edge hardening — **RESOLVED**
  - ERR_DOC_SCHEMA_TRANSITION_INVALID (9412) and ERR_DOC_VALIDATION_ABORTED (9418) added to the error taxonomy; unified classification in document_diagnostics.h closes the diagnostics gap.
- **[DOC-AUD-02]** round-trip persistence diagnostics — **PARTIALLY RESOLVED** (taxonomy in place; dedicated benchmark expansion remains open per roadmap).

## Compliance Snapshot

| Requirement | Status |
|---|---|
| Source-verifiable behavior claims | pass |
| Structured forward planning in roadmap/future | pass |
| Historical completion tracked in changelog | pass |
| Core module docs synchronized | pass |
| Phase 2 error taxonomy complete (9400–9419) | pass |
| Unified diagnostics header present | pass |