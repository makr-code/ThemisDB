---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Toolbox module architecture

**Scope:** Source-backed overview of `src/toolbox` implementation paths. Public declarations in [`include/toolbox/README.md`](../../include/toolbox/README.md) and their header comments remain the contract.

## Main paths

1. **Workflow orchestration:** `IngestionToolbox` builds an `ExtractionContext` and invokes the configured `ingestion::WorkflowEngine`. `extractEntities()` returns nodes; `extractEntitySet()` returns nodes, edges, and chunks. Empty results can represent empty input, no match, or a failure; consult the applicable metrics/logs and workflow configuration.
2. **Construction and bridges:** `ToolboxBuilder` configures the engine, profiles, text backend, format extractors, tensor adapters, and optional writers. `buildWithBridges()` may construct AQL/RAG bridge objects depending on configured sinks. `ContentToolboxBridge` sends raw bytes through `ContentManager`, retrieves assembled text, then optionally enriches and writes graph/vector results. A sink write can fail while the overall bridge result remains successful.
3. **Process-global access and routing:** `ToolboxRegistry` stores an `IngestionToolbox` instance. It is not a tool-function registry: access before initialization throws, and reinitialization replaces the instance. `ToolboxComposite` routes by MIME-prefix in insertion order, followed by its optional fallback.
4. **Text helpers:** `TextChunker` delegates to `rag::DocumentSplitter`; the normalizer, quality scorer, language detector, and fingerprinter provide their respective helper contracts. Fingerprinting is SHA-256 over non-empty input bytes.
5. **Streaming:** `extractEntitiesStream` is a free function. It chunks text and calls the supplied callback for each extracted entity; it is not a `ToolboxStreaming` class or mode.

## Dependencies and limits

- Toolbox orchestration depends on ingestion interfaces and workflow configuration. Header comments document the intended direction (toolbox uses ingestion); current evidence is comments/review, not an automated build check preventing reverse dependencies.
- `DefaultLanguageDetector` uses English/German stopword ratios; it is a heuristic, not general language identification.
- Bridge result records do not prove durable persistence to every optional writer.
- The module exports object-level metric text; it does not create an HTTP metrics endpoint. The host service owns endpoint exposure and scrape configuration.

## Source references

- Public header map: [`../../include/toolbox/ARCHITECTURE.md`](../../include/toolbox/ARCHITECTURE.md).
- Implementation files: `ingestion_toolbox.cpp`, `toolbox_builder.cpp`, `content_toolbox_bridge.cpp`, `toolbox_registry.cpp`, `toolbox_composite.cpp`, `toolbox_streaming.cpp`, and the helper `.cpp` files in this directory.
- Tests and benchmarks: [`../../tests/toolbox/README.md`](../../tests/toolbox/README.md), [`../../benchmarks/toolbox/README.md`](../../benchmarks/toolbox/README.md).

> Exact model backend not verifiable from final document text alone.
