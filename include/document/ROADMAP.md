<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/document/ROADMAP.md -->

# Roadmap — Document Module (Public Headers)

> Implementation roadmap: `../../src/document/ROADMAP.md`

## Current Status

v1.2.0 — Public headers production-ready. `encrypted_entities.h` is production-ready. `xdomea_connector.h` provides full XDOMEA 2.1/3.0 DMS/RM connector. `document_manager_deprecated.h` is maintained for backward compatibility only.

## Completed ✅

- [x] `SecureDocument` encrypted entity interface (`encrypted_entities.h`)
- [x] Deprecated document manager headers with migration warnings
- [x] `IXDOMEAConnector` / `InMemoryXDOMEAConnector` — XDOMEA 3.0/2.1 document management and records management (`xdomea_connector.h`)

## Planned

- [x] Remove `document_manager_deprecated.h` after migration period (Target: v2.0.0)
- [x] Expose `IDocumentStore` abstract interface for pluggable backends (Target: v1.3.0)
- [x] Expose `IDocumentManager` CRUD interface with `Result<T>` error propagation (Target: v1.3.0)
- [x] `IDocumentLifecycleHook` / `DocumentLifecycleEvent` for audit and side-effect dispatch (Target: v1.3.0)
- [x] `IDocumentSchemaEvolution` with schema-version registry, `SchemaDescriptor`, `ValidationReport` (Target: v1.3.0)
- [x] `IDocumentDiffMerge` with `DocumentDiff`, `MergeResult`, `MergeConflict` (Target: v1.3.0)
- [x] `IEncryptedDocumentEntity` opaque handle with `KeyRotationDescriptor` / `reencrypt()` (Target: v1.3.0)

## Production Readiness Checklist

- [x] Public headers compile cleanly with `-Wall -Wextra`
- [x] Deprecated symbols annotated with `[[deprecated]]`
- [x] `IXDOMEAConnector` header + 30 acceptance-criteria tests
- [x] `IDocumentStore` abstract interface published (`document_store.h`)
- [x] `IDocumentManager` + lifecycle + schema + diff/merge published (`document_manager.h`, `document_lifecycle.h`, `document_schema_evolution.h`, `document_diff_merge.h`)
- [x] 38 acceptance-criteria tests in `tests/test_document_store.cpp` (DS/DM/DL/DSE/DDM)
- [ ] Migration guide from deprecated API completed
