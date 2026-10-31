---
Author: ThemisDB Team
Created: 2026-10-30
Last Updated: 2026-10-30
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

# Image Embedding Status and Integration Notes

## Purpose

This page prevents a shared-interface declaration from being mistaken for a
working embedding implementation. It is for developers integrating image
vectors or planning an embedding backend. Current concrete behavior is
source-backed by the two plugin implementations and their public headers.

## Current capability

The shared [`IImageAnalysisBackend`](../../include/plugins/image_analysis_interface.h)
declares `generateEmbedding()` and `generateTextEmbedding()`, and
`EmbeddingResult` carries a success flag, error text, float vector, dimension,
model name, and inference time. These declarations describe extension points;
they do not establish that the image-analysis module produces vectors.

Neither current backend implements embeddings:

* `TesseractOCRPlugin::generateEmbedding()` returns `success = false` with an
  unsupported-operation error.
* `YOLOv8OnnxPlugin::generateEmbedding()` does the same.
* Both concrete `PluginInfo` records advertise detection, not embedding.

Therefore callers must not route image vectorization to either plugin, create
an embedding index from their detection/OCR outputs, or assume a dimension or
model-space compatibility. OCR transcript text and detection boxes are not
substitutes for image embeddings.

## Contract for a future embedding backend

An implementation must explicitly advertise the capability, define supported
input formats and preprocessing, and return a stable model name and dimension
on success. Document model/version identity and normalization, error behavior,
resource limits, backend/provider selection, and whether image and text vectors
share a compatible space. Reject or version incompatible dimensions rather
than silently mixing them in downstream vector search.

Evaluation should include deterministic fixtures, dimension and finiteness
checks, image/text parity where cross-modal search is claimed, task-specific
retrieval metrics, and release-profile latency/memory evidence. Add tests for
unsupported or malformed input, missing model/dependency, and result-schema
compatibility. Until such implementation and evidence land, embedding remains
unsupported in these plugins.

The future-work document tracks embedding extension and output-dimension
compatibility as planned work, not completed functionality:
[FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md).

## Source references

* [`IImageAnalysisBackend` and `EmbeddingResult`](../../include/plugins/image_analysis_interface.h)
* [`TesseractOCRPlugin`](../../include/plugins/tesseract_ocr_plugin.h)
* [`YOLOv8OnnxPlugin`](../../include/plugins/yolov8_onnx_plugin.h)
* [API use and capability limits](API.md)
