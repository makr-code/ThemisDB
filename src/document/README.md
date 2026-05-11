> **Build:** `cmake --preset linux-release && cmake --build build/linux-release`

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: ../../include/document/README.md · ../ROADMAP.md · ../FUTURE_ENHANCEMENTS.md -->

# document module

Implementation overview for the `document` module.

## Current Implementation Layout

`document` is currently implemented as a header-first module in `include/document/`; there are no dedicated `src/document/*.cpp` units at this time.

## Main Components

| Component | Implementation Location | Runtime Behavior |
|----------|--------------------------|------------------|
| Store layer | `include/document/document_store.h` | Thread-safe in-memory CRUD with `Result<T>` error propagation |
| Manager layer | `include/document/document_manager.h` | Lifecycle dispatch + encrypted handle factory on top of store |
| Lifecycle hooks | `include/document/document_lifecycle.h` | Synchronous before/after create/update/delete callbacks |
| Schema evolution | `include/document/document_schema_evolution.h` | Version registry, seal mechanism, validation reports |
| Diff / merge | `include/document/document_diff_merge.h` | Store-backed field diff + three-way merge strategies |
| XDOMEA connector | `include/document/xdomea_connector.h` | XDOMEA import/export and in-memory repository operations |
| Encrypted entities | `include/document/encrypted_entities.h` | JSON-serialisable encrypted sample entities |

## Runtime Behavior, Errors, and Limits

- Core APIs use `Result<T>` and avoid exception-based public error signaling, except legacy XDOMEA store methods that throw on invalid/duplicate IDs.
- In-memory implementations are process-local and non-persistent.
- Merge operations can fail with explicit conflict errors when `MergeStrategy::FAIL` is selected.
- Schema validation reports missing required fields and JSON type mismatches.

## Installation

This module is header-first and built as part of the main ThemisDB build:

```bash
cmake --preset linux-release
cmake --build build/linux-release
```

## Usage Snippets

```cpp
#include "document/document_schema_evolution.h"

using namespace themis::document;

InMemoryDocumentSchemaEvolution schema;
schema.registerVersion(1, SchemaDescriptor{{{"title", SchemaFieldType::STRING, true, {}}}});
schema.seal();

auto report = schema.validate("doc-1", nlohmann::json{{"title", "memo"}}, 1);
```

## Troubleshooting

- Validation fails with `ERR_DOC_SCHEMA_SEALED`: create/register all versions before calling `seal()`.
- Diff/merge returns not found: ensure all referenced base/branch document IDs exist in the same collection.
- Lifecycle hooks not called: confirm hook object is registered and remains alive while manager operations run.

## Related Docs

- Public headers: [`../../include/document/README.md`](../../include/document/README.md)
- German module overview: [`../../docs/de/document/README.md`](../../docs/de/document/README.md)
- Primary sources (DE): [`../../docs/de/document/PRIMARY_SOURCES.md`](../../docs/de/document/PRIMARY_SOURCES.md)
- Primary sources (EN): [`../../docs/en/document/PRIMARY_SOURCES.md`](../../docs/en/document/PRIMARY_SOURCES.md)
- Cross-module roadmap: [`../ROADMAP.md`](../ROADMAP.md)
- Cross-module future enhancements: [`../FUTURE_ENHANCEMENTS.md`](../FUTURE_ENHANCEMENTS.md)
