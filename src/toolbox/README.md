> **Build:** `cmake --preset release && cmake --build build/release --target themisdb`

# Toolbox Module

**Module Path:** `src/toolbox/`
**Namespace:** `themis::toolbox`
**Status:** 🟢 Production-Ready

---

## Module Purpose

The Toolbox module provides a system-wide integration layer that exposes the ingestion
infrastructure (NER, entity extraction, workflow engine) to other ThemisDB modules such
as AQL query enrichment and the RAG subsystem. It bridges the `ingestion/` pipeline
with the `content/` storage layer.

---

## Public Classes

| Class | Header | Role |
|-------|--------|------|
| `IngestionToolbox` | `include/toolbox/ingestion_toolbox.h` | System-wide injectable service exposing `WorkflowEngine`, `StepRegistry`, and `ITextGenerationBackend` |
| `ToolboxBuilder` | `include/toolbox/toolbox_builder.h` | Fluent builder for constructing `IngestionToolbox` instances with custom profiles, sinks, and backends |
| `ContentToolboxBridge` | `include/toolbox/content_toolbox_bridge.h` | Unified ingest entry-point combining `ContentManager` (security, storage) with `IngestionToolbox` enrichment |

---

## Source Files

| File | Class |
|------|-------|
| `ingestion_toolbox.cpp` | `IngestionToolbox` |
| `toolbox_builder.cpp` | `ToolboxBuilder` |
| `content_toolbox_bridge.cpp` | `ContentToolboxBridge` |

---

## Quick-Start Example

```cpp
#include "toolbox/toolbox_builder.h"
#include "toolbox/content_toolbox_bridge.h"

// Build a configured IngestionToolbox
auto toolbox = themis::toolbox::ToolboxBuilder()
    .withWorkflowProfile("/etc/themis/profiles/legal.yaml")
    .withTextBackend(llm_backend)
    .withGraphWriter(graph_sink)
    .build();

// Bridge with ContentManager for combined ingest
auto bridge = std::make_shared<themis::toolbox::ContentToolboxBridge>(
    toolbox, content_manager, graph_writer, vector_writer);

auto result = bridge->ingest(raw_bytes, "document.pdf");
// result.content_id — stored in ContentManager
// result.entities  — NER entities from IngestionToolbox
```

---

## Dependency Direction

```
toolbox/ → ingestion/   (permitted)
toolbox/ → content/     (permitted via ContentToolboxBridge)
ingestion/ → toolbox/   (FORBIDDEN)
content/   → toolbox/   (FORBIDDEN)
```

---

## See Also

- `ARCHITECTURE.md` — component diagram and dependency rules
- `FUTURE_ENHANCEMENTS.md` — planned metrics and streaming path
- [`../../include/toolbox/README.md`](../../include/toolbox/README.md) — public API documentation

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/toolbox/README.md`](../../include/toolbox/README.md) for the public API.
