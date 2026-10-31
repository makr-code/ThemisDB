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

# ONNX CLIP Modeling Contract

Exact model backend not verifiable from final document text alone.

## Scope

This document describes the vector-generation model implemented in
[`onnx_clip_plugin.cpp`](./onnx_clip_plugin.cpp), not the architecture of a
pretrained CLIP network. The current algorithm is deterministic synthetic
feature generation. The configured model name and backend are metadata and do
not cause an ONNX session or learned model to be loaded.

## Image vector

For non-empty `image_data`, the implementation computes a 64-bit FNV-1a-style
seed from the bytes, using the initial constant in the source. If `metadata` is
provided, the seed is additionally mixed with `width`, `height`, `channels`,
and `bits_per_channel`; format, EXIF, and color space are not used. The seed
drives xorshift64* to generate `embedding_dim` values in `[-1, 1]`, then the
vector is normalized to unit L2 norm.

The input is treated as opaque bytes. No JPEG/PNG parsing, resizing, color
conversion, or tensor construction occurs in this code path.

## Text vector

For non-empty text, the source lowercases non-separator characters and splits
tokens at whitespace and punctuation. It hashes the original string and folds
token hashes into the seed before generating and L2-normalizing the configured
number of values. This tokenizer has no vocabulary, merges, token limit,
padding, or model-specific token IDs; it is not CLIP's tokenizer.

Text and image paths do not share a learned projection or alignment objective.
Matching dimensions and normalized vectors do not make their cosine similarity
semantically meaningful.

## Configuration and reproducibility

`model.embedding_dim` defaults to `512`; values less than or equal to zero are
replaced with `512`. `model.name` defaults to `clip-vit-base-patch32` and labels
results only. The generated vector is deterministic for the same bytes,
metadata fields, or text and dimension on the same implementation. Changing
the dimension changes the output vector; changing image metadata fields also
changes the image seed.

## Evaluation boundary

The unit tests verify deterministic output properties, vector dimensions,
normalization, and batch equivalence. They do not establish classification
accuracy, retrieval quality, calibration, robustness, or image/text alignment.
Use [`RESEARCH.md`](./RESEARCH.md) for a model-backed evaluation plan and do not
interpret these reference vectors as pretrained CLIP embeddings.

## Trade-offs

This implementation provides a low-dependency way to exercise API, batching,
statistics, and lifecycle contracts. It deliberately omits learned inference;
integrating a real model requires specifying preprocessing, tokenizer and
artifact provenance, provider behavior, quality benchmarks, and failure
handling as a separate implementation change.

## Sources

- [`onnx_clip_plugin.cpp`](./onnx_clip_plugin.cpp) — `computeEmbedding()`,
  `computeTextEmbedding()`, `tokenize()`, and seed helpers
- [`onnx_clip_plugin.h`](./onnx_clip_plugin.h) — module entry points
- [`tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`](../../tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp)
