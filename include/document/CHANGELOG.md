<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Document Module (Public Headers)

All notable changes to the Document module public headers are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
For implementation-level changes see `../../src/document/CHANGELOG.md`.

## [Unreleased]

## [1.3.0] — 2026-04-15
### Added
- `document_store.h`: `DocumentId`, `CollectionId`, `DocumentRecord`, `IDocumentStore`, `InMemoryDocumentStore` — pluggable backend storage interface for raw document persistence; thread-safe in-memory reference implementation; `put`, `get`, `update`, `remove`, `list`, `count`; all methods return `Result<T>`; document-specific error codes ERR_DOC_NOT_FOUND / ERR_DOC_ALREADY_EXISTS / ERR_DOC_INVALID_ID registered in `ErrorRegistry`
- `document_lifecycle.h`: `DocumentEventType`, `DocumentLifecycleEvent`, `IDocumentLifecycleHook` — observer interface for create/update/delete lifecycle side-effects; all callbacks `noexcept`; hooks guaranteed to fire even on storage failure for `afterDelete`
- `document_manager.h`: `KeyRotationDescriptor`, `IEncryptedDocumentEntity`, `IDocumentManager`, `InMemoryDocumentManager`, `InMemoryEncryptedEntity` — primary document CRUD interface with `Result<T>` error propagation; opaque encrypted-entity factory via `createEncrypted()`; `registerLifecycleHook` / `unregisterLifecycleHook` with thread-safe shared_mutex dispatch
- `document_schema_evolution.h`: `SchemaVersion`, `SchemaFieldType`, `SchemaFieldDescriptor`, `SchemaDescriptor`, `FieldViolationKind`, `FieldViolation`, `ValidationReport`, `IDocumentSchemaEvolution`, `InMemoryDocumentSchemaEvolution` — schema-version registry with `registerVersion`, `validate`, `seal`; immutable after `seal()`
- `document_diff_merge.h`: `FieldChange`, `DocumentDiff`, `MergeConflict`, `MergeStrategy`, `MergeResult`, `IDocumentDiffMerge`, `InMemoryDocumentDiffMerge` — field-level diff and three-way merge; strategies OURS_WINS / THEIRS_WINS / FAIL; operates on document IDs not raw payloads
- Document-specific error codes (9400-9411) added to `include/utils/error_registry.h` and `src/utils/error_registry.cpp`
- 38 acceptance-criteria tests in `tests/test_document_store.cpp` (DS-01..DS-15, DM-01..DM-08, DL-01..DL-05, DSE-01..DSE-08, DDM-01..DDM-11); CI target: `test_document_store_focused`

## [1.2.0] — 2026-03-24
### Added
- `xdomea_connector.h`: `XDOMEAVersion`, `XDOMEAObjectType`, `XDOMEARetentionCategory`, `XDOMEADocument`, `XDOMEAImportResult`, `XDOMEAExportResult`, `IXDOMEAConnector`, `InMemoryXDOMEAConnector` — thread-safe connector for XDOMEA 2.1/3.0 (KoSIT) document management and records management; supports storeDocument, getDocument, removeDocument, listByType, listByRetention, listChildren, importFromXML (Nachrichtentypen 0201/0202/0203/0401/0501/0601), exportToXML with XML escaping; 30 tests in `tests/test_xdomea_connector.cpp`; CI: `xdomea-connector-ci.yml`

## [1.0.0] — 2026-03-22
### Added
- `encrypted_entities.h`: `SecureDocument` with AES-256-GCM field-level encryption
- `document_manager_deprecated.h`: legacy `DocumentManager` interface (`User`, `Customer` structs) retained for ABI backward compatibility
