> **Build:** `cmake --preset linux-release && cmake --build build/linux-release`

# Document Module — Public Headers

**Module Path:** `include/document/`
**Implementation Overview:** `../../src/document/README.md`

## Purpose

Public interfaces and in-memory reference implementations for document storage, lifecycle hooks, schema evolution, diff/merge, encrypted entity helpers, and XDOMEA connectivity.

## Header Entry-Points

| Header | Primary API | Runtime Role |
|--------|-------------|--------------|
| `document_store.h` | `IDocumentStore`, `InMemoryDocumentStore` | CRUD backend abstraction with thread-safe in-memory key/value store |
| `document_manager.h` | `IDocumentManager`, `InMemoryDocumentManager`, `IEncryptedDocumentEntity` | High-level CRUD orchestration, lifecycle hook dispatch, encrypted handle factory |
| `document_lifecycle.h` | `IDocumentLifecycleHook`, `DocumentLifecycleEvent` | Observer contract for create/update/delete phases |
| `document_schema_evolution.h` | `IDocumentSchemaEvolution`, `InMemoryDocumentSchemaEvolution` | Versioned schema registration, sealing, validation reports |
| `document_diff_merge.h` | `IDocumentDiffMerge`, `InMemoryDocumentDiffMerge` | Field-level diff and three-way merge on store-backed JSON docs |
| `round_trip_editor.h` | `IRoundTripEditor`, `StoreBackedRoundTripEditor` | Store-backed persistence for DELEGATE-52 round-trip seed/intermediate snapshots |
| `xdomea_connector.h` | `IXDOMEAConnector`, `InMemoryXDOMEAConnector` | XDOMEA XML import/export and document repository operations |
| `encrypted_entities.h` | `User`, `Customer`, `SecureDocument` | Example encrypted entity payloads using `EncryptedField<T>` |
| `document_manager_deprecated.h` | Deprecated alias shim | Legacy forwarding to `projects/DocumentManager` |

## Public API Behavior

### Document CRUD (`IDocumentStore`, `IDocumentManager`)

- Empty document IDs are rejected (`ERR_DOC_INVALID_ID`).
- Duplicate create/put operations return `ERR_DOC_ALREADY_EXISTS`.
- Missing reads return success with `std::nullopt`.
- Missing updates return `ERR_DOC_NOT_FOUND`.
- `remove()` is idempotent (success even when entry is absent).

### Lifecycle Hooks (`IDocumentLifecycleHook`)

- Hooks are dispatched synchronously in registration order.
- `afterDelete` is fired even if storage removal fails.
- Hook registration/unregistration is thread-safe.

### Schema Evolution (`IDocumentSchemaEvolution`)

- Versions are immutable once registered.
- `seal()` blocks future registrations (`ERR_DOC_SCHEMA_SEALED`).
- Validation returns structured field violations (`MISSING_REQUIRED_FIELD`, `TYPE_MISMATCH`).

### Diff/Merge (`IDocumentDiffMerge`)

- Diff/merge is document-ID based (store-backed resolution).
- Missing IDs return `ERR_DOC_DIFF_NOT_FOUND`.
- `MergeStrategy::FAIL` returns `ERR_DOC_MERGE_CONFLICT` when conflicts exist.

### XDOMEA Connector (`IXDOMEAConnector`)

- Supports version flags `V2_1` and `V3_0`.
- Supports message types 0201, 0202, 0203, 0401, 0501, 0601.
- `storeDocument()` throws on empty IDs or duplicate IDs.
- In-memory connector uses lightweight tag scanning (no XSD validation).

## Configuration and Limits

- No module-global runtime config file is required; behavior is controlled via API parameters and enum strategy choices.
- In-memory implementations are intended for tests/dev and are non-persistent.
- `InMemoryEncryptedEntity` tracks key rotation IDs but does not re-cipher persisted bytes.
- `InMemoryXDOMEAConnector` is not a full XML validator; production systems should use XSD-validating connectors.

## Installation

Headers are included with ThemisDB. Ensure your target includes the project include directory:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

```cpp
#include "document/document_manager.h"

using namespace themis::document;

InMemoryDocumentManager manager;
manager.create("cases", "doc-1", nlohmann::json{{"title", "Akte 42"}});

auto doc = manager.get("cases", "doc-1");
if (doc && doc->has_value()) {
    manager.update("cases", "doc-1", nlohmann::json{{"title", "Akte 42 (updated)"}});
}
```

```cpp
#include "document/xdomea_connector.h"

themis::document::InMemoryXDOMEAConnector connector;
auto import = connector.importFromXML("<dokument><id>abc</id></dokument>",
                                      themis::document::XDOMEAVersion::V3_0);
```

## Troubleshooting

- `ERR_DOC_INVALID_ID`: ensure document IDs are non-empty.
- `ERR_DOC_SCHEMA_VERSION_NOT_FOUND`: register the schema version before validation.
- `ERR_DOC_MERGE_CONFLICT`: retry with `OURS_WINS` / `THEIRS_WINS` if conflict-tolerant merge is desired.
- Empty XDOMEA import result: verify XML contains properly closed `<dokument>` or `<akte>` tags.

## Related Docs

- Implementation overview: [`../../src/document/README.md`](../../src/document/README.md)
- Module index (DE): [`../../docs/de/document/README.md`](../../docs/de/document/README.md)
- Primary sources (DE): [`../../docs/de/document/PRIMARY_SOURCES.md`](../../docs/de/document/PRIMARY_SOURCES.md)
- Primary sources (EN): [`../../docs/en/document/PRIMARY_SOURCES.md`](../../docs/en/document/PRIMARY_SOURCES.md)
- Cross-module roadmap: [`../../src/ROADMAP.md`](../../src/ROADMAP.md)
- Cross-module future enhancements: [`../../src/FUTURE_ENHANCEMENTS.md`](../../src/FUTURE_ENHANCEMENTS.md)
