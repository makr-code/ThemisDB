> **Build:** `cmake --preset release && cmake --build build/release`

# Toolbox Module Headers

Public API for the ThemisDB `toolbox` module (`themis::toolbox` namespace).

## Module Purpose

`toolbox` provides reusable ingestion-facing primitives and bridge components for:

- extraction/enrichment orchestration (`IngestionToolbox`, `ToolboxBuilder`)
- content + enrichment integration (`ContentToolboxBridge`)
- global process wiring (`ToolboxRegistry` + free functions)
- text-processing helpers (chunking, normalization, language/quality signals, fingerprinting)
- MIME-routed and streaming extraction entry points

## Header Surface (Public API)

| Header | Main API | Purpose |
|---|---|---|
| `ingestion_toolbox.h` | `IngestionToolbox` | Thread-safe extraction facade (`extractEntities`, `extractEntitySet`, metrics export). |
| `toolbox_builder.h` | `ToolboxBuilder`, `BuiltToolbox` | Fluent configuration of workflows/backends/sinks and optional auto-wired AQL/RAG bridges. |
| `content_toolbox_bridge.h` | `ContentToolboxBridge` | Unified ingest path combining `ContentManager` storage/security with toolbox enrichment. |
| `toolbox_registry.h` | `ToolboxRegistry`, `initializeToolbox`, `globalToolbox`, `extractEntities`, `extractEntitySet`, `getMetricsText` | Controlled process-global toolbox access. |
| `text_chunker.h` | `TextChunker`, `chunkText` | Token-based chunking facade over `rag::DocumentSplitter`. |
| `text_normalizer.h` | `TextNormalizer`, `normalizeText` | Unicode/umlaut normalization helper facade. |
| `content_fingerprinter.h` | `ContentFingerprinter`, `ContentFingerprint`, `fingerprint` | Canonical SHA-256 content fingerprint contract for deduplication. |
| `text_quality_scorer.h` | `TextQualityScorer`, `TextQualityScore`, `scoreText` | Lightweight quality gate before expensive extraction/LLM steps. |
| `language_detector.h` | `ILanguageDetector`, `DefaultLanguageDetector`, `detectLanguage` | ISO-639-1 language detection interface + default stopword heuristic. |
| `toolbox_composite.h` | `ToolboxComposite`, `ToolboxCompositeBuilder` | MIME-prefix routing across multiple `IngestionToolbox` instances. |
| `toolbox_streaming.h` | `extractEntitiesStream` | Callback-based, chunk-wise streaming extraction API. |

## Configuration Options (Public API)

Primary public configuration knobs:

| API | Key Options |
|---|---|
| `ToolboxBuilder` | `withWorkflowProfile`, `withWorkflowEngine`, `withTextBackend`, `withGraphWriter`, `withVectorWriter`, `withFormatExtractor`, `withFormatExtractorFactory`, `withTensorDecompositionBackend`, `withTensorCoreSink` |
| `TextChunker` | `DocumentSplitterConfig`: `chunk_size`, `overlap`, `strategy` |
| `DefaultLanguageDetector` | constructor threshold `min_ratio` (stopword confidence floor) |
| `extractEntitiesStream` | callback contract + per-call MIME/filename hints |

## Runtime Behavior, Failure Cases, Limits

- `IngestionToolbox::extractEntities*` returns empty outputs for empty text; failures are represented as empty results and reflected in metrics.
- `ToolboxBuilder::build()` / `buildWithBridges()` are single-use per builder instance (`std::logic_error` on repeated build calls).
- `ContentToolboxBridge` constructor is fail-fast (`std::invalid_argument`) for null required dependencies.
- `ContentToolboxBridge::ingest()` returns `ok=true` with empty entities/vectors if no extracted text is available (binary-only content path).
- `ToolboxRegistry::instance()` and global free functions throw `std::logic_error` until initialization has happened.
- `ToolboxComposite` route matching is prefix-based in insertion order; if no route and no fallback exist, extraction returns empty output.
- `DefaultLanguageDetector` currently targets English/German stopword heuristics and may return `"und"` for short or mixed text.

## Usage Snippets

```cpp
#include "toolbox/toolbox_builder.h"
#include "toolbox/toolbox_registry.h"

auto built = themis::toolbox::ToolboxBuilder()
    .withWorkflowProfile("/etc/themis/workflows/default.yaml")
    .withTextBackend(llm_backend)
    .withGraphWriter(graph_sink)
    .buildWithBridges();

themis::toolbox::initializeToolbox(built.toolbox);
auto entities = themis::toolbox::extractEntities("Hello world", "text/plain", "sample.txt");
```

```cpp
#include "toolbox/toolbox_streaming.h"

themis::toolbox::extractEntitiesStream(
    "Long text ...",
    "text/plain",
    "doc.txt",
    [](const themis::ingestion::BaseEntity& entity) {
        // consume partial results
    });
```

## Troubleshooting

| Symptom | Likely Cause | Recommended Check |
|---|---|---|
| `ToolboxRegistry::instance` throws | Global toolbox not initialized | Ensure `initializeToolbox(...)` is called during bootstrap before consumers run |
| Builder throws `build() called more than once` | Reusing a consumed builder | Create a fresh `ToolboxBuilder` per build cycle |
| `ContentToolboxBridge` returns `ok=true` but no entities | Content extraction produced no text or extraction found no entities | Inspect assembled text from `ContentManager`; verify MIME/profile alignment |
| `extractEntitiesStream` emits no callbacks | Empty input text or callback not set | Validate non-empty source text and callback wiring |
| Language detection returns `"und"` | Input too short, mixed, or outside EN/DE heuristics | Provide longer plain-language text or custom `ILanguageDetector` implementation |

## Related Documentation

- [Implementation Overview (`src/toolbox/README.md`)](../../src/toolbox/README.md)
- [Architecture (`src/toolbox/ARCHITECTURE.md`)](../../src/toolbox/ARCHITECTURE.md)
- [Roadmap (`src/toolbox/ROADMAP.md`)](../../src/toolbox/ROADMAP.md)
- [Future Enhancements (`src/toolbox/FUTURE_ENHANCEMENTS.md`)](../../src/toolbox/FUTURE_ENHANCEMENTS.md)
- [Security Notes (`src/toolbox/SECURITY.md`)](../../src/toolbox/SECURITY.md)
- [Secondary Docs EN (`docs/en/toolbox/index.md`)](../../docs/en/toolbox/index.md)
- [Secondary Docs DE (`docs/de/toolbox/index.md`)](../../docs/de/toolbox/index.md)

## Implementation

See `../../src/toolbox/` for the implementation code and module-internal architecture notes.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
