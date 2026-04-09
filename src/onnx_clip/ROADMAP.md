<!-- Status: current | validated: 2026-04-09 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# ONNX CLIP Plugin Roadmap

## Current Status

v0.0.2 — `ONNXClipPlugin` implements the full `IImageAnalysisBackend` interface with
pImpl isolation and multi-backend support (CPU/CUDA/DirectML/TensorRT/AUTO). The public
API is production-quality. Native batch path and focused tests are in progress.

---

## Completed ✅

- [x] `ONNXClipPlugin` class implementing `IImageAnalysisBackend`
- [x] pImpl design isolating all ONNX Runtime types from the public header
- [x] `initialize(config, backend)` with BackendType selection
- [x] `AUTO` backend selection: CUDA → TensorRT → DirectML → CPU probe
- [x] `generateEmbedding()` single-image inference path
- [x] `generateEmbeddingBatch()` sequential batch wrapper
- [x] `warmup()` pre-compilation of CUDA/TensorRT kernels
- [x] `healthCheck()` output tensor shape validation
- [x] `getStatistics()` JSON metrics (calls, avg_latency_ms, backend, model_variant)
- [x] `THEMIS_IMAGE_PLUGIN` macro export for dynamic plugin loading
- [x] Thread-safe inference via `std::mutex`

---

## Planned Features

### v0.1.0 — Native Batch and Tests (Target: Q3 2026)

- [~] Native batch implementation for `generateEmbeddingBatch()` (Target: Q3 2026)
  - Inputs: N × image_data; batch tensor shape [N, 3, 224, 224]
  - Outputs: N × embedding float vectors
  - Constraints: max batch size 64; OOM → split into sub-batches of 8
  - Tests: batch result size, per-item success/error handling, output shape and L2 norm
  - Perf: ≥ 6× speedup vs sequential on RTX-class GPU for batch=64
- [x] Unit tests: CPU backend, model load, embedding shape, `healthCheck()` (Target: Q3 2026) — `tests/test_onnx_clip_plugin.cpp` (9 tests, `OnnxClipPluginTests`); registered in `tests/CMakeLists.txt` under `THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX` guard (2026-04-08)
- [ ] Integration tests: ViT-B/32 and ViT-L/14 golden embedding comparison (Target: Q3 2026)
- [ ] Performance benchmark: ViT-B/32 CPU ≤ 150 ms/image; CUDA ≤ 20 ms/image (Target: Q3 2026)

### v0.2.0 — Text Encoder and Multi-modal Search (Target: Q4 2026)

- [ ] CLIP text encoder support — `generateTextEmbedding(const std::string& text)` (Target: Q4 2026)
  - ONNX model: `clip_text_encoder_vit_b32.onnx` (tokenizer + transformer)
  - Output: 512-dim float vector compatible with image embedding space
  - Tests: cosine similarity of "cat" text vs cat image > 0.25
- [ ] Shared embedding space validation (image ↔ text cosine similarity > 0.20) (Target: Q4 2026)
- [ ] Prometheus metrics: `clip_embeddings_total`, `clip_latency_seconds` histogram (Target: Q4 2026)

### v0.3.0 — Production Hardening (Target: Q1 2027)

- [ ] ONNX model integrity check (SHA-256 hash on load) (Target: Q1 2027)
- [ ] Dynamic model hot-swap without server restart (Target: Q1 2027)
- [ ] Memory-mapped model loading for large ViT-L/14 files (Target: Q1 2027)

---

## Implementation Phases

### Phase 1: Design / API Contract ✅
- [x] Define `IImageAnalysisBackend` compliance points
- [x] Define pImpl struct layout
- [x] Define `BackendType` enum and AUTO selection logic

### Phase 2: Core Implementation ✅
- [x] Single-image inference pipeline (decode → preprocess → infer → normalise)
- [x] Multi-backend initialization (CPU/CUDA/DirectML/TensorRT)
- [x] `generateEmbeddingBatch()` sequential wrapper

### Phase 3: Error Handling & Edge Cases ✅
- [x] Model file not found → `initialize()` returns `false`
- [x] Image decode failure → `EmbeddingResult{ok=false}`
- [x] CUDA unavailable → falls back to CPU in AUTO mode
- [x] Session Run exception → caught; error result returned

### Phase 4: Tests [~]
- [x] Unit tests for CPU backend and embedding shape (Target: Q3 2026)
- [x] Added `tests/test_onnx_clip_plugin.cpp` and registered `OnnxClipPluginTests` (9 tests, 2026-04-08)
- [ ] Integration tests with real ONNX models (Target: Q3 2026)

### Phase 5: Performance / Hardening [ ]
- [ ] Native batched session (Target: Q3 2026)
- [ ] Perf benchmark (Target: Q3 2026)

### Phase 6: Documentation & Acceptance ✅
- [x] README, ARCHITECTURE, AUDIT, CHANGELOG, ROADMAP, SECURITY, FUTURE_ENHANCEMENTS

---

## Production Readiness Checklist

| Area | Status | Notes |
|------|--------|-------|
| API contract | ✅ | Full `IImageAnalysisBackend` implementation |
| pImpl ABI isolation | ✅ | No ONNX Runtime types in public header |
| Thread safety | ✅ | Mutex-serialised session runs |
| Error handling | ✅ | All failure paths return error results |
| Multi-backend | ✅ | CPU / CUDA / DirectML / TensorRT / AUTO |
| Batch inference | ⚠️ | Sequential only; native batch planned Q3 2026 |
| Text encoder | ❌ | Planned Q4 2026 |
| Unit/integration tests | ⚠️ | Unit tests added; integration tests still pending |
| Performance benchmarks | ❌ | Planned Q3 2026 |

---

## Known Issues & Limitations

- `generateEmbeddingBatch()` iterates single calls; not suitable for high-throughput batching.
- DirectML backend requires Windows; silently falls back to CPU on Linux.
- No ONNX model integrity verification (hash check) on load.
