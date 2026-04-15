<!-- Status: current | validated: 2026-04-15 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# ONNX CLIP Plugin Roadmap

## Current Status

v0.1.0 — `ONNXClipPlugin` implements the full `IImageAnalysisBackend` interface with
pImpl isolation and multi-backend support (CPU/CUDA/DirectML/TensorRT/AUTO). Native
batch sub-batch splitting and CLIP text encoder are now implemented.

---

## Completed ✅

- [x] `ONNXClipPlugin` class implementing `IImageAnalysisBackend`
- [x] pImpl design isolating all ONNX Runtime types from the public header
- [x] `initialize(config, backend)` with BackendType selection
- [x] `AUTO` backend selection: CUDA → TensorRT → DirectML → CPU probe
- [x] `generateEmbedding()` single-image inference path
- [x] `generateEmbeddingBatch()` with native sub-batch splitting and `max_batch_size` config
- [x] `generateTextEmbedding()` — CLIP text encoder with BPE-style tokenization
- [x] `warmup()` pre-compilation of CUDA/TensorRT kernels
- [x] `healthCheck()` output tensor shape validation
- [x] `getStatistics()` JSON metrics (calls, avg_latency_ms, backend, model_variant, max_batch_size, total_text_inferences)
- [x] `THEMIS_IMAGE_PLUGIN` macro export for dynamic plugin loading
- [x] Thread-safe inference via `std::mutex`
- [x] Unit tests: CPU backend, model load, embedding shape, `healthCheck()` (Target: Q3 2026) — `tests/test_onnx_clip_plugin.cpp` (21 tests, `OnnxClipPluginTests`); registered in `tests/CMakeLists.txt` under `THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX` guard

---

## Planned Features

### v0.2.0 — Integration Tests and Benchmarks (Target: Q3 2026)

- [ ] Integration tests: ViT-B/32 and ViT-L/14 golden embedding comparison (Target: Q3 2026)
- [ ] Performance benchmark: ViT-B/32 CPU ≤ 150 ms/image; CUDA ≤ 20 ms/image (Target: Q3 2026)
- [ ] Prometheus metrics: `clip_embeddings_total`, `clip_latency_seconds` histogram (Target: Q3 2026)

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
- [x] `generateEmbeddingBatch()` with sub-batch splitting and `max_batch_size` config
- [x] `generateTextEmbedding()` with BPE-style tokenization

### Phase 3: Error Handling & Edge Cases ✅
- [x] Model file not found → `initialize()` returns `false`
- [x] Image decode failure → `EmbeddingResult{ok=false}`
- [x] CUDA unavailable → falls back to CPU in AUTO mode
- [x] Session Run exception → caught; error result returned
- [x] Empty text input → `EmbeddingResult{ok=false}`

### Phase 4: Tests [~]
- [x] Unit tests for CPU backend and embedding shape (21 tests covering image embedding, text embedding, batch splitting, ViT-L/14 768-dim)
- [ ] Integration tests with real ONNX models (Target: Q3 2026)

### Phase 5: Performance / Hardening [ ]
- [ ] Perf benchmark (Target: Q3 2026)
- [ ] Prometheus metrics (Target: Q3 2026)

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
| Batch inference | ✅ | Sub-batch splitting with `max_batch_size` config |
| Text encoder | ✅ | `generateTextEmbedding()` with BPE tokenization |
| Unit/integration tests | ⚠️ | 21 unit tests; integration tests still pending |
| Performance benchmarks | ❌ | Planned Q3 2026 |
| Prometheus metrics | ❌ | Planned Q3 2026 |

---

## Known Issues & Limitations

- Integration tests with real ViT-B/32 / ViT-L/14 ONNX model files still pending.
- DirectML backend requires Windows; silently falls back to CPU on Linux.
- No ONNX model integrity verification (hash check) on load.
- Prometheus metrics not yet wired.
