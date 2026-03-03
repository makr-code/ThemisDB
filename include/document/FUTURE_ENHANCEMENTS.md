# Document Module - Future Header Enhancements

## Scope

- `IDocumentManager` interface extensions for lifecycle hooks and schema-aware operations
- Encrypted document entity API (`IEncryptedDocumentEntity`) as an opaque-handle extension to `encrypted_entities.h`
- Document lifecycle hooks (create/update/delete) via `IDocumentLifecycleHook` for audit and side-effect dispatch
- Schema evolution API for documents — `IDocumentSchemaEvolution` — enabling forward/backward-compatible field migration
- Document diff and merge API (`IDocumentDiffMerge`) for three-way merge and structured change computation
- `Result<T>`-based error handling across all new document APIs; no exception propagation through interface boundaries

## Design Constraints

- `[ ]` Headers in `document_manager_deprecated.h` are not extended; all new document APIs are defined in new header files distinct from the deprecated surface
- `[ ]` All new document APIs use `Result<T>` for error propagation; no new APIs throw exceptions
- `[ ]` Encrypted entity API is opaque to callers; `IEncryptedDocumentEntity` exposes no key material, cipher parameters, or internal buffer pointers
- `[ ]` `IDocumentLifecycleHook` callbacks are `noexcept`; exceptions in hook implementations terminate the process
- `[ ]` `IDocumentSchemaEvolution` is immutable after registration; schema versions can be added but not modified post-registration
- `[ ]` All `IDocumentManager` methods accept and return documents by value or via `Result<T>`; raw pointer ownership transfer is prohibited in new APIs

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IDocumentManager` | `QueryEngine`, `StorageLayer`, `ReplicationLayer` | Primary document CRUD interface; all methods return `Result<T>`; thread-safe |
| `IEncryptedDocumentEntity` | `EncryptionLayer`, `StorageLayer`, `AuditService` | Opaque encrypted document handle; key material never exposed; obtained via factory method |
| `IDocumentLifecycleHook` | `AuditService`, `CDCLayer`, `MaterializedViewEngine` | Receives pre/post event for create, update, delete; all methods are `noexcept` |
| `IDocumentSchemaEvolution` | `SchemaRegistry`, `MigrationService` | Registers schema versions and migration strategies; immutable after `seal()` |
| `IDocumentDiffMerge` | `CollaborationLayer`, `ReplicationLayer` | Computes structural diff and three-way merge for document pairs |

## Planned Features

### Document Lifecycle Hook Interface

- `[ ]` Define `IDocumentLifecycleHook` with `beforeCreate`, `afterCreate`, `beforeUpdate`, `afterUpdate`, `beforeDelete`, `afterDelete` as pure-virtual `noexcept` methods
- `[ ]` Expose `DocumentLifecycleEvent` as a plain-data struct: document ID, collection ID, event type enum, timestamp, and actor identity
- `[ ]` Add `IDocumentManager::registerLifecycleHook(IDocumentLifecycleHook&)` and `unregisterLifecycleHook(IDocumentLifecycleHook&)` to the manager interface
- `[ ]` Document that hook registration and unregistration are thread-safe; hooks in flight during unregistration complete before the call returns

### Encrypted Entity API Extensions

- `[ ]` Define `IEncryptedDocumentEntity::documentId() -> DocumentId` and `collectionId() -> CollectionId` as the only observable properties
- `[ ]` Add `IEncryptedDocumentEntity::reencrypt(KeyRotationDescriptor) -> Result<void>` for key rotation without decrypting to plaintext in the public API
- `[ ]` Expose `KeyRotationDescriptor` as a plain-data struct: old key ID, new key ID, rotation timestamp; no key material fields
- `[ ]` Document that `IEncryptedDocumentEntity` instances are obtained only via `IDocumentManager::createEncrypted()` factory; no public constructors

### Schema-Aware Document Validation

- `[ ]` Define `IDocumentSchemaEvolution::registerVersion(SchemaVersion, SchemaDescriptor) -> Result<void>`
- `[ ]` Expose `SchemaDescriptor` with field definitions (name, type, required flag, default value) as a composable plain-data struct
- `[ ]` Add `IDocumentSchemaEvolution::validate(DocumentId, SchemaVersion) -> Result<ValidationReport>` for point-in-time schema validation
- `[ ]` Define `ValidationReport` value type: list of `FieldViolation` structs with field name, violation kind, and suggested fix

### Document Diff and Merge API

- `[ ]` Define `IDocumentDiffMerge::diff(DocumentId baseId, DocumentId targetId) -> Result<DocumentDiff>`
- `[ ]` Expose `DocumentDiff` as a structured change list: added fields, removed fields, modified fields (old value, new value)
- `[ ]` Add `IDocumentDiffMerge::merge(DocumentId base, DocumentId ours, DocumentId theirs) -> Result<MergeResult>`
- `[ ]` Expose `MergeResult` with merged document ID, conflict list (`std::vector<MergeConflict>`), and merge strategy applied

## Test Strategy

- Lifecycle hook ordering tests assert that `beforeCreate` fires before storage write and `afterCreate` fires after the `Result<DocumentId>` is returned to the caller
- Encrypted entity opacity tests use reflection and memory inspection to verify no key material appears in any `IEncryptedDocumentEntity` return value
- Schema evolution tests register two incompatible schema versions and assert `validate()` returns a `ValidationReport` with the expected `FieldViolation` list
- Diff tests compare known document pairs and assert `DocumentDiff` output matches a golden fixture; empty diffs on identical documents
- Merge conflict tests construct a three-way merge with a known conflict and assert `MergeResult::conflicts` contains exactly the expected `MergeConflict` entries
- `Result<T>` error path tests verify every `IDocumentManager` method returns a typed `Result::error()` for invalid inputs rather than throwing or returning null

## Performance Targets

- `IDocumentManager::create()` and `update()` end-to-end latency ≤ 5 ms including lifecycle hook dispatch at the interface boundary
- `IDocumentLifecycleHook` per-hook dispatch overhead ≤ 500 µs per event (measured with a no-op hook implementation)
- `IDocumentDiffMerge::diff()` computation ≤ 10 ms for a 100 KB document pair with up to 500 field changes
- `IDocumentSchemaEvolution::validate()` latency ≤ 2 ms for a document with up to 200 fields against a registered schema
- `IEncryptedDocumentEntity::reencrypt()` latency ≤ 50 ms per document (key rotation without full decryption)
- `IDocumentDiffMerge::merge()` three-way merge ≤ 20 ms for 100 KB documents with up to 50 conflicts

## Security / Reliability

- Encrypted entity key material is never exposed through any method of `IEncryptedDocumentEntity`; the interface is intentionally opaque with no key accessor
- Document access is validated against the collection-level ACL at the `IDocumentManager` interface boundary before any operation proceeds
- Lifecycle events are audit-logged for every create, update, and delete operation; `IDocumentLifecycleHook::afterDelete` is guaranteed to fire even on storage-level failures
- `IDocumentSchemaEvolution` is sealed after server startup; runtime schema modification is rejected with `Result::error(SchemaSealedError)` to prevent injection of malicious field definitions
- `IDocumentDiffMerge` operates on document IDs, not raw document payloads; the interface never exposes plaintext content of encrypted entities
- `KeyRotationDescriptor` contains only key identifiers, never key material; passing raw key bytes to any `IEncryptedDocumentEntity` method is not possible through the public API
