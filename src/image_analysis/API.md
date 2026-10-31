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

# Image Analysis API Contract

## Audience and source of truth

This is the integration-facing contract for callers of image-analysis
plugins. The canonical declarations are
[`image_analysis_interface.h`](../../include/plugins/image_analysis_interface.h)
and the concrete plugin headers in `include/plugins/`. This module currently
ships Tesseract OCR and YOLOv8 object detection implementations.

## Shared interface versus implemented operations

| Operation/capability | Shared interface | Tesseract plugin | YOLOv8 ONNX plugin |
|---|---|---|---|
| `initialize`, `shutdown`, `isReady`, `healthCheck` | Required lifecycle | Implemented | Implemented |
| `detectObjects(image_data, metadata, threshold)` | Generic operation | OCR word regions | YOLO object boxes |
| `generateEmbedding(image_data, metadata)` | Declared | Explicit unsupported error | Explicit unsupported error |
| Captioning, segmentation, generation, visual QA | Declared/default unsupported | Not implemented | Not implemented |
| Batch processing | Default sequential embedding helper exists | Not advertised | Not advertised |

`PluginCapabilities` and default interface methods describe the broad plugin
contract; they do not prove a concrete backend supports every operation. Both
concrete `getInfo()` methods set detection capability and do not advertise
embedding.

## Results and input contract

Input is `const std::vector<uint8_t>&` containing image bytes. Optional
`ImageMetadata*` is accepted but ignored by both current plugin implementations.
`DetectionResult` reports `success`, `error_message`, `detections`,
`model_name`, and `inference_time_ms`. A box uses top-left `x`, `y`, `width`,
and `height` normalized to 0–1, plus a label and confidence intended to be
0–1. OCR stores full text and per-word data in `OcrResult`, retrieved through
`TesseractOCRPlugin::getLastOcrResult()` after `detectObjects()`.

A missing/uninitialized runtime, decode failure, or inference error is
represented by `success=false` and an explanatory message; callers must check
success before consuming detections. Empty results with `success=true` mean
the operation ran and returned no regions, not that an error occurred.

The per-call detection threshold overrides configuration only when it is
greater than zero; otherwise the configured threshold is used (or 0.25 for
YOLO / 0.0 for OCR). For OCR the effective threshold is compared against
normalized word confidence. Avoid thresholds outside 0–1; the focused tests
use a value above 1 only as a check that YOLO returns no boxes.

## Plugin-specific configuration

* **Tesseract:** `tessdata_path` (empty/default path), `language` (`eng`),
  `page_seg_mode` (3), `dpi` (70), `whitelist_chars` (empty), and
  `min_confidence` (0.0). Tesseract language data must be installed. Without
  OpenCV, only raw grayscale bytes are meaningful; encoded image bytes will
  not be decoded. The header describes `min_confidence` in 0–100 units, but the
  implementation compares its unchanged configured value against normalized
  0–1 confidence; see the scale caveat in [MODELING.md](MODELING.md).
* **YOLOv8:** `model_path` (required only when ONNX Runtime is compiled in),
  `labels_path` (optional), `input_width`/`input_height` (640),
  `confidence_threshold` (0.25), `nms_iou_threshold` (0.45), and
  `max_detections` (100). Runtime expects a compatible YOLOv8 ONNX model;
  OpenCV is needed for actual image decoding. The no-OpenCV path constructs a
  zero-filled input tensor and is not suitable for real image inference.

`PluginConfig` reads JSON pointer-style nested keys, converting dots to `/`;
missing or invalid values fall back to the caller-provided default.

## Lifecycle and concurrency

Initialize before inference and call shutdown before destroying managed
resources. Missing optional runtimes can still yield `initialize() == true`
and `isReady() == true`; use an inference call and check its result to confirm
usable backend functionality. Tesseract serializes its API instance internally,
but its plugin metadata says not thread-safe; do not concurrently invoke
`detectObjects()` on one instance when also relying on its last-result accessor.
YOLO metadata likewise says not thread-safe; do not overlap lifecycle changes
with inference. The `ImageAnalysisManager` declaration publishes loading,
selection, and convenience operations, but the inspected module source set
does not contain a manager implementation; verify the linked implementation
and routing before depending on those behaviors.

## Source references

* [`IImageAnalysisBackend`, result structures, and `PluginConfig`](../../include/plugins/image_analysis_interface.h)
* [`ImageAnalysisManager` declarations](../../include/plugins/image_analysis_manager.h)
* [`TesseractOCRPlugin`](../../include/plugins/tesseract_ocr_plugin.h)
* [`YOLOv8OnnxPlugin`](../../include/plugins/yolov8_onnx_plugin.h)
* [Embedding capability boundary](EMBEDDING.md)
