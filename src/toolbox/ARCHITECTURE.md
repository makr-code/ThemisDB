# ARCHITECTURE

> **Status:** 2026-04-20 – Architecture reflects actual source and headers.

## Overview

The Toolbox module (`src/toolbox/`) is a bridge layer that wires ThemisDB's ingestion
pipeline (`ingestion/`) to other subsystems. It exposes three classes and a
process-global registry with free functions for system-wide use.

## Class Hierarchy

```
themis::toolbox
├── IngestionToolbox            (ingestion_toolbox.h/.cpp)
│   ├── createDefault()         → factory: builds with NullTextGenerationBackend
│   ├── setWorkflowEngine()     → inject ingestion::WorkflowEngine
│   ├── setTextBackend()        → inject ingestion::ITextGenerationBackend
│   ├── workflowEngine()        → accessor
│   ├── stepRegistry()          → accessor (ingestion::StepRegistry&)
│   ├── textBackend()           → accessor
│   ├── extractEntities(text, mime, filename) → std::vector<ingestion::BaseEntity>
│   ├── extractEntitySet(text, mime, filename) → ingestion::BaseEntitySet
│   ├── recordExtraction(entity_count, latency_ms, success)
│   └── getMetricsText()        → Prometheus text (4 families)
│
├── ToolboxBuilder              (toolbox_builder.h/.cpp)
│   ├── withWorkflowProfile()   → add YAML profile path
│   ├── withGraphWriter()       → inject ingestion::IGraphWriter
│   ├── withTextBackend()       → inject ingestion::ITextGenerationBackend
│   ├── withWorkflowEngine()    → inject ingestion::WorkflowEngine
│   ├── withFormatExtractor()   → register ingestion::IFormatExtractor
│   ├── withFormatExtractorFactory() → register all extractors from factory
│   └── build()                 → shared_ptr<IngestionToolbox>
│
├── ContentToolboxBridge        (content_toolbox_bridge.h/.cpp)
│   ├── BridgeResult            → { content_id, child_ids, entities, vectors, ok, error }
│   ├── ingest(data, filename, mime_type, collection, user_context) → BridgeResult
│   ├── enrichExisting(content_id, collection) → BridgeResult
│   ├── toolbox()               → shared_ptr<IngestionToolbox>
│   ├── contentManager()        → shared_ptr<content::ContentManager>
│   ├── graphWriter()           → shared_ptr<ingestion::IGraphWriter>
│   └── vectorWriter()          → shared_ptr<ingestion::IVectorWriter>
│
└── ToolboxRegistry             (toolbox_registry.h/.cpp)  ← NEW
    ├── initialize(toolbox)     → register the global IngestionToolbox
    ├── instance()              → shared_ptr<IngestionToolbox> (throws if uninit)
    ├── isInitialized()         → bool (health-check guard)
    └── reset()                 → clear for test isolation
```

## Global Free Functions (themis::toolbox namespace)

These wrappers delegate to `ToolboxRegistry::instance()` and are the recommended
entry point for modules that do not own their own `IngestionToolbox` instance:

```cpp
void                              initializeToolbox(shared_ptr<IngestionToolbox>);
shared_ptr<IngestionToolbox>      globalToolbox();
vector<ingestion::BaseEntity>     extractEntities(text, mime, filename);
ingestion::BaseEntitySet          extractEntitySet(text, mime, filename);
string                            getMetricsText();
```

## Dual Access Pattern

```
Server bootstrap:             Tests / isolated subsystems:
  initializeToolbox(t);         auto t = make_shared<IngestionToolbox>();
  extractEntities(text);        t->extractEntities(text);
```

Both patterns coexist without interference.

## Dependency Direction

```
toolbox/ → ingestion/    (permitted)
toolbox/ → content/      (permitted — ContentToolboxBridge only)
ingestion/ → toolbox/    (FORBIDDEN)
content/   → toolbox/    (FORBIDDEN)
```

Any module (aql/, rag/, analytics/, …) may depend on `toolbox/`.

## Interfaces

- **External (public API):** `IngestionToolbox`, `ToolboxBuilder`, `ContentToolboxBridge`, `ToolboxRegistry` + free functions via headers in `include/toolbox/`
- **Internal:** pimpl pattern in all four classes

## Nicht-Ziele
- Keine Duplizierung von Logik, die in übergeordneten Core-Modulen verankert ist
- Keine Umgehung zentraler Sicherheits- und Compliance-Vorgaben
