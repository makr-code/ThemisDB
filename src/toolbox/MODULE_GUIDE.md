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

# Toolbox module guide

**Purpose and audience:** Onboarding guide for developers wiring extraction, bridge, or text-helper behavior into another ThemisDB subsystem.

## Choose the integration surface

- Use `IngestionToolbox` directly when a component owns its toolbox instance or needs only extraction.
- Use `ToolboxBuilder` when profiles, a real text backend, format extractors, tensor adapters, or graph/vector sinks must be configured.
- Use `ToolboxRegistry` only when consumers intentionally share a process-wide instance. Initialize it during server bootstrap before use.
- Use `ContentToolboxBridge` when raw bytes must pass through `ContentManager` before toolbox enrichment.
- Use `TextChunker`, `TextNormalizer`, `TextQualityScorer`, `DefaultLanguageDetector`, and `ContentFingerprinter` for standalone helper operations.

## Build and inject a toolbox

```cpp
auto toolbox = themis::toolbox::ToolboxBuilder()
    .withWorkflowProfile("/etc/themis/profiles/legal.yaml")
    .withTextBackend(text_backend)
    .build();

themis::toolbox::ToolboxRegistry::initialize(toolbox);
```

`text_backend` must implement `ingestion::ITextGenerationBackend`. If no backend is supplied, the default null backend remains active. Workflow-profile load errors are logged as warnings and do not make `build()` fail; check logs and verify the loaded workflow before relying on its steps.

To create downstream bridges, supply the relevant sinks and call `buildWithBridges()`:

```cpp
auto built = themis::toolbox::ToolboxBuilder()
    .withGraphWriter(graph_writer)
    .withVectorWriter(vector_writer)
    .buildWithBridges();
```

The resulting `BuiltToolbox` always contains the toolbox when build succeeds. The AQL bridge is created only when a graph writer is configured; the RAG bridge is attempted when either graph or vector writer is configured. A bridge-construction exception is logged and leaves that bridge pointer empty.

## Call extraction with the required result shape

```cpp
auto entities = toolbox->extractEntities(text, "text/plain", "input.txt");
auto complete = toolbox->extractEntitySet(text, "text/plain", "input.txt");
```

Choose `extractEntitySet()` when callers need relation edges or vector chunks. Both calls return empty results for empty text, and workflow failures also produce empty outputs; inspect metrics rather than interpreting every empty result as success.

For global access, `ToolboxRegistry::instance()` throws if startup did not initialize the registry. Do not hide this lifecycle error with a fallback toolbox.

## Input and ownership cautions

- Pass UTF-8 text where a text helper or extraction method expects text. `ContentToolboxBridge::ingest()` instead accepts raw bytes and delegates content detection/storage to `ContentManager`.
- A successful bridge result does not guarantee every optional sink write succeeded. Check separate graph/vector failure counters.
- `enrichExisting()` does not repeat raw-content security scanning or deduplication and may repeat sink writes; use only where the caller's idempotency policy permits.
- `TextChunker` is not safe for concurrent calls through one instance; use per-thread instances or synchronization.
- A `ToolboxBuilder` is single-use and not thread-safe. Construct and build it in one thread.

## Tests and references

- Start with `ctest --preset linux-release -R test_toolbox_primitives_toolbox_FocusedTests --output-on-failure` for helper behavior.
- Run `ctest --preset linux-release -R "test_toolbox_(contract_hardening_focused|ingestion)_toolbox_FocusedTests" --output-on-failure` for lifecycle and bridge integration.
- See [`API.md`](./API.md), [`IMPLEMENTATION.md`](./IMPLEMENTATION.md), [`EMBEDDING.md`](./EMBEDDING.md), and [`tests/toolbox/README.md`](../../tests/toolbox/README.md).

> Exact model backend not verifiable from final document text alone.
