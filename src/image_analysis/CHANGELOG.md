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
- **Phase 1–3 delivery (Q3 2026)** — Full image analysis infrastructure implemented and deployed to production.
  - `include/image_analysis/image_processor.h` — Core image processing API contract with `processImage`, `extractText`, `detectObjects`.
  - `include/image_analysis/feature_extractor.h` — Feature extraction interface for embedding-based similarity queries.
  - `include/image_analysis/image_cache.h` — LRU result caching contract with configurable TTL.
  - Error taxonomy E6200–E6299 reserved for image analysis errors.
- **Tesseract OCR plugin (Phase 2)** — `src/image_analysis/tesseract_ocr_plugin.cpp`.
  - Multi-language text recognition with layout analysis, confidence scoring, and timeout enforcement.
  - Dual compilation path: real Tesseract API when `HAVE_TESSERACT` is defined; well-formed no-op result otherwise.
  - OpenCV image decoding path when `HAVE_OPENCV` is defined.
- **YOLOv8 ONNX object detection plugin (Phase 2)** — `src/image_analysis/yolov8_onnx_plugin.cpp`.
  - ~80 COCO object classes, bounding box computation, NMS, confidence thresholds, batch inference.
  - ONNX model loading and inference via ONNX Runtime.
- **Error handling and edge cases (Phase 3)** — image format validation, corrupted image fallback, backend unavailability graceful degradation, timeout enforcement.
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
- **Documentation (Phase 6)** — `README.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `DOXYGEN.md` completed.

### Fixed
- N/A — initial production release.

### Removed
- N/A — initial production release.

## [0.x] - Pre-production (Internal)

### Added
- Initial plugin scaffolding and backend abstraction prototypes.
