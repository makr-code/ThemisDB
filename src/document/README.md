# ThemisDB Document Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · SECURITY.md -->

## Purpose and implementation boundary

The document module defines C++ interfaces and in-memory reference implementations for document storage and management, lifecycle hooks, schema validation, field-level diff/three-way merge, encrypted-entity helpers, XDOMEA exchange, and round-trip snapshots. The public headers are predominantly inline; `src/document/round_trip_editor.cpp` supplies the store-backed round-trip implementation. The presence of `IDocumentStore` does not itself provide a durable backend.

## Main interfaces

| Source | Responsibility |
|---|---|
| `include/document/document_store.h` | `IDocumentStore` contract and mutex-protected `InMemoryDocumentStore` |
| `include/document/document_manager.h` | `IDocumentManager`, in-memory CRUD orchestration, lifecycle dispatch, and encrypted-entity factory |
| `include/document/document_lifecycle.h` | `IDocumentLifecycleHook` and CRUD lifecycle events |
| `include/document/document_schema_evolution.h` | In-memory schema-version registry, sealing, and field validation |
| `include/document/document_diff_merge.h` | Store-backed field diff and three-way merge strategies |
| `include/document/document_diagnostics.h` | Error classification/formatting and in-memory diagnostic counts |
| `include/document/xdomea_connector.h` | XDOMEA connector contract and in-memory XML/repository implementation |
| `include/document/round_trip_editor.h`, `src/document/round_trip_editor.cpp` | Store-backed relay snapshots and interaction lookup |
| `include/document/encrypted_entities.h` | JSON helpers for entities containing `EncryptedField<T>` values |

## Behavior and limits

- Store and manager operations use `Result<T>` and optional results for reads; the XDOMEA in-memory connector also has methods that throw for invalid or duplicate stored IDs.
- `InMemoryDocumentStore`, `InMemoryDocumentManager`, the schema reference implementation, and `InMemoryXDOMEAConnector` are in-memory implementations, not durable production backends.
- Schema validation checks the registered field descriptors; round-trip persistence stores a JSON record and does not invoke schema validation itself.
- `MergeStrategy::FAIL` returns a conflict error; the other merge strategies select the configured branch's value for conflicts.
- Snapshot IDs are formed from the relay ID and interaction index. Store errors are propagated by the round-trip implementation.
- `InMemoryEncryptedEntity::reencrypt()` records a key ID only; it does not re-cipher document bytes.
- `InMemoryDocumentManager::createEncrypted()` stores the supplied JSON body in its in-memory store and returns an in-memory handle; it does not encrypt that body.
- The in-memory XDOMEA parser is not an XSD validator. See [SECURITY.md](SECURITY.md) for security boundaries.

## Build and test entry points

The repository build is configured from the root; the public-header guide gives the documented `linux-release` build command. `tests/document/CMakeLists.txt` matches the six direct `test_document_*.cpp` sources and registers focused unit-test targets (120-second timeout); the high-cardinality test receives additional stress labels. `benchmarks/document/CMakeLists.txt` conditionally registers four benchmark executables. These are build/test registration facts, not evidence that a build or test run passed.

## Verification basis

Reviewed public headers, `src/document/round_trip_editor.cpp`, `tests/document/*.cpp`, `tests/document/CMakeLists.txt`, and `benchmarks/document/` plus its CMake registration on 2026-10-31. This was source/configuration inspection; no test or benchmark execution is claimed here. Current planning is in [ROADMAP.md](ROADMAP.md); prior snapshots are historical.