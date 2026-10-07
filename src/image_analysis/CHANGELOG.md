> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-09-21 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Image Analysis Module

All notable changes to the image analysis module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Documentation governance sync: CHANGELOG.md, FUTURE_ENHANCEMENTS.md, AUDIT.md, SECURITY.md, MODULE_GAPS.md, PERFORMANCE_EXPECTATIONS.md, and PRODUCTION_REQUIREMENTS.md added to align with repository compliance requirements.

## [1.0.0] - 2026-08-10

### Added
- **Phase 1–3 delivery (Q3 2026)** — The image-analysis module was expanded around the shared plugin API and runtime manager contract.
  - `include/plugins/image_analysis_interface.h` — canonical backend contract and result payloads.
  - `include/plugins/image_analysis_manager.h` — discovery, registration, and capability-based backend selection.
  - `include/plugins/tesseract_ocr_plugin.h` and `include/plugins/yolov8_onnx_plugin.h` — OCR and object-detection plugin contracts.
  - Error handling remains explicit and dependency-gated for optional native runtimes.
- **Tesseract OCR plugin (Phase 2)** — `src/image_analysis/tesseract_ocr_plugin.cpp`.
  - Multi-language text recognition with layout analysis, confidence scoring, and timeout enforcement when the dependency is available.
  - Dual compilation path: real Tesseract API when `HAVE_TESSERACT` is defined; well-formed failure result otherwise.
  - OpenCV image decoding path when `HAVE_OPENCV` is defined.
- **YOLOv8 ONNX object detection plugin (Phase 2)** — `src/image_analysis/yolov8_onnx_plugin.cpp`.
  - ~80 COCO object classes, bounding box computation, NMS, confidence thresholds, and runtime backend selection.
  - ONNX model loading and inference via ONNX Runtime when available.
- **Error handling and edge cases (Phase 3)** — image format validation, dependency-aware fallback, backend unavailability graceful degradation, and timeout handling.
- **Test suite (Phase 4)** — unit, focused, soak, stress, and integration tests across:
  - `tests/image_analysis/test_image_analysis_phase1_focused.cpp`
  - `tests/image_analysis/test_image_analysis_highcardinality_stress.cpp`
  - `tests/integration/test_image_analysis_soak.cpp`
  - `tests/legacy/image/test_image_analysis_interface.cpp`
  - Three additional test artifacts.
- **Benchmarks (Phase 5)** — latency and throughput validation:
  - `benchmarks/image_analysis/bench_image_analysis.cpp`
  - `benchmarks/image_analysis/bench_image_analysis_latency.cpp`
  - `benchmarks/image_analysis/benchmark_image_analysis.cpp`
- **Documentation (Phase 6)** — source-aligned governance docs and module narrative restored under `src/image_analysis/`.

### Fixed
- N/A — initial production release.

### Removed
- N/A — initial production release.

## [0.x] - Pre-production (Internal)

### Added
- Initial plugin scaffolding and backend abstraction prototypes.
