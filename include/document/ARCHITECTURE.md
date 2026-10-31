> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/document/ARCHITECTURE.md -->

# Document Module — Public Header Architecture

`include/document/` contains the document contracts and reference implementations. The namespace for these APIs is `themis::document` (the entity examples in `encrypted_entities.h` are in `themis`). Runtime composition is summarized in the [implementation architecture](../../src/document/ARCHITECTURE.md).

## Header groups

| Group | Headers | Source-backed purpose |
|---|---|---|
| Store and manager | `document_store.h`, `document_manager.h`, `document_manager_deprecated.h` | Store/manager interfaces, in-memory implementations, and a deprecated compatibility shim |
| Lifecycle and schema | `document_lifecycle.h`, `document_schema_evolution.h` | Synchronous CRUD hooks and in-memory schema registration/sealing/field validation |
| Diff and diagnostics | `document_diff_merge.h`, `document_diagnostics.h` | Field-level diff, three-way merge, error classification, and diagnostic counters |
| Round-trip and exchange | `round_trip_editor.h`, `xdomea_connector.h` | Store-backed snapshot contract and XDOMEA connector contract/reference implementation |
| Entity helpers | `encrypted_entities.h` | JSON helpers for entities using `EncryptedField<T>` |

## Contract boundaries

- `IDocumentStore` is a backend interface, not a persistent store implementation. The included `InMemoryDocumentStore` is process-local.
- The public API includes reference implementations; no stable ABI or supported-platform matrix is declared by these headers.
- `InMemoryDocumentSchemaEvolution` validates descriptors; it does not implement a general migration engine.
- `InMemoryEncryptedEntity` tracks a new key ID but has no encrypted bytes to re-cipher.
- The in-memory XDOMEA implementation does not validate XML against an XSD.
- TODO: confirm any module-layer classification and public compatibility policy in their owning canonical governance sources before documenting them here.

For versioning policy see [`../../VERSIONING.md`](../../VERSIONING.md).
