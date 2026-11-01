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

# Toolbox public-header architecture

**Scope:** Public declarations in `include/toolbox/`. This page is a map, not a substitute for the header contracts or module implementation documentation. `src/toolbox/ARCHITECTURE.md` is the implementation overview.

## Public header groups

| Header | Declared interface | Responsibility |
|---|---|---|
| `ingestion_toolbox.h` | `IngestionToolbox` | Runs configured ingestion workflows and returns entity nodes or a complete entity set. |
| `toolbox_builder.h` | `ToolboxBuilder`, `BuiltToolbox` | Configures workflow profiles, engines, backends, extractors, sinks, and optional AQL/RAG bridges. |
| `content_toolbox_bridge.h` | `ContentToolboxBridge` | Connects `ContentManager` ingestion/text retrieval to toolbox enrichment and optional graph/vector writers. |
| `toolbox_registry.h` | `ToolboxRegistry` and free functions | Holds one process-global `IngestionToolbox`; it is not a registry of tool functions. |
| `toolbox_composite.h` | `ToolboxComposite`, `ToolboxCompositeBuilder` | Routes extraction by MIME-prefix and optional fallback. |
| `toolbox_streaming.h` | `extractEntitiesStream` free-function overloads | Chunks input and invokes a callback for extracted entities. There is no `ToolboxStreaming` type. |
| `text_chunker.h` | `TextChunker`, `chunkText` | Facade over `rag::DocumentSplitter`. |
| `text_normalizer.h` | `TextNormalizer`, `normalizeText` | Umlaut normalization helpers. |
| `text_quality_scorer.h` | `TextQualityScorer`, `TextQualityScore`, `scoreText` | Heuristic text signals; no calibrated scalar quality score. |
| `language_detector.h` | `ILanguageDetector`, `DefaultLanguageDetector`, `detectLanguage` | Interface and default English/German stopword-ratio heuristic. |
| `content_fingerprinter.h` | `ContentFingerprinter`, `ContentFingerprint`, `fingerprint` | SHA-256 digest of non-empty input bytes and byte/token-estimate metadata. |
| `toolbox_api_contract.h` | `ToolboxError`, `Fingerprint` | Declares only the error-code enum and fixed-size fingerprint alias. Doxygen prose describing `ContentToolboxBridge::process()` and a processor registry is stale/unresolved, not an active contract. |

## Contract boundaries

- The toolbox delegates workflow execution to `ingestion::WorkflowEngine`; workflow configuration and backend availability affect results.
- `extractEntities()` returns nodes. `extractEntitySet()` also exposes edges and chunks. Empty output alone does not distinguish an ordinary no-match from a workflow failure.
- `ToolboxRegistry::initialize()` requires a non-null toolbox. `instance()` throws before initialization; a subsequent `initialize()` replaces the stored instance. `reset()` is for controlled teardown/test isolation, not concurrent live work.
- Composite routes are checked in insertion order; the first matching MIME-prefix wins, then the optional fallback is used.
- Streaming is synchronous and callback-based. The injected overload does nothing for empty input or an empty callback; callback exceptions propagate.
- The default language detector considers only English and German stopword sets and returns `"und"` for fewer than three whitespace-separated tokens.
- `ContentFingerprinter` uses SHA-256, not MinHash or SimHash. Its empty-input result has an empty digest.
- `IngestionToolbox` must not become an ingestion dependency: toolbox uses ingestion interfaces, not the reverse.

## Sources

- Public contracts: headers linked in the table above.
- Implementation detail and behavior summary: [`../../src/toolbox/ARCHITECTURE.md`](../../src/toolbox/ARCHITECTURE.md).
- Test and benchmark registration: [`../../tests/toolbox/CMakeLists.txt`](../../tests/toolbox/CMakeLists.txt) and [`../../benchmarks/toolbox/CMakeLists.txt`](../../benchmarks/toolbox/CMakeLists.txt).

> Exact model backend not verifiable from final document text alone.
