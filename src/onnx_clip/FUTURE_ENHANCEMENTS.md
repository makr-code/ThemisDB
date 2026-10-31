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

Exact model backend not verifiable from final document text alone.

> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-09-22 -->
<!-- Links: README.md · PRODUCTION_REQUIREMENTS.md · ROADMAP.md · ARCHITECTURE.md -->

# Future Enhancements — ONNX CLIP Plugin

---

## 1. Native Batched Inference

### Scope
Replace sequential single-call loop in `generateEmbeddingBatch()` with a native
batched `Ort::Session::Run()` call for 4–6× throughput improvement on GPU.

### Design Constraints
- Maximum batch size: 64 images (configurable via `max_batch_size` config key).
- Oversized batches split into sub-batches of `max_batch_size`.
- GPU OOM during batch Run caught and retried with half the batch size.
- CPU batch ≤ 16 (memory bound).

### Required Interfaces
- `generateEmbeddingBatch()` signature unchanged (returns `std::vector<EmbeddingResult>`).
- Internal: batch input tensor `[N, 3, 224, 224]`, batch output `[N, dim]`.

### Test Strategy
- Unit: batch of 1, 8, 64 images → correct count of output embeddings.
- Regression: single-image and batch-of-1 produce identical embedding (L2 delta < 1e-6).
- Perf: batch of 64 on CUDA ≥ 6× faster than 64 sequential calls.

### Performance Targets
- ViT-B/32 CUDA: ≤ 20 ms for batch of 64 (≤ 0.31 ms/image)
- ViT-B/32 CPU: ≤ 2.5 s for batch of 16

---

## 2. Model-backed CLIP Text Encoder

### Scope
The `generateTextEmbedding(const std::string& text) -> EmbeddingResult` API
already exists, but currently produces a deterministic hash/PRNG reference
vector. Replace that reference path with a real CLIP text encoder compatible
with the selected image model before claiming joint image-text similarity.

### Design Constraints
- ONNX model: `clip_text_encoder_{variant}.onnx` loaded at init if `text_model_path`
  config key is present.
- Tokenizer: model-matched CLIP tokenizer (max 77 tokens; truncate/pad).
- Output dimension must match the image encoder (512 for ViT-B/32, 768 for ViT-L/14).
- Both encoders may share the same `Ort::Env` but must use separate `Ort::Session`
  objects (different model graphs).

### Required Interfaces
- Preserve `IImageAnalysisBackend::generateTextEmbedding(const std::string& text)` and its `EmbeddingResult` contract.
- Add model-specific text session/tokenizer state to `ONNXClipPlugin::Impl`.

### Test Strategy
- Unit: "dog" text embedding cosine similarity with dog image embedding > 0.20.
- Unit: identical text → identical embedding (deterministic tokenizer).
- Perf: text encoding ≤ 5 ms on CPU.

### Performance Targets
- Text encoding latency ≤ 5 ms at p95 (CPU).

---

## 3. Hardened ONNX Artifact Integrity

### Scope
The current source optionally compares a configured digest when both
`model.path` and `model.expected_sha256` are present. OpenSSL builds compute
SHA-256; non-OpenSSL builds use `ModelHashFn` when installed and otherwise skip
the check. Harden this policy so integrity-required deployments cannot
initialize or reload without an active verifier, and bind the digest to a
trusted artifact provenance mechanism before any future model session uses the
file.

### Design Constraints
- Keep current explicit `model.path` and `model.expected_sha256` inputs backward-compatible unless a migration is approved.
- Missing verifier in an integrity-required mode must fail closed.
- A mismatch or read failure must reject initialization/reload and surface an actionable diagnostic.

### Required Interfaces
- Define how hardened mode is selected and how verifier availability is reported.
- Keep the hash callback thread-safe and test failure/mismatch cases.

### Test Strategy
- Unit: correct digest succeeds; tampered/unreadable file fails.
- Security test: absent verifier fails in integrity-required mode and never silently proceeds.

---

## 4. Prometheus Metrics

### Scope
Expose embedding generation metrics via the ThemisDB Prometheus endpoint.

### Planned Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `clip_embeddings_total` | Counter | Total embeddings generated (label: `model_variant`, `backend`) |
| `clip_latency_seconds` | Histogram | Per-call inference latency |
| `clip_batch_size` | Histogram | Batch sizes submitted |
| `clip_errors_total` | Counter | Failed embedding calls (label: `error_type`) |

### Performance Targets
- Metrics collection overhead ≤ 0.05 ms per call.
