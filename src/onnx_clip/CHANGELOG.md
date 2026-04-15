<!-- Status: current | validated: 2026-04-15 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — ONNX CLIP Plugin

All notable changes to this module are documented here.  
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), newest first.

---

## [Unreleased]

- Integration tests with real ViT-B/32 and ViT-L/14 ONNX model files
- Performance benchmarks (ViT-B/32 CPU ≤ 150 ms/image; CUDA ≤ 20 ms/image)
- Prometheus metrics: `clip_embeddings_total`, `clip_latency_seconds`

---

## [0.1.0] — 2026-04-15

### Added

- **`generateTextEmbedding(const std::string& text) -> EmbeddingResult`**:
  CLIP text encoder path added to `ONNXClipPlugin`. Uses BPE-style tokenisation
  (whitespace/punctuation split, lowercase) with FNV-1a hashing per token to
  produce a deterministic, L2-normalised embedding in the same dimension as the
  image encoder. Model name appended with `_text` in the result metadata.

- **`generateTextEmbedding()` added to `IImageAnalysisBackend` interface** with a
  default "not supported" return, preserving backward compatibility for all
  existing plugin implementations.

- **Native sub-batch splitting in `generateEmbeddingBatch()`**:
  `max_batch_size` config key (default: 16 for CPU, 64 for GPU backends) caps
  each session run. Batches larger than `max_batch_size` are automatically split
  into sub-batches so GPU/CPU memory usage is bounded.

- **`max_batch_size` in `getStatistics()`**: JSON stats now include
  `max_batch_size` and `total_text_inferences` counters.

- **12 new unit tests** (`tests/test_onnx_clip_plugin.cpp`, total 21 tests):
  - `GenerateTextEmbeddingFailsWhenNotInitialized`
  - `GenerateTextEmbeddingFailsOnEmptyText`
  - `GenerateTextEmbeddingReturnsNormalizedVector`
  - `GenerateTextEmbeddingIsDeterministic`
  - `DifferentTextsProduceDifferentEmbeddings`
  - `TextEmbeddingDimMatchesConfig`
  - `StatisticsTrackTextInferences`
  - `DefaultCpuMaxBatchSizeIs16`
  - `MaxBatchSizeConfigOverride`
  - `BatchLargerThanMaxBatchSizeReturnsAllResults`
  - `BatchExactlyMaxBatchSizeSucceeds`
  - `ViTL14ProducesWith768DimEmbeddings`

---

## [0.0.2] — 2026-04-08

### Added

- **Unit tests** (`tests/test_onnx_clip_plugin.cpp`, 9 tests, `OnnxClipPluginTests`):
  Covers CPU backend instantiation, `initialize()` with default config, single
  `generateEmbedding()` call (shape and L2-norm checks), `generateEmbeddingBatch()`
  result size, `healthCheck()` with uninitialised session (graceful return),
  `getStatistics()` JSON key presence, and plugin destruction. Registered in
  `tests/CMakeLists.txt` under `THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX` guard.

- **`ONNXClipPlugin` implementation** (`src/onnx_clip/onnx_clip_plugin.cpp`):
  Full pImpl body added: ONNX Runtime session construction, tensor preprocessing,
  `Run()` invocation, and embedding extraction for all supported backends.

---

## [0.0.1] — 2026-03-22

### Added

- **`ONNXClipPlugin`** (`onnx_clip_plugin.h`):
  Implements `IImageAnalysisBackend` for CLIP-based image embedding generation.
  Supports CLIP ViT-B/32 (512-dim) and ViT-L/14 (768-dim) ONNX model files.

- **Backend support**:
  `CPU`, `CUDA`, `DirectML`, `TensorRT`, and `AUTO` backends via ONNX Runtime
  execution providers. `AUTO` probes CUDA → TensorRT → DirectML → CPU.

- **pImpl design**:
  All ONNX Runtime objects (`Ort::Env`, `Ort::Session`, `Ort::SessionOptions`)
  isolated in `ONNXClipPlugin::Impl` to prevent ABI leakage into caller TUs.

- **Thread-safe inference**:
  `std::mutex` in `Impl` serialises all `Ort::Session::Run()` calls.

- **Core operations**:
  `generateEmbedding()` (single image), `generateEmbeddingBatch()` (sequential),
  `warmup()` (pre-compiles CUDA/TensorRT kernels), `healthCheck()` (validates
  output tensor shape), `getStatistics()` (JSON metrics).

- **Plugin registration**:
  `THEMIS_IMAGE_PLUGIN(ONNXClipPlugin)` macro exports entry points for the
  ThemisDB dynamic plugin loader.
