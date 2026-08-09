> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: current | validated: 2026-04-15 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# ONNX CLIP Plugin Roadmap

## Current Status

v0.2.0 — `ONNXClipPlugin` is Production-ready. Full `IImageAnalysisBackend` interface with
pImpl isolation, multi-backend support (CPU/CUDA/DirectML/TensorRT/AUTO), native batch
sub-batch splitting, CLIP text encoder, Prometheus-style metrics, and ONNX model integrity
check (SHA-256) are all implemented.

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
- [x] Prometheus metrics: `clip_embeddings_total`, `clip_text_embeddings_total`, `clip_batch_embeddings_total` (Target: Q3 2026) ✅
- [x] ONNX model integrity check (SHA-256 hash on load via OpenSSL EVP; skipped gracefully without OpenSSL) (Target: Q1 2027) ✅
- [x] Unit tests: CPU backend, model load, embedding shape, `healthCheck()` (Target: Q3 2026) — `tests/test_onnx_clip_plugin.cpp` (26 tests, `ONNXClipPluginTest`); registered in `tests/CMakeLists.txt` under `THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX` guard
- [x] Contract-hardening focused tests (Target: Q3 2026) — `tests/test_onnx_clip_plugin_contract_hardening_focused.cpp` (16 tests, `OnnxClipContractHardeningTest`); covers OCP-01..16 interface/backend/embedding/batch contracts; registered as `test_onnx_clip_plugin_contract_hardening_focused` target in `tests/CMakeLists.txt`

---

## Planned Features

### v0.3.0 — Production Hardening (Target: Q1 2027)

#### Phase 1: Integration Tests & Golden Embeddings (Target: Q3 2026)
- [~] Task 1A: Integration test infrastructure (OCP-IT-01..08) — **In Progress**
- [ ] Task 1B: Reproducibility & regression testing (OCP-IT-09..12)
- [ ] Task 1C: Test harness & CI integration
- **Acceptance:** All 12 integration tests pass, coverage > 80%

#### Phase 2: Performance Benchmarking (Target: Q3 2026)
- [~] Task 2A: Benchmark framework & fixtures — **In Progress**
- [ ] Task 2B: Latency & throughput analysis
- [ ] Task 2C: Performance gates (FCP-01..06) registration
- **Acceptance:** Gates FCP-01 (≤150ms), FCP-02 (≤2.4s), FCP-03 (≤5ms), FCP-04 (<500ms), FCP-05 (≥6x batch), FCP-06 (memory tracked)

#### Phase 3: Dynamic Model Hot-Swap (Target: Q1 2027)
- [~] Task 3A: Hot-swap API design & contract — **In Progress**
- [~] Task 3B: Hot-swap implementation — **In Progress**
- [ ] Task 3C: Hot-swap testing & validation (OCP-HS-01..12)
- **Acceptance:** All 12 tests pass, no request disruption, rollback verified

#### Phase 4: Memory-Mapped Model Loading (Target: Q1 2027)
- [~] Task 4A: Memory-mapped API & design — **In Progress**
- [~] Task 4B: Mmap implementation — **In Progress**
- [ ] Task 4C: Mmap testing & optimization (OCP-MM-01..12)
- **Acceptance:** All 12 tests pass, ≥30% memory reduction for ViT-L/14

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

### Phase 4: Tests ✅
- [x] Unit tests for CPU backend and embedding shape (26 tests covering image embedding, text embedding, batch splitting, ViT-L/14 768-dim, Prometheus counters, integrity check)
- [x] Contract-hardening focused tests (16 tests: OCP-01..16 covering interface contract, backend selection, embedding generation, batch processing)
- [ ] Integration tests with real ONNX models (Target: Q3 2026)


### Phase 5: Performance / Hardening ✅ (partial)
- [ ] Perf benchmark (Target: Q3 2026)
- [x] Prometheus metrics (Target: Q3 2026)
- [x] ONNX model integrity check (SHA-256 via OpenSSL) (Target: Q1 2027)

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
| Prometheus metrics | ✅ | `clip_embeddings_total`, `clip_text_embeddings_total`, `clip_batch_embeddings_total` |
| Model integrity check | ✅ | SHA-256 via OpenSSL EVP; graceful skip without OpenSSL |
| Unit/integration tests | ⚠️ | 26 unit tests + 16 contract-hardening focused tests (OCP-01..16); integration tests still pending (Q3 2026 target) |
| Performance benchmarks | ❌ | Planned Q3 2026 |

---

## Known Issues & Limitations

- Integration tests with real ViT-B/32 / ViT-L/14 ONNX model files still pending.
- DirectML backend requires Windows; silently falls back to CPU on Linux.
- SHA-256 model integrity check requires OpenSSL at build time (`THEMIS_HAS_OPENSSL`); skipped without it.

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### ✅ Aktiv (implementiert + externer Aufrufer bestätigt)

- `computeEmbedding` – Berechnet CLIP-Embedding via ONNX; genutzt in gnn_embeddings + inference_engine

