> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/toolbox/ARCHITECTURE.md -->

# Toolbox Module — Public Header Architecture

**Module Path:** `include/toolbox/`  
**Implementation:** `../../src/toolbox/`  
**Canonical architecture doc:** [`../../src/toolbox/ARCHITECTURE.md`](../../src/toolbox/ARCHITECTURE.md)

---

## 1. Overview

`include/toolbox/` defines the **public shared text-processing, language detection, chunking, normalisation, quality scoring, fingerprinting, and content-toolbox bridging utilities API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/toolbox/ARCHITECTURE.md`](../../src/toolbox/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Text Processing

| Header | Public Type | Purpose |
|--------|------------|---------|
| `text_chunker.h` | `TextChunker` | Semantic and fixed-window text chunking |
| `text_normalizer.h` | `TextNormalizer` | Unicode/whitespace text normalisation |
| `text_quality_scorer.h` | `TextQualityScorer` | Heuristic and ML-based text quality scoring |
| `language_detector.h` | `LanguageDetector` | Compact language identification |
### 2.2 Content Utilities

| Header | Public Type | Purpose |
|--------|------------|---------|
| `content_fingerprinter.h` | `ContentFingerprinter` | MinHash/SimHash content fingerprinting |
| `content_toolbox_bridge.h` | `ContentToolboxBridge` | Bridge from content module to toolbox |
| `ingestion_toolbox.h` | `IngestionToolbox` | Toolbox utilities for ingestion pipelines |
### 2.3 Registry and Composition

| Header | Public Type | Purpose |
|--------|------------|---------|
| `toolbox_registry.h` | `ToolboxRegistry` | Tool function registry and lookup |
| `toolbox_builder.h` | `ToolboxBuilder` | Fluent toolbox composition builder |
| `toolbox_composite.h` | `ToolboxComposite` | Composable toolbox pipeline |
| `toolbox_streaming.h` | `ToolboxStreaming` | Streaming toolbox execution mode |

---

## 3. Namespace Layout

All public types reside in the `themis::toolbox` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/toolbox/` expose the **stable public API**; internal types live in `src/toolbox/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **ANN/Tensor**.
