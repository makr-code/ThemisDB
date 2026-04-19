> **Build:** `cmake --preset release && cmake --build build/release`

# Document Module — Public Headers

**Module Path:** `include/document/`
**Implementation:** `../../src/document/`

## Purpose

Public interfaces for ThemisDB's document storage, lifecycle, schema evolution, and encrypted entity handling.

## Header Files

| Header | Primary Class / Interface |
|--------|--------------------------|
| `document_store.h` | `DocumentStore` — primary document CRUD and query interface |
| `document_manager.h` | `DocumentManager` — document lifecycle orchestrator |
| `document_lifecycle.h` | `DocumentLifecycle`, `LifecycleState` — state-machine for document states |
| `document_schema_evolution.h` | `DocumentSchemaEvolution` — schema migration and versioning |
| `document_diff_merge.h` | `DocumentDiffMerge` — three-way diff and conflict resolution |
| `encrypted_entities.h` | `EncryptedEntity`, `FieldEncryption` — field-level encryption support |
| `xdomea_connector.h` | `XdomeaConnector` — XDOMEA records management standard connector |
| `document_manager_deprecated.h` | *(deprecated)* Forwarding shim — use `document_manager.h` instead |

## Build

```cmake
cmake --preset release && cmake --build build/release --target themis-document
```

## See Also

- [`../../src/document/README.md`](../../src/document/README.md) — implementation details

## Installation

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
