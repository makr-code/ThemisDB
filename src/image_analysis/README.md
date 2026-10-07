# Image Analysis Module

<!-- Status: IN_PROGRESS | plugin-backed implementation | source-verified: 2026-10-07 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · CMakeLists.txt -->

## Module Purpose

The image analysis module provides the ThemisDB vision layer for OCR and object detection. The live implementation is plugin-first: `src/image_analysis/CMakeLists.txt` builds optional OCR and detection backends, and the canonical runtime contract is defined in `include/plugins/image_analysis_interface.h` together with the manager and plugin wrappers under `include/plugins/`.

The current source-backed implementation focuses on:
- OCR via the Tesseract plugin (`src/image_analysis/tesseract_ocr_plugin.cpp`)
- Object detection via the YOLOv8 ONNX plugin (`src/image_analysis/yolov8_onnx_plugin.cpp`)
- Backend selection and lifecycle orchestration (`include/plugins/image_analysis_manager.h`)
- Graceful fallback when optional dependencies are absent

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| `include/plugins/image_analysis_interface.h` | Canonical image-analysis backend contract and result types |
| `include/plugins/image_analysis_manager.h` | Discovery, registration, and selection of loaded plugins |
| `include/plugins/tesseract_ocr_plugin.h` | OCR plugin interface and OCR-result structures |
| `include/plugins/yolov8_onnx_plugin.h` | YOLOv8 object-detection plugin interface |
| `src/image_analysis/CMakeLists.txt` | Optional dependency wiring for Tesseract and ONNX Runtime |

## Scope

In scope:
- OCR text extraction and region detection
- Object detection with bounding boxes and confidence scores
- Optional plugin deployment under build-time feature gates
- Graceful degradation when Tesseract or ONNX Runtime is unavailable

Out of scope:
- Real-time video analytics
- Custom model training pipelines
- Broad image-generation or multimodal captioning features beyond the plugin interface contract

## Runtime Behavior and Limits

- OCR and detection operate as optional plugins and succeed only when their supporting dependency is enabled.
- When dependencies are absent, the plugins still initialise and return explicit failure results instead of crashing.
- The module intentionally prioritises fail-closed runtime behaviour: no silent fallback beyond documented error responses.
- Batch and latency targets remain tracked in `ROADMAP.md` and `PERFORMANCE_EXPECTATIONS.md` rather than being treated as settled production claims.

## Sourcecode Verification (Module: image_analysis/readme)

- Verified files:
  - `src/image_analysis/tesseract_ocr_plugin.cpp`
  - `src/image_analysis/yolov8_onnx_plugin.cpp`
  - `src/image_analysis/CMakeLists.txt`
  - `include/plugins/image_analysis_interface.h`
  - `include/plugins/image_analysis_manager.h`
- Verified behavior surfaces:
  - OCR text extraction and confidence scoring when `HAVE_TESSERACT` is enabled
  - Object detection with bounding boxes when `HAVE_ONNXRUNTIME` is enabled
  - Graceful error results when the optional native backends are unavailable
  - Plugin lifecycle, configuration, and manager selection logic
- Note:
  - forward planning is tracked in `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md`
  - historical entries remain in `CHANGELOG.md`
