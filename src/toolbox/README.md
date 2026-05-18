> **Build:** `cmake --preset release && cmake --build build/release --target themisdb`

# Toolbox Module

**Module Path:** `src/toolbox/`
**Namespace:** `themis::toolbox`
**Status:** 🟢 Production-Ready

---

## Module Purpose

The Toolbox implementation module provides the runtime wiring between:

- `ingestion/` workflow execution and extraction facilities
- `content/` ingestion/storage pipeline integration
- module-facing helper APIs for chunking, normalization, quality checks, language detection, and fingerprinting
- optional global registry access and streaming/composite extraction paths

---

## Public API Entry Points

The implementation in `src/toolbox/` backs the public headers in
[`../../include/toolbox/README.md`](../../include/toolbox/README.md), including:

- core extraction/orchestration: `ingestion_toolbox.h`, `toolbox_builder.h`, `content_toolbox_bridge.h`, `toolbox_registry.h`
- primitives: `text_chunker.h`, `text_normalizer.h`, `content_fingerprinter.h`, `text_quality_scorer.h`, `language_detector.h`
- orchestration helpers: `toolbox_composite.h`, `toolbox_streaming.h`

---

## Main Implementation Components (`src/toolbox`)

| File | Responsibility | Notable Runtime Behavior |
|---|---|---|
| `ingestion_toolbox.cpp` | Implements `IngestionToolbox` + metrics counters | Empty input returns immediately; extraction failures map to empty results and increment error counters |
| `toolbox_builder.cpp` | Implements `ToolboxBuilder` and `buildWithBridges()` | Builder is single-use; invalid/null injections throw; profile load failures warn and continue |
| `content_toolbox_bridge.cpp` | Implements combined content ingest + enrichment flow | Returns `ok=true` for no-text content path; sink write failures are logged and do not hard-fail ingest |
| `toolbox_registry.cpp` | Implements process-global toolbox registry and free-function wrappers | `instance()` throws until initialized; reset is intended for test isolation |
| `text_chunker.cpp` | Chunking facade over `rag::DocumentSplitter` | Uses configurable chunk size/overlap and sentence strategy via convenience API |
| `text_normalizer.cpp` | Umlaut/Unicode normalization facade | Stateless utility wrapper over `utils::Normalizer` |
| `content_fingerprinter.cpp` | SHA-256 content fingerprint + token estimate | Uses OpenSSL SHA-256; empty input yields empty digest |
| `text_quality_scorer.cpp` | Quality gate heuristics (`TextQualityScore`) | Flags boilerplate based on token sparsity/repetition/word-length heuristics |
| `language_detector.cpp` | Stopword-based language detection | Defaults to EN/DE heuristics with `"und"` fallback |
| `toolbox_composite.cpp` | MIME-prefix routing across toolboxes | First-match route in insertion order; fallback optional |
| `toolbox_streaming.cpp` | Chunk-wise callback streaming extraction | Synchronous loop; no callback calls for empty text |

---

## Configuration Options (Implementation-Relevant)

Primary configuration paths:

- `ToolboxBuilder`:
  - `withWorkflowProfile(...)`
  - `withWorkflowEngine(...)`
  - `withTextBackend(...)`
  - `withGraphWriter(...)`
  - `withVectorWriter(...)`
  - `withFormatExtractor(...)`
  - `withFormatExtractorFactory(...)`
  - `withTensorDecompositionBackend(...)`
  - `withTensorCoreSink(...)`
- `TextChunker::setConfig(...)` (`chunk_size`, `overlap`, strategy)
- `DefaultLanguageDetector(double min_ratio)`

## Runtime Flow

### 1. Content + Toolbox bridge flow (`ContentToolboxBridge::ingest`)

1. `ContentManager::ingestRawBlob(...)` stores/scans/extracts content.
2. Extracted text is assembled (`assembleContent(..., include_text=true)`).
3. `IngestionToolbox::extractEntitySet(...)` performs enrichment.
4. Optional graph/vector sinks receive entities/chunks.
5. `BridgeResult` returns IDs, entities, vectors, status, and optional error text.

### 2. Toolbox-only extraction flow (`IngestionToolbox`)

1. Build extraction context (`mime`, `filename`, `raw_text`).
2. Run `WorkflowEngine::execute(...)`.
3. Return nodes (`extractEntities`) or full set (`extractEntitySet`).
4. Record Prometheus-style counters via `recordExtraction(...)`.

## Runtime Errors, Failure Cases, and Limits

- Constructor guards:
  - `ContentToolboxBridge` throws `std::invalid_argument` for null required dependencies.
  - `ToolboxBuilder` throws `std::invalid_argument` for invalid inputs (e.g., empty profile path, null required injected objects).
- Lifecycle guards:
  - `ToolboxBuilder::build*()` throws `std::logic_error` on second invocation.
  - `ToolboxRegistry::instance()` throws `std::logic_error` before initialization.
- Soft-failure behavior:
  - profile load failures are logged and remaining profiles continue loading.
  - graph/vector sink write failures are logged; `BridgeResult.ok` can still be `true`.
  - extraction functions return empty outputs on workflow failure.
- Documented limits:
  - streaming API is synchronous and single-threaded.
  - default language detection is heuristic (EN/DE + `"und"` fallback).
  - zero-entity outcomes for short text are treated as non-error in extraction metrics.

## Usage Patterns

### 1. Global (production bootstrap)

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

### 3. Streaming extraction for large text

```cpp
#include "toolbox/toolbox_streaming.h"

themis::toolbox::extractEntitiesStream(
    *toolbox,
    long_text,
    "text/plain",
    "bulk.txt",
    [](const themis::ingestion::BaseEntity& e) {
        // handle entities incrementally
    });
```

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

## Troubleshooting

| Symptom | Likely Cause | Recommended Check |
|---|---|---|
| `ToolboxRegistry::instance` throws | Registry not initialized | Ensure bootstrap calls `initializeToolbox(...)` before first consumer request |
| `build()` throws `called more than once` | Builder reused after build | Instantiate a new `ToolboxBuilder` object for each build |
| `BridgeResult.ok == false` with `ContentManager::ingestRawBlob failed` | Content-layer rejection (security, decode, storage) | Inspect `BridgeResult.error` and underlying `ContentManager` logs |
| `BridgeResult.ok == true` but no entities/vectors | Extracted text missing or no entities/chunks produced | Verify `assembleContent(...).assembled_text`, MIME hint, and workflow profiles |
| No streaming callback events | Empty text/chunks or callback not provided | Validate input text and callback wiring |

---

## See Also

- `ARCHITECTURE.md` — component diagram and dependency rules
- `CHANGELOG.md` — implementation-level changes
- `FUTURE_ENHANCEMENTS.md` — planned metrics and streaming path
- `ROADMAP.md` — delivery phases and production-readiness checklist
- `SECURITY.md` — security baseline for this module
- [`../../docs/en/toolbox/index.md`](../../docs/en/toolbox/index.md) — secondary documentation (EN)
- [`../../docs/de/toolbox/index.md`](../../docs/de/toolbox/index.md) — secondary documentation (DE)
- [`../../include/toolbox/README.md`](../../include/toolbox/README.md) — public API documentation

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
