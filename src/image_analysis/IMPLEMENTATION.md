---
Author: ThemisDB Team
Created: 2026-10-30
Last Updated: 2026-10-30
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

# Image Analysis Implementation Guide

## Purpose and ownership

This guide is for developers building or changing the module. Implementations
are in `src/image_analysis/`; the shared contracts and plugin classes are in
`include/plugins/`. The implementations in scope are Tesseract OCR and YOLOv8
detection—not every capability declared in the shared interface.

## Build composition and dependency modes

`src/image_analysis/CMakeLists.txt` uses
`THEMIS_PLUGIN_IMAGE_YOLOV8` and `THEMIS_PLUGIN_IMAGE_TESSERACT` to decide
whether to create each shared plugin target. ONNX Runtime and Tesseract are
searched optionally. When a plugin target is enabled but its native runtime
is absent, CMake still creates the library and warns that it is in stub mode;
it does not enable real inference. `HAVE_ONNXRUNTIME` and `HAVE_TESSERACT` are
private compile definitions set only when their respective packages are
found. OpenCV (`core`, `imgproc`, `imgcodecs`) is also optional and enables
`HAVE_OPENCV` for either plugin. A static test library is added when tests are
enabled.

Do not treat “plugin initialized” or “ready” as evidence that inference is
available: both implementations intentionally set ready in no-runtime builds
so the host can query state, then return explicit inference errors.

## Runtime invariants and edge cases

* **Tesseract:** defaults to `eng`; an empty `tessdata_path` is passed as a
  null path to `TessBaseAPI::Init`, which relies on Tesseract's environment or
  system configuration. Inference serializes access to one API instance with
  a mutex. With OpenCV, input bytes are decoded as grayscale. Without OpenCV,
  the bytes are interpreted as raw grayscale pixels and a square side is
  estimated from byte count; compressed JPEG/PNG bytes are not decoded by
  this path. Do not deploy encoded image input on the no-OpenCV path as if it
  were decoded.
* **YOLOv8:** with ONNX Runtime, `model_path` is required and session creation
  failure makes initialization fail. With no ONNX Runtime, initialization
  succeeds even with an empty model path, but inference returns a
  `success=false` result. With OpenCV, images are decoded and resized. Without
  OpenCV, preprocessing supplies an all-zero tensor rather than decoding or
  rejecting the input; this can produce misleading model output if paired
  with a real ONNX session and must not be used as a valid image-inference
  path.
* Both plugins return unsupported-operation errors for `generateEmbedding()`.
  YOLO output postprocessing assumes the fixed standard 80-class/8,400-anchor
  layout; see [MODELING.md](MODELING.md).
* Error results are structured, but this code does not establish an input-size
  cap, execution timeout, model integrity policy, or request admission limit.
  Callers/deployment must impose appropriate resource controls.

## Configuration and integration checklist

Use `PluginConfig` (JSON-backed) with documented keys. For YOLO provide a
readable compatible `model_path`; optionally provide a matching one-label-per-
line `labels_path`, `input_width`, `input_height`, `confidence_threshold`,
`nms_iou_threshold`, and `max_detections`. For OCR set `tessdata_path` and
`language` explicitly in controlled deployments; optionally set `page_seg_mode`,
`dpi`, `whitelist_chars`, and `min_confidence`. The plugin headers are the
canonical key definitions:
[`YOLOv8OnnxPlugin`](../../include/plugins/yolov8_onnx_plugin.h) and
[`TesseractOCRPlugin`](../../include/plugins/tesseract_ocr_plugin.h).

When changing result schemas or preprocessing, update the API and operational
docs, add real-backend validation where feasible, and keep the module roadmap
and performance targets honest about pending evidence.

## Focused validation

`tests/image_analysis/CMakeLists.txt` compiles plugin sources into standalone
test executables and only explicitly adds OpenCV when found. Its normal focused
target does not turn on the optional Tesseract/ONNX compile paths. Build the
target `module_image_analysis_test_image_analysis_phase1_focused_focused` and
run the registered CTest case
`test_image_analysis_phase1_focused_ImageAnalysisFocusedTests` to validate
interface/stub behavior. Treat this as contract coverage, not model accuracy,
real inference, or benchmark validation. The separate stress and soak files
use in-process stubs; see [RESEARCH.md](RESEARCH.md).

## Related docs

* [API and result contract](API.md)
* [Module onboarding](MODULE_GUIDE.md)
* [Operational diagnosis](RUNBOOK.md)
* [Current release acceptance state](ROADMAP.md)
