# ARCHITECTURE

> **Status:** 2026-04-19 – Architecture reflects actual source and headers.

## Overview

The Toolbox module (`src/toolbox/`) is a bridge layer that wires ThemisDB's ingestion
pipeline (`ingestion/`) to other subsystems. It provides three classes:

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
│   └── extractEntities(text, mime, filename) → std::vector<ingestion::BaseEntity>
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
└── ContentToolboxBridge        (content_toolbox_bridge.h/.cpp)
    ├── BridgeResult            → { content_id, child_ids, entities, vectors, ok, error }
    ├── ingest(data, filename, mime_type, collection, user_context) → BridgeResult
    ├── enrichExisting(content_id, collection) → BridgeResult
    ├── toolbox()               → shared_ptr<IngestionToolbox>
    ├── contentManager()        → shared_ptr<content::ContentManager>
    ├── graphWriter()           → shared_ptr<ingestion::IGraphWriter>
    └── vectorWriter()          → shared_ptr<ingestion::IVectorWriter>
```

## Dependency Direction

```
toolbox/ → ingestion/    (permitted)
toolbox/ → content/      (permitted — ContentToolboxBridge only)
ingestion/ → toolbox/    (FORBIDDEN)
content/   → toolbox/    (FORBIDDEN)
```

## Interfaces

- **External (public API):** `IngestionToolbox`, `ToolboxBuilder`, `ContentToolboxBridge` via headers in `include/toolbox/`
- **Internal:** `IngestionToolbox::Impl`, `ToolboxBuilder::Impl`, `ContentToolboxBridge::Impl` (pimpl pattern)

## Nicht-Ziele
- Keine Duplizierung von Logik, die in übergeordneten Core-Modulen verankert ist
- Keine Umgehung zentraler Sicherheits- und Compliance-Vorgaben
