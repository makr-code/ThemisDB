# Image Analysis Module

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

<!-- Status: plugin-backed implementation | source reviewed: 2026-10-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · CMakeLists.txt -->

## Module Purpose

The image-analysis source directory builds optional OCR and object-detection plugin backends. Their shared contract is `include/plugins/image_analysis_interface.h`; plugin headers are under `include/plugins/`. `include/plugins/image_analysis_manager.h` declares a manager API, but the module CMake file does not build an `ImageAnalysisManager` implementation.

The current source-backed implementation focuses on:
- OCR via the Tesseract plugin (`src/image_analysis/tesseract_ocr_plugin.cpp`)
- Object detection via the YOLOv8 ONNX plugin (`src/image_analysis/yolov8_onnx_plugin.cpp`)
- Structured error results when Tesseract or ONNX Runtime is absent
- Optional OpenCV decoding for encoded image data

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| `include/plugins/image_analysis_interface.h` | Canonical image-analysis backend contract and result types |
| `include/plugins/image_analysis_manager.h` | Manager API declarations; manager implementation is not built by this module's CMake file |
| `include/plugins/tesseract_ocr_plugin.h` | OCR plugin interface and OCR-result structures |
| `include/plugins/yolov8_onnx_plugin.h` | YOLOv8 object-detection plugin interface |
| `src/image_analysis/CMakeLists.txt` | Optional dependency wiring for Tesseract and ONNX Runtime |

## Scope

In scope:
- OCR text extraction and region detection
- Object detection with bounding boxes and confidence scores
- Optional plugin deployment under build-time feature gates
- Structured inference errors when Tesseract or ONNX Runtime is unavailable

Out of scope:
- Real-time video analytics
- Custom model training pipelines
- Broad image-generation or multimodal captioning features beyond the plugin interface contract
- Embedding generation in the Tesseract and YOLOv8 plugins (both return unsupported-operation errors)

## Runtime Behavior and Limits

- OCR and detection operate as optional plugins and succeed only when their supporting dependency is enabled.
- When Tesseract or ONNX Runtime is absent, the corresponding plugin can initialize but inference returns an explicit failure result.
- OpenCV is optional too. Without it, Tesseract interprets the input byte vector as raw grayscale data and YOLO preprocessing supplies a zero-filled tensor without decoding the image. Encoded-image inference therefore requires the OpenCV path.
- The plugin metadata reports batch processing and thread safety as unsupported for both backends.
- Numeric values in `PERFORMANCE_EXPECTATIONS.md` are unverified proposals, not measured release gates. The available benchmarks use mock/synthetic plugins.

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
  - Structured error results when the optional inference runtimes are unavailable
  - No manager implementation or real-backend test/benchmark execution is asserted here
  - Plugin lifecycle and configuration in the two backend implementations
- Note:
  - forward planning is tracked in `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md`
  - historical entries remain in `CHANGELOG.md`
