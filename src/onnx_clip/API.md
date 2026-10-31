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

# ONNX CLIP API

Exact model backend not verifiable from final document text alone.

## Interface

`themis::plugins::image::ONNXClipPlugin` is declared in
[`onnx_clip_plugin.h`](./onnx_clip_plugin.h). It implements
[`IImageAnalysisBackend`](../../include/plugins/image_analysis_interface.h).
The source header is currently in `src/onnx_clip/`; in-tree consumers need the
repository `src/` include directory as well as `include/`.

| Method | Contract |
|---|---|
| `getInfo() const` | Returns plugin identity, advertised formats/capabilities, and backend list. Advertised capabilities are metadata; they do not prove native inference paths are active. |
| `initialize(config, backend)` | Sets model label/dimension/backend/batch chunk size and optionally performs file digest check/mmap. Returns false if configured digest verification fails. `AUTO` selects CPU. |
| `shutdown()` | Marks the current implementation not ready and releases any mapping it owns. |
| `isReady() const` | Returns the current implementation's ready flag. |
| `getBackend() const` | Returns the stored backend enum, not a provider health/probe result. |
| `generateEmbedding(bytes, metadata)` | Generates one normalized reference vector from non-empty bytes and selected metadata fields. |
| `generateEmbeddingBatch(images)` | Returns one result per item in order, processed sequentially in configured chunks. |
| `generateTextEmbedding(text)` | Generates one normalized reference vector from non-empty text. |
| `healthCheck() const` | True when ready and configured dimension is positive; does not validate an ONNX model or provider. |
| `getStatistics() const` | Returns local JSON counters and average integer-millisecond latency; no exporter is created by this API. |
| `warmup()` | If ready, computes one deterministic embedding from a fixed four-byte payload. It does not load a model or compile provider kernels. |
| `reloadModel(config)` | Validates digest inputs and publishes a replacement state, then waits up to 30 seconds for old-snapshot requests. See timeout semantics below. |
| `setModelHashFn(fn)` | Installs the process-wide digest callback used when OpenSSL support is absent. |

## Result semantics

All embedding methods return `EmbeddingResult`. Check `success` and
`error_message` before consuming `embedding`; `dimension` is the output length
and `model_name` is a configured label. Empty image bytes fail with
`"Image data is empty"` and empty text fails with `"Text input is empty"`.
Calls before successful initialization fail with
`"ONNXClipPlugin not initialized"`. Batch calls return individual failures
rather than stopping at the first empty item.

Outputs are normalized deterministic reference vectors, not outputs from a
pretrained CLIP model. Image and text vector spaces are not semantically
aligned. See [`EMBEDDING.md`](./EMBEDDING.md).

## Configuration

Configuration is read through `PluginConfig::get`, which uses the provided
default for missing or ill-typed values.

| Key | Type | Default / handling |
|---|---|---|
| `model.name` | string | `clip-vit-base-patch32`; result/statistics label |
| `model.embedding_dim` | integer | `512`; values <= 0 reset to `512` |
| `max_batch_size` | integer | `16` for CPU, `64` otherwise; values < 1 become `1`; no upper clamp |
| `model.path` | string | Empty; used for digest verification and optional mmap |
| `model.expected_sha256` | string | Empty; compared only when both path and digest are non-empty |
| `enable_mmap_loading` | boolean | `false`; top-level key, independent of `model.path` nesting |

`initialize()` receives the backend as an enum argument. `reloadModel()` instead
reads the top-level string key `backend`; unknown values resolve to CPU.
`BackendType::AUTO` resolves to CPU. No provider availability check is performed
by the embedding algorithm.

## Statistics keys

`getStatistics()` returns `ready`, `backend`, `model_name`, `embedding_dim`,
`max_batch_size`, `total_inferences`, `total_batches`, `total_images`,
`total_text_inferences`, `total_errors`, `avg_latency_ms`,
`clip_embeddings_total`, `clip_text_embeddings_total`, and
`clip_batch_embeddings_total`. Batch metrics count images for the
`clip_batch_embeddings_total` value. These are per-plugin JSON values, not a
Prometheus endpoint or process-wide metric registry.

## Reload timeout contract

On configuration/hash failure before pointer publication, the previous
implementation remains active. A valid new implementation is published before
the old snapshot's in-flight requests are drained. If that drain exceeds 30
seconds, `reloadModel()` returns `false` while the new implementation remains
active. The return value alone cannot distinguish this case from pre-publication
failure; inspect readiness/statistics before taking corrective action.

## Ownership and concurrency

Embedding calls snapshot a shared implementation and serialize on its mutex.
Reload uses a short pointer-swap lock and a per-old-snapshot drain condition.
Callers own input buffers for the duration of a call; results own their vectors.
The static model-hash callback is shared across plugin instances. No caller
authentication, rate limiting, or persistence policy is provided by this API.
