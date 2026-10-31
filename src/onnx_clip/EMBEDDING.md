---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# ONNX CLIP Embedding Interface

Exact model backend not verifiable from final document text alone.

## Purpose and contract

This page explains the image and text embedding inputs and outputs for
`ONNXClipPlugin`. The public class is declared in
[`onnx_clip_plugin.h`](./onnx_clip_plugin.h) and implements
[`IImageAnalysisBackend`](../../include/plugins/image_analysis_interface.h).
The current vectors are deterministic reference outputs, not embeddings
computed by a pretrained CLIP model; do not use them as a semantic retrieval
quality guarantee.

## Inputs and outputs

| Entry point | Input | Success result | Empty/not-ready behavior |
|---|---|---|---|
| `generateEmbedding(bytes, metadata)` | Opaque byte vector; metadata is optional | `EmbeddingResult` with `embedding_dim` floats, `dimension`, and `model_name` | Empty bytes: `"Image data is empty"`; uninitialized plugin: `"ONNXClipPlugin not initialized"` |
| `generateEmbeddingBatch(images)` | Vector of opaque byte vectors | One result per input, preserving order; each item is independently evaluated | Uninitialized: one not-ready error result per item; empty items fail individually |
| `generateTextEmbedding(text)` | Text string | `EmbeddingResult` with `embedding_dim` floats and model label suffixed with `_text` | Empty string: `"Text input is empty"`; uninitialized plugin: `"ONNXClipPlugin not initialized"` |

Image bytes are not decoded. When supplied, only metadata width, height,
channels, and bits-per-channel affect the deterministic image seed; format,
EXIF, and color space do not. Text is lowercased and split on whitespace and
punctuation before hashing; there is no CLIP vocabulary or token limit.

Successful vectors are L2-normalized by the implementation. Result
`inference_time_ms` is an integer millisecond duration and can be zero for fast
calls. The result model name comes from `model.name` (default
`clip-vit-base-patch32`); it is not evidence that a corresponding model was
loaded.

## Batch semantics

`max_batch_size` controls chunk boundaries in `generateEmbeddingBatch()`. The
default is `16` when the selected backend is CPU and `64` otherwise; configured
values are clamped to at least `1` but have no upper clamp in this source.
Chunks are processed sequentially, and each item is computed through the
single-image reference function. The API still allocates a result per input,
so chunking is not a total-request-size limit. The batch API has no metadata
parameter.

## Configuration keys

| Key | Default | Effect |
|---|---|---|
| `model.name` | `clip-vit-base-patch32` | Result/statistics label |
| `model.embedding_dim` | `512` | Output length; non-positive values fall back to `512` |
| `max_batch_size` | `16` CPU / `64` other backend | Sequential batch chunk size; minimum `1` |
| `model.path` | empty | File path consulted for configured digest verification and optional mmap attempt |
| `model.expected_sha256` | empty | Enables digest comparison only when `model.path` is also non-empty |
| `enable_mmap_loading` | `false` | Top-level switch; attempts read-only mapping when a non-empty model path is set |

The plugin configuration getter returns the supplied default for absent or
ill-typed values. `initialize()` does not validate that a model file is a valid
ONNX graph, and the mapped bytes are not passed to an inference runtime.

## Validation expectations for callers

- Call `initialize()` and check its result before embedding requests.
- Check `EmbeddingResult.success` before using its vector; do not use an error
  result's vector.
- Confirm result dimension matches the collection/index schema.
- Treat image and text vectors as distinct reference data; do not compare them
  as aligned CLIP vectors.
- Bound total request size at the caller/API layer. `max_batch_size` bounds
  processing chunks, not total input count or allocation.

## Test evidence and limitations

Focused tests under [`tests/onnx_clip/`](../../tests/onnx_clip/) use generated
byte sequences and mock files. They exercise determinism, normalization,
dimensions, ordering, reload, and mmap resource paths, but do not validate
semantic similarity, actual image decoding, or inference by ONNX Runtime.
See [`RESEARCH.md`](./RESEARCH.md) before making model-quality claims.
