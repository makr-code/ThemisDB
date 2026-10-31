> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | synchronized: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Image Analysis Module

**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** active
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** OpenAI ChatGPT
**Human_Review:** false
**Human_Author:** ThemisDB Contributors

Exact model backend not verifiable from final document text alone.

All notable changes to the image analysis module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Reconciled module documentation with the optional-dependency paths and actual test/benchmark source scope; clarified that the checked-in mock benchmarks do not provide real OCR/detection performance evidence.

## [1.0.0] - 2026-08-10

> Historical entry retained with source-backed corrections; its version label and listed capabilities are not current production-readiness or release-validation evidence.

### Added
- **Phase 1–3 delivery (Q3 2026)** — The image-analysis module was expanded around the shared plugin API and manager API declaration.
  - `include/plugins/image_analysis_interface.h` — canonical backend contract and result payloads.
  - `include/plugins/image_analysis_manager.h` — discovery, registration, and capability-based backend selection.
  - `include/plugins/tesseract_ocr_plugin.h` and `include/plugins/yolov8_onnx_plugin.h` — OCR and object-detection plugin contracts.
  - Error handling remains explicit and dependency-gated for optional native runtimes.
- **Tesseract OCR plugin (Phase 2)** — `src/image_analysis/tesseract_ocr_plugin.cpp`.
  - Multi-language text recognition with layout analysis and confidence scoring when the dependency is available. The current implementation does not enforce a request timeout.
  - Dual compilation path: real Tesseract API when `HAVE_TESSERACT` is defined; well-formed failure result otherwise.
  - OpenCV image decoding path when `HAVE_OPENCV` is defined.
- **YOLOv8 ONNX object detection plugin (Phase 2)** — `src/image_analysis/yolov8_onnx_plugin.cpp`.
  - ~80 COCO object classes, bounding box computation, NMS, confidence thresholds, and runtime backend selection.
  - ONNX model loading and inference via ONNX Runtime when available.
- **Error handling and edge cases (Phase 3)** — dependency-aware fallback and structured error paths are present; actual behavior varies with optional dependencies.
- **Test sources (Phase 4)** — focused, integration, stress, and legacy test files exist, including:
  - `tests/image_analysis/test_image_analysis_phase1_focused.cpp`
  - `tests/image_analysis/test_image_analysis_highcardinality_stress.cpp`
  - `tests/integration/test_image_analysis_soak.cpp`
  - `tests/legacy/image/test_image_analysis_interface.cpp`
- Additional test files under `tests/legacy/image/` and `tests/`.
- **Benchmark sources (Phase 5)** — three synthetic/mock benchmark programs; they are not real Tesseract or YOLOv8 latency/throughput validation:
  - `benchmarks/image_analysis/bench_image_analysis.cpp`
  - `benchmarks/image_analysis/bench_image_analysis_latency.cpp`
  - `benchmarks/image_analysis/benchmark_image_analysis.cpp`
- **Documentation (Phase 6)** — source-aligned governance docs and module narrative restored under `src/image_analysis/`.

### Fixed
- Historical release note; no current production-readiness assertion.

### Removed
- No removal recorded in this historical entry.

## [0.x] - Pre-production (Internal)

### Added
- Initial plugin scaffolding and backend abstraction prototypes.
