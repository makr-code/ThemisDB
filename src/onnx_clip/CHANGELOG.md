<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — ONNX CLIP Plugin

All notable changes to this module are documented here.  
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), newest first.

---

## [Unreleased]

- Native batched ONNX session execution for `generateEmbeddingBatch()`
- CLIP text encoder support (`generateTextEmbedding()`)
- Unit and integration tests
- Performance benchmarks (ViT-B/32 CPU ≤ 150 ms/image)

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
