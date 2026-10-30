**Author:** ThemisDB Team
**Created:** 2026-10-30
**Last Updated:** 2026-10-30
**Status:** active
**Source level:** Level 1 — `src/content/`
**SOT domain:** `module-behavior`
**Canonical source references:** [README](README.md), [embedding implementation](embedding_pipeline.cpp), [embedding API](../../include/content/embedding_pipeline.h), [embedding tests](../../tests/content/test_content_embedding_pipeline.cpp), [architecture](ARCHITECTURE.md)
**Related issue:** #7109
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team
**Provenance note:** Exact model backend not verifiable from final document text alone.

# Content, chunk, and processor-output representations

## Purpose and scope

This document describes the data representations used by content ingestion and
enrichment. It is for developers mapping extracted values into persisted content,
chunks, and optional embeddings. These structures describe data exchanged by the
module; they are not a learned model architecture.

The inspected module sources show no in-module model-training workflow, and no
content-owned model-quality benchmark is established here. Embedding integration is
described only as a representation and runtime contract; no quality or model-selection
claim is made.

## Content and chunk metadata

`ContentMeta` represents a content item. Its fields include:

- identity, MIME type/category, original filename, byte size, timestamps, and SHA-256
  content hash;
- compression/encryption flags and type labels;
- processing flags and counts (`text_extracted`, `chunked`, `indexed`, `chunk_count`,
  and `embedding_dim`);
- extracted metadata, application-defined user metadata, and tags;
- parent/child IDs and virtual path/directory state.

`ChunkMeta` represents an ordered part of a content item. It carries a content ID,
zero-based sequence number, chunk type, one or more data representations (`text`, JSON
`data`, or `blob_ref`), positional offsets, and optional embedding/indexed state. These
fields do not establish that every processor populates every field or that all
offsets use the same unit across formats; inspect the processor path before depending
on such semantics.

## Typed processor outputs

`ExtractionResult` carries:

- `ok`, extracted text, structured metadata, optional embedding, and an error message;
- archive extraction fields (`success`, extracted file names, and temporary directory);
- optional typed `GeoData`, `MediaData`, and `CADData` structures.

The header describes `success` as a backwards-compatible alias for `ok`. `GeoData`
contains coordinate/projection/property values; `MediaData` carries duration, dimensions,
codec, and bitrate; `CADData` carries part IDs, bill-of-materials JSON, and dimensions
JSON. These are output containers, not proof of extraction completeness or domain
accuracy.

## Embedding integration as a representation

The `ContentPolicy::embedding_model` string and `EmbeddingPipelineConfig` configure
whether the embedding path is enabled. The pipeline returns vectors of floats; content
and chunk metadata can hold an embedding and a dimension/indexed indicator. They do not,
by themselves, identify the backend or guarantee vector normalization, dimensional
consistency, semantic quality, or successful indexing.

In the current implementation, an empty `model_name` disables the pipeline, and a
non-empty value enables it; the embedding call is `THEMIS_LLM().embed(text)`, with no
configured model name passed in that call. Batch API behavior is sequential per input,
not evidence of a backend-native batched inference operation. See
[`EMBEDDING.md`](EMBEDDING.md) for runtime details.

## Evidence and limits

[`test_content_embedding_pipeline.cpp`](../../tests/content/test_content_embedding_pipeline.cpp)
covers enabled/disabled configuration, batch and timeout clamping, empty output,
configured/reported dimension, result-count shape, and failure-counter export. It does
not exercise model training or semantic-quality evaluation. The public structures are
declared in [`content_manager.h`](../../include/content/content_manager.h) and
[`content_processor.h`](../../include/content/content_processor.h); their presence
defines representation shape, not application-level meaning for every format.

## Sources

- [Content and chunk declarations](../../include/content/content_manager.h) and
  [typed processor outputs](../../include/content/content_processor.h).
- [Embedding configuration/API](../../include/content/embedding_pipeline.h),
  [policy representation](../../include/content/content_policy.h),
  [embedding implementation](embedding_pipeline.cpp), and
  [manager integration](content_manager_embedding.cpp).
- [Embedding tests](../../tests/content/test_content_embedding_pipeline.cpp).
- [Module overview](README.md), [architecture](ARCHITECTURE.md), and
  [embedding guide](EMBEDDING.md).
