> **Build:** `cmake --preset release && cmake --build build/release --target themisdb`

# Toolbox Module

**Module Path:** `src/toolbox/`
**Namespace:** `themis::toolbox`
**Status:** 🟢 Production-Ready

---

## Module Purpose

The Toolbox module provides a system-wide integration layer that exposes the ingestion
infrastructure (NER, entity extraction, workflow engine) to **all ThemisDB modules**.
It bridges the `ingestion/` pipeline with the `content/` storage layer, and persists
a process-global instance through `ToolboxRegistry` for modules that do not hold
their own injected reference.

---

## Public Classes

| Class | Header | Role |
|-------|--------|------|
| `IngestionToolbox` | `include/toolbox/ingestion_toolbox.h` | System-wide injectable service exposing `WorkflowEngine`, `StepRegistry`, `ITextGenerationBackend`, metrics |
| `ToolboxBuilder` | `include/toolbox/toolbox_builder.h` | Fluent builder for constructing `IngestionToolbox` instances with custom profiles, sinks, and backends |
| `ContentToolboxBridge` | `include/toolbox/content_toolbox_bridge.h` | Unified ingest entry-point combining `ContentManager` (security, storage) with `IngestionToolbox` enrichment |
| `ToolboxRegistry` | `include/toolbox/toolbox_registry.h` | Process-global registry + free functions (`initializeToolbox`, `globalToolbox`, `extractEntities`, `extractEntitySet`, `getMetricsText`) |

---

## Source Files

| File | Class |
|------|-------|
| `ingestion_toolbox.cpp` | `IngestionToolbox` |
| `toolbox_builder.cpp` | `ToolboxBuilder` |
| `content_toolbox_bridge.cpp` | `ContentToolboxBridge` |
| `toolbox_registry.cpp` | `ToolboxRegistry` + free functions |

---

## Usage Patterns

### 1. Global (production, server bootstrap)

```cpp
#include "toolbox/toolbox_registry.h"
#include "toolbox/toolbox_builder.h"

// Bootstrap — once at startup
themis::toolbox::initializeToolbox(
    themis::toolbox::ToolboxBuilder()
        .withWorkflowProfile("/etc/themis/profiles/legal.yaml")
        .withTextBackend(llm_backend)
        .build());

// Any module — no explicit toolbox reference needed
auto entities = themis::toolbox::extractEntities(text, "text/plain", "doc.txt");
auto metrics  = themis::toolbox::getMetricsText();
```

### 2. Injected (tests, isolated subsystems)

```cpp
#include "toolbox/toolbox_builder.h"
#include "toolbox/content_toolbox_bridge.h"

// Build a configured IngestionToolbox explicitly
auto toolbox = themis::toolbox::ToolboxBuilder()
    .withTextBackend(llm_backend)
    .withGraphWriter(graph_sink)
    .build();

// Bridge with ContentManager for combined ingest
auto bridge = std::make_shared<themis::toolbox::ContentToolboxBridge>(
    toolbox, content_manager, graph_writer, vector_writer);

auto result = bridge->ingest(raw_bytes, "document.pdf");
// result.content_id — stored in ContentManager
// result.entities   — NER entities from IngestionToolbox
// result.vectors    — embedding chunks from BaseEntitySet
```

Both patterns coexist.  The global pattern is preferred for production modules;
the injected pattern is preferred for unit tests.

---

## Dependency Direction

```
toolbox/ → ingestion/   (permitted)
toolbox/ → content/     (permitted via ContentToolboxBridge)
ingestion/ → toolbox/   (FORBIDDEN)
content/   → toolbox/   (FORBIDDEN)
aql/       → toolbox/   (permitted)
rag/       → toolbox/   (permitted)
analytics/ → toolbox/   (permitted)
```

---

## See Also

- `ARCHITECTURE.md` — component diagram and dependency rules
- `FUTURE_ENHANCEMENTS.md` — planned metrics and streaming path
- [`../../include/toolbox/README.md`](../../include/toolbox/README.md) — public API documentation

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

