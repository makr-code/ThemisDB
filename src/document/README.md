# ThemisDB Document Module

<!-- Status: current | validated: 2026-07-28 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · DEVELOPMENT_STATUS_2026_07_28.md -->

## Module Purpose

The document module provides document-centric runtime surfaces for ThemisDB, including store and manager abstractions, lifecycle hooks, schema evolution, diff/merge support, XDOMEA exchange, and round-trip snapshot editing.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| include/document/document_store.h | store contract and in-memory CRUD primitives |
| include/document/document_manager.h | manager orchestration on top of document store interfaces |
| include/document/document_lifecycle.h | before/after lifecycle hook integration |
| include/document/document_schema_evolution.h | schema registry, sealing, and validation reports |
| include/document/document_diff_merge.h | diff and merge strategies across versions |
| include/document/xdomea_connector.h | XDOMEA import/export and repository operations |
| include/document/round_trip_editor.h | round-trip editor interface contract |
| src/document/round_trip_editor.cpp | store-backed round-trip snapshot persistence |

## Scope

In scope:
- document store/manager and lifecycle orchestration contracts
- schema evolution, diff/merge, and document snapshot workflows
- XDOMEA exchange interfaces and document round-trip persistence paths

Out of scope:
- non-document domain ownership in unrelated modules
- storage-engine internals outside document interface boundaries
- external protocol ownership not mapped to document contracts

## Runtime Behavior and Limits

- public document paths are Result-based and primarily non-throwing.
- in-memory/reference implementations are process-local by default.
- merge and schema paths return explicit conflict/validation errors.
- round-trip persistence uses deterministic relay-indexed snapshot IDs.

## Sourcecode Verification (Module: document/readme)

- Verified files:
  - include/document/document_store.h
  - include/document/document_manager.h
  - include/document/document_lifecycle.h
  - include/document/document_schema_evolution.h
  - include/document/document_diff_merge.h
  - include/document/xdomea_connector.h
  - include/document/round_trip_editor.h
  - src/document/round_trip_editor.cpp
- Verified behavior surfaces:
  - store/manager contracts and lifecycle integration boundaries
  - schema evolution plus diff/merge runtime semantics
  - XDOMEA and round-trip editor persistence integration
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md