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

# Toolbox research and evaluation

**Purpose and audience:** This page records the evidence and evaluation limits relevant to developers changing `src/toolbox`. It is not a claim that the toolbox owns a trained language or embedding model.

## Questions the implementation answers

- Can the workflow façade produce stable extraction results for the same workflow configuration and input? The toolbox delegates execution to `ingestion::WorkflowEngine`; extraction failures become empty result objects and are reflected in extraction metrics. See [`ingestion_toolbox.cpp`](./ingestion_toolbox.cpp) and [`ingestion_toolbox.h`](../../include/toolbox/ingestion_toolbox.h).
- How are short text and unknown language handled by the local helpers? The detector returns `"und"` for fewer than three whitespace-delimited tokens and considers only English and German stopword sets. The quality scorer marks text with fewer than five tokens as boilerplate. See [`language_detector.cpp`](./language_detector.cpp), [`text_quality_scorer.cpp`](./text_quality_scorer.cpp), and [`test_toolbox_primitives.cpp`](../../tests/toolbox/test_toolbox_primitives.cpp).
- Which costs belong to toolbox-native helpers versus the surrounding ingestion and content systems? Use native toolbox measurements for helper costs; bridge measurements include content-manager, workflow, and optional sink work.

## Evidence and assumptions

| Area | Source-grounded behavior | Evaluation caveat |
|---|---|---|
| Extraction | `IngestionToolbox` constructs an `ExtractionContext` and invokes the configured workflow engine. | In the `extractEntities()` path, a zero-entity result for text of at least eight bytes is counted as an extraction error; `extractEntitySet()` also treats non-empty chunks as success. Neither counter identifies a specific cause. |
| Language | `DefaultLanguageDetector` lowercases whitespace-separated tokens and compares English/German stopword hit ratios against a configurable threshold. | It is a small heuristic, not general language identification. Mixed, technical, short, or unsupported-language samples may return `"und"` or a misleading best match. |
| Text quality | `TextQualityScorer` reports byte count, whitespace-token count, language, emptiness, and a boilerplate flag. | The flag is heuristic: fewer than five tokens, average token length above 25, or repeated-token ratio above 0.50. It is not a calibrated quality probability. |
| Fingerprinting | `ContentFingerprinter` calculates SHA-256 over the provided bytes and estimates tokens by rounding byte length divided by four. | The estimate is not a tokenizer output; an empty input has an empty digest rather than a SHA-256 digest. |
| Chunking | `TextChunker` delegates splitting and token estimates to `rag::DocumentSplitter`. | Chunk semantics, token estimation, and accepted configuration are owned by the RAG splitter contract. |

These are implementation facts, not claims about corpus-wide accuracy or production performance. The helper behavior is covered by [`test_toolbox_primitives.cpp`](../../tests/toolbox/test_toolbox_primitives.cpp); workflow, bridge, and stress cases are listed in [`tests/toolbox/README.md`](../../tests/toolbox/README.md).

## Reproducible evaluation

1. Run the focused primitive suite: `ctest --preset linux-release -R test_toolbox_primitives_toolbox_FocusedTests --output-on-failure`.
2. Run the workflow and bridge contract suites when changing extraction or integration behavior: `ctest --preset linux-release -R "test_toolbox_(contract_hardening_focused|ingestion)_toolbox_FocusedTests" --output-on-failure`.
3. For performance claims, build and run `bench_toolbox_native_workloads` as described in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md). Record the commit, compiler/build preset, CPU, operating system, sample sizes, repetitions, and raw output alongside any new baseline.

The performance document now lists measured source symbols and their limits; it does not establish certified thresholds or current baselines. `BM_BridgeProxy_EntitySetLatency` measures `IngestionToolbox::extractEntitySet()` only, not bridge or embedding latency. See [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).

## Trade-offs and limits

- The local language and boilerplate methods favor low-cost deterministic rules over broad coverage or calibrated confidence.
- Token counts are estimates or whitespace-token counts, not model tokenizer counts.
- End-to-end extraction results depend on workflow profiles, registered ingestion steps, and injected backends; helper tests alone do not characterize them.
- The toolbox exposes vector records/chunks to downstream writers but does not specify an embedding model, vector dimension, or retrieval quality metric. See [`EMBEDDING.md`](./EMBEDDING.md).

## Sources

- [`ARCHITECTURE.md`](./ARCHITECTURE.md)
- [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md)
- [`include/toolbox` public headers](../../include/toolbox/README.md)
- [`tests/toolbox/README.md`](../../tests/toolbox/README.md)

> Exact model backend not verifiable from final document text alone.
