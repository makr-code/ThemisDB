> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ARCHITECTURE.md -->

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

## 2. CLIP Text Encoder

### Scope
Add `generateTextEmbedding(const std::string& text) -> EmbeddingResult` to enable
joint image-text similarity search in the ThemisDB vector index.

### Design Constraints
- ONNX model: `clip_text_encoder_{variant}.onnx` loaded at init if `text_model_path`
  config key is present.
- Tokenizer: BPE tokenizer from CLIP (max 77 tokens; truncate/pad).
- Output dimension must match the image encoder (512 for ViT-B/32, 768 for ViT-L/14).
- Both encoders may share the same `Ort::Env` but must use separate `Ort::Session`
  objects (different model graphs).

### Required Interfaces
- `IImageAnalysisBackend::generateTextEmbedding(const std::string& text)` added to interface.
- `ONNXClipPlugin::Impl` gains `text_session_` field.

### Test Strategy
- Unit: "dog" text embedding cosine similarity with dog image embedding > 0.20.
- Unit: identical text → identical embedding (deterministic tokenizer).
- Perf: text encoding ≤ 5 ms on CPU.

### Performance Targets
- Text encoding latency ≤ 5 ms at p95 (CPU).

---

## 3. ONNX Model Integrity Verification

### Scope
On `initialize()`, compute SHA-256 hash of the model file and compare against a
trusted manifest before loading the model into the ONNX session.

### Design Constraints
- Manifest file: `{model_path}.sha256` containing a single hex digest.
- If manifest is absent: log warning, proceed (not a hard failure in dev mode).
- If manifest present and hash mismatch: `initialize()` returns `false` and logs a
  security event at ERROR level.

### Required Interfaces
- New `config["verify_hash"]` bool key (default: `true` in production config).
- Internal `verifyModelHash(const std::string& model_path) -> bool`.

### Test Strategy
- Unit: correct hash → init succeeds; tampered model → init fails.
- Security test: verify behaviour with absent manifest file.

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
