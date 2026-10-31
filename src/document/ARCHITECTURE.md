# Architecture — Document Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Composition

The module is header-first. Its public contracts are in `include/document/`; the only module-local `.cpp` implementation is `src/document/round_trip_editor.cpp`. Reference implementations use in-memory state. `IDocumentStore` is an abstraction that callers may implement; this module does not bind it to a particular durable storage engine.

## Runtime planes

| Plane | Implementation evidence | Behavior |
|---|---|---|
| Store and manager | `document_store.h`, `document_manager.h` | Collection/ID operations; in-memory reference store is mutex-protected; manager dispatches lifecycle hooks |
| Lifecycle and schema | `document_lifecycle.h`, `document_schema_evolution.h` | Synchronous hook callbacks; schema registration, sealing, and validation against field descriptors |
| Diff and merge | `document_diff_merge.h` | Loads documents through `IDocumentStore`; reports field changes and supports `OURS_WINS`, `THEIRS_WINS`, and `FAIL` |
| Exchange | `xdomea_connector.h` | XDOMEA import/export and repository operations; in-memory XML handling is not XSD validation |
| Round-trip snapshots | `round_trip_editor.h`, `round_trip_editor.cpp` | Stores relay/index records through `IDocumentStore`; propagates store errors |
| Diagnostics and entity helpers | `document_diagnostics.h`, `encrypted_entities.h`, `document_manager.h` | Classifies/formats error codes; in-memory re-encryption records a key ID but does not transform stored bytes |

## Error and concurrency boundaries

- Store/manager, schema, diff/merge, and round-trip APIs expose `Result` return values; absent reads are represented as `std::optional`.
- The in-memory store and schema registry protect their own state with mutexes. The interface does not prescribe thread-safety for third-party `IDocumentStore` implementations.
- Lifecycle hooks are invoked synchronously in registration order. Hook methods are declared `noexcept`; `afterDelete` is dispatched even when store removal reports failure.
- `InMemoryDocumentDiffMerge` reports missing documents and merge conflicts through errors. Non-object JSON payloads do not receive field-level diff/merge processing.

## Integration boundary

`StoreBackedRoundTripEditor` depends on `IDocumentStore`, and `InMemoryDocumentDiffMerge` reads through the same interface. The headers depend on shared utility/error types and, for encrypted entity helpers, the security encryption types. No direct metadata-module schema registry, persistent storage adapter, server consumer, or stable-ABI guarantee is established by the module sources reviewed here. TODO: record those relationships only after confirming them in their owning canonical sources.

## Verification basis

Reviewed the public headers, round-trip implementation, focused test sources, and CMake definitions on 2026-10-31. This page describes source-level structure, not a claim that the full module or external integrations were built or executed.