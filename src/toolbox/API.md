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

# Toolbox API contracts

**Purpose and audience:** API-level reference for maintainers and consumers of `include/toolbox`. Public headers remain normative; this page summarizes rather than replaces their declarations and Doxygen comments.

## Primary interfaces

| Interface | Contract and edge behavior |
|---|---|
| [`IngestionToolbox`](../../include/toolbox/ingestion_toolbox.h) | `createDefault()` supplies built-in steps with a null text backend. `extractEntities()` returns entity nodes; `extractEntitySet()` also returns edges and chunks. Empty text and workflow failure both return empty output; use metrics to distinguish failures. |
| [`ToolboxBuilder`](../../include/toolbox/toolbox_builder.h) | Fluent, single-use configuration for engine, profiles, text backend, format extractors, tensor backends, and optional sinks. Null required dependencies and empty profile paths throw `std::invalid_argument`; repeated build throws `std::logic_error`. Profile load failures are non-fatal warnings. |
| [`ToolboxRegistry`](../../include/toolbox/toolbox_registry.h) | Optional process-global access. Initialize before use; `instance()` throws `std::logic_error` when not initialized. A second initialize replaces the instance. `reset()` is intended for test isolation or controlled teardown. |
| [`ContentToolboxBridge`](../../include/toolbox/content_toolbox_bridge.h) | Combines content-manager ingest and toolbox enrichment. Empty raw input and missing required constructor dependencies fail; absent extracted text skips enrichment. Sink write failures are counted and logged but may leave `BridgeResult::ok` true. |
| [`ToolboxComposite`](../../include/toolbox/toolbox_composite.h) | Resolves the first matching MIME-prefix route, then the optional fallback. With no match and no fallback, extraction returns an empty vector. The builder rejects null route toolboxes and cannot build without a route or fallback. |
| [`TextChunker`](../../include/toolbox/text_chunker.h) | Wraps `rag::DocumentSplitter`; empty/whitespace text yields no chunks. Invalid chunk configuration is rejected by the underlying splitter. Instances are not thread-safe. |
| Text helpers | [`TextNormalizer`](../../include/toolbox/text_normalizer.h), [`TextQualityScorer`](../../include/toolbox/text_quality_scorer.h), [`DefaultLanguageDetector`](../../include/toolbox/language_detector.h), and [`ContentFingerprinter`](../../include/toolbox/content_fingerprinter.h) expose deterministic helper contracts described in [`MODELING.md`](./MODELING.md). |
| Streaming | [`toolbox_streaming.h`](../../include/toolbox/toolbox_streaming.h) chunks input and calls the callback for each extracted entity. Empty results do not invoke the callback; a callback exception propagates to the caller. The registry overload requires prior registry initialization. |

## Result and error semantics

- `IngestionToolbox` uses empty output values for empty input and workflow failures; extraction metrics record calls, errors, empty results, counts, and coarse latency buckets.
- `ContentToolboxBridge::BridgeResult::ok` reports the bridge-level operation outcome, not guaranteed persistence to every optional sink. `sinks_written` is true when at least one write succeeds.
- Bridge failure, graph-write failure, and vector-write failure have separate counters. A caller that requires durable graph/vector writes must inspect the relevant counter or add a stronger application-level result contract.
- `ToolboxRegistry::initialize(nullptr)` and invalid required builder inputs throw `std::invalid_argument`; use before initialization throws `std::logic_error`.

## Thread-safety and ownership

- `IngestionToolbox` and its workflow execution are intended for concurrent use; configure dependencies before requests begin.
- `ToolboxBuilder` is single-threaded and single-use.
- Registry `initialize()` and `reset()` belong to startup/teardown; do not reset during active work.
- `ContentToolboxBridge::ingest()` synchronizes content-manager access and calls toolbox extraction outside that lock.
- `TextChunker` instances require external synchronization if shared; stateless normalizer, scorer, detector, and fingerprinter helpers can be shared.

## Integration guidance

Prefer injected `IngestionToolbox` instances when ownership is local or test isolation matters. Use the global registry only where process-wide lifecycle is explicitly managed. Use `extractEntitySet()` when downstream code needs vector chunks or edges; `extractEntities()` intentionally returns nodes only.

See [`MODULE_GUIDE.md`](./MODULE_GUIDE.md) for assembly examples and [`IMPLEMENTATION.md`](./IMPLEMENTATION.md) for lifecycle details.

> Exact model backend not verifiable from final document text alone.
